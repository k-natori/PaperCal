# PaperCal
PaperCal is a M5Paper sketch to display your Google Calendar events.

## How to run
1. This sketch requires M5Paper and microSD card (<= 16GB)
2. Put "settings.txt" into microSD. In this settings file you have to specify following
3. Put TTF font file you use into microSD. GenShinGothic-Medium (http://jikasei.me/font/genshin/) is recommended
4. Put PEM file (root CA for Google Calendar, should be google-com.pem) into microSD
5. Get private URL for your iCalendar (You can copy it from Google Calendar)
6. Specify your WiFi, font file name, PEM file name, private URL in "settings.txt"
7. Build and transfer this project as PlatformIO project

## How it works
Once transfered and run, it displays your Google Calendar events in this month. It shutdown power automatically and reboot on next 0:00AM.
You can specify multiple iCalendar URLs (See an example of settings.txt")

## Limitations
This sketch has following limications to display calendar:
- Not showing recurring events
- Not showing 
- Date format is YYYY/MM/DD (Japan)

## Dependencies
This PlatformIO project depends on following libraries:
- M5EPD https://github.com/m5stack/M5EPD
