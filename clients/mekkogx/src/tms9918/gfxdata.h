/*
 * gfxdata.h -- the tables tools/mkgfx_tms.py generates (gen/tms9918/gfx.c).
 *
 * x is scaled 3/2 from the Atari's colour clocks (see gfx.c): a plot's 16
 * clocks are 24 pixels, three cells. Shapes stay 1 bit a colour clock and
 * are widened as their sprite patterns are made (wide12).
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

extern const uint8_t tms_font[64 * 8];      /* 6 pixels, bit 7 left */
extern const uint8_t plot_parts[12][96];    /* 32 lines x 24 pixels (3 bytes) */
extern const uint8_t plot_dots[8][2];       /* line, pixel (2 pixels wide) */
extern const ShapeDef tms_shapes[];
extern const uint8_t tms_pics[4][8];        /* 8 x 8 clocks, shown 2 lines a row */
extern const uint16_t wide12[256];          /* 8 clocks -> 12 pixels, bit 11 left */
/* The town: cells 6..24 of cell rows 2..19, each cell row its 152 pattern
 * bytes then its 152 colour bytes (VRAM order), packed (see gfx_town). */
extern const uint8_t town_rle[];
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

/* Where the tables are out of reach (the MSX keeps them in a ROM bank that
 * shares the overlay window), the drawing brings them in around its work. */
#ifdef __MSX__
void bank_art(uint8_t on);
#define ART_BEGIN() bank_art(1)
#define ART_END()   bank_art(0)
#else
#define ART_BEGIN()
#define ART_END()
#endif

#endif
