/*
 *  YASAG  -  screen capture  (optional)
 *  ---------------------------------------------------------------------------
 *  Built only when YASAG_HAS_SCREENSHOT is defined (set per target by CMake).
 *  One implementation per target:
 *    screenshot_picocalc.c  - reads the ST7789 GRAM back over SPI (RAMRD) and
 *                             streams a 24-bit .BMP to the SD-card root.
 *    screenshot_sdl.c       - dumps the HAL's RGB565 stand-in buffer to a
 *                             .BMP beside the executable.            (Stage 2)
 *
 *  screenshot_capture() returns a short status string to flash on screen
 *  ("Saved SHOT1A2B.BMP", "No SD card", ...).  It points at static storage -
 *  copy it before the next call.  game.c calls it only inside
 *  #ifdef YASAG_HAS_SCREENSHOT, so the declaration is harmless when the
 *  feature is off.
 */
#ifndef YASAG_SCREENSHOT_H
#define YASAG_SCREENSHOT_H

const char *screenshot_capture(void);

#endif /* YASAG_SCREENSHOT_H */
