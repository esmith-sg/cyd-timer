#pragma once

// WiFi name/password live in secrets.h (edit that file).
#include "secrets.h"

// POSIX timezone string. US Pacific with daylight saving:
//   Eastern  "EST5EDT,M3.2.0,M11.1.0"   Central "CST6CDT,M3.2.0,M11.1.0"
//   Mountain "MST7MDT,M3.2.0,M11.1.0"   Arizona "MST7"
#define TIMEZONE "PST8PDT,M3.2.0,M11.1.0"

// Device shows up as http://cyd-timer.local on your network
#define HOSTNAME "cyd-timer"

#define USE_12H 0               // 1 = 12-hour clock with AM/PM
#define REMINDER_SNOOZE_MIN 5   // "Snooze" on a reminder alert
#define TIMER_SNOOZE_MIN 1      // "+1 min" on a finished timer
#define ALERT_SOUND_SECONDS 120 // beeping stops after this; the alert stays on screen

// "Extra dim at night" window for ambient mode (24h clock hours)
#define NIGHT_START_HOUR 22
#define NIGHT_END_HOUR 7
