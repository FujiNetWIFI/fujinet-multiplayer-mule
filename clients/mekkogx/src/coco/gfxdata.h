/*
 * gfxdata.h -- the tables tools/mkgfx_pmode4.py generates (gen/coco/gfx.c,
 * gen/coco/town.c).
 */
#ifndef GFXDATA_H
#define GFXDATA_H

#include <stdint.h>

typedef struct {
    const uint8_t *data;    /* frames one after another, rows of wb bytes */
    uint8_t wb;             /* bytes a row (1 bit a colour pixel, MSB left) */
    uint8_t w;              /* width in colour pixels */
    uint8_t h;              /* lines */
} ShapeDef;

extern const uint8_t pm_font[64 * 8];       /* 6 pixels: bits 7..2 */
extern const uint8_t plot_parts[12][64];    /* 32 lines x 12 colour pixels */
extern const uint8_t plot_dots[8][2];       /* line, colour pixel */
extern const ShapeDef pm_shapes[];
extern const uint8_t pm_pics[4][8];
extern const uint8_t pm_town[];             /* 144 x 19 bytes from byte 6, packed */

/* plot_parts indexes */
#define PP_BORDER 0
#define PP_ICON   1     /* + ICON_* - 1 */
#define PP_MOUNT  7     /* + mountains - 1 */
#define PP_STORE  10
#define PP_RIVER  11

#endif
