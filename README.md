# CYD Timer

Timers and reminders on a 3.5" ESP32 touchscreen: the LCDwiki **ESP32-32E, model E32R35T**. It has an ST7796U 480×320 display, XPT2046 resistive touch and an ESP32-WROOM-32E.

## Features

- **Clock:** a big 24-hour clock with animated digits, the date, your next reminder and the timer closest to finishing.
- **Timers:** up to 6 at once, started from presets (1m to 1h) or a custom time. Each shows a progress ring that changes from green to yellow to orange as time runs out. Tap a timer to pause it.
- **Reminders:** up to 24, one-time or repeating on chosen weekdays. They're saved to flash, so they survive power loss.
- **Alerts:** an animated full-screen card, a flashing red LED and a beep through the speaker header. Reminders can be snoozed for 5 minutes, and finished timers can get +1 minute.
- **Clock screen:** after an idle period you choose, the screen dims to a minimal clock. It dims further and turns orange from 10pm to 7am.
- **Web page:** manage everything from a phone at `http://cyd-timer.local` or at the device's IP address, which is shown in the ⚙ tab.
- **Settings:** volume, brightness, accent color, the idle delay before the clock screen, extra dimming at night, a test alert and touch recalibration.

## Setup

1. Install [PlatformIO](https://platformio.org/).
2. Copy `include/secrets.example.h` to `include/secrets.h` and enter your 2.4 GHz WiFi name and password.
3. Optionally, change the timezone, 12/24-hour clock or night hours in `include/config.h`.
4. Connect the board over USB and run `pio run -t upload`.
5. On first boot, tap each corner arrow to calibrate the touchscreen. To recalibrate later, hold **BOOT** while powering on, or use ⚙ → Fix touch.

## Layout

| Path | What |
|---|---|
| `src/main.cpp` | Hardware: display and touch drivers, backlight, speaker, LED, WiFi |
| `src/model.cpp` | Timers, reminders, alert queue and saving to flash (NVS) |
| `src/ui.cpp` | LVGL interface: theme, tabs, animations, alert overlay, clock screen |
| `src/web.cpp` | Web page and JSON API (`/api/state`, `/api/timer`, `/api/reminder`, …) |
| `src/font_*.c` | Fonts generated from Rajdhani |
| `tools/ttf2lvgl.py` | Converts a TTF into an LVGL font using Pillow |
| `fonts/src/` | Rajdhani TTF files and their license (SIL OFL) |

To regenerate a font:

```sh
python3 tools/ttf2lvgl.py fonts/src/Rajdhani-Bold.ttf 150 font_clock_150 "0123456789: " src/font_clock_150.c
```

## Notes

- The speaker output has not been tested on hardware yet.
- Board pins come from <https://www.lcdwiki.com/3.5inch_ESP32-32E_Display>.
