/*
 *  YASAG  -  SDL backend, internal hooks
 *  ---------------------------------------------------------------------------
 *  Shared only between hal_sdl.c and screenshot_sdl.c (both SDL-target only).
 *  The game never sees this - it talks to the HAL through hal.h.
 */
#ifndef YASAG_HAL_SDL_H
#define YASAG_HAL_SDL_H

#include <stdint.h>

/* The RGB565 "stand-in GRAM": SCREEN_W * SCREEN_H uint16_t, row-major.
 * screenshot_sdl.c reads it to write a .BMP. */
const uint16_t *sdl_fb(void);

#endif /* YASAG_HAL_SDL_H */
