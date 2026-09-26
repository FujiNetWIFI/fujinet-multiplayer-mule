/*
 * gfxdata.h -- the tables tools/mkgfx_lynx.py generates (gen/lynx/gfx.c).
 * Pictures are 1 bit a pixel, MSB left, a line after another, half the
 * Atari's height; widths are the Atari's colour clocks.
 */
#ifndef GFXDATA_H
#define GFXDATA_H

#include <stdint.h>

typedef struct {
    const uint8_t *data;    /* frames one after another */
    uint8_t fsize;          /* bytes a frame */
    uint8_t wb;             /* bytes a line */
    uint8_t w;              /* pixels across */
    uint8_t h;              /* lines (Lynx) */
} ShapeDef;

extern const uint8_t lynx_pal[32];          /* Mikey: green[16], blue<<4|red[16] */
extern const uint8_t lynx_font[64][5];      /* 3 x 5, ASCII 32..95 */
extern const uint8_t plot_spr[12][32];      /* 16 x 16 */
extern const uint8_t plot_dots[8][2];       /* line, x (in the plot) */
extern const ShapeDef lynx_shapes[];        /* SH_COUNT */
extern const uint8_t lynx_pics[4][8];       /* 8 x 8 */
extern const uint8_t town_lx[];             /* packed; see mkgfx_lynx.py */

/* plot_spr indexes */
#define PP_BORDER 0
#define PP_ICON   1     /* + ICON_* - 1 */
#define PP_MOUNT  7     /* + mountains - 1 */
#define PP_STORE  10
#define PP_RIVER  11

#define PEN_RIVER 15

#endif
