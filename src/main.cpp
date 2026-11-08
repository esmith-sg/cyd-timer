// CYD Timer — hardware setup, display/touch drivers, alert sound + LED, WiFi.
#include <Arduino.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <lvgl.h>

#include "app.h"
#include "config.h"

// E32R35T pins (from lcdwiki.com)
#define PIN_BL 27      // backlight, high = on
#define PIN_SPK 26     // DAC out -> speaker amp
#define PIN_AMP_EN 4   // amp enable, LOW = on
#define PIN_LED_R 22   // RGB LED, common anode (LOW = on)
#define PIN_LED_G 16
#define PIN_LED_B 17
#define PIN_BOOT 0     // hold at power-on to recalibrate touch

#define CH_SPK 0
#define CH_BL 1

static TFT_eSPI tft;
static lv_disp_draw_buf_t drawBuf;
static const uint32_t BUF_PX = 480 * 24;
static Preferences hwPrefs;

// ---------------- backlight & sound ----------------

void setBacklight(uint8_t v) { ledcWrite(CH_BL, v); }

static void playTone(uint16_t freq) {
  static bool ampOn = false;
  if (freq == 0 || settings.volume == 0) {
    ledcWrite(CH_SPK, 0);
    if (ampOn) { digitalWrite(PIN_AMP_EN, HIGH); ampOn = false; }
    return;
  }
  if (!ampOn) { digitalWrite(PIN_AMP_EN, LOW); ampOn = true; }
  ledcWriteTone(CH_SPK, freq); // 10-bit resolution, 50% duty
  uint32_t v = settings.volume;
  uint32_t duty = 512UL * v * v / 10000UL; // roughly perceptual volume curve
  ledcWrite(CH_SPK, duty ? duty : 1);
}

static uint32_t testBeepUntil = 0;
void soundTest() { testBeepUntil = millis() + 300; }

static void alertOutputTick() {
  uint32_t now = millis();
  uint32_t since = now - alertStartMs;
  bool ringing = alertCount > 0 && since < ALERT_SOUND_SECONDS * 1000UL;
  uint16_t f = 0;
  bool red = false;
  if (ringing) {
    uint32_t p = since % 1400; // beep-beep-beep ... pause
    if (p < 120 || (p >= 200 && p < 320) || (p >= 400 && p < 520)) f = 2700;
    red = p < 700;
  } else if (alertCount > 0) {
    red = (now / 1000) % 2; // silent but unacknowledged: slow blink
  }
  if (!ringing && (int32_t)(testBeepUntil - now) > 0) f = 2700;

  static uint16_t lastF = 0xFFFF;
  if (f != lastF) { playTone(f); lastF = f; }
  digitalWrite(PIN_LED_R, red ? LOW : HIGH);
}

// ---------------- display & touch ----------------

static void flushCb(lv_disp_drv_t *d, const lv_area_t *a, lv_color_t *c) {
  uint32_t w = lv_area_get_width(a), h = lv_area_get_height(a);
  tft.startWrite();
  tft.setAddrWindow(a->x1, a->y1, w, h);
  tft.pushColors((uint16_t *)&c->full, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(d);
}

static void touchCb(lv_indev_drv_t *, lv_indev_data_t *data) {
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

static void calibrateTouch() {
  uint16_t cal[5];
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Touch screen setup", 240, 130, 2);
  tft.drawString("Press each corner arrow firmly (fingernail or stylus)", 240, 160, 2);
  tft.calibrateTouch(cal, TFT_MAGENTA, TFT_BLACK, 15);
  hwPrefs.putBytes("cal", cal, sizeof(cal));
  hwPrefs.putBool("recal", false);
  tft.fillScreen(TFT_BLACK);
}

void requestTouchCalibration() {
  hwPrefs.putBool("recal", true);
  delay(100);
  ESP.restart();
}

// ---------------- WiFi ----------------

static void netTick() {
  static bool wasUp = false;
  bool up = WiFi.status() == WL_CONNECTED;
  if (up && !wasUp) {
    Serial.printf("WiFi connected: http://%s  (http://%s.local)\n",
                  WiFi.localIP().toString().c_str(), HOSTNAME);
    MDNS.end();
    if (MDNS.begin(HOSTNAME)) MDNS.addService("http", "tcp", 80);
  }
  wasUp = up;
}

// ---------------- setup / loop ----------------

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  digitalWrite(PIN_LED_R, HIGH);
  digitalWrite(PIN_LED_G, HIGH);
  digitalWrite(PIN_LED_B, HIGH);
  pinMode(PIN_AMP_EN, OUTPUT);
  digitalWrite(PIN_AMP_EN, HIGH);
  pinMode(PIN_BOOT, INPUT_PULLUP);

  ledcSetup(CH_SPK, 2000, 10);
  ledcAttachPin(PIN_SPK, CH_SPK);
  ledcWrite(CH_SPK, 0);

  modelInit();
  hwPrefs.begin("cydhw", false);

  tft.init();
  tft.setRotation(1); // landscape 480x320
  tft.fillScreen(TFT_BLACK);
  ledcSetup(CH_BL, 5000, 8);
  ledcAttachPin(PIN_BL, CH_BL);
  setBacklight(settings.brightness);

  uint16_t cal[5];
  bool haveCal = hwPrefs.getBytesLength("cal") == sizeof(cal);
  if (!haveCal || hwPrefs.getBool("recal", false) || digitalRead(PIN_BOOT) == LOW) {
    calibrateTouch();
  }
  hwPrefs.getBytes("cal", cal, sizeof(cal));
  tft.setTouch(cal);

  lv_init();
  lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(BUF_PX * sizeof(lv_color_t), MALLOC_CAP_DMA);
  lv_disp_draw_buf_init(&drawBuf, buf1, nullptr, BUF_PX);
  static lv_disp_drv_t dispDrv;
  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = 480;
  dispDrv.ver_res = 320;
  dispDrv.flush_cb = flushCb;
  dispDrv.draw_buf = &drawBuf;
  lv_disp_drv_register(&dispDrv);

  static lv_indev_drv_t indevDrv;
  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = touchCb;
  lv_indev_drv_register(&indevDrv);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  configTzTime(TIMEZONE, "pool.ntp.org", "time.nist.gov");

  uiInit();
  webInit();
}

void loop() {
  lv_timer_handler();
  modelTick();
  alertOutputTick();
  uiTick();
  webTick();
  netTick();
  delay(5);
}
