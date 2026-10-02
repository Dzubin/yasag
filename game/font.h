/*
 *  YASAG  -  8x10 bitmap font interface
 *  ---------------------------------------------------------------------------
 *  Portable core header for the vendored glyph table in font_8x10.c (copied
 *  from picocalc-text-starter's drivers/font-8x10.c, by Blair Leduc - see that
 *  file's own header).  Same layout as the starter's drivers/font.h so the
 *  vendored data compiles unchanged, but with no  pico/stdlib.h  dependency so
 *  it builds for the SDL target too.  YASAG only ships the 8x10 face (the
 *  5x10 one the starter also carries is not used and not built).
 */
#ifndef YASAG_FONT_H
#define YASAG_FONT_H

#include <stdint.h>

#define GLYPH_HEIGHT 10          /* pixels per glyph row; 8 px wide, fixed cell */

typedef struct {
    uint8_t width;
    uint8_t glyphs[];            /* 256 glyphs, GLYPH_HEIGHT bytes each, MSB = left */
} font_t;

extern const font_t font_8x10;

#endif /* YASAG_FONT_H */
