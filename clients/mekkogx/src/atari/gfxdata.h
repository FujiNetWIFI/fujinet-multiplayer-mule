/*
 * gfxdata.h -- the tables tools/mkgfx_atari.py makes from the original
 * disk (gen/atari/gfx.c). The charsets are the original's, byte for byte,
 * placed at their addresses by segment (atari.h).
 */
#ifndef GFXDATA_H
#define GFXDATA_H

#include <stdint.h>

/* A sprite: frames of h bytes (one a scanline, bit 7 leftmost), shown at
 * player size `size` (SIZEP: 0 normal, 1 double, 3 quad). */
typedef struct {
    const uint8_t *data;
    uint8_t h, size;
} ShapeDef;

extern const ShapeDef atari_shapes[];       /* SH_COUNT */
extern const uint8_t town_scr[18 * 40];     /* the town's mode 4 screen */
extern const uint8_t town_pal[18][5];       /* per row: COLBK, PF0-3 */
extern const uint8_t mtn_shape[4][6];       /* the mountains, bottom aligned */
extern const uint8_t mtn_h[4];
extern const uint8_t store_p3[8];           /* the store: player 3, 4 lines a byte */
extern const uint8_t pics[4][8];            /* gfx.h PIC_*: 8 x 8, 1 bit a colour clock */

#endif
