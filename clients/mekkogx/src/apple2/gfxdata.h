/*
 * gfxdata.h -- the tables tools/mkgfx_hgr.py generates (gen/apple2/gfx.c).
 */
#ifndef GFXDATA_H
#define GFXDATA_H

#include <stdint.h>

typedef struct {            /* (8 bytes: hires.s src_of indexes it so) */
    const uint8_t *data;    /* frames one after another, rows of wb bytes */
    uint8_t wb;             /* bytes a row (1 bit a colour pixel, MSB left) */
    uint8_t w;              /* width in colour pixels */
    uint8_t h;              /* lines */
    uint8_t sz;             /* bytes a frame: wb * h */
    uint8_t st;             /* a save ring's row: the most screen bytes a row covers */
    uint8_t rows;           /* rows in a ring: SAVE_MAX / st */
} ShapeDef;

extern const uint8_t hgr_font[64 * 8];
extern const uint8_t plot_parts[12][128];
extern const uint8_t plot_dots[8][5];
extern const ShapeDef hgr_shapes[];
extern const uint8_t hgr_pics[4][8];
extern const uint8_t hgr_town[144 * 24];

/* plot_parts indexes */
#define PP_BORDER 0
#define PP_ICON   1     /* + ICON_* - 1 */
#define PP_MOUNT  7     /* + mountains - 1 */
#define PP_STORE  10
#define PP_RIVER  11

#endif
