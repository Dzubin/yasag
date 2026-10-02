/*
 *  YASAG  -  screen capture for the SDL target
 *  ---------------------------------------------------------------------------
 *  Built only when CMake defines YASAG_HAS_SCREENSHOT.  Implements
 *  screenshot_capture() from screenshot.h by writing the HAL's RGB565
 *  stand-in buffer (hal_sdl.c) out to a 24-bit .BMP beside the executable.
 *  No SDL dependency here - just stdio.
 */
#include "game.h"

#ifdef YASAG_HAS_SCREENSHOT

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "screenshot.h"
#include "hal_sdl.h"        /* sdl_fb() */

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

const char *screenshot_capture(void)
{
    static char result[32];
    const uint16_t *fb = sdl_fb();
    if (!fb) return "no framebuffer";

    /* pick a free-ish random name */
    static uint32_t s;
    if (!s) s = (uint32_t)time(NULL) | 1u;
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;

    char path[24];
    snprintf(path, sizeof path, "SHOT%04X.bmp", (unsigned)(s & 0xFFFFu));

    FILE *f = fopen(path, "wb");
    if (!f) return "BMP create failed";

    const uint32_t row_bytes = ((uint32_t)SCREEN_W * 3 + 3) & ~3u;   /* 4-aligned */
    const uint32_t pix_bytes = row_bytes * (uint32_t)SCREEN_H;

    uint8_t hdr[54] = { 0 };
    hdr[0] = 'B'; hdr[1] = 'M';
    put_u32(hdr + 2,  54 + pix_bytes);
    put_u32(hdr + 10, 54);
    put_u32(hdr + 14, 40);
    put_u32(hdr + 18, (uint32_t)SCREEN_W);
    put_u32(hdr + 22, (uint32_t)SCREEN_H);     /* +ve height -> bottom-up rows */
    hdr[26] = 1;
    hdr[28] = 24;
    put_u32(hdr + 34, pix_bytes);
    put_u32(hdr + 38, 2835);
    put_u32(hdr + 42, 2835);

    int ok = (fwrite(hdr, 1, sizeof hdr, f) == sizeof hdr);

    uint8_t *row = (uint8_t *)calloc(1, row_bytes);
    if (!row) { fclose(f); remove(path); return "BMP write failed"; }

    for (int y = SCREEN_H - 1; y >= 0 && ok; y--)   /* bottom-up */
    {
        const uint16_t *src = &fb[(size_t)y * SCREEN_W];
        for (int x = 0; x < SCREEN_W; x++)
        {
            uint16_t p = src[x];
            uint8_t r5 = (p >> 11) & 0x1F, g6 = (p >> 5) & 0x3F, b5 = p & 0x1F;
            row[x * 3 + 0] = (uint8_t)((b5 << 3) | (b5 >> 2));   /* B */
            row[x * 3 + 1] = (uint8_t)((g6 << 2) | (g6 >> 4));   /* G */
            row[x * 3 + 2] = (uint8_t)((r5 << 3) | (r5 >> 2));   /* R */
        }
        if (fwrite(row, 1, row_bytes, f) != row_bytes) ok = 0;
    }

    free(row);
    fclose(f);
    if (!ok) { remove(path); return "BMP write failed"; }

    snprintf(result, sizeof result, "Saved %s", path);
    return result;
}

#endif /* YASAG_HAS_SCREENSHOT */
