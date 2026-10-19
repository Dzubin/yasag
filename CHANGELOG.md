# Changelog

All notable changes to YASAG (Yet Another Space Alien Game).
Format loosely follows [Keep a Changelog](https://keepachangelog.com/);
the version here matches `VERSION_STR` in `game/game.h` and
`pico_set_program_version(...)` in `CMakeLists.txt`.

The **V1.xx** line is [picocalc-space-game](https://github.com/Dzubin/picocalc-space-game) - ClockworkPi
PicoCalc hardware only. **V2.xx** is YASAG: the same gameplay, made
hardware-independent. The 320x320 PicoCalc build of V2.00A is byte-for-byte
identical to picocalc-space-game V1.00A.

## [Unreleased]

- Updated the vendored `picocalc-text-starter` to Blair Leduc's code of 2026-10-18 (commit
  `85a002e`) and moved to Pico SDK 2.3.1. The search-hint, deleted-entry and busy-card fixes this
  copy carried are in his code now; four small local fixes remain (README, Third-party). The audio
  driver in it now sounds 20 to 20000 Hz, where it used to be silent outside 100 to 2000 Hz.
- Vendored starter: `fat32.c` `fat32_write()` no longer leaks a cluster each time a write starts on a
  cluster boundary. The program itself is unchanged; not rebuilt.

- Vendored starter: `fat32.c` skips deleted directory entries before long-name parts (an
  out-of-bounds stack write) and `sdcard.c` waits for a busy card by the clock and before
  every command (failed reads and writes on a card that was still programming). The
  program itself is unchanged; not rebuilt.

### Changed
- Vendored `drivers/fat32.c`: `get_next_free_cluster()` now advances its search
  hint past the cluster it hands out (a local fix, not in upstream), so a long
  sequential write to the SD card no longer re-scans every cluster already given
  out and looks hung.
- Build outputs now carry the platform / chip in their file names and are
  copied to the top-level project folder: `yasag-RP2040.uf2` /
  `yasag-RP2350.uf2` (picocalc target, by board) and `yasag-Windows.exe` /
  `yasag-Linux` (sdl target), so all the builds can sit side by side.
- Linux (sdl target): SDL2 is now always linked dynamically there (Debian's
  `libsdl2-dev` also ships a static archive, which used to win), and the build
  copies the real `libSDL2` next to the executable as `libSDL2-2.0.so.0` (build
  folder and top level) with an `$ORIGIN` run-path, so the two files travel
  together.

## [V2.00A] - 2026-09-10

First YASAG release - a hardware-independent fork of picocalc-space-game V1.00A.
The gameplay, physics, aliens, autopilot, audio design, 15-minute countdown and
status strip are unchanged; what changed is that **the game no longer knows what
it runs on**.

### Hardware abstraction layer
- New `hal/hal.h` seam: `vid_*` (video), `inp_*` (input), `snd_*` (audio),
  `hal_now_ms` / `hal_sleep_ms` / `hal_entropy` (time), plus `SCREEN_W/H`,
  `RGB()`, the `KEY_*` / `KEY_STATE_*` numbering and the `SCALE_*` macros.
- `game/game.c` and `game/game.h` contain **no** `pico/...`, `hardware/...`,
  `drivers/...` or `SDL...` include - verified with `grep` and a clean
  `-Wall -Wextra` compile in both the PicoCalc and SDL configurations.
- One HAL implementation per target, selected by `-DYASAG_TARGET`:
  - **`picocalc`** - `hal/hal_picocalc.c` forwards to the vendored
    `picocalc-text-starter` drivers (LCD, south-bridge keyboard, PIO audio) by
    Blair Leduc, plus the Raspberry Pi Pico SDK. Builds for RP2040 and RP2350.
  - **`sdl`** - `hal/hal_sdl.c`, Windows / Linux via statically-linked SDL2.
  - **`picosystem`** - Pimoroni PicoSystem: planned, not yet implemented.

### Rendering model (all targets)
- **No framebuffer in the game.** It still draws incrementally (erase old sprite,
  draw new) through a single primitive, `vid_fill_rect()` - exactly as V1.00A
  drove `lcd_solid_rectangle()`. On the MCUs that writes straight to panel GRAM.
  The SDL backend keeps one private RGB565 buffer standing in for the absent
  panel and dirty-rect-blits it to the window surface in `vid_present()` (no
  `SDL_Renderer`).

### Resolution independence
- Every geometry `#define` in `game/game.h` is now `SCALE_X/Y/S(...)` of its
  320x320 base value. At 320x320 with `YASAG_TEXT_SCALE 1` each one is the
  identity, so **the PicoCalc build is byte-for-byte V1.00A** (compiler-verified:
  every integer and float macro matches). On a wider screen the play box grows
  (bigger starfield); sprites and world speeds scale uniformly so squares stay
  square; vertical positions keep their fraction of screen height.
- Screen size is a compile-time constant (`-DYASAG_SCREEN_W/H`); SDL defaults to
  **640x480**.

### Text
- New `game/text.c` - fixed 8x10-cell text on `vid_fill_rect()`, replacing the
  vendored `lcd_putstr` / `lcd_set_foreground` / `lcd_set_background` /
  `lcd_clear_screen` / `lcd_enable_cursor`. Vendored 8x10 glyph data
  (`game/font_8x10.c`, MIT, (c) Blair Leduc); the 5x10 face is dropped.
- `text_set_scale()` - runtime glyph magnification. The dense controls / help
  legend and the `?`-pause drop to 1x so they fit any screen; the status line
  and menus stay at `YASAG_TEXT_SCALE` (2 on SDL). No-op on the MCUs.
- `big_text()` now renders at `scale * YASAG_TEXT_SCALE` px; new
  `big_text_centered()`.

### Start screen
- **Up / Down move a `>` cursor over the four difficulty rows; Enter (or Space)
  picks it.** `0`-`3` still jump straight to a level. On the PicoCalc the only
  visible change from V1.00A is that `>` marker and the reworded prompt line.
  This applies to every target; back-port to picocalc-space-game is optional.

### SDL backend
- Standalone `yasag.exe`: static SDL2 + static libgcc / libstdc++ / winpthread,
  no non-system DLLs to ship.
- Square-wave audio callback (two voices). `hal_sleep_ms()` presents the frame
  and pumps the SDL event queue, so the ~5 s win jingle can't freeze the window.
- `YASAG_SDL_ZOOM` - `1` = native 640x480 window (default), `0` = largest zoom
  that fits the desktop, `N` = force NxN (nearest-neighbour, stays blocky).
- `YASAG_SDL_SCRIPT` (env, inert otherwise) - timed synthetic key input for
  head-less screenshots / regression runs.

### Screenshot (F5)
- Now built for any target that defines `YASAG_HAS_SCREENSHOT` (PicoCalc, SDL).
  `hal/screenshot_picocalc.c` reads the ST7789 GRAM back over SPI to a BMP on the
  SD card (unchanged from V1.00A); `hal/screenshot_sdl.c` dumps the stand-in
  buffer to a BMP beside the executable.

### Build
- One `CMakeLists.txt`; the MCU targets pull in the Pico SDK, the SDL target does
  not. `pico_set_program_version` and `VERSION_STR` are `V2.00A`.
- Vendored `third_party/picocalc-text-starter-main/` is used only by the
  `picocalc` target and is left read-only.
