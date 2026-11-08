// Timers, reminders, alert queue, and saving to flash.
#include "app.h"
#include "config.h"
#include <Preferences.h>

Timer timers[MAX_TIMERS];
Reminder reminders[MAX_REMINDERS];
Alert alerts[MAX_ALERTS];
int alertCount = 0;
uint32_t alertStartMs = 0;
uint32_t dataVersion = 1;
uint32_t alertVersion = 1;
Settings settings = {60, 200, 0, 2, 1};

static Preferences prefs;

void markChanged() { dataVersion++; }

static void remindersPersist() { prefs.putBytes("rem", reminders, sizeof(reminders)); }

void settingsSave() { prefs.putBytes("set", &settings, sizeof(settings)); }

void modelInit() {
  prefs.begin("cydtimer", false);
  if (prefs.getBytesLength("rem") == sizeof(reminders))
    prefs.getBytes("rem", reminders, sizeof(reminders));
  size_t n = prefs.getBytesLength("set");
  if (n && n <= sizeof(settings)) prefs.getBytes("set", &settings, n); // older, shorter saves keep new defaults
  if (settings.brightness < 10) settings.brightness = 10;
  if (settings.volume > 100) settings.volume = 100;
  settings.accent %= 4;
  settings.dimIdx %= 4;
}

bool timeValid() { return time(nullptr) > 1700000000; }

// ---------------- timers ----------------

int timerStart(uint32_t seconds, const char *label) {
  if (seconds == 0) return -1;
  for (int i = 0; i < MAX_TIMERS; i++) {
    Timer &t = timers[i];
    if (t.active) continue;
    memset(&t, 0, sizeof(t));
    t.active = true;
    t.durationS = seconds;
    t.endMs = millis() + seconds * 1000UL;
    if (label) strlcpy(t.label, label, sizeof(t.label));
    markChanged();
    return i;
  }
  return -1;
}

void timerCancel(int i) {
  if (i < 0 || i >= MAX_TIMERS) return;
  timers[i].active = false;
  markChanged();
}

uint32_t timerRemainingMs(const Timer &t) {
  if (!t.active) return 0;
  if (t.paused) return t.remainMs;
  int32_t r = (int32_t)(t.endMs - millis());
  return r > 0 ? r : 0;
}

void timerTogglePause(int i) {
  if (i < 0 || i >= MAX_TIMERS || !timers[i].active) return;
  Timer &t = timers[i];
  if (t.paused) {
    t.endMs = millis() + t.remainMs;
    t.paused = false;
  } else {
    t.remainMs = timerRemainingMs(t);
    t.paused = true;
  }
  markChanged();
}

void timerDisplayLabel(int i, char *buf, size_t n) {
  const Timer &t = timers[i];
  if (t.label[0]) {
    strlcpy(buf, t.label, n);
    return;
  }
  char d[16];
  formatDuration(d, sizeof(d), t.durationS * 1000UL);
  snprintf(buf, n, "%s timer", d);
}

// ---------------- reminders ----------------

int reminderSave(int idx, const Reminder &r) {
  if (idx < 0) {
    for (int i = 0; i < MAX_REMINDERS; i++)
      if (!reminders[i].used) { idx = i; break; }
    if (idx < 0) return -1;
  }
  if (idx >= MAX_REMINDERS) return -1;
  Reminder old = reminders[idx];
  Reminder &dst = reminders[idx];
  dst = r;
  dst.used = true;
  if (!dst.text[0]) strlcpy(dst.text, "Reminder", sizeof(dst.text));
  // Editing a reminder without moving its time shouldn't make it go off twice today
  dst.lastFired = (old.used && old.hour == r.hour && old.minute == r.minute) ? old.lastFired : 0;
  remindersPersist();
  markChanged();
  return idx;
}

void reminderDelete(int i) {
  if (i < 0 || i >= MAX_REMINDERS) return;
  memset(&reminders[i], 0, sizeof(Reminder));
  remindersPersist();
  markChanged();
}

void reminderSetEnabled(int i, bool on) {
  if (i < 0 || i >= MAX_REMINDERS || !reminders[i].used) return;
  reminders[i].enabled = on;
  remindersPersist();
  markChanged();
}

int nextReminder(time_t *when) {
  if (!timeValid()) return -1;
  time_t now = time(nullptr);
  struct tm base;
  localtime_r(&now, &base);
  int best = -1;
  time_t bestT = 0;
  for (int i = 0; i < MAX_REMINDERS; i++) {
    const Reminder &r = reminders[i];
    if (!r.used || !r.enabled) continue;
    for (int d = 0; d <= 7; d++) {
      struct tm t = base;
      t.tm_mday += d;
      t.tm_hour = r.hour;
      t.tm_min = r.minute;
      t.tm_sec = 0;
      t.tm_isdst = -1;
      time_t ts = mktime(&t); // also fills in t.tm_wday
      if (ts <= now) continue;
      if (r.days && !((r.days >> t.tm_wday) & 1)) continue;
      if (best < 0 || ts < bestT) { best = i; bestT = ts; }
      break;
    }
  }
  if (when) *when = bestT;
  return best;
}

// ---------------- alerts ----------------

void alertPush(const char *title, const char *text, uint16_t snoozeMin) {
  if (alertCount == MAX_ALERTS) { // drop the oldest
    memmove(&alerts[0], &alerts[1], sizeof(Alert) * (MAX_ALERTS - 1));
    alertCount--;
  }
  Alert &a = alerts[alertCount++];
  strlcpy(a.title, title, sizeof(a.title));
  strlcpy(a.text, text, sizeof(a.text));
  a.snoozeMin = snoozeMin;
  alertStartMs = millis();
  alertVersion++;
}

void alertPop() {
  if (alertCount == 0) return;
  memmove(&alerts[0], &alerts[1], sizeof(Alert) * (MAX_ALERTS - 1));
  alertCount--;
  alertStartMs = millis();
  alertVersion++;
}

// ---------------- tick ----------------

void modelTick() {
  uint32_t now = millis();
  for (int i = 0; i < MAX_TIMERS; i++) {
    Timer &t = timers[i];
    if (!t.active || t.paused || (int32_t)(now - t.endMs) < 0) continue;
    char label[40];
    timerDisplayLabel(i, label, sizeof(label));
    t.active = false;
    alertPush("Time's up!", label, TIMER_SNOOZE_MIN);
    markChanged();
  }

  static uint32_t lastCheck = 0;
  if (now - lastCheck < 1000) return;
  lastCheck = now;
  if (!timeValid()) return;

  time_t tnow = time(nullptr);
  struct tm tm;
  localtime_r(&tnow, &tm);
  uint32_t dayKey = (tm.tm_year + 1900) * 1000 + tm.tm_yday;
  bool dirty = false;
  for (int i = 0; i < MAX_REMINDERS; i++) {
    Reminder &r = reminders[i];
    if (!r.used || !r.enabled) continue;
    if (r.hour != tm.tm_hour || r.minute != tm.tm_min || r.lastFired == dayKey) continue;
    if (r.days && !((r.days >> tm.tm_wday) & 1)) continue;
    r.lastFired = dayKey;
    if (r.days == 0) r.enabled = false; // one-time reminder is done
    alertPush("Reminder", r.text, REMINDER_SNOOZE_MIN);
    dirty = true;
  }
  if (dirty) {
    remindersPersist();
    markChanged();
  }
}

// ---------------- formatting ----------------

void formatClock(char *buf, size_t n, uint8_t h, uint8_t m) {
#if USE_12H
  snprintf(buf, n, "%d:%02d %s", h % 12 ? h % 12 : 12, m, h < 12 ? "AM" : "PM");
#else
  snprintf(buf, n, "%02d:%02d", h, m);
#endif
}

void formatDays(char *buf, size_t n, uint8_t days) {
  static const char *names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  days &= 0x7F;
  if (days == 0) { strlcpy(buf, "Once", n); return; }
  if (days == 0x7F) { strlcpy(buf, "Every day", n); return; }
  if (days == 0x3E) { strlcpy(buf, "Weekdays", n); return; }
  if (days == 0x41) { strlcpy(buf, "Weekends", n); return; }
  buf[0] = 0;
  for (int i = 0; i < 7; i++) {
    if (!((days >> i) & 1)) continue;
    if (buf[0]) strlcat(buf, " ", n);
    strlcat(buf, names[i], n);
  }
}

void formatDuration(char *buf, size_t n, uint32_t ms) {
  uint32_t s = (ms + 999) / 1000;
  uint32_t h = s / 3600, m = (s / 60) % 60, sec = s % 60;
  if (h) snprintf(buf, n, "%lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)sec);
  else snprintf(buf, n, "%lu:%02lu", (unsigned long)m, (unsigned long)sec);
}
