/*
 * gfx.h -- the screen, in the Atari original's terms.
 *
 * Coordinates are the Atari's: x in colour clocks (0..159 across the
 * playfield), y in scanlines (0..191). Text is a TCOLS x 24 grid of cells:
 * 40 across, a cell 4 colour clocks by 8 lines, except on the TMS9918A (the
 * Adam, the MSX), whose text is a whole 8 x 8 cell of its own, 32 across.
 * Each platform scales to its screen.
 *
 *   Map    plot p at x 8 + 16 * (p % 9), y 16 + 32 * (p / 9), 16 x 32
 *   Town   x 32..127, y 16..159
 *
 * Sprites are numbered slots the game moves around, like hardware sprites;
 * spr_flush() puts the changes on the screen (once a frame). Background
 * drawing is safe under sprites: they are redrawn over it.
 */
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

/* Colours (the Atari colours the game uses; a platform picks the nearest). */
#define C_BLACK   0
#define C_WHITE   1
#define C_BLUE    2     /* player 0 ($84) */
#define C_PURPLE  3     /* player 1 ($54) */
#define C_GREEN   4     /* player 2 ($C4) */
#define C_ORANGE  5     /* player 3 ($34) */
#define C_RED     6
#define C_YELLOW  7
#define C_GREY    8
#define C_CYAN    9
#define C_BROWN   10
#define C_TAN     11
#define C_LTBLUE  12
#define C_DKGREEN 13
#define C_PINK    14
#define C_P3      15    /* player 3: orange where a platform can (the CoCo: stripes) */

/* The text grid. TCOLS cells across; BIGW cells a big character; TX(c)
 * is the cell over the same spot of the picture as the 40-column cell c
 * (text laid over the art: the town's prices, the auction's scale). */
#if defined(__ADAM__) || defined(__MSX__)
#define TCOLS 32
#define BIGW  1
#define TX(c) ((uint8_t)(((c) * 3 + 2) >> 2))
#else
#define TCOLS 40
#define BIGW  2
#define TX(c) (c)
#endif

/* Screen layout helpers. */
#define PLOT_X(p)   (8 + 16 * ((p) % 9))
#define PLOT_Y(p)   (16 + 32 * ((p) / 9))

void gfx_init(void);

/* Blank the whole screen (sprites are hidden too). */
void gfx_clear(void);

/* Text. Strings are ASCII; lower case prints as upper case. */
void gfx_text(uint8_t tx, uint8_t ty, const char *s, uint8_t col);
void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col);
void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col);
void gfx_clear_text(uint8_t tx, uint8_t ty, uint8_t w);    /* w cells */

/* Big text: double-width characters (TCOLS / BIGW across), for coloured
 * text (small text may only look right in white). tx is a cell, rounded
 * down to even where BIGW is 2; a character covers BIGW cells. */
void gfx_big(uint8_t tx, uint8_t ty, const char *s, uint8_t col);
void gfx_bign(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col);

/* Rectangles and lines, in Atari coordinates. */
void gfx_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col);
void gfx_hdots(uint8_t x, uint8_t y, uint8_t w, uint8_t col);    /* dotted */

/* A map plot. */
#define ICON_NONE     0
#define ICON_HOUSE    1
#define ICON_FOOD     2     /* ICON_FOOD + GOOD_* */
#define ICON_ENERGY   3
#define ICON_SMITHORE 4
#define ICON_CRYSTITE 5
#define ICON_MULE     6
typedef struct {
    uint8_t store;          /* 1: the store */
    uint8_t river;          /* 1: the river runs through it */
    uint8_t mountains;      /* 0..3 */
    uint8_t owned;          /* 1: bordered in col */
    uint8_t icon;           /* ICON_* (owned plots) */
    uint8_t dots;           /* production dots 0..8 */
    uint8_t col;            /* the owner's colour, or C_WHITE outlined */
} PlotLook;
void gfx_plot(uint8_t p, const PlotLook *look);
void gfx_map_frame(void);   /* the map's surroundings (clear + frame) */

/* The town (the Atari's picture of it), in its box. */
void gfx_town(void);

/* Small pictures drawn into the background. */
#define PIC_STOREICON 0
#define PIC_FLAG      1
#define PIC_FIRE0     2
#define PIC_FIRE1     3
void gfx_pic(uint8_t x, uint8_t y, uint8_t pic, uint8_t col);

/* ---- sprites -------------------------------------------------------- */
#define SPR_SLOTS 10

#define SH_SPECIES   0      /* frame = species * 11 + f (8 x 22) */
#define SH_FIGURE    1      /* frame = species * 10 + f (map figure, 8 x 8) */
#define SH_MULE      2      /* frames 0-1 */
#define SH_WAMPUS    3
#define SH_CURSOR    4      /* land grant cursor: a plot's outline */
#define SH_NOSE      5      /* frame 0 transport, 1 pirate */
#define SH_HULL_TOP  6      /* frame 0 transport, 1 pirate */
#define SH_HULL_BOT  7
#define SH_DOCK      8      /* frames 0-3 (largest first) */
#define SH_PEST      9      /* frames 0-3 */
#define SH_CLOUD     10     /* frames 0-3 */
#define SH_BOLT      11
#define SH_SPARK     12
#define SH_METEOR    13
#define SH_COUNT     14

#define SPR_FLIPX    1      /* mirror left-right */

void spr_set(uint8_t slot, uint8_t shape, uint8_t frame, int16_t x, int16_t y,
             uint8_t col, uint8_t flags);
void spr_hide(uint8_t slot);
void spr_hide_all(void);
void spr_flush(void);       /* draw what changed (once a frame) */

#endif
