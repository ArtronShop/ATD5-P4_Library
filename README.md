# ATD5-P4 Library

Arduino library for the **ATD5-P4** HMI board by ArtronShop, built around the **ESP32-P4**.

It wraps the on-board peripherals behind a few ready-to-use global objects:

| Object    | Peripheral                                  | Header          |
|-----------|---------------------------------------------|-----------------|
| `Display` | 800×480 RGB565 parallel-RGB LCD + backlight | `LCD.h`         |
| `Touch`   | GT911 capacitive touch controller (I²C)     | `Touch.h`       |
| `Sound`   | I²S audio output (speaker amp)              | `Sound.h`       |
| `Card`    | MicroSD card slot (SPI)                     | `MicroSDCard.h` |

[LVGL](https://lvgl.io) (v9) integration is built in: one `useLVGL()` call per peripheral wires the display, touch, beep feedback and SD card into LVGL.

## Features

- Raw drawing API (pixel, line, rect, rounded rect, circle, triangle, arrow, bitmap) with RGB565 color helpers
- LVGL display driver: renders directly into two PSRAM frame buffers, with VSYNC-synchronized swap (tear-free)
- GT911 touch driver with rotation-aware coordinates, registered as an LVGL pointer device
- Touch beep feedback through I²S
- MicroSD card as an Arduino `FS` (usable with `SD`-style APIs and `ESP32-audioI2S`) and as LVGL drive `S:`
- Display auto-dim / auto-sleep on touch inactivity
- `lv_img_set_src_from_sd_card()` to show images stored on the SD card
- Thread-safe LVGL updates from other tasks (`lv_safe_update`)

## Requirements

- Board: ATD5-P4 (ESP32-P4), Arduino-ESP32 core with ESP32-P4 support, **PSRAM enabled**
- [LVGL](https://github.com/lvgl/lvgl) 9.x (needed for LVGL features and the LVGL examples)
- [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) (only for the `MP3Player` example)

## Installation

- **Arduino IDE:** Library Manager → search `ATD5-P4`, or download this repository as ZIP and use *Sketch → Include Library → Add .ZIP Library*.
- **PlatformIO:** add `https://github.com/ArtronShop/ATD5-P4_Library.git` to `lib_deps`.

## Quick start

### Drawing without LVGL

```cpp
#include <Arduino.h>
#include <ATD5-P4.h>

void setup() {
  Display.begin();                       // rotation 0, brightness 100 %
  Display.fillScreen(Display.color565(0, 0, 0));
  Display.fillCircle(400, 240, 80, 0xF800);   // red circle
  Display.drawRoundRect(100, 100, 300, 200, 16, 0xFFFF);
}

void loop() {}
```

### With LVGL

```cpp
#include <Arduino.h>
#include <lvgl.h>
#include <ATD5-P4.h>

void setup() {
  Display.begin(0);   // rotation
  Touch.begin();
  Sound.begin();

  Display.useLVGL();  // map display to LVGL
  Touch.useLVGL();    // map touch screen to LVGL
  Sound.useLVGL();    // beep on click

  lv_obj_t * label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Hello ATD5-P4");
  lv_obj_center(label);
}

void loop() {
  Display.loop();     // runs lv_timer_handler(), auto-sleep and safe-update queue
}
```

Call `Display.loop()` often from `loop()`; it is what drives LVGL.

### Image from the MicroSD card

```cpp
Card.begin();
Display.useLVGL();
Touch.useLVGL();

lv_obj_t * img = lv_img_create(lv_scr_act());
lv_img_set_src_from_sd_card(img, "/logo.png");
lv_obj_center(img);
```

The file is read into PSRAM once and cached by path, so repeated calls with the same path are cheap.

## API

### `Display` (class `LCD`)

| Method | Description |
|--------|-------------|
| `begin(rotation = 0, brightness = 100)` | Initialize the RGB panel and backlight (`brightness` 0–100) |
| `getWidth()` / `getHeight()` | Panel size (800 × 480) |
| `setRotation(m)` / `getRotation()` | Store rotation (0–3); used for touch coordinate mapping |
| `on()` / `off()` | Turn panel and backlight on/off |
| `setBrightness(level)` / `getBrightness()` | Backlight level 0–100 |
| `enableAutoSleep(sec)` / `disableAutoSleep()` | Dim at half the timeout, turn off at the full timeout; wake on touch (LVGL mode, requires `loop()`) |
| `color565(r, g, b)` / `color24to16(0xRRGGBB)` | Color conversion to RGB565 |
| `drawPixel(x, y, color)` | |
| `fillScreen(color)` | |
| `fillRect(x1, y1, x2, y2, color)` | Corner-to-corner coordinates (not width/height) |
| `drawLine(x1, y1, x2, y2, color)` | |
| `drawRect(x1, y1, x2, y2, color)` | |
| `drawRoundRect(x1, y1, x2, y2, r, color)` | |
| `drawRectAngle(xc, yc, w, h, angle, color)` | Rotated rectangle around its center |
| `drawTriangle(xc, yc, w, h, angle, color)` | Rotated triangle around its center |
| `drawCircle(x, y, r, color)` / `fillCircle(...)` | |
| `drawArrow(x0, y0, x1, y1, w, color)` / `fillArrow(...)` | |
| `drawBitmap(x1, y1, x2, y2, data)` | Blit an RGB565 buffer |
| `useLVGL()` | Initialize LVGL and register the display (call after `begin()`) |
| `loop()` | Service LVGL, auto-sleep and the safe-update queue |

Colors are 16-bit RGB565 (`0xF800` red, `0x07E0` green, `0x001F` blue).

### `Touch` (class `GT911`)

| Method | Description |
|--------|-------------|
| `begin()` | Start I²C and reset the controller |
| `read(&x, &y)` | Returns the number of touch points (0 = no touch); the first point's coordinates are written to `x`, `y`, adjusted for the display rotation |
| `useLVGL()` | Register as an LVGL pointer input device |

### `Sound` (class `ATD_Sound`)

| Method | Description |
|--------|-------------|
| `begin()` | Install the I²S driver (16 kHz, 16-bit, mono) |
| `play(data, len)` | Write raw PCM data (`len` in bytes) |
| `useLVGL()` | Play a short beep on every LVGL click/key event |

For MP3 playback, `begin()` the `Sound` object and let [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) use the same I²S pins; see the `MP3Player` example.

### `Card` (class `MicroSDCard`, derives from `SDFS`)

| Method | Description |
|--------|-------------|
| `begin()` | Check card-detect, mount the card; returns `false` if no card or mount fails |
| `useLVGL()` | Register the card as LVGL file-system drive `S:` (e.g. `"S:/logo.png"`) |

All the usual Arduino `FS` calls (`open`, `exists`, `mkdir`, …) are available.

### LVGL helpers (`LVGLHelper.h`)

| Function | Description |
|----------|-------------|
| `lv_safe_update(fn, user_data = NULL, sync = false)` | Queue `fn` to run inside `Display.loop()`. Use it to touch LVGL objects from other FreeRTOS tasks. With `sync = true` the call waits (up to 100 ms) until `fn` has run |
| `lv_timer_run_once(cb, period_ms, user_data = NULL)` | One-shot LVGL timer |
| `lv_img_set_src_from_sd_card(obj, path)` | Load an image file from the SD card into an `lv_img` (cached in PSRAM) |

```cpp
// Update a label from a background task
lv_safe_update([](void * arg) {
  lv_label_set_text((lv_obj_t *) arg, "Updated");
}, label);
```

## Pin map

| Function | Pins (GPIO) |
|----------|-------------|
| LCD backlight / DISP enable | 20 / 42 |
| LCD HSYNC / VSYNC / DE / PCLK | 41 / 40 / 39 / 43 |
| LCD data D0–D15 | 48, 47, 46, 45, 44, 26, 27, 28, 29, 30, 31, 53, 52, 51, 50, 49 |
| Touch SDA / SCL / RST | 24 / 25 / 6 (INT not used) |
| MicroSD CS / card-detect | 34 / 13 |
| I²S DOUT / BCLK / LRC | 21 / 22 / 23 |

Pin and panel-timing constants live in [src/Common.h](src/Common.h).

## Build options

Define these as build flags to tune LVGL rendering:

| Flag | Default | Meaning |
|------|---------|---------|
| `LCD_LVGL_DIRECT` | `1` | LVGL renders straight into two panel frame buffers (full-refresh, double buffered). Set `0` for partial rendering into a separate draw buffer |
| `LCD_LVGL_BUF_LINES` | `480` | Draw buffer height in lines (only when `LCD_LVGL_DIRECT=0`) |
| `LCD_LVGL_BUF_INTERNAL` | `0` | `1` = double buffer in internal SRAM, `0` = single buffer in PSRAM (only when `LCD_LVGL_DIRECT=0`) |
| `LCD_LVGL_PROFILE` | `0` | Print flush statistics to `Serial` every second |

## Examples

- [drawTest](examples/drawTest/drawTest.ino) – color fills and shape drawing without LVGL
- [withLVGL](examples/withLVGL/withLVGL.ino) – LVGL demo with touch and beep
- [loadImageFromMicroSDCard](examples/loadImageFromMicroSDCard/loadImageFromMicroSDCard.ino) – show `/logo.png` from the SD card
- [MP3Player](examples/MP3Player/MP3Player.ino) – play `/test.mp3` from the SD card

## Notes

- The raw drawing functions write to the panel one row/pixel at a time and are meant for simple graphics and tests; use LVGL for complex or animated UIs.
- Don't mix raw drawing calls and LVGL on the same screen; LVGL owns the frame buffers once `Display.useLVGL()` is called.
- `Display.setRotation()` currently only stores the value used for touch mapping.

## License

See [LICENSE](LICENSE).
