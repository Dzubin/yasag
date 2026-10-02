/*
 *  YASAG  -  fixed-cell text  (see text.h)
 *  ---------------------------------------------------------------------------
 *  Replaces the vendored lcd_putstr path.  Each glyph row is drawn as a few
 *  vid_fill_rect() spans (equal-colour runs are coalesced, so a blank column
 *  costs one call, not eight) - fast enough on the PicoCalc / PicoSystem SPI
 *  panels, trivial on a PC.
 */
#include "text.h"
#include "hal.h"
#include "font.h"

static uint16_t s_fg = 0xFFFF;   /* white on ... */
static uint16_t s_bg = 0x0000;   /* ... black, until the game sets its own */
static int      s_scale = YASAG_TEXT_SCALE;   /* runtime glyph magnification, >= 1 */

void text_set_fg(uint16_t rgb565) { s_fg = rgb565; }
void text_set_bg(uint16_t rgb565) { s_bg = rgb565; }
void text_set_scale(int scale)    { s_scale = (scale < 1) ? 1 : scale; }
int  text_scale(void)             { return s_scale; }

void text_clear(void)
{
    vid_fill_rect(s_bg, 0, 0, SCREEN_W, SCREEN_H);
}

static void put_glyph(int col, int row, unsigned char ch)
{
    const uint8_t *g  = &font_8x10.glyphs[(unsigned)ch * GLYPH_HEIGHT];
    const int      s  = s_scale;             /* 1 -> 8x10 px cell (identity) */
    const int      x0 = col * 8  * s;
    const int      y0 = row * 10 * s;

    for (int gy = 0; gy < GLYPH_HEIGHT; gy++)
    {
        const uint8_t bits = g[gy];
        int gx = 0;
        while (gx < 8)                       /* coalesce equal-colour runs */
        {
            const int on  = (bits >> (7 - gx)) & 1;
            int       run = 1;
            while (gx + run < 8 && (((bits >> (7 - gx - run)) & 1) == on))
                run++;
            vid_fill_rect(on ? s_fg : s_bg, x0 + gx * s, y0 + gy * s, run * s, s);
            gx += run;
        }
    }
}

void text_putstr(int col, int row, const char *s)
{
    for (; *s; s++, col++)
        put_glyph(col, row, (unsigned char)*s);
}
