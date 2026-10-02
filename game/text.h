/*
 *  YASAG  -  fixed-cell text
 *  ---------------------------------------------------------------------------
 *  The game's replacement for the vendored lcd_putstr / lcd_set_foreground /
 *  lcd_set_background / lcd_clear_screen.  Built entirely on vid_fill_rect(),
 *  so it works identically on every target.
 *
 *  (col, row) are 8x10 glyph CELLS: the pixel origin is (col*8, row*10).  The
 *  cell is 8 wide and GLYPH_HEIGHT tall, and is painted in full - foreground
 *  where the glyph bit is set, background everywhere else - so a shorter
 *  string overwrites ("self-erases") a longer one drawn earlier at the same
 *  spot, exactly as the old LCD driver behaved.  Deliberately no font
 *  scaling: chunky pixels on a large SDL window are the intended retro look.
 */
#ifndef YASAG_TEXT_H
#define YASAG_TEXT_H

#include <stdint.h>

void text_set_fg(uint16_t rgb565);
void text_set_bg(uint16_t rgb565);
void text_putstr(int col, int row, const char *s);
void text_clear(void);            /* fill the whole screen with the current background */

/* Runtime glyph magnification, >= 1 (starts at YASAG_TEXT_SCALE).  (col,row)
 * then address cells of 8*scale x 10*scale px.  A dense screen (the controls
 * legend) drops to 1 so it fits; the status line and menus stay at the default.  On the
 * MCUs YASAG_TEXT_SCALE is 1, so this only ever moves on desktop. */
void text_set_scale(int scale);
int  text_scale(void);

#endif /* YASAG_TEXT_H */
