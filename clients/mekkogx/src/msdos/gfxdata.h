/*
 * gfxdata.h -- the tables tools/mkgfx_dos.py generates (gen/msdos/gfx.c).
 * Widths are in Atari colour clocks (two pixels each, on CGA and PCjr).
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

extern const uint8_t dos_font[64 * 8];      /* Namco, 8 pixels, bit 7 left */
extern const uint8_t plot_parts[12][64];    /* 32 lines x 16 colour clocks */
extern const uint8_t plot_dots[8][2];       /* line, colour clock */
extern const ShapeDef dos_shapes[];
extern const uint8_t dos_pics[4][8];        /* 8 x 8, shown 2 lines a row */
extern const uint8_t __far town_jr[];       /* packed RGBI; see mkgfx_dos.py */
extern const uint8_t rgbi_cga[16];          /* RGBI -> CGA palette 1 */
extern const uint8_t town_corral[4];        /* its grey box: x0 y0 x1 y1 */

/* plot_parts indexes */
#define PP_BORDER 0
#define PP_ICON   1     /* + ICON_* - 1 */
#define PP_MOUNT  7     /* + mountains - 1 */
#define PP_STORE  10
#define PP_RIVER  11

#endif
