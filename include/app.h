#pragma once
#include <Arduino.h>
#include <time.h>

#define MAX_TIMERS 6
#define MAX_REMINDERS 24
#define MAX_ALERTS 8

struct Timer {
  bool active;
  bool paused;
  uint32_t durationS;
  uint32_t endMs;    // millis() when it finishes (while running)
  uint32_t remainMs; // remaining time (while paused)
  char label[32];
};

struct Reminder {
  bool used;
  bool enabled;
  uint8_t hour, minute;
  uint8_t days;       // bit0 = Sunday ... bit6 = Saturday; 0 = one-time
  char text[48];
  uint32_t lastFired; // (year * 1000 + day-of-year) it last went off
};

struct Alert {
  char title[24];
  char text[48];
  uint16_t snoozeMin;
};

// Append new fields at the end only: saved settings are loaded by prefix.
struct Settings {
  uint8_t volume;     // 0..100
  uint8_t brightness; // 10..255
  uint8_t accent;     // 0 green, 1 blue, 2 yellow, 3 orange
  uint8_t dimIdx;     // ambient mode after: 0 never, 1 30s, 2 1min, 3 5min
  uint8_t nightDim;   // extra-dim ambient mode at night
};

extern Timer timers[MAX_TIMERS];
extern Reminder reminders[MAX_REMINDERS];
extern Alert alerts[MAX_ALERTS];
extern int alertCount;
extern uint32_t alertStartMs;
extern uint32_t dataVersion;  // bumps whenever timers/reminders change
extern uint32_t alertVersion; // bumps whenever the alert queue changes
extern Settings settings;

// ---- model.cpp ----
void modelInit();
void modelTick();
void markChanged();
int timerStart(uint32_t seconds, const char *label);
void timerCancel(int i);
void timerTogglePause(int i);
uint32_t timerRemainingMs(const Timer &t);
void timerDisplayLabel(int i, char *buf, size_t n);
int reminderSave(int idx, const Reminder &r); // idx -1 = new; returns slot or -1 if full
void reminderDelete(int i);
void reminderSetEnabled(int i, bool on);
void settingsSave();
bool timeValid();
int nextReminder(time_t *when);
void alertPush(const char *title, const char *text, uint16_t snoozeMin);
void alertPop();
void formatClock(char *buf, size_t n, uint8_t h, uint8_t m);
void formatDays(char *buf, size_t n, uint8_t days);
void formatDuration(char *buf, size_t n, uint32_t ms);

// ---- main.cpp (hardware) ----
void setBacklight(uint8_t v);
void soundTest();
void requestTouchCalibration();

// ---- ui.cpp ----
void uiInit();
void uiTick();

// ---- web.cpp ----
void webInit();
void webTick();
