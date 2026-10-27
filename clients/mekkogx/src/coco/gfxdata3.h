/*
 * gfxdata3.h -- tables from tools/mkgfx_coco3.py (gen/coco3/). Widths are
 * in Atari color clocks, one byte each on screen. art3.c is in block 3,
 * behind the screen: read it through the window with ART().
 */
#ifndef GFXDATA3_H
#define GFXDATA3_H

#include <stdint.h>

typedef struct {
    const uint8_t *data;    /* frames one after another, rows of rowb bytes */
    uint8_t rowb;           /* bytes a row (1 bit a color clock, MSB left) */
    uint8_t wid;            /* color clocks */
    uint8_t hgt;            /* lines */
} ShapeDef;

extern const uint8_t c3_pal[16];            /* C_* as GIME RGB codes */
extern const uint8_t c3_pal_cmp[16];        /* and as composite codes */
extern const uint8_t c3_font[64 * 8];       /* Namco, 8 pixels, bit 7 left */
extern const uint8_t plot_parts[12][64];    /* 32 lines x 16 color clocks */
extern const uint8_t plot_dots[8][2];       /* line, color clock */
extern const ShapeDef c3_shapes[];
extern const uint8_t *const c3_slide[];     /* per shape, its rows' changing clocks, or 0 (slide.c) */
extern const uint8_t c3_pics[4][8];         /* 8 x 8, shown 2 lines a row */
extern const uint8_t town_rle[];            /* a nibble a clock, packed; see mkgfx_coco3.py */

/* gfx3.c, for town3.c and auction3.c: the drawing window and its hooks */
#define MMU6     (*(volatile uint8_t *)0xFFA6)
#define WINDOW   ((uint8_t *)0xC000)
#define TOWN_BLK 4
#define ART_BLK  3
#define ART(ptr) ((const uint8_t *)(ptr) - 0x2000)
#define LINE     160
#define CC(col)  ((uint8_t)((col) * 0x11))  /* a clock of color col */
uint8_t *gfx3_row(uint8_t line);
uint8_t gfx3_cc(uint8_t col);       /* color col as a clock, black as the background */
void gfx3_done(void);
void spr_bg(uint8_t left, uint8_t wid, uint8_t top, uint8_t hgt);
void bg_done(void);

/* plot_parts indexes */
enum plot_part {
    PP_BORDER,
    PP_ICON,                /* + ICON_* - 1 */
    PP_MOUNT = 7,           /* + mountains - 1 */
    PP_STORE = 10,
    PP_RIVER
};

#endif
