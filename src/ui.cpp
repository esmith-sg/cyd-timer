// LVGL touch UI: Clock / Timers / Reminders / Settings tabs, alert overlay, ambient mode.
#include <WiFi.h>
#include <lvgl.h>

#include "app.h"
#include "config.h"

LV_FONT_DECLARE(font_clock_150);
LV_FONT_DECLARE(font_raj_48);
LV_FONT_DECLARE(font_raj_34);
LV_FONT_DECLARE(font_raj_26);

// ---------------- palette ----------------
#define C_BG lv_color_hex(0x0A1628)
#define C_SURF lv_color_hex(0x12243F)
#define C_SURF2 lv_color_hex(0x1C3559)
#define C_LINE lv_color_hex(0x27456F)
#define C_TEXT lv_color_hex(0xE6EEF9)
#define C_DIM lv_color_hex(0x8296B8)
#define C_GREEN lv_color_hex(0x3DDC84)
#define C_YELLOW lv_color_hex(0xFFC93C)
#define C_ORANGE lv_color_hex(0xFF8A3D)
#define C_BLUE lv_color_hex(0x3D8BFD)
#define C_DANGER lv_color_hex(0xE5533D)

static const uint32_t ACCENTS[] = {0x3DDC84, 0x3D8BFD, 0xFFC93C, 0xFF8A3D};
static const uint16_t DIM_AFTER_S[] = {0, 30, 60, 300};
static lv_color_t accent() { return lv_color_hex(ACCENTS[settings.accent % 4]); }

#define IDX(e) ((int)(intptr_t)lv_event_get_user_data(e))
#define UD(i) ((void *)(intptr_t)(i))

// ---------------- theme ----------------

static lv_style_t stScr, stCard, stTransp, stBtn, stBtnPr, stItems, stItemsPr, stItemsChk, stField, stFieldFocus,
    stSel, stIndic, stKnob, stTabBar, stTabItems, stTabItemsChk, stKb, stAccentText;
static lv_style_transition_dsc_t trPress, trRelease;
static const lv_style_prop_t trProps[] = {LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_TRANSFORM_HEIGHT, LV_STYLE_PROP_INV};
static lv_color_filter_dsc_t darkFilter;
static lv_theme_t theme;

static lv_color_t darkenCb(const lv_color_filter_dsc_t *, lv_color_t c, lv_opa_t opa) { return lv_color_darken(c, opa); }

static void stylesAccent() {
  lv_color_t a = accent();
  lv_style_set_bg_color(&stBtn, a);
  lv_style_set_bg_color(&stItemsChk, a);
  lv_style_set_border_color(&stFieldFocus, a);
  lv_style_set_bg_color(&stSel, a);
  lv_style_set_bg_color(&stIndic, a);
  lv_style_set_border_color(&stTabItemsChk, a);
  lv_style_set_text_color(&stAccentText, a);
  lv_obj_report_style_change(nullptr);
}

static void stylesInit() {
  lv_color_filter_dsc_init(&darkFilter, darkenCb);
  lv_style_transition_dsc_init(&trPress, trProps, lv_anim_path_ease_out, 70, 0, nullptr);
  lv_style_transition_dsc_init(&trRelease, trProps, lv_anim_path_overshoot, 260, 0, nullptr);

  lv_style_init(&stScr);
  lv_style_set_bg_color(&stScr, C_BG);
  lv_style_set_bg_opa(&stScr, LV_OPA_COVER);
  lv_style_set_text_color(&stScr, C_TEXT);
  lv_style_set_border_width(&stScr, 0);

  lv_style_init(&stCard);
  lv_style_set_bg_color(&stCard, C_SURF);
  lv_style_set_bg_opa(&stCard, LV_OPA_COVER);
  lv_style_set_border_width(&stCard, 0);
  lv_style_set_shadow_width(&stCard, 0);
  lv_style_set_radius(&stCard, 16);
  lv_style_set_text_color(&stCard, C_TEXT);

  lv_style_init(&stTransp);
  lv_style_set_bg_opa(&stTransp, LV_OPA_TRANSP);
  lv_style_set_border_width(&stTransp, 0);
  lv_style_set_shadow_width(&stTransp, 0);

  lv_style_init(&stBtn);
  lv_style_set_text_color(&stBtn, C_BG);
  lv_style_set_radius(&stBtn, 14);
  lv_style_set_shadow_width(&stBtn, 0);
  lv_style_set_border_width(&stBtn, 0);
  lv_style_set_transition(&stBtn, &trRelease);

  lv_style_init(&stBtnPr); // springy press: shrink a little, darken, bounce back on release
  lv_style_set_transform_width(&stBtnPr, -4);
  lv_style_set_transform_height(&stBtnPr, -4);
  lv_style_set_color_filter_dsc(&stBtnPr, &darkFilter);
  lv_style_set_color_filter_opa(&stBtnPr, LV_OPA_20);
  lv_style_set_transition(&stBtnPr, &trPress);

  lv_style_init(&stItems);
  lv_style_set_bg_color(&stItems, C_SURF2);
  lv_style_set_bg_opa(&stItems, LV_OPA_COVER);
  lv_style_set_text_color(&stItems, C_TEXT);
  lv_style_set_radius(&stItems, 12);
  lv_style_set_border_width(&stItems, 0);
  lv_style_set_shadow_width(&stItems, 0);

  lv_style_init(&stItemsPr);
  lv_style_set_color_filter_dsc(&stItemsPr, &darkFilter);
  lv_style_set_color_filter_opa(&stItemsPr, LV_OPA_30);

  lv_style_init(&stItemsChk);
  lv_style_set_text_color(&stItemsChk, C_BG);

  lv_style_init(&stField);
  lv_style_set_bg_color(&stField, C_SURF2);
  lv_style_set_bg_opa(&stField, LV_OPA_COVER);
  lv_style_set_border_width(&stField, 0);
  lv_style_set_radius(&stField, 12);
  lv_style_set_text_color(&stField, C_TEXT);

  lv_style_init(&stFieldFocus);
  lv_style_set_border_width(&stFieldFocus, 2);

  lv_style_init(&stSel);
  lv_style_set_bg_opa(&stSel, LV_OPA_COVER);
  lv_style_set_text_color(&stSel, C_BG);

  lv_style_init(&stIndic);
  lv_style_set_bg_opa(&stIndic, LV_OPA_COVER);

  lv_style_init(&stKnob);
  lv_style_set_bg_color(&stKnob, C_TEXT);
  lv_style_set_bg_opa(&stKnob, LV_OPA_COVER);

  lv_style_init(&stTabBar);
  lv_style_set_bg_color(&stTabBar, C_BG);
  lv_style_set_bg_opa(&stTabBar, LV_OPA_COVER);
  lv_style_set_border_side(&stTabBar, LV_BORDER_SIDE_BOTTOM);
  lv_style_set_border_width(&stTabBar, 1);
  lv_style_set_border_color(&stTabBar, C_LINE);

  lv_style_init(&stTabItems);
  lv_style_set_text_color(&stTabItems, C_DIM);
  lv_style_set_bg_opa(&stTabItems, LV_OPA_TRANSP);

  lv_style_init(&stTabItemsChk);
  lv_style_set_text_color(&stTabItemsChk, C_TEXT);
  lv_style_set_bg_opa(&stTabItemsChk, LV_OPA_TRANSP);
  lv_style_set_border_side(&stTabItemsChk, LV_BORDER_SIDE_BOTTOM);
  lv_style_set_border_width(&stTabItemsChk, 3);

  lv_style_init(&stKb);
  lv_style_set_bg_color(&stKb, C_BG);
  lv_style_set_bg_opa(&stKb, LV_OPA_COVER);
  lv_style_set_border_width(&stKb, 0);
  lv_style_set_pad_all(&stKb, 4);
  lv_style_set_pad_gap(&stKb, 4);

  lv_style_init(&stAccentText);
  stylesAccent();
}

static bool parentIs(lv_obj_t *o, const lv_obj_class_t *c) {
  lv_obj_t *p = o ? lv_obj_get_parent(o) : nullptr;
  return p && lv_obj_check_type(p, c);
}

static void themeApply(lv_theme_t *, lv_obj_t *o) {
  lv_obj_t *par = lv_obj_get_parent(o);
  if (!par || lv_obj_check_type(o, &lv_tabview_class)) { lv_obj_add_style(o, &stScr, 0); return; }
  if (lv_obj_check_type(o, &lv_obj_class)) {
    // tabview content area and tab pages are see-through; everything else is a card
    if (lv_obj_check_type(par, &lv_tabview_class) || parentIs(par, &lv_tabview_class)) lv_obj_add_style(o, &stTransp, 0);
    else lv_obj_add_style(o, &stCard, 0);
    return;
  }
  if (lv_obj_check_type(o, &lv_btn_class)) {
    lv_obj_add_style(o, &stBtn, 0);
    lv_obj_add_style(o, &stBtnPr, LV_STATE_PRESSED);
    return;
  }
  if (lv_obj_check_type(o, &lv_keyboard_class) || lv_obj_check_type(o, &lv_btnmatrix_class)) {
    if (lv_obj_check_type(par, &lv_tabview_class)) {
      lv_obj_add_style(o, &stTabBar, 0);
      lv_obj_add_style(o, &stTabItems, LV_PART_ITEMS);
      lv_obj_add_style(o, &stTabItemsChk, LV_PART_ITEMS | LV_STATE_CHECKED);
      return;
    }
    lv_obj_add_style(o, lv_obj_check_type(o, &lv_keyboard_class) ? &stKb : &stTransp, 0);
    lv_obj_add_style(o, &stItems, LV_PART_ITEMS);
    lv_obj_add_style(o, &stItemsPr, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_style(o, &stSel, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_add_style(o, &stItemsChk, LV_PART_ITEMS | LV_STATE_CHECKED);
    return;
  }
  if (lv_obj_check_type(o, &lv_roller_class)) {
    lv_obj_add_style(o, &stField, 0);
    lv_obj_add_style(o, &stSel, LV_PART_SELECTED);
    return;
  }
  if (lv_obj_check_type(o, &lv_textarea_class)) {
    lv_obj_add_style(o, &stField, 0);
    lv_obj_add_style(o, &stFieldFocus, LV_STATE_FOCUSED);
    return;
  }
  if (lv_obj_check_type(o, &lv_slider_class)) {
    lv_obj_add_style(o, &stField, 0);
    lv_obj_add_style(o, &stIndic, LV_PART_INDICATOR);
    lv_obj_add_style(o, &stKnob, LV_PART_KNOB);
    return;
  }
  if (lv_obj_check_type(o, &lv_switch_class)) {
    lv_obj_add_style(o, &stField, 0);
    lv_obj_add_style(o, &stIndic, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_style(o, &stKnob, LV_PART_KNOB);
  }
}

// ---------------- animation helpers ----------------

static void exTY(void *o, int32_t v) { lv_obj_set_style_translate_y((lv_obj_t *)o, v, 0); }
static void exTX(void *o, int32_t v) { lv_obj_set_style_translate_x((lv_obj_t *)o, v, 0); }
static void exOpa(void *o, int32_t v) { lv_obj_set_style_opa((lv_obj_t *)o, v, 0); }
static void exTextOpa(void *o, int32_t v) { lv_obj_set_style_text_opa((lv_obj_t *)o, v, 0); }
static void exBgOpa(void *o, int32_t v) { lv_obj_set_style_bg_opa((lv_obj_t *)o, v, 0); }
static void exBorderW(void *o, int32_t v) { lv_obj_set_style_border_width((lv_obj_t *)o, v, 0); }

static void anim(void *o, lv_anim_exec_xcb_t cb, int32_t from, int32_t to, uint32_t ms,
                 lv_anim_path_cb_t path = lv_anim_path_ease_out, uint32_t delay = 0,
                 lv_anim_ready_cb_t ready = nullptr) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, o);
  lv_anim_set_exec_cb(&a, cb);
  lv_anim_set_values(&a, from, to);
  lv_anim_set_time(&a, ms);
  lv_anim_set_path_cb(&a, path);
  lv_anim_set_delay(&a, delay);
  lv_anim_set_early_apply(&a, true);
  if (ready) lv_anim_set_ready_cb(&a, ready);
  lv_anim_start(&a);
}

static void loopAnim(void *o, lv_anim_exec_xcb_t cb, int32_t from, int32_t to, uint32_t ms, uint32_t pause) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, o);
  lv_anim_set_exec_cb(&a, cb);
  lv_anim_set_values(&a, from, to);
  lv_anim_set_time(&a, ms);
  lv_anim_set_playback_time(&a, ms);
  lv_anim_set_repeat_delay(&a, pause);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);
}

static void delParentReady(lv_anim_t *a) { lv_obj_del_async(lv_obj_get_parent((lv_obj_t *)a->var)); }

// ---------------- widget helpers ----------------

static lv_obj_t *mkLabel(lv_obj_t *p, const char *txt, const lv_font_t *font = nullptr, lv_color_t *color = nullptr) {
  lv_obj_t *l = lv_label_create(p);
  lv_label_set_text(l, txt);
  if (font) lv_obj_set_style_text_font(l, font, 0);
  if (color) lv_obj_set_style_text_color(l, *color, 0);
  return l;
}
static lv_obj_t *mkLabelC(lv_obj_t *p, const char *txt, const lv_font_t *font, lv_color_t color) {
  return mkLabel(p, txt, font, &color);
}

static lv_obj_t *mkBtn(lv_obj_t *p, const char *txt, lv_event_cb_t cb, void *ud, lv_coord_t w, lv_coord_t h) {
  lv_obj_t *b = lv_btn_create(p);
  lv_obj_set_size(b, w, h);
  if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_center(l);
  return b;
}
static void btnColor(lv_obj_t *b, lv_color_t bg, lv_color_t fg) {
  lv_obj_set_style_bg_color(b, bg, 0);
  lv_obj_set_style_text_color(b, fg, 0);
}

static lv_obj_t *mkBox(lv_obj_t *p, lv_flex_flow_t flow) { // invisible layout container
  lv_obj_t *c = lv_obj_create(p);
  lv_obj_remove_style_all(c);
  lv_obj_set_size(c, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(c, flow);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  return c;
}

static lv_obj_t *mkCard(lv_obj_t *p, lv_coord_t w, lv_coord_t h) {
  lv_obj_t *c = lv_obj_create(p);
  lv_obj_set_size(c, w, h);
  lv_obj_set_style_pad_all(c, 10, 0);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  return c;
}

static lv_obj_t *mkRing(lv_obj_t *p, lv_coord_t size, lv_coord_t w) {
  lv_obj_t *a = lv_arc_create(p);
  lv_obj_set_size(a, size, size);
  lv_arc_set_rotation(a, 270);
  lv_arc_set_bg_angles(a, 0, 360);
  lv_arc_set_range(a, 0, 1000);
  lv_arc_set_value(a, 1000);
  lv_obj_remove_style(a, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(a, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_pad_all(a, 0, 0);
  lv_obj_set_style_bg_opa(a, LV_OPA_TRANSP, 0);
  lv_obj_set_style_arc_width(a, w, LV_PART_MAIN);
  lv_obj_set_style_arc_width(a, w, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(a, C_SURF2, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(a, true, LV_PART_INDICATOR);
  return a;
}

// colour band for a timer ring: 0 green, 1 yellow, 2 orange, 3 paused
static int ringBand(const Timer &t, uint32_t rem) {
  if (t.paused) return 3;
  uint32_t total = t.durationS * 1000UL;
  if (rem * 2 > total) return 0;
  if (rem * 5 > total) return 1;
  return 2;
}
static lv_color_t bandColor(int b) {
  switch (b) {
  case 0: return C_GREEN;
  case 1: return C_YELLOW;
  case 2: return C_ORANGE;
  default: return C_DIM;
  }
}

// Update a ring to show a timer. band caches the colour so we only restyle on change.
static void ringShow(lv_obj_t *arc, const Timer &t, uint32_t rem, int8_t *band) {
  uint32_t total = t.durationS * 1000UL;
  lv_arc_set_value(arc, total ? (int32_t)((uint64_t)rem * 1000 / total) : 0);
  int b = ringBand(t, rem);
  if (b != *band) {
    *band = b;
    lv_obj_set_style_arc_color(arc, bandColor(b), LV_PART_INDICATOR);
  }
  // last 10 seconds: the ring breathes
  lv_opa_t opa = LV_OPA_COVER;
  if (!t.paused && rem < 10000) opa = 150 + (lv_trigo_sin((millis() / 3) % 360) + 32767) * 105 / 65534;
  if (lv_obj_get_style_arc_opa(arc, LV_PART_INDICATOR) != opa) lv_obj_set_style_arc_opa(arc, opa, LV_PART_INDICATOR);
}

static lv_obj_t *mkModalBg() {
  lv_obj_t *bg = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(bg);
  lv_obj_set_size(bg, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(bg, C_BG, 0);
  lv_obj_add_flag(bg, LV_OBJ_FLAG_CLICKABLE);
  anim(bg, exBgOpa, 0, LV_OPA_70, 200);
  return bg;
}

static void modalClose(lv_obj_t *panel) { // slide the panel away, then delete the whole modal
  lv_obj_t *bg = lv_obj_get_parent(panel);
  lv_obj_clear_flag(bg, LV_OBJ_FLAG_CLICKABLE);
  anim(bg, exBgOpa, LV_OPA_70, 0, 200);
  anim(panel, exTY, 0, 320, 220, lv_anim_path_ease_in, 0, delParentReady);
}

static void toast(const char *msg) {
  lv_obj_t *t = lv_label_create(lv_layer_top());
  lv_label_set_text(t, msg);
  lv_obj_set_style_bg_color(t, C_SURF2, 0);
  lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(t, C_TEXT, 0);
  lv_obj_set_style_pad_hor(t, 18, 0);
  lv_obj_set_style_pad_ver(t, 10, 0);
  lv_obj_set_style_radius(t, 20, 0);
  lv_obj_align(t, LV_ALIGN_BOTTOM_MID, 0, -14);
  anim(t, exTY, 40, 0, 260, lv_anim_path_overshoot);
  lv_obj_del_delayed(t, 2000);
}

static void dayWord(time_t ts, char *buf, size_t n) {
  static const char *names[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  time_t now = time(nullptr);
  struct tm a, b;
  localtime_r(&now, &a);
  localtime_r(&ts, &b);
  struct tm a0 = a, b0 = b;
  a0.tm_hour = b0.tm_hour = 12;
  a0.tm_min = b0.tm_min = a0.tm_sec = b0.tm_sec = 0;
  int diff = (int)((mktime(&b0) - mktime(&a0) + 43200) / 86400);
  if (diff == 0) strlcpy(buf, "Today", n);
  else if (diff == 1) strlcpy(buf, "Tomorrow", n);
  else strlcpy(buf, names[b.tm_wday], n);
}

static void splitClock(const struct tm &tm, char *hm, size_t n, const char **ampm) {
#if USE_12H
  snprintf(hm, n, "%d:%02d", tm.tm_hour % 12 ? tm.tm_hour % 12 : 12, tm.tm_min);
  *ampm = tm.tm_hour < 12 ? "AM" : "PM";
#else
  snprintf(hm, n, "%02d:%02d", tm.tm_hour, tm.tm_min);
  *ampm = "";
#endif
}

// soonest-finishing running timers first
static int sortedTimers(int *order) {
  int n = 0;
  for (int i = 0; i < MAX_TIMERS; i++)
    if (timers[i].active) order[n++] = i;
  for (int a = 0; a < n; a++)
    for (int b = a + 1; b < n; b++)
      if (timerRemainingMs(timers[order[b]]) < timerRemainingMs(timers[order[a]])) {
        int t = order[a]; order[a] = order[b]; order[b] = t;
      }
  return n;
}

// ---------------- Clock tab ----------------

static lv_obj_t *lblHM, *lblSec, *lblAmPm, *lblDate, *lblNextWhen, *lblNextText, *clkTimerCard, *clkRing, *clkRingLbl,
    *clkTimerName;
static int8_t clkBand = -1;

static void buildClock(lv_obj_t *tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_top(tab, 14, 0);
  lv_obj_set_style_pad_row(tab, 4, 0);
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *row = mkBox(tab, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  lv_obj_set_style_pad_column(row, 12, 0);
  lblHM = mkLabel(row, "--:--", &font_clock_150);
  lv_obj_t *col = mkBox(row, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_bottom(col, 4, 0);
  lblAmPm = mkLabelC(col, "", &font_raj_34, C_DIM);
  lblSec = mkLabel(col, "", &font_raj_48);
  lv_obj_add_style(lblSec, &stAccentText, 0);

  lblDate = mkLabelC(tab, "", &font_raj_34, C_DIM);

  lv_obj_t *bottom = mkBox(tab, LV_FLEX_FLOW_ROW);
  lv_obj_set_width(bottom, LV_PCT(100));
  lv_obj_set_style_pad_column(bottom, 8, 0);
  lv_obj_set_style_pad_top(bottom, 10, 0);

  lv_obj_t *next = mkCard(bottom, 0, 78);
  lv_obj_set_flex_grow(next, 1);
  lv_obj_set_flex_flow(next, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(next, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(next, 12, 0);
  mkLabelC(next, LV_SYMBOL_BELL, &lv_font_montserrat_28, C_YELLOW);
  lv_obj_t *ncol = mkBox(next, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_grow(ncol, 1);
  lblNextWhen = mkLabelC(ncol, "", &font_raj_26, C_YELLOW);
  lblNextText = mkLabel(ncol, "");
  lv_label_set_long_mode(lblNextText, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lblNextText, LV_PCT(100));

  clkTimerCard = mkCard(bottom, 176, 78);
  lv_obj_set_flex_flow(clkTimerCard, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(clkTimerCard, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(clkTimerCard, 10, 0);
  lv_obj_set_style_pad_all(clkTimerCard, 8, 0);
  clkRing = mkRing(clkTimerCard, 58, 7);
  lv_obj_t *tcol = mkBox(clkTimerCard, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_grow(tcol, 1);
  clkRingLbl = mkLabel(tcol, "", &font_raj_34);
  clkTimerName = mkLabelC(tcol, "", &lv_font_montserrat_14, C_DIM);
  lv_label_set_long_mode(clkTimerName, LV_LABEL_LONG_DOT);
  lv_obj_set_width(clkTimerName, LV_PCT(100));
  lv_obj_add_flag(clkTimerCard, LV_OBJ_FLAG_HIDDEN);
}

static void setTextAnimated(lv_obj_t *l, const char *txt, int dy, uint32_t ms) {
  if (!strcmp(lv_label_get_text(l), txt)) return;
  lv_label_set_text(l, txt);
  anim(l, exTY, dy, 0, ms);
  anim(l, exTextOpa, 0, LV_OPA_COVER, ms);
}

static void updateClock() {
  char buf[96];
  if (timeValid()) {
    time_t now = time(nullptr);
    struct tm tm;
    localtime_r(&now, &tm);
    const char *ampm;
    splitClock(tm, buf, sizeof(buf), &ampm);
    setTextAnimated(lblHM, buf, -16, 420);
    lv_label_set_text(lblAmPm, ampm);
    snprintf(buf, sizeof(buf), "%02d", tm.tm_sec);
    setTextAnimated(lblSec, buf, -10, 220);
    strftime(buf, sizeof(buf), "%A, %B %e", &tm);
    lv_label_set_text(lblDate, buf);

    time_t when;
    int i = nextReminder(&when);
    if (i >= 0) {
      struct tm wt;
      localtime_r(&when, &wt);
      char clk[16], day[16];
      formatClock(clk, sizeof(clk), wt.tm_hour, wt.tm_min);
      dayWord(when, day, sizeof(day));
      snprintf(buf, sizeof(buf), "%s  %s", day, clk);
      lv_label_set_text(lblNextWhen, buf);
      lv_label_set_text(lblNextText, reminders[i].text);
    } else {
      lv_label_set_text(lblNextWhen, "No reminders");
      lv_label_set_text(lblNextText, "Add one in the Remind tab");
    }
  } else {
    lv_label_set_text(lblHM, "--:--");
    lv_label_set_text(lblSec, "");
    lv_label_set_text(lblAmPm, "");
    lv_label_set_text(lblDate, WiFi.status() == WL_CONNECTED ? "Getting the time..." : "Waiting for WiFi...");
    lv_label_set_text(lblNextWhen, "Reminders paused");
    lv_label_set_text(lblNextText, "They need WiFi to know the time");
  }
}

static void updateClockTimer() {
  int order[MAX_TIMERS];
  int n = sortedTimers(order);
  if (n == 0) {
    if (!lv_obj_has_flag(clkTimerCard, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(clkTimerCard, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  if (lv_obj_has_flag(clkTimerCard, LV_OBJ_FLAG_HIDDEN)) {
    lv_obj_clear_flag(clkTimerCard, LV_OBJ_FLAG_HIDDEN);
    anim(clkTimerCard, exTX, 60, 0, 350, lv_anim_path_overshoot);
  }
  const Timer &t = timers[order[0]];
  uint32_t rem = timerRemainingMs(t);
  ringShow(clkRing, t, rem, &clkBand);
  char buf[40];
  formatDuration(buf, sizeof(buf), rem);
  lv_label_set_text(clkRingLbl, buf);
  if (n > 1) snprintf(buf, sizeof(buf), "+%d more", n - 1);
  else timerDisplayLabel(order[0], buf, sizeof(buf));
  lv_label_set_text(clkTimerName, buf);
}

// ---------------- Timers tab ----------------

static const char *presetMap[] = {"1m", "3m", "5m", "10m", "15m", "30m", "1h", LV_SYMBOL_EDIT, ""};
static const uint32_t presetSecs[] = {60, 180, 300, 600, 900, 1800, 3600, 0};

static lv_obj_t *timerList;
static lv_obj_t *tRing[MAX_TIMERS], *tTime[MAX_TIMERS], *tState[MAX_TIMERS];
static int8_t tBand[MAX_TIMERS];
static uint8_t prevTimerMask = 0;

static struct { lv_obj_t *panel, *rh, *rm, *rs; } ct;
static char optsHour[24 * 7], optsH[24 * 3 + 1], opts60[60 * 3 + 1];

static lv_obj_t *mkRoller(lv_obj_t *p, const char *opts, int sel, lv_coord_t w) {
  lv_obj_t *r = lv_roller_create(p);
  lv_roller_set_options(r, opts, LV_ROLLER_MODE_NORMAL);
  lv_obj_set_style_text_font(r, &font_raj_34, 0);
  lv_roller_set_visible_row_count(r, 3);
  lv_roller_set_selected(r, sel, LV_ANIM_OFF);
  lv_obj_set_width(r, w);
  return r;
}

static void customCancelCb(lv_event_t *) { modalClose(ct.panel); }
static void customStartCb(lv_event_t *) {
  uint32_t s = lv_roller_get_selected(ct.rh) * 3600 + lv_roller_get_selected(ct.rm) * 60 + lv_roller_get_selected(ct.rs);
  if (s == 0) { toast("Pick a time first"); return; }
  if (timerStart(s, "") < 0) toast("All 6 timer slots are in use");
  modalClose(ct.panel);
}

static void openCustomTimer() {
  lv_obj_t *bg = mkModalBg();
  ct.panel = mkCard(bg, 420, 290);
  lv_obj_set_style_pad_all(ct.panel, 14, 0);
  lv_obj_center(ct.panel);
  lv_obj_t *t = mkLabel(ct.panel, "Custom timer", &font_raj_34);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, -4);

  lv_obj_t *row = mkBox(ct.panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(row, 6, 0);
  lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 36);
  ct.rh = mkRoller(row, optsH, 0, 74);
  mkLabelC(row, "h", &font_raj_26, C_DIM);
  ct.rm = mkRoller(row, opts60, 5, 74);
  mkLabelC(row, "m", &font_raj_26, C_DIM);
  ct.rs = mkRoller(row, opts60, 0, 74);
  mkLabelC(row, "s", &font_raj_26, C_DIM);

  lv_obj_t *c = mkBtn(ct.panel, "Cancel", customCancelCb, nullptr, 170, 50);
  btnColor(c, C_SURF2, C_TEXT);
  lv_obj_align(c, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_t *s = mkBtn(ct.panel, LV_SYMBOL_PLAY "  Start", customStartCb, nullptr, 170, 50);
  lv_obj_align(s, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  anim(ct.panel, exTY, 80, 0, 380, lv_anim_path_overshoot);
}

static void presetCb(lv_event_t *e) {
  lv_obj_t *m = lv_event_get_target(e);
  uint32_t id = lv_btnmatrix_get_selected_btn(m);
  if (id == LV_BTNMATRIX_BTN_NONE || id > 7) return;
  if (id == 7) { openCustomTimer(); return; }
  if (timerStart(presetSecs[id], "") < 0) toast("All 6 timer slots are in use");
}

static void timerCardCb(lv_event_t *e) { timerTogglePause(IDX(e)); }
static void timerCancelCb(lv_event_t *e) { timerCancel(IDX(e)); }

static void buildTimers(lv_obj_t *tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(tab, 10, 0);
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *m = lv_btnmatrix_create(tab);
  lv_btnmatrix_set_map(m, presetMap);
  lv_obj_set_size(m, LV_PCT(100), 52);
  lv_obj_set_style_pad_all(m, 0, 0);
  lv_obj_set_style_pad_column(m, 6, 0);
  lv_obj_set_style_text_font(m, &font_raj_26, LV_PART_ITEMS);
  lv_obj_add_event_cb(m, presetCb, LV_EVENT_VALUE_CHANGED, nullptr);

  timerList = mkBox(tab, LV_FLEX_FLOW_ROW);
  lv_obj_add_flag(timerList, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(timerList, LV_DIR_HOR);
  lv_obj_set_scroll_snap_x(timerList, LV_SCROLL_SNAP_START);
  lv_obj_set_scrollbar_mode(timerList, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_width(timerList, LV_PCT(100));
  lv_obj_set_flex_grow(timerList, 1);
  lv_obj_set_style_pad_column(timerList, 10, 0);
  lv_obj_set_style_pad_top(timerList, 6, 0);
}

static void rebuildTimers() {
  lv_obj_clean(timerList);
  uint8_t mask = 0;
  int n = 0;
  for (int i = 0; i < MAX_TIMERS; i++) {
    tRing[i] = nullptr;
    if (!timers[i].active) continue;
    mask |= 1 << i;
    lv_obj_t *card = mkCard(timerList, 146, 186);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_add_event_cb(card, timerCardCb, LV_EVENT_CLICKED, UD(i));

    tBand[i] = -1;
    tRing[i] = mkRing(card, 120, 10);
    lv_obj_align(tRing[i], LV_ALIGN_TOP_MID, 0, 2);
    tTime[i] = mkLabel(card, "", &font_raj_34);
    lv_obj_align_to(tTime[i], tRing[i], LV_ALIGN_CENTER, 0, -6);
    tState[i] = mkLabelC(card, timers[i].paused ? LV_SYMBOL_PAUSE " paused" : "tap to pause", &lv_font_montserrat_14, C_DIM);
    lv_obj_align_to(tState[i], tRing[i], LV_ALIGN_CENTER, 0, 22);

    char lbl[40];
    timerDisplayLabel(i, lbl, sizeof(lbl));
    lv_obj_t *name = mkLabel(card, lbl);
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
    lv_obj_set_width(name, LV_PCT(100));
    lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(name, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *x = mkBtn(card, LV_SYMBOL_CLOSE, timerCancelCb, UD(i), 34, 34);
    btnColor(x, C_SURF2, C_DIM);
    lv_obj_set_style_radius(x, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_ext_click_area(x, 8);
    lv_obj_align(x, LV_ALIGN_TOP_RIGHT, 4, -4);

    if (!(prevTimerMask & (1 << i))) { // new card: pop in
      anim(card, exTY, 50, 0, 420, lv_anim_path_overshoot, n * 60);
      anim(card, exOpa, 0, LV_OPA_COVER, 250, lv_anim_path_ease_out, n * 60);
      n++;
    }
  }
  prevTimerMask = mask;
  if (!mask) {
    lv_obj_t *l = mkLabelC(timerList, "Tap a time above to start a timer.\nTap a running timer to pause it.",
                           nullptr, C_DIM);
    lv_obj_set_width(l, LV_PCT(100));
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(l, 50, 0);
  }
}

static void updateTimers() {
  char buf[16];
  for (int i = 0; i < MAX_TIMERS; i++) {
    if (!tRing[i] || !timers[i].active) continue;
    uint32_t rem = timerRemainingMs(timers[i]);
    ringShow(tRing[i], timers[i], rem, &tBand[i]);
    formatDuration(buf, sizeof(buf), rem);
    if (strcmp(lv_label_get_text(tTime[i]), buf)) {
      lv_obj_set_style_text_font(tTime[i], strlen(buf) > 5 ? &font_raj_26 : &font_raj_34, 0);
      lv_label_set_text(tTime[i], buf);
      lv_obj_align_to(tTime[i], tRing[i], LV_ALIGN_CENTER, 0, -6);
    }
  }
}

// ---------------- Reminder editor ----------------

static const char *dayMap[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa", ""};
static struct { lv_obj_t *panel, *ta, *rh, *rm, *days, *kb; int idx; } ed;

static void edCancelCb(lv_event_t *) { modalClose(ed.panel); }
static void edDeleteCb(lv_event_t *) {
  reminderDelete(ed.idx);
  toast(LV_SYMBOL_TRASH "  Reminder deleted");
  modalClose(ed.panel);
}
static void edSaveCb(lv_event_t *) {
  Reminder r = {};
  strlcpy(r.text, lv_textarea_get_text(ed.ta), sizeof(r.text));
  r.hour = lv_roller_get_selected(ed.rh);
  r.minute = lv_roller_get_selected(ed.rm);
  for (int i = 0; i < 7; i++)
    if (lv_btnmatrix_has_btn_ctrl(ed.days, i, LV_BTNMATRIX_CTRL_CHECKED)) r.days |= 1 << i;
  r.enabled = true;
  if (reminderSave(ed.idx, r) < 0) { toast("Reminder list is full"); return; }
  toast(LV_SYMBOL_OK "  Reminder saved");
  modalClose(ed.panel);
}

static void edSetDays(uint8_t days) {
  for (int i = 0; i < 7; i++) {
    if ((days >> i) & 1) lv_btnmatrix_set_btn_ctrl(ed.days, i, LV_BTNMATRIX_CTRL_CHECKED);
    else lv_btnmatrix_clear_btn_ctrl(ed.days, i, LV_BTNMATRIX_CTRL_CHECKED);
  }
}
static void edQuickCb(lv_event_t *e) { edSetDays((uint8_t)IDX(e)); }

static void edKbShowCb(lv_event_t *) {
  lv_keyboard_set_textarea(ed.kb, ed.ta);
  lv_obj_add_state(ed.ta, LV_STATE_FOCUSED);
  if (lv_obj_has_flag(ed.kb, LV_OBJ_FLAG_HIDDEN)) {
    lv_obj_clear_flag(ed.kb, LV_OBJ_FLAG_HIDDEN);
    anim(ed.kb, exTY, 200, 0, 260);
  }
}
static void edKbHideCb(lv_event_t *) {
  lv_obj_add_flag(ed.kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_state(ed.ta, LV_STATE_FOCUSED);
}

static void openEditor(int idx) {
  ed.idx = idx;
  Reminder r = {};
  if (idx >= 0) {
    r = reminders[idx];
  } else {
    r.hour = 8;
    if (timeValid()) {
      time_t now = time(nullptr);
      struct tm tm;
      localtime_r(&now, &tm);
      r.hour = (tm.tm_hour + 1) % 24;
    }
  }

  lv_obj_t *bg = mkModalBg();
  lv_obj_t *p = ed.panel = mkCard(bg, 480, 320);
  lv_obj_set_style_radius(p, 0, 0);
  lv_obj_set_style_bg_color(p, C_BG, 0);
  lv_obj_set_style_pad_all(p, 8, 0);

  ed.ta = lv_textarea_create(p);
  lv_textarea_set_one_line(ed.ta, true);
  lv_textarea_set_max_length(ed.ta, sizeof(r.text) - 1);
  lv_textarea_set_placeholder_text(ed.ta, "What should I remind you about?");
  lv_textarea_set_text(ed.ta, idx >= 0 ? r.text : "");
  lv_obj_set_width(ed.ta, LV_PCT(100));
  lv_obj_align(ed.ta, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_add_event_cb(ed.ta, edKbShowCb, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ed.ta, edKbHideCb, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(ed.ta, edKbHideCb, LV_EVENT_CANCEL, nullptr);

  ed.rh = mkRoller(p, optsHour, r.hour, 118);
  lv_obj_align(ed.rh, LV_ALIGN_TOP_LEFT, 0, 52);
  ed.rm = mkRoller(p, opts60, r.minute, 70);
  lv_obj_align(ed.rm, LV_ALIGN_TOP_LEFT, 124, 52);

  ed.days = lv_btnmatrix_create(p);
  lv_btnmatrix_set_map(ed.days, dayMap);
  lv_btnmatrix_set_btn_ctrl_all(ed.days, LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_obj_set_size(ed.days, 264, 50);
  lv_obj_set_style_pad_all(ed.days, 0, 0);
  lv_obj_set_style_pad_column(ed.days, 4, 0);
  lv_obj_set_style_text_font(ed.days, &font_raj_26, LV_PART_ITEMS);
  lv_obj_align(ed.days, LV_ALIGN_TOP_RIGHT, 0, 54);
  edSetDays(r.days);

  lv_obj_t *q;
  q = mkBtn(p, "Once", edQuickCb, UD(0), 80, 40);
  btnColor(q, C_SURF2, C_TEXT);
  lv_obj_align(q, LV_ALIGN_TOP_RIGHT, -184, 112);
  q = mkBtn(p, "Weekdays", edQuickCb, UD(0x3E), 96, 40);
  btnColor(q, C_SURF2, C_TEXT);
  lv_obj_align(q, LV_ALIGN_TOP_RIGHT, -84, 112);
  q = mkBtn(p, "Daily", edQuickCb, UD(0x7F), 80, 40);
  btnColor(q, C_SURF2, C_TEXT);
  lv_obj_align(q, LV_ALIGN_TOP_RIGHT, 0, 112);
  lv_obj_t *hint = mkLabelC(p, "No days picked = one time only", &lv_font_montserrat_14, C_DIM);
  lv_obj_align(hint, LV_ALIGN_TOP_RIGHT, 0, 162);

  lv_obj_t *b = mkBtn(p, "Cancel", edCancelCb, nullptr, 140, 50);
  btnColor(b, C_SURF2, C_TEXT);
  lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  if (idx >= 0) {
    b = mkBtn(p, LV_SYMBOL_TRASH "  Delete", edDeleteCb, nullptr, 140, 50);
    btnColor(b, C_DANGER, C_TEXT);
    lv_obj_align(b, LV_ALIGN_BOTTOM_MID, 0, 0);
  }
  b = mkBtn(p, LV_SYMBOL_OK "  Save", edSaveCb, nullptr, 140, 50);
  lv_obj_align(b, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  ed.kb = lv_keyboard_create(p);
  lv_obj_set_size(ed.kb, LV_PCT(100), 210);
  lv_obj_align(ed.kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_event_cb(ed.kb, edKbHideCb, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(ed.kb, edKbHideCb, LV_EVENT_CANCEL, nullptr);
  lv_obj_add_flag(ed.kb, LV_OBJ_FLAG_HIDDEN);

  anim(p, exTY, 320, 0, 320); // slide up from the bottom
  if (idx < 0) edKbShowCb(nullptr);
}

// ---------------- Reminders tab ----------------

static lv_obj_t *remList;
static uint32_t prevRemMask = 0;
static bool remFirstBuild = true;

static void remNewCb(lv_event_t *) { openEditor(-1); }
static void remRowCb(lv_event_t *e) { openEditor(IDX(e)); }
static void remSwitchCb(lv_event_t *e) {
  reminderSetEnabled(IDX(e), lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED));
}

static void buildReminders(lv_obj_t *tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(tab, 8, 0);
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
  mkBtn(tab, LV_SYMBOL_PLUS "  New reminder", remNewCb, nullptr, LV_PCT(100), 48);

  remList = mkBox(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_add_flag(remList, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(remList, LV_DIR_VER);
  lv_obj_set_width(remList, LV_PCT(100));
  lv_obj_set_flex_grow(remList, 1);
  lv_obj_set_style_pad_row(remList, 8, 0);
}

static void rebuildReminders() {
  lv_obj_clean(remList);
  int order[MAX_REMINDERS], n = 0;
  uint32_t mask = 0;
  for (int i = 0; i < MAX_REMINDERS; i++)
    if (reminders[i].used) { order[n++] = i; mask |= 1UL << i; }
  for (int a = 0; a < n; a++)
    for (int b = a + 1; b < n; b++) {
      const Reminder &ra = reminders[order[a]], &rb = reminders[order[b]];
      if (rb.hour * 60 + rb.minute < ra.hour * 60 + ra.minute) {
        int t = order[a]; order[a] = order[b]; order[b] = t;
      }
    }

  int stagger = 0;
  for (int k = 0; k < n; k++) {
    int i = order[k];
    const Reminder &r = reminders[i];
    lv_obj_t *row = mkCard(remList, LV_PCT(100), 66);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 10, 0);
    lv_obj_set_style_pad_hor(row, 14, 0);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, remRowCb, LV_EVENT_CLICKED, UD(i));

    // little coloured bar: yellow for repeating, blue for one-time
    lv_obj_t *bar = lv_obj_create(row);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 5, 40);
    lv_obj_set_style_radius(bar, 3, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar, r.days ? C_YELLOW : C_BLUE, 0);

    char clk[16];
    formatClock(clk, sizeof(clk), r.hour, r.minute);
    lv_obj_t *t = mkLabel(row, clk, &font_raj_34);
    lv_obj_set_width(t, 120);

    lv_obj_t *col = mkBox(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(col, 1);
    lv_obj_t *txt = mkLabel(col, r.text);
    lv_label_set_long_mode(txt, LV_LABEL_LONG_DOT);
    lv_obj_set_width(txt, LV_PCT(100));
    char days[40];
    formatDays(days, sizeof(days), r.days);
    mkLabelC(col, days, &lv_font_montserrat_14, C_DIM);

    if (!r.enabled) {
      lv_obj_set_style_text_opa(t, LV_OPA_40, 0);
      lv_obj_set_style_text_opa(col, LV_OPA_40, 0);
      lv_obj_set_style_bg_opa(bar, LV_OPA_40, 0);
    }

    lv_obj_t *sw = lv_switch_create(row);
    if (r.enabled) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, remSwitchCb, LV_EVENT_VALUE_CHANGED, UD(i));

    if (remFirstBuild || !(prevRemMask & (1UL << i))) { // new rows slide in
      uint32_t d = stagger++ * 70;
      anim(row, exTX, 60, 0, 380, lv_anim_path_overshoot, d);
      anim(row, exOpa, 0, LV_OPA_COVER, 300, lv_anim_path_ease_out, d);
    }
  }
  prevRemMask = mask;
  remFirstBuild = false;
  if (n == 0) {
    lv_obj_t *l = mkLabelC(remList, "No reminders yet.\nTap  " LV_SYMBOL_PLUS "  New reminder  to add one.", nullptr, C_DIM);
    lv_obj_set_width(l, LV_PCT(100));
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(l, 40, 0);
  }
}

// ---------------- Settings tab ----------------

static lv_obj_t *lblNet, *lblStatusIcon, *swatch[4];

static void volCb(lv_event_t *e) {
  settings.volume = lv_slider_get_value(lv_event_get_target(e));
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) { settingsSave(); soundTest(); }
}
static int curBl = -1;
static void briCb(lv_event_t *e) {
  settings.brightness = lv_slider_get_value(lv_event_get_target(e));
  curBl = settings.brightness;
  setBacklight(settings.brightness);
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) settingsSave();
}
static void testAlertCb(lv_event_t *) { alertPush("Test alert", "This is what an alert looks like", 0); }
static void recalCb(lv_event_t *) { requestTouchCalibration(); }

static void swatchRefresh() {
  for (int i = 0; i < 4; i++) {
    bool sel = i == settings.accent;
    lv_obj_set_style_border_width(swatch[i], sel ? 4 : 0, 0);
    lv_obj_t *l = lv_obj_get_child(swatch[i], 0);
    lv_label_set_text(l, sel ? LV_SYMBOL_OK : "");
  }
}
static void swatchCb(lv_event_t *e) {
  settings.accent = IDX(e);
  settingsSave();
  stylesAccent();
  swatchRefresh();
}
static void dimCb(lv_event_t *e) {
  uint32_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
  if (id > 3) return;
  settings.dimIdx = id;
  settingsSave();
}
static void nightCb(lv_event_t *e) {
  settings.nightDim = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  settingsSave();
}

static lv_obj_t *settingsRow(lv_obj_t *tab, const char *name, lv_coord_t h = 58) {
  lv_obj_t *row = mkCard(tab, LV_PCT(100), h);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_hor(row, 16, 0);
  lv_obj_set_style_pad_column(row, 12, 0);
  lv_obj_t *l = mkLabel(row, name);
  lv_obj_set_width(l, 150);
  return row;
}

static void mkSlider(lv_obj_t *tab, const char *name, int min, int max, int val, lv_event_cb_t cb) {
  lv_obj_t *row = settingsRow(tab, name);
  lv_obj_t *s = lv_slider_create(row);
  lv_slider_set_range(s, min, max);
  lv_slider_set_value(s, val, LV_ANIM_OFF);
  lv_obj_set_flex_grow(s, 1);
  lv_obj_set_style_pad_right(row, 26, 0);
  lv_obj_add_event_cb(s, cb, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(s, cb, LV_EVENT_RELEASED, nullptr);
}

static const char *dimMap[] = {"Never", "30s", "1m", "5m", ""};

static void buildSettings(lv_obj_t *tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(tab, 8, 0);
  lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_ACTIVE);

  mkSlider(tab, LV_SYMBOL_VOLUME_MAX "  Volume", 0, 100, settings.volume, volCb);
  mkSlider(tab, LV_SYMBOL_EYE_OPEN "  Brightness", 10, 255, settings.brightness, briCb);

  lv_obj_t *row = settingsRow(tab, LV_SYMBOL_TINT "  Accent");
  for (int i = 0; i < 4; i++) {
    swatch[i] = mkBtn(row, "", swatchCb, UD(i), 42, 42);
    lv_obj_set_style_radius(swatch[i], LV_RADIUS_CIRCLE, 0);
    btnColor(swatch[i], lv_color_hex(ACCENTS[i]), C_BG);
    lv_obj_set_style_border_color(swatch[i], C_TEXT, 0);
  }
  swatchRefresh();

  row = settingsRow(tab, LV_SYMBOL_IMAGE "  Clock screen\nafter idle", 66);
  lv_obj_t *m = lv_btnmatrix_create(row);
  lv_btnmatrix_set_map(m, dimMap);
  lv_btnmatrix_set_btn_ctrl_all(m, LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(m, true);
  lv_btnmatrix_set_btn_ctrl(m, settings.dimIdx, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_set_flex_grow(m, 1);
  lv_obj_set_height(m, 44);
  lv_obj_set_style_pad_all(m, 0, 0);
  lv_obj_set_style_pad_column(m, 4, 0);
  lv_obj_add_event_cb(m, dimCb, LV_EVENT_VALUE_CHANGED, nullptr);

  row = settingsRow(tab, LV_SYMBOL_EYE_CLOSE "  Extra dim at night");
  lv_obj_set_width(lv_obj_get_child(row, 0), 260);
  lv_obj_t *sw = lv_switch_create(row);
  if (settings.nightDim) lv_obj_add_state(sw, LV_STATE_CHECKED);
  lv_obj_add_event_cb(sw, nightCb, LV_EVENT_VALUE_CHANGED, nullptr);

  row = mkBox(tab, LV_FLEX_FLOW_ROW);
  lv_obj_set_width(row, LV_PCT(100));
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t *b = mkBtn(row, LV_SYMBOL_BELL "  Test alert", testAlertCb, nullptr, 224, 50);
  btnColor(b, C_SURF2, C_TEXT);
  b = mkBtn(row, LV_SYMBOL_REFRESH "  Fix touch", recalCb, nullptr, 224, 50);
  btnColor(b, C_SURF2, C_TEXT);

  lv_obj_t *net = mkCard(tab, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(net, 14, 0);
  lblNet = mkLabelC(net, "", &lv_font_montserrat_14, C_DIM);
  lv_obj_set_width(lblNet, LV_PCT(100));
}

static void updateNet() {
  char buf[200];
  bool up = WiFi.status() == WL_CONNECTED;
  if (up) {
    snprintf(buf, sizeof(buf),
             LV_SYMBOL_WIFI "  Connected to %s\nManage from your phone:  http://%s.local  or  http://%s",
             WiFi.SSID().c_str(), HOSTNAME, WiFi.localIP().toString().c_str());
  } else {
    snprintf(buf, sizeof(buf), LV_SYMBOL_WARNING "  WiFi not connected (trying \"%s\")\nTimers still work without WiFi.",
             WIFI_SSID);
  }
  lv_label_set_text(lblNet, buf);
  lv_obj_set_style_text_color(lblStatusIcon, up ? C_DIM : C_ORANGE, 0);
  lv_label_set_text(lblStatusIcon, up ? LV_SYMBOL_WIFI : LV_SYMBOL_WARNING);
}

// ---------------- Backlight fades & ambient mode ----------------

static int blVar; // anim identity for the backlight
static void exBacklight(void *, int32_t v) { curBl = v; setBacklight(v); }
static void fadeBacklight(int to, uint32_t ms, lv_anim_ready_cb_t ready = nullptr) {
  lv_anim_del(&blVar, nullptr);
  anim(&blVar, exBacklight, curBl < 0 ? settings.brightness : curBl, to, ms, lv_anim_path_ease_in_out, 0, ready);
}

static lv_obj_t *amb, *ambHM, *ambAmPm, *ambDate, *ambNext, *ambTimer;
static bool ambOn = false;

static bool isNight() {
  if (!timeValid()) return false;
  time_t now = time(nullptr);
  struct tm tm;
  localtime_r(&now, &tm);
  return NIGHT_START_HOUR > NIGHT_END_HOUR ? (tm.tm_hour >= NIGHT_START_HOUR || tm.tm_hour < NIGHT_END_HOUR)
                                           : (tm.tm_hour >= NIGHT_START_HOUR && tm.tm_hour < NIGHT_END_HOUR);
}
static int ambientLevel() {
  if (settings.nightDim && isNight()) return 3;
  return LV_MAX(12, settings.brightness * 30 / 100);
}

static void updateAmbient() {
  char buf[96];
  bool night = settings.nightDim && isNight();
  lv_color_t c = night ? C_ORANGE : C_TEXT;
  lv_obj_set_style_text_color(ambHM, c, 0);
  if (timeValid()) {
    time_t now = time(nullptr);
    struct tm tm;
    localtime_r(&now, &tm);
    const char *ampm;
    splitClock(tm, buf, sizeof(buf), &ampm);
    setTextAnimated(ambHM, buf, -16, 600);
    lv_label_set_text(ambAmPm, ampm);
    strftime(buf, sizeof(buf), "%A, %B %e", &tm);
    lv_label_set_text(ambDate, buf);
    time_t when;
    int i = nextReminder(&when);
    if (i >= 0) {
      struct tm wt;
      localtime_r(&when, &wt);
      char clk[16], day[16];
      formatClock(clk, sizeof(clk), wt.tm_hour, wt.tm_min);
      dayWord(when, day, sizeof(day));
      snprintf(buf, sizeof(buf), LV_SYMBOL_BELL "  %s %s  -  %s", day, clk, reminders[i].text);
      lv_label_set_text(ambNext, buf);
    } else {
      lv_label_set_text(ambNext, "");
    }
  } else {
    lv_label_set_text(ambHM, "--:--");
    lv_label_set_text(ambAmPm, "");
    lv_label_set_text(ambDate, "");
    lv_label_set_text(ambNext, "");
  }
  int order[MAX_TIMERS];
  int n = sortedTimers(order);
  if (n) {
    char rem[16], lbl[40];
    formatDuration(rem, sizeof(rem), timerRemainingMs(timers[order[0]]));
    timerDisplayLabel(order[0], lbl, sizeof(lbl));
    snprintf(buf, sizeof(buf), LV_SYMBOL_LOOP "  %s  %s%s", rem, lbl, n > 1 ? "  (+more)" : "");
    lv_label_set_text(ambTimer, buf);
  } else {
    lv_label_set_text(ambTimer, "");
  }
}

static void ambShowReady(lv_anim_t *) { // screen is dark: swap to the clock face, then fade up
  updateAmbient();
  lv_obj_clear_flag(amb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(amb);
  fadeBacklight(ambientLevel(), 900);
}
static void ambEnter() {
  ambOn = true;
  fadeBacklight(0, 350, ambShowReady);
}
static void ambExit() {
  if (!ambOn) return;
  ambOn = false;
  lv_obj_add_flag(amb, LV_OBJ_FLAG_HIDDEN);
  fadeBacklight(settings.brightness, 300);
}
static void ambTouchCb(lv_event_t *) { ambExit(); }

static void buildAmbient() {
  amb = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(amb);
  lv_obj_set_size(amb, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(amb, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(amb, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(amb, C_DIM, 0);
  lv_obj_add_flag(amb, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(amb, ambTouchCb, LV_EVENT_PRESSED, nullptr);
  lv_obj_set_flex_flow(amb, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(amb, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(amb, 6, 0);

  lv_obj_t *row = mkBox(amb, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  lv_obj_set_style_pad_column(row, 10, 0);
  ambHM = mkLabel(row, "", &font_clock_150);
  ambAmPm = mkLabel(row, "", &font_raj_34);
  lv_obj_set_style_pad_bottom(ambAmPm, 6, 0);
  ambDate = mkLabel(amb, "", &font_raj_34);
  ambNext = mkLabelC(amb, "", nullptr, C_YELLOW);
  lv_obj_set_style_pad_top(ambNext, 12, 0);
  ambTimer = mkLabelC(amb, "", nullptr, C_ORANGE);
  lv_obj_add_flag(amb, LV_OBJ_FLAG_HIDDEN);
}

// ---------------- Alert overlay ----------------

static lv_obj_t *alertBg = nullptr, *alertCard, *alertBell, *alertTitle, *alertText, *alertMore, *alertSnooze,
                *alertSnoozeLbl;
static bool alertShaking = false;

static void alertDismissCb(lv_event_t *) { alertPop(); }
static void alertSnoozeCb(lv_event_t *) {
  if (alertCount == 0) return;
  if (timerStart(alerts[0].snoozeMin * 60, alerts[0].text) < 0) toast("All 6 timer slots are in use");
  alertPop();
}

static void showAlert() {
  if (alertCount == 0) {
    if (alertBg) { // slide away
      lv_obj_clear_flag(alertBg, LV_OBJ_FLAG_CLICKABLE);
      anim(alertBg, exBgOpa, 235, 0, 260);
      anim(alertCard, exTY, 0, 320, 260, lv_anim_path_ease_in, 0, delParentReady);
      alertBg = nullptr;
    }
    return;
  }
  ambExit();
  bool fresh = !alertBg;
  if (fresh) {
    alertBg = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(alertBg);
    lv_obj_set_size(alertBg, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(alertBg, C_BG, 0);
    lv_obj_add_flag(alertBg, LV_OBJ_FLAG_CLICKABLE);

    alertCard = mkCard(alertBg, 440, 290);
    lv_obj_center(alertCard);
    lv_obj_set_style_radius(alertCard, 22, 0);
    lv_obj_set_style_border_opa(alertCard, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(alertCard, 14, 0);

    alertBell = mkLabelC(alertCard, LV_SYMBOL_BELL, &lv_font_montserrat_28, C_YELLOW);
    lv_obj_align(alertBell, LV_ALIGN_TOP_LEFT, 6, 4);
    alertTitle = mkLabel(alertCard, "", &font_raj_34);
    lv_obj_align(alertTitle, LV_ALIGN_TOP_LEFT, 48, 0);
    alertMore = mkLabelC(alertCard, "", &lv_font_montserrat_14, C_DIM);
    lv_obj_align(alertMore, LV_ALIGN_TOP_RIGHT, 0, 8);

    alertText = mkLabel(alertCard, "", &font_raj_48);
    lv_label_set_long_mode(alertText, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(alertText, LV_PCT(100));
    lv_obj_set_style_text_align(alertText, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(alertText, LV_ALIGN_CENTER, 0, -14);

    alertSnooze = mkBtn(alertCard, "", alertSnoozeCb, nullptr, 190, 64);
    alertSnoozeLbl = lv_obj_get_child(alertSnooze, 0);
    btnColor(alertSnooze, C_SURF2, C_TEXT);
    lv_obj_set_style_text_font(alertSnooze, &font_raj_34, 0);
    lv_obj_align(alertSnooze, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t *d = mkBtn(alertCard, "Dismiss", alertDismissCb, nullptr, 190, 64);
    lv_obj_set_style_text_font(d, &font_raj_34, 0);
    lv_obj_align(d, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  }

  const Alert &a = alerts[0];
  bool isTimer = !strcmp(a.title, "Time's up!");
  bool isReminder = !strcmp(a.title, "Reminder");
  lv_color_t c = isTimer ? C_ORANGE : isReminder ? C_YELLOW : accent();
  lv_obj_set_style_border_color(alertCard, c, 0);
  lv_obj_set_style_text_color(alertTitle, c, 0);
  lv_obj_set_style_text_color(alertBell, c, 0);
  lv_label_set_text(alertTitle, isTimer ? "TIME'S UP" : isReminder ? "REMINDER" : a.title);
  lv_label_set_text(alertText, a.text);
  if (alertCount > 1) lv_label_set_text_fmt(alertMore, "+%d waiting", alertCount - 1);
  else lv_label_set_text(alertMore, "");
  if (a.snoozeMin) {
    lv_label_set_text_fmt(alertSnoozeLbl, isReminder ? "Snooze %d min" : "+%d min", a.snoozeMin);
    lv_obj_clear_flag(alertSnooze, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(alertSnooze, LV_OBJ_FLAG_HIDDEN);
  }

  if (fresh) {
    anim(alertBg, exBgOpa, 0, 235, 250);
    anim(alertCard, exTY, 300, 0, 520, lv_anim_path_overshoot);
  } else {
    anim(alertCard, exTY, -24, 0, 380, lv_anim_path_bounce);
  }
  loopAnim(alertBell, exTX, -7, 7, 70, 900); // ring-a-ding
  loopAnim(alertCard, exBorderW, 3, 10, 550, 0);
  alertShaking = true;
  lv_obj_move_foreground(alertBg);
}

static void updateAlert() {
  if (!alertBg || !alertShaking) return;
  if (millis() - alertStartMs > ALERT_SOUND_SECONDS * 1000UL) { // calm down once the beeping stops
    lv_anim_del(alertBell, exTX);
    lv_obj_set_style_translate_x(alertBell, 0, 0);
    alertShaking = false;
  }
}

// ---------------- init / tick ----------------

static lv_obj_t *tabview;
static uint32_t seenData = 0, seenAlert = 0;

void uiInit() {
  optsHour[0] = optsH[0] = opts60[0] = 0;
  char tmp[8];
  for (int h = 0; h < 24; h++) {
#if USE_12H
    snprintf(tmp, sizeof(tmp), "%d %s", h % 12 ? h % 12 : 12, h < 12 ? "AM" : "PM");
#else
    snprintf(tmp, sizeof(tmp), "%02d", h);
#endif
    if (h) strlcat(optsHour, "\n", sizeof(optsHour));
    strlcat(optsHour, tmp, sizeof(optsHour));
    snprintf(tmp, sizeof(tmp), "%d", h);
    if (h) strlcat(optsH, "\n", sizeof(optsH));
    strlcat(optsH, tmp, sizeof(optsH));
  }
  for (int m = 0; m < 60; m++) {
    snprintf(tmp, sizeof(tmp), "%02d", m);
    if (m) strlcat(opts60, "\n", sizeof(opts60));
    strlcat(opts60, tmp, sizeof(opts60));
  }

  lv_disp_t *disp = lv_disp_get_default();
  lv_theme_t *base = lv_theme_default_init(disp, accent(), C_ORANGE, true, LV_FONT_DEFAULT);
  stylesInit();
  theme = *base;
  lv_theme_set_parent(&theme, base);
  lv_theme_set_apply_cb(&theme, themeApply);
  lv_disp_set_theme(disp, &theme);
  lv_obj_t *scr = lv_scr_act();
  lv_obj_add_style(scr, &stScr, 0);

  tabview = lv_tabview_create(scr, LV_DIR_TOP, 46);
  lv_obj_t *tabs[4] = {
      lv_tabview_add_tab(tabview, LV_SYMBOL_HOME "  Clock"),
      lv_tabview_add_tab(tabview, LV_SYMBOL_LOOP "  Timers"),
      lv_tabview_add_tab(tabview, LV_SYMBOL_BELL "  Remind"),
      lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS),
  };
  for (lv_obj_t *t : tabs) lv_obj_set_style_pad_all(t, 8, 0);
  lv_obj_clear_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE); // tap tabs, no swiping

  lblStatusIcon = mkLabel(tabview, "", &lv_font_montserrat_14); // tiny WiFi indicator in the tab bar corner
  lv_obj_add_flag(lblStatusIcon, LV_OBJ_FLAG_FLOATING);
  lv_obj_align(lblStatusIcon, LV_ALIGN_TOP_RIGHT, -6, 4);

  buildClock(tabs[0]);
  buildTimers(tabs[1]);
  buildReminders(tabs[2]);
  buildSettings(tabs[3]);
  buildAmbient();
  curBl = settings.brightness;
}

void uiTick() {
  if (seenAlert != alertVersion) { seenAlert = alertVersion; showAlert(); }
  if (seenData != dataVersion) { seenData = dataVersion; rebuildTimers(); rebuildReminders(); }

  uint32_t now = millis();
  static uint32_t lastFast = 0, lastSlow = 0, lastNet = 0;
  if (now - lastFast >= 50) { // rings move smoothly
    lastFast = now;
    updateTimers();
    updateClockTimer();
  }
  if (now - lastSlow < 250) return;
  lastSlow = now;
  updateClock();
  updateAlert();
  if (ambOn) {
    updateAmbient();
    if (!lv_anim_get(&blVar, nullptr) && curBl != ambientLevel()) fadeBacklight(ambientLevel(), 3000);
  }
  uint16_t dimS = DIM_AFTER_S[settings.dimIdx % 4];
  if (!ambOn && dimS && alertCount == 0 && lv_disp_get_inactive_time(nullptr) > dimS * 1000UL) ambEnter();
  if (now - lastNet > 2000 || lastNet == 0) { lastNet = now; updateNet(); }
}
