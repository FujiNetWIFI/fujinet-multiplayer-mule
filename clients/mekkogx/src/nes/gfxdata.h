/*
 * gfxdata.h -- the tables tools/mkgfx_nes.py generates (gen/nes/gfx.c),
 * in the ART bank (src/nes/mule-nes.cfg), all but the town's: ART_BEGIN
 * brings it into the overlay window, ART_END puts the phase's overlay back.
 *
 * x is scaled 3/2 from the Atari's colour clocks, as on the TMS9918A (the
 * Adam's art, src/tms9918/gfxdata.h): a plot's 16 clocks are 24 pixels,
 * three cells. Shapes stay 1 bit a colour clock and are widened as their
 * sprite patterns are made (wide12).
 */
#ifndef GFXDATA_H
#define GFXDATA_H

#include <stdint.h>

typedef struct {
    const uint8_t *data;    /* frames one after another, rows of wb bytes */
    uint8_t wb;             /* bytes a row (1 bit a colour clock, MSB left) */
    uint8_t w;              /* width in colour clocks */
    uint8_t h;              /* lines */
} ShapeDef;

extern const uint8_t nes_font[64 * 8];      /* 8 x 8 (Namco), bit 7 left */
extern const uint8_t plot_parts[12][96];    /* 32 lines x 24 pixels (3 bytes) */
extern const uint8_t plot_dots[8][2];       /* line, pixel (2 pixels wide) */
extern const ShapeDef nes_shapes[];
extern const uint8_t nes_pics[4][8];        /* 8 x 8 clocks, shown 2 lines a row */
extern const uint8_t river_pat[4][32];      /* the river's sprites: 2 x 2 pairs */
extern const uint8_t river_x0;              /* their first pixel in the plot */
extern const uint16_t wide12[256];          /* 8 clocks -> 12 pixels, bit 11 left */

/* The town: cells 6..24 of rows 2..19, each row its 19 cells' tiles (16
 * bytes: plane 0, then plane 1), packed (see gfx_town); and the palette of
 * each attribute block it covers (blocks 3..12 of block rows 2..10). These
 * two are in the development phase's overlay, not the art's bank. */
extern const uint8_t town_rle[];
extern const uint8_t town_pal[9][10];
#define TOWN_CX    6
#define TOWN_CW    19
#define TOWN_CY    2
#define TOWN_CH    18

/* plot_parts indexes */
#define PP_BORDER 0
#define PP_ICON   1     /* + ICON_* - 1 */
#define PP_MOUNT  7     /* + mountains - 1 */
#define PP_STORE  10
#define PP_RIVER  11

void bank_art(uint8_t on);
#define ART_BEGIN() bank_art(1)
#define ART_END()   bank_art(0)

#endif
