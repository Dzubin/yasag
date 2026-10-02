/*
 *  YASAG  -  HAL for Windows / Linux via SDL2
 *  ---------------------------------------------------------------------------
 *  Author: Thomas Dzubin
 *
 *  Same "no framebuffer in the game" model as the MCUs: game.c draws
 *  incrementally through vid_fill_rect().  A PC has no panel to hold those
 *  pixels, so this file keeps ONE private RGB565 buffer (g_fb) that stands in
 *  for the absent GRAM, tracks a dirty rectangle, and blits it to the window
 *  surface in vid_present().  No SDL_Renderer, no GPU textures - a straight
 *  software surface blit, converted from 565 to the window's native format by
 *  SDL_BlitSurface (or SDL_BlitScaled when YASAG_SDL_ZOOM > 1).
 *
 *  Entry point: game.c's plain main().  SDL_MAIN_HANDLED + SDL_SetMainReady()
 *  means we do NOT link SDL2main and SDL does not #define main - so game.c
 *  stays SDL-free.  A console window rides along (MinGW default subsystem);
 *  harmless for a dev build, and handy for stdout.
 */
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "hal_sdl.h"
#include "screenshot.h"    /* only called from the scripted-input dev aid */

/* Integer upscale of the SCREEN_W x SCREEN_H render into the window
 * (nearest-neighbour, so it stays blocky).  Default 1 = a native 640x480
 * window.  Set to 0 for "auto: largest zoom that fits the desktop", or to a
 * positive N to force NxN. */
#ifndef YASAG_SDL_ZOOM
#  define YASAG_SDL_ZOOM 1
#endif

/* Square-wave amplitude for the audio callback (0..32767).  Kept modest -
 * a raw square voice is harsh. */
#define SND_AMP  6000

/* ======================================================================= *
 *  Video
 * ===================================================================== */

static SDL_Window  *g_win;
static SDL_Surface *g_winsurf;                 /* the window's own surface        */
static SDL_Surface *g_fbsurf;                  /* 565 view of g_fb (no copy)      */
static uint16_t     g_fb[SCREEN_W * SCREEN_H]; /* the stand-in GRAM              */
static int          g_zoom = 1;                /* window px per render px          */

/* dirty rectangle, half-open [x0,x1) x [y0,y1); empty when x0 >= x1 */
static int g_dx0, g_dy0, g_dx1, g_dy1;

static void dirty_reset(void)  { g_dx0 = g_dy0 = 0x7fffffff; g_dx1 = g_dy1 = 0; }

static void dirty_add(int x, int y, int w, int h)
{
    if (x < g_dx0) g_dx0 = x;
    if (y < g_dy0) g_dy0 = y;
    if (x + w > g_dx1) g_dx1 = x + w;
    if (y + h > g_dy1) g_dy1 = y + h;
}

const uint16_t *sdl_fb(void) { return g_fb; }

void vid_init(void)
{
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
    {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        exit(1);
    }

    /* Window zoom: forced by YASAG_SDL_ZOOM, else the biggest that fits the
     * desktop (leaving room for the title bar + taskbar). */
    g_zoom = YASAG_SDL_ZOOM;
    if (g_zoom < 1)
    {
        SDL_DisplayMode dm;
        int zw = 1, zh = 1;
        if (SDL_GetDesktopDisplayMode(0, &dm) == 0)
        {
            zw = dm.w / SCREEN_W;
            zh = (dm.h - 96) / SCREEN_H;      /* 96 px for chrome */
        }
        g_zoom = (zw < zh) ? zw : zh;
        if (g_zoom < 1) g_zoom = 1;
    }

    g_win = SDL_CreateWindow("Yet Another Space Alien Game",
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             SCREEN_W * g_zoom, SCREEN_H * g_zoom,
                             SDL_WINDOW_SHOWN);
    if (!g_win) { SDL_Log("CreateWindow: %s", SDL_GetError()); exit(1); }

    g_winsurf = SDL_GetWindowSurface(g_win);
    if (!g_winsurf) { SDL_Log("GetWindowSurface: %s", SDL_GetError()); exit(1); }

    g_fbsurf = SDL_CreateRGBSurfaceWithFormatFrom(g_fb, SCREEN_W, SCREEN_H, 16,
                                                  SCREEN_W * 2, SDL_PIXELFORMAT_RGB565);
    if (!g_fbsurf) { SDL_Log("CreateRGBSurfaceFrom: %s", SDL_GetError()); exit(1); }

    memset(g_fb, 0, sizeof g_fb);
    SDL_FillRect(g_winsurf, NULL, 0);
    SDL_UpdateWindowSurface(g_win);
    dirty_reset();
}

void vid_fill_rect(uint16_t rgb565, int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (x < 0)            { w += x; x = 0; }
    if (y < 0)            { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;

    for (int row = 0; row < h; row++)
    {
        uint16_t *p = &g_fb[(y + row) * SCREEN_W + x];
        for (int col = 0; col < w; col++)
            p[col] = rgb565;
    }
    dirty_add(x, y, w, h);
}

void vid_present(void)
{
    if (g_dx0 >= g_dx1 || g_dy0 >= g_dy1) return;      /* nothing changed */

    /* window surface can be invalidated (resize etc.) - re-fetch cheaply */
    g_winsurf = SDL_GetWindowSurface(g_win);

    SDL_Rect src = { g_dx0, g_dy0, g_dx1 - g_dx0, g_dy1 - g_dy0 };
    if (g_zoom > 1)
    {
        SDL_Rect dst = { src.x * g_zoom, src.y * g_zoom,
                         src.w * g_zoom, src.h * g_zoom };
        SDL_BlitScaled(g_fbsurf, &src, g_winsurf, &dst);
        SDL_UpdateWindowSurfaceRects(g_win, &dst, 1);
    }
    else
    {
        SDL_Rect dst = src;
        SDL_BlitSurface(g_fbsurf, &src, g_winsurf, &dst);
        SDL_UpdateWindowSurfaceRects(g_win, &dst, 1);
    }
    dirty_reset();
}

void vid_reboot_to_loader(void)     /* the '~' key: on a PC, just quit */
{
    SDL_Quit();
    exit(0);
}

/* ======================================================================= *
 *  Input  -  SDL events -> the {state, code} queue game.c expects
 * ===================================================================== */

#define IQ_CAP 128
static inp_event_t g_iq[IQ_CAP];
static int g_iq_head, g_iq_tail;      /* head == tail => empty */

static void iq_push(uint8_t state, uint8_t code)
{
    int n = (g_iq_tail + 1) % IQ_CAP;
    if (n == g_iq_head) return;       /* full - drop */
    g_iq[g_iq_tail].state = state;
    g_iq[g_iq_tail].code  = code;
    g_iq_tail = n;
}

static uint8_t key_code(SDL_Keysym ks)
{
    switch (ks.sym)
    {
        case SDLK_UP:        return KEY_UP;
        case SDLK_DOWN:      return KEY_DOWN;
        case SDLK_LEFT:      return KEY_LEFT;
        case SDLK_RIGHT:     return KEY_RIGHT;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:  return KEY_RETURN;
        case SDLK_SPACE:     return KEY_SPACE;     /* 0x20 */
        case SDLK_ESCAPE:    return KEY_ESC;
        case SDLK_F5:        return KEY_F5;
        case SDLK_BACKQUOTE: return (ks.mod & KMOD_SHIFT) ? '~' : '`';
        case SDLK_SLASH:     return (ks.mod & KMOD_SHIFT) ? '?' : '/';
        default:
            /* letters and digits arrive as their lowercase ASCII; the game's
             * switch handles upper/lower itself. */
            if (ks.sym >= 0x20 && ks.sym < 0x7f) return (uint8_t)ks.sym;
            return 0;
    }
}

/* --- Scripted input (dev aid; inert unless YASAG_SDL_SCRIPT is set) --------- *
 *  YASAG_SDL_SCRIPT = "T:tok T:tok ..."  - T is ms since start; tok is one
 *  printable char, or  up down left right space enter esc f5 tilde  or the
 *  actions  shot  (screenshot_capture) and  quit .  Each fires PRESSED then
 *  RELEASED at time T.  Lets the shared game core be driven head-less for
 *  screenshots / regression runs.  Not compiled into anything that ships. */
#define SCRIPT_MAX 64
static struct { uint32_t t; uint8_t code; char act; int done; } g_scr[SCRIPT_MAX];
static int      g_scr_n = -1;          /* -1 = not parsed yet */
static uint32_t g_scr_t0;

static uint8_t tok_code(const char *s, size_t n, char *act)
{
    *act = 0;
    if (n == 1)                       return (uint8_t)s[0];
    if (!strncmp(s, "up",    n))      return KEY_UP;
    if (!strncmp(s, "down",  n))      return KEY_DOWN;
    if (!strncmp(s, "left",  n))      return KEY_LEFT;
    if (!strncmp(s, "right", n))      return KEY_RIGHT;
    if (!strncmp(s, "space", n))      return KEY_SPACE;
    if (!strncmp(s, "enter", n))      return KEY_RETURN;
    if (!strncmp(s, "esc",   n))      return KEY_ESC;
    if (!strncmp(s, "f5",    n))      return KEY_F5;
    if (!strncmp(s, "tilde", n))      return '~';
    if (!strncmp(s, "shot",  n))    { *act = 's'; return 0; }
    if (!strncmp(s, "quit",  n))    { *act = 'q'; return 0; }
    return 0;
}

static void script_parse(void)
{
    g_scr_n = 0;
    g_scr_t0 = SDL_GetTicks();
    const char *e = getenv("YASAG_SDL_SCRIPT");
    if (!e) return;
    while (*e && g_scr_n < SCRIPT_MAX)
    {
        while (*e == ' ') e++;
        if (!*e) break;
        char *end;
        unsigned long t = strtoul(e, &end, 10);
        if (end == e || *end != ':') break;
        const char *tok = end + 1;
        size_t n = 0;
        while (tok[n] && tok[n] != ' ') n++;
        char act;
        g_scr[g_scr_n].t    = (uint32_t)t;
        g_scr[g_scr_n].code = tok_code(tok, n, &act);
        g_scr[g_scr_n].act  = act;
        g_scr[g_scr_n].done = 0;
        g_scr_n++;
        e = tok + n;
    }
}

static void script_tick(void)
{
    if (g_scr_n < 0) script_parse();
    uint32_t now = SDL_GetTicks() - g_scr_t0;
    for (int i = 0; i < g_scr_n; i++)
    {
        if (g_scr[i].done || now < g_scr[i].t) continue;
        g_scr[i].done = 1;
        if (g_scr[i].act == 'q') { SDL_Quit(); exit(0); }
        if (g_scr[i].act == 's')
        {
#ifdef YASAG_HAS_SCREENSHOT
            SDL_Log("script shot -> %s", screenshot_capture());
#endif
            continue;
        }
        iq_push(KEY_STATE_PRESSED,  g_scr[i].code);
        iq_push(KEY_STATE_RELEASED, g_scr[i].code);
    }
}

/* Drain SDL's queue into g_iq.  Also where SDL_QUIT (window close) is honoured. */
static void pump(void)
{
    script_tick();

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
            case SDL_QUIT:
                SDL_Quit();
                exit(0);
            case SDL_KEYDOWN:
                iq_push(e.key.repeat ? KEY_STATE_HOLD : KEY_STATE_PRESSED,
                        key_code(e.key.keysym));
                break;
            case SDL_KEYUP:
                iq_push(KEY_STATE_RELEASED, key_code(e.key.keysym));
                break;
            default:
                break;
        }
    }
}

void inp_init(void) { g_iq_head = g_iq_tail = 0; }

bool inp_poll(inp_event_t *out)
{
    if (g_iq_head == g_iq_tail) pump();
    if (g_iq_head == g_iq_tail) return false;
    *out = g_iq[g_iq_head];
    g_iq_head = (g_iq_head + 1) % IQ_CAP;
    return true;
}

void inp_flush(void)
{
    pump();
    g_iq_head = g_iq_tail = 0;
}

/* ======================================================================= *
 *  Audio  -  two square-wave voices, generated in the callback
 * ===================================================================== */

static SDL_AudioDeviceID g_dev;
static volatile int g_hz_l, g_hz_r;
static double g_ph_l, g_ph_r;

static void audio_cb(void *ud, Uint8 *stream, int len)
{
    (void)ud;
    SDL_AudioSpec *have = NULL; (void)have;
    int16_t *out = (int16_t *)stream;
    int frames = len / (int)(2 * sizeof(int16_t));
    int hz_l = g_hz_l, hz_r = g_hz_r;

    for (int i = 0; i < frames; i++)
    {
        int16_t sl = 0, sr = 0;
        if (hz_l > 0) {
            g_ph_l += (double)hz_l / 44100.0;
            g_ph_l -= (double)(int)g_ph_l;
            sl = (g_ph_l < 0.5) ? SND_AMP : -SND_AMP;
        } else g_ph_l = 0.0;
        if (hz_r > 0) {
            g_ph_r += (double)hz_r / 44100.0;
            g_ph_r -= (double)(int)g_ph_r;
            sr = (g_ph_r < 0.5) ? SND_AMP : -SND_AMP;
        } else g_ph_r = 0.0;
        out[2 * i]     = sl;
        out[2 * i + 1] = sr;
    }
}

void snd_init(void)
{
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq     = 44100;
    want.format   = AUDIO_S16SYS;
    want.channels = 2;
    want.samples  = 512;
    want.callback = audio_cb;

    g_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (g_dev) SDL_PauseAudioDevice(g_dev, 0);       /* start */
}

void snd_tone(uint16_t left_hz, uint16_t right_hz)
{
    g_hz_l = left_hz;
    g_hz_r = right_hz;
}

void snd_off(void)
{
    g_hz_l = g_hz_r = 0;
}

/* ======================================================================= *
 *  Time  -  hal_sleep_ms flushes the frame and keeps the window alive
 * ===================================================================== */

uint32_t hal_now_ms(void) { return SDL_GetTicks(); }

/* The MCUs draw straight to panel GRAM, so the game never calls vid_present().
 * On SDL the stand-in buffer has to be pushed to the window - and the game
 * ends every frame (and every menu-loop tick) with hal_sleep_ms(FRAME_MS), so
 * flushing here shows each screen at exactly the game's own cadence with no
 * change to game.c. */
void hal_sleep_ms(uint32_t ms)
{
    vid_present();

    uint32_t deadline = SDL_GetTicks() + ms;
    for (;;)
    {
        pump();                          /* window stays responsive; keys buffered */
        int32_t left = (int32_t)(deadline - SDL_GetTicks());
        if (left <= 0) break;
        SDL_Delay(left > 2 ? 1 : 0);     /* spin the final ~2 ms for tighter timing */
    }
}

uint32_t hal_entropy(void)
{
    return (uint32_t)SDL_GetPerformanceCounter();
}
