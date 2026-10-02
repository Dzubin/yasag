/*
 *  YASAG  -  Yet Another Space Alien Game  -  V2.00A  -  hardware abstraction layer
 *  ---------------------------------------------------------------------------
 *  Author: Thomas Dzubin
 *
 *  YASAG V2.xx is the HARDWARE-INDEPENDENT edition of picocalc-space-game
 *  (V1.xx, PicoCalc only): the same gameplay behind this seam.  One C codebase
 *  builds for the ClockworkPi PicoCalc (RP2040 / RP2350), the Pimoroni
 *  PicoSystem (RP2040), and Windows / Linux via statically-linked SDL2.  Every
 *  hardware-specific operation the game needs is one of the calls below -
 *  game.c contains no other platform code (no  pico/... , hardware/... ,
 *  drivers/... , or SDL... include).
 *
 *  One implementation file per target, selected by CMake -DYASAG_TARGET:
 *      hal_picocalc.c    - forwards to the vendored picocalc-text-starter
 *                          drivers (LCD / south-bridge / PIO audio) by
 *                          Blair Leduc, plus the Raspberry Pi Pico SDK.
 *      hal_picosystem.c  - bare Pico SDK: ST7789 240x240, PWM piezo, buttons.
 *      hal_sdl.c         - SDL2: one RGB565 stand-in buffer + window surface.
 *
 *  Rendering model (all targets, by design): NO framebuffer in the game.  It
 *  draws incrementally - erase the old sprite, draw the new one - through
 *  vid_fill_rect(), exactly as V1.00A drove lcd_solid_rectangle().  On the
 *  MCUs that writes straight into the panel's own GRAM (zero extra RAM).  The
 *  PC has no panel, so hal_sdl.c keeps ONE private RGB565 buffer that stands
 *  in for the GRAM and blits the dirty region in vid_present(); that buffer
 *  never leaves the SDL HAL.
 */
#ifndef YASAG_HAL_H
#define YASAG_HAL_H

#include <stdbool.h>
#include <stdint.h>

/* --- Screen size --------------------------------------------------------- *
 *  Compile-time constant.  Override per build on the build machine:
 *      -DYASAG_SCREEN_W=320 -DYASAG_SCREEN_H=320
 *  Target defaults: PicoCalc 320x320, PicoSystem 240x240, SDL 320x320. */
#ifndef YASAG_SCREEN_W
#  define YASAG_SCREEN_W 320
#endif
#ifndef YASAG_SCREEN_H
#  define YASAG_SCREEN_H 320
#endif
#define SCREEN_W  YASAG_SCREEN_W
#define SCREEN_H  YASAG_SCREEN_H

/* --- Layout scaling ---------------------------------------------------------- *
 *  All game geometry in game.h is authored for a 320x320 panel.  These macros
 *  are the IDENTITY at 320x320 (v * 320 / 320 == v, exactly, for int and for
 *  the float form) and scale the base value to the real screen elsewhere:
 *
 *    SCALE_X(v)   horizontal chrome / box width      (* SCREEN_W / 320)
 *    SCALE_Y(v)   vertical positions & sizes         (* SCREEN_H / 320)
 *                 -> keeps "the same vertical ratios" on any screen
 *    SCALE_S(v)   uniform, min-axis: sprites, so a square stays square
 *    SCALE_SF(v)  the float form of SCALE_S (world speeds, streak lengths)
 *
 *  The play box takes SCALE_X for its width and SCALE_Y for its height, so a
 *  4:3 screen (e.g. SDL 640x480) gets a wider box = a bigger starfield. */
#define YASAG_MIN_DIM  (SCREEN_W < SCREEN_H ? SCREEN_W : SCREEN_H)
#define SCALE_X(v)     ((int)(v) * SCREEN_W      / 320)
#define SCALE_Y(v)     ((int)(v) * SCREEN_H      / 320)
#define SCALE_S(v)     ((int)(v) * YASAG_MIN_DIM / 320)
#define SCALE_SF(v)    ((float)(v) * YASAG_MIN_DIM / 320.0f)

/* --- Text magnification --------------------------------------------------- *
 *  Blocky integer scale for the 8x10 bitmap font (text.c).  1 on the MCUs;
 *  the SDL target sets 2 (crisp, chunky - the deliberate retro look).  The
 *  text cell is TEXT_CW x TEXT_CH px. */
#ifndef YASAG_TEXT_SCALE
#  define YASAG_TEXT_SCALE 1
#endif
#define TEXT_CW  (8  * YASAG_TEXT_SCALE)
#define TEXT_CH  (10 * YASAG_TEXT_SCALE)

/* --- Colour  -  16-bit 5-6-5, the order the ST7789 wants.  Kept token-for-token
 *  identical to the vendored drivers/lcd.h RGB() so a translation unit that
 *  pulls in both (screenshot_picocalc.c) sees a benign redefinition. --------- */
#define RGB(r,g,b)      ((uint16_t)(((r) >> 3) << 11 | ((g) >> 2) << 5 | ((b) >> 3)))

/* --- Video ------------------------------------------------------------- */
void vid_init(void);                                              /* bring up panel / window   */
void vid_fill_rect(uint16_t rgb565, int x, int y, int w, int h);  /* the ONE draw primitive    */
void vid_present(void);                        /* flush pending writes; no-op on GRAM targets  */
void vid_reboot_to_loader(void);               /* the '~' key: USB bootloader on MCU, exit()   */

/* --- Input ----------------------------------------------------------------- *
 *  The event model is copied verbatim from the PicoCalc south-bridge: poll
 *  hands back one {state, code} at a time and returns false when the queue is
 *  empty (the old `e == 0`).  `code` is ASCII for printable keys and the KEY_*
 *  block below for the rest - the same numbers drivers/keyboard.h used, so the
 *  PicoCalc backend needs no translation and game.c's switch() is unchanged. */
enum {
    KEY_STATE_IDLE     = 0,
    KEY_STATE_PRESSED  = 1,
    KEY_STATE_HOLD     = 2,
    KEY_STATE_RELEASED = 3
};

#define KEY_ENTER   0x0A
#define KEY_RETURN  0x0D
#define KEY_SPACE   0x20
#define KEY_F5      0x85
#define KEY_ESC     0xB1
#define KEY_LEFT    0xB4
#define KEY_UP      0xB5
#define KEY_DOWN    0xB6
#define KEY_RIGHT   0xB7

typedef struct {
    uint8_t state;   /* KEY_STATE_*                    */
    uint8_t code;    /* ASCII, or a KEY_* value above  */
} inp_event_t;

void inp_init(void);
bool inp_poll(inp_event_t *out);   /* false = nothing pending */
void inp_flush(void);              /* drop every pending event */

/* --- Audio --------------------------------------------------------------- *
 *  Two independent square-wave voices, frequency in Hz, 0 = silent.  The
 *  game's own sound_* latch and its pew / chime / hiss / jingle helpers sit
 *  on top of this unchanged.  Mono targets play `left_hz` and ignore the rest. */
void snd_init(void);
void snd_tone(uint16_t left_hz, uint16_t right_hz);
void snd_off(void);

/* --- Time -------------------------------------------------------------- *
 *  hal_sleep_ms() must keep the host healthy while it waits: the SDL build
 *  pumps its event queue here so play_win_song()'s ~5 s blocking jingle can
 *  never mark the window "not responding". */
uint32_t hal_now_ms(void);
void     hal_sleep_ms(uint32_t ms);
uint32_t hal_entropy(void);         /* one-shot seed for the game's RNG */

#endif /* YASAG_HAL_H */
