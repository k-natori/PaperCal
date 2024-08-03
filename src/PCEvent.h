#ifndef PCEVENT_H_INCLUDE
#define PCEVENT_H_INCLUDE

#include <Arduino.h>
#include <map>
#include <time.h>

int dayOfWeek(int year, int month, int day);
int numberOfDaysInMonth(int year, int month);
tm tmFromICalDateString(String iCalDateString, float toTimezone);


class PCEvent
{
public:
    PCEvent(String sourceString, float toTimezone);
    time_t getTimeT() const;
    int getYear();
    int getMonth();
    int getDay();
    int getDayOfWeek();
    int getHour();
    int getMinute();
    int getSecond();
    String descriptionForDay(boolean isToday);
    double duration();
    String getTitle();
    boolean isHolidayEvent;

    static float defaultTimezone;
    static int currentYear;
    static int currentMonth;
    static int currentDay;
    static int nextMonthYear;
    static int nextMonth;
    static void setRootCA(String newRootCA);
    static void setTimeInfo(tm timeInfo);
    static boolean loadICalendar(String urlString, boolean holiday);
    static int numberOfEventsInDayOfThisMonth(int day);
    static std::vector<PCEvent> eventsInDayOfThisMonth(int day);
    static int numberOfHolidaysInDayOfThisMonth(int day);
    static std::vector<PCEvent> holidaysInDayOfThisMonth(int day);
    static std::vector<PCEvent> getEventsInNextMonth();

private:
    tm startTM;
    tm endTM;
    boolean isDayEvent;
    float timezone;
    String title;

    static String rootCA;
    static std::multimap<int, PCEvent> eventsInThisMonth;
    static std::multimap<int, PCEvent> holidaysInThisMonth;
    static std::vector<PCEvent> eventsInNextMonth;
};

bool operator<(const PCEvent&left, const PCEvent&right) ;
bool operator>(const PCEvent&left, const PCEvent&right) ;

#endif