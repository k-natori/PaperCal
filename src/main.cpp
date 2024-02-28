#include <Arduino.h>
#include <M5EPD.h>
#include <vector>
#include <map>
#include <time.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include "PCEvent.h"

#define screenWidth 540
#define screenHeight 960
#define rowHeight 84
#define columnWidth 77
#define maxNumberOfRows 10

int fontSize = 32;
int smallFontSize = 24;
String fontName = "/font.ttf";
String pemFileName = "/root_ca.pem";
std::vector<String> iCalendarURLs;
String rootCA = "";
boolean loaded = false;
boolean loginScreen = false;
float timezone = 0;

int currentYear = 0;
int currentMonth = 0;
int nextMonthYear = 0;
int nextMonth = 0;
String dateString = "";

std::multimap<int, PCEvent> eventsInThisMonth;
std::vector<PCEvent> eventsInNextMonth;
std::vector<PCEvent> eventsToDisplay;

M5EPD_Canvas canvas(&M5.EPD);
M5EPD_Canvas widthCanvas(&M5.EPD);

// put function declarations here:
void load();
void shutdownWithMessage(String message, int sleepDuration);
void loadICalendar(String urlString);
int widthOfString(String string, int fontSize);

void setup()
{
  // Initialize M5Paper
  M5.begin();
  M5.EPD.SetRotation(90);
  M5.TP.SetRotation(90);
  M5.EPD.Clear(true);

  canvas.createCanvas(screenWidth, screenHeight);
  widthCanvas.createCanvas(columnWidth, rowHeight);

  // Load settings from "settings.txt" in SD card
  String wifiIDString = "wifiID";
  String wifiPWString = "wifiPW";

  File settingFile = SD.open("/settings.txt");
  if (settingFile)
  {
    while (settingFile.available() > 0)
    {
      String line = settingFile.readStringUntil('\n');
      if (line.startsWith("//"))
        continue;
      int separatorLocation = line.indexOf(":");
      if (separatorLocation > -1)
      {
        String key = line.substring(0, separatorLocation);
        String content = line.substring(separatorLocation + 1);

        // WiFi SSID and paassword
        if (key == "SSID")
          wifiIDString = content;

        else if (key == "PASS")
          wifiPWString = content;

        // Font settings
        else if (key == "fontSize")
          fontSize = content.toInt();

        else if (key == "smallFontSize")
          smallFontSize = content.toInt();

        else if (key == "fontName")
        {
          if (content.endsWith(".ttf"))
          {
            fontName = content;
            if (!fontName.startsWith("/"))
            {
              fontName = "/" + fontName;
            }
          }
        }

        // HTTPS access
        else if (key == "pemFileName")
          pemFileName = content;

        else if (key == "iCalendarURL")
          iCalendarURLs.push_back(content);

        else if (key == "timezone")
          timezone = content.toFloat();
      }
    }
    settingFile.close();

    // Start Wifi connection
    WiFi.begin(wifiIDString.c_str(), wifiPWString.c_str());
    Serial.println(wifiIDString);
    // Wait until wifi connected
    int i = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
      i++;
      if (i > 120)
        break;
    }
    Serial.print("\n");

    // Setup NTP
    configTime(60 * 60 * timezone, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

    // load font
    canvas.loadFont(fontName, SD);
    canvas.createRender(fontSize, 256);
    canvas.createRender(smallFontSize, 256);
    canvas.setTextSize(fontSize);
    canvas.setTextColor(WHITE, BLACK);

    // Load PEM file in SD card
    File pemFile = SD.open(pemFileName.c_str());
    if (pemFile)
    {
      rootCA = pemFile.readString();
      pemFile.close();
      Serial.println("pem file loaded:" + pemFileName);
    }
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (!loaded)
  { // First loop after boot
    if (WiFi.status() != WL_CONNECTED)
    {
      // Not connected
      shutdownWithMessage("WiFi not connected", 60*10);
    }
    load();
  }
}

// put function definitions here:
void load()
{
  // Get local time
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Waiting getLocalTime");
    delay(500);
    return;
  }

  int year = timeinfo.tm_year + 1900;
  int month = timeinfo.tm_mon + 1;
  int day = timeinfo.tm_mday;
  currentYear = year;
  currentMonth = month;
  if (currentMonth == 12)
  {
    nextMonthYear = currentYear + 1;
    nextMonth = 1;
  }
  else
  {
    nextMonthYear = currentYear;
    nextMonth = currentMonth + 1;
  }

  Serial.printf("%d/%d/%d\n", year, month, day);

  // Load iCalendar
  for (auto &urlString : iCalendarURLs)
  {
    loadICalendar(urlString);
  }
  // loadICalendar(iCalendarURL);

  // Draw calendar
  int firstDayOfWeek = dayOfWeek(year, month, 1);
  int numberOfDays = numberOfDaysInMonth(year, month);
  int numberOfRows = (firstDayOfWeek + numberOfDays - 1) / 7 + 1;

  // draw horizontal lines
  for (int i = 1; i <= numberOfRows; i++)
  {
    canvas.drawFastHLine(0, i * rowHeight, screenWidth, WHITE);
  }

  // draw vertical lines
  int lineHeight = numberOfRows * rowHeight;
  for (int i = 1; i < 7; i++)
  {
    canvas.drawFastVLine(i * columnWidth, 0, lineHeight, WHITE);
  }

  // draw days in month
  for (int i = 1; i <= numberOfDays; i++)
  {
    int row = (firstDayOfWeek + i - 1) / 7;
    int column = (6 + firstDayOfWeek + i) % 7;

    // invert color if it is today
    uint16_t textColor = WHITE;
    if (day == i)
    {
      canvas.fillRect(column * columnWidth, row * rowHeight, columnWidth, rowHeight, WHITE);
      textColor = BLACK;
    }

    // draw dot
    if (eventsInThisMonth.count(i) > 0)
    {
      canvas.fillCircle(column * columnWidth + columnWidth / 2, row * rowHeight + 10, 3, textColor);

      // add events to list
      if (i >= day)
      {
        auto itr = eventsInThisMonth.lower_bound(i);
        auto last = eventsInThisMonth.upper_bound(i);
        while (itr != last)
        {
          eventsToDisplay.push_back(itr->second);
          ++itr;
        }
      }
    }

    // draw day
    canvas.setTextColor(textColor);
    int dayWidth = widthOfString(String(i), fontSize);
    canvas.setCursor(column * columnWidth + (columnWidth - dayWidth) / 2, row * rowHeight + (rowHeight - fontSize) / 2);
    canvas.printf("%d", i);
  }

  // count events in list
  int count = eventsToDisplay.size();
  if (count > maxNumberOfRows - numberOfRows)
  {
    // Number of events is larger than rows
    count = maxNumberOfRows - numberOfRows;
  }
  else
  {
    // Add events for next month if rows are available
    eventsToDisplay.insert(eventsToDisplay.end(), eventsInNextMonth.begin(), eventsInNextMonth.end());
    count = eventsToDisplay.size();
    if (count > maxNumberOfRows - numberOfRows)
      count = maxNumberOfRows - numberOfRows;
  }
  sort(std::begin(eventsToDisplay), std::end(eventsToDisplay));

  // draw events
  canvas.setTextColor(WHITE);
  boolean boldLineDrawn = false;
  for (int i = 0; i < count; i++)
  {
    PCEvent event = eventsToDisplay[i];
    boolean isToday = (event.getDay() == day);
    Serial.println(event.descriptionForDay(isToday) + event.getTitle());
    canvas.setCursor(10, (numberOfRows + i) * rowHeight + (rowHeight - fontSize) / 2);
    canvas.print(event.descriptionForDay(isToday));
    canvas.setCursor(160, (numberOfRows + i) * rowHeight + (rowHeight - fontSize) / 2);
    canvas.print(event.getTitle());
    canvas.drawFastHLine(8, (numberOfRows + i + 1) * rowHeight, screenWidth - 16, WHITE);

    if (!boldLineDrawn && !isToday)
    {
      canvas.fillRect(8, (numberOfRows + i) * rowHeight, screenWidth - 16, 5, WHITE);
      boldLineDrawn = true;
    }
  }


  // draw date footer
  canvas.setTextSize(smallFontSize);
  canvas.setCursor(8, screenHeight - (smallFontSize + 8));
  canvas.printf("%d/%d/%d", year, month, day);

  // draw battery bar
  float voltage = M5.getBatteryVoltage() / 1000.0;
  float battery = ((voltage - 3.2) / (4.25 - 3.2));
  canvas.fillRect(0, screenHeight - 10, screenWidth * battery, 10, WHITE);

  // push canvas to EPD screen
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
  loaded = true;

  // shutdown until next day
  delay(500);
  RTC_Time wakeUpTime = RTC_Time(0, 0, 0);
  M5.shutdown(wakeUpTime);
}

void shutdownWithMessage(String message, int sleepDuration)
{
  // Display message and shutdown
  canvas.drawString(message, 8, (84 - fontSize) / 2);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
  delay(500);
  if (sleepDuration > 0)
    M5.shutdown(sleepDuration);
  else
    M5.shutdown();
}

void loadICalendar(String urlString)
{
  HTTPClient httpClient;
  httpClient.begin(urlString, rootCA.c_str());
  dateString = "";
  String lastModified = "";

  const char *headerKeys[] = {"Date", "Last-Modified"};
  httpClient.collectHeaders(headerKeys, 1);

  int result = httpClient.GET();
  if (result == HTTP_CODE_OK)
  {
    dateString = httpClient.header("Date");
    lastModified = httpClient.header("Last-Modified");
    Serial.println("Last-Modified:" + lastModified);
    WiFiClient *stream = httpClient.getStreamPtr();
    if (httpClient.connected())
    {
      String eventBlock = "";
      boolean loadingEvent = false;
      while (stream->available())
      {
        String line = stream->readStringUntil('\n');
        if (line.startsWith("BEGIN:VEVENT"))
        { // begin VEVENT block
          loadingEvent = true;
        }
        else if (line.startsWith("DTSTART"))
        { // read start date
          int position = line.indexOf(":");
          String timeString = line.substring(position + 1);
          timeString.trim();
          tm timeInfo = tmFromICalDateString(timeString, timezone);
          if (!(timeInfo.tm_year + 1900 == currentYear && timeInfo.tm_mon + 1 == currentMonth) && !(timeInfo.tm_year + 1900 == nextMonthYear && timeInfo.tm_mon + 1 == nextMonth))
          {
            // discard event if not scheduled in this month to next month 
            loadingEvent = false;
            eventBlock = "";
          }
        }

        if (loadingEvent)
        {
          eventBlock += line + "\n";
        }
        if (loadingEvent && line.startsWith("END:VEVENT"))
        {
          loadingEvent = false;
          PCEvent event = PCEvent(eventBlock, timezone);
          if (event.getMonth() == currentMonth)
          {
            // Will be displayed as this month
            eventsInThisMonth.insert(std::make_pair(event.getDay(), event));
          }
          else
          {
            // Next month
            eventsInNextMonth.push_back(event);
          }
        }
      }
    }
    httpClient.end();
  }
  else
  {
    // HTTP Error
    httpClient.end();
    shutdownWithMessage("HTTP Error Code:" + result, 60*60*24);
  }
}

int widthOfString(String string, int fontSize)
{
  widthCanvas.setTextSize(fontSize);
  widthCanvas.setCursor(0, 0);
  widthCanvas.print(string);
  return widthCanvas.getCursorX();
}