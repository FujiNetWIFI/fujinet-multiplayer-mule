/*
 * gfx.c -- gfx.h on the Atari, drawn the way the original draws.
 *
 * Nothing is scaled: gfx.h's coordinates are the Atari's. The screen is 24
 * rows of 8 scanlines, each row in the ANTIC mode and with the charset and
 * colours it needs (the row tables of gfxa.s):
 *   text   mode 2, the original's font (small text is one colour a row)
 *   big    mode 6, the font 20 across in up to four colours a row
 *   draw   mode 4 cells made on the fly (fills, dotted lines, pictures)
 *   map    mode 6, the original's map characters (plots, bar, dots)
 *   town   mode 4, the original's town characters and picture
 * Sprites are players (single-line PMG) multiplexed down the screen by row:
 * a player can show several sprites whose rows don't meet. On the map
 * player 2 draws the mountains (one a row) and player 3 the river and the
 * store, as in the original.
 */
#ifdef __ATARI__

#include <string.h>
#include "gfx.h"
#include "atari.h"
#include "gfxdata.h"

/* ---- colours ---------------------------------------------------------- */
const uint8_t atari_col[16] = {
    0x00, 0x0E, 0x84, 0x54, 0xC4, 0x34, 0x32, 0x1C,
    0x08, 0xA8, 0x24, 0x2A, 0x88, 0xD4, 0x5A, 0x34,
};

/* ---- rows ------------------------------------------------------------- */
#define RM_TEXT 0
#define RM_BIG  1
#define RM_DRAW 2
#define RM_MAP  3
#define RM_TOWN 4
#define SK_GEN  0
#define SK_MAP  1
#define SK_TOWN 2
uint8_t scr_kind;                       /* draw.s */
uint8_t row_mode[ROWS];                 /* sprite.s: which colours a row uses */
uint8_t text_bg, text_fg;               /* the text rows' PF2 / PF1 (draw.s) */
extern uint8_t pf_dirty, sp_dirty;      /* sprite.s: rebuild the row flags */
extern uint8_t spr_on[SPR_SLOTS], spr_shape[SPR_SLOTS], spr_frame[SPR_SLOTS];
extern uint8_t spr_xl[SPR_SLOTS], spr_xh[SPR_SLOTS], spr_yl[SPR_SLOTS], spr_yh[SPR_SLOTS];
extern uint8_t spr_col[SPR_SLOTS], spr_flags[SPR_SLOTS], spr_changed, spr_players;

/* cells.s: the drawing rows */
extern uint8_t row_used[ROWS], row_lo[ROWS], row_hi[ROWS];
extern uint8_t cp_r, cp_cx, cp_line, cp_mask, cp_col;
void cell_put(void);
uint8_t __fastcall__ row_slot(uint8_t col);     /* in row cp_r */
void dyn_reset(void);
void __fastcall__ dyn_release(uint8_t c);

/* mapgfx.s: the plots */
extern uint8_t mp_uses[20], mbar[ROWS], mbar_col, map_pr;
void map_reset(void);
void __fastcall__ map_bar(uint8_t r);
uint8_t __fastcall__ map_slot(uint8_t col);      /* in plot row map_pr */

#define ROWADDR(r) ((uint8_t *)(row_lo[r] | (row_hi[r] << 8)))

/* draw.s: the rows, text and drawing */
void __fastcall__ row_setmode(uint8_t r, uint8_t m);
void dl_build(void);

/* ---- screens ----------------------------------------------------------- */
static void pmg_clear(void)
{
    memset(MISSILES, 0, 0x500);
    memset(row_hp, 0, sizeof(uint8_t) * 4 * ROWS);
    memset(row_sz, 0, sizeof(uint8_t) * 4 * ROWS);
    dsp_hposm[3] = 0;
}

static void screen_start(uint8_t kind, uint8_t bk, uint8_t tbg, uint8_t tfg)
{
    uint8_t r;
    scr_kind = kind;
    spr_hide_all();
    spr_flush();
    /* the players the sprites may use: on the map 2 and 3 are terrain, in
     * the town 3 colours the (missile) time bar */
    spr_players = kind == SK_MAP ? 2 : kind == SK_TOWN ? 3 : 4;
    pmg_clear();
    dsp_colbk = bk;
    text_bg = tbg;
    text_fg = tfg;
    dsp_prior = kind == SK_MAP ? 0x02 : 0x01;
    memset(SCREEN, 0, 40 * ROWS);
    dyn_reset();
    for (r = 0; r < ROWS; ++r)
        row_setmode(r, RM_TEXT);
    dl_build();
}

void gfx_clear(void)
{
    screen_start(SK_GEN, 0x00, 0x00, 0x0E);
}

/* ---- the map ----------------------------------------------------------- */
#define MAP_ROW0 2              /* rows 2-21: 5 plot rows of 4 */
#define PMY(y) (32 + (y))       /* scanline -> PMG byte */
#define P2HP (row_hp + 2 * ROWS)
#define P3HP (row_hp + 3 * ROWS)
#define P3CM (row_cm + 3 * ROWS)
#define P3SZ (row_sz + 3 * ROWS)

/* The river: player 3, dotted, wandering a little from row to row. */
static const uint8_t river_dots[8] = { 0x12, 0x14, 0x28, 0x2A, 0x48, 0x4A, 0x52, 0x54 };

static void map_river(void)
{
    uint8_t y, r, *pl = PLAYER(3) + PMY(16);
    uint8_t x = PLOT_X(4) + 48 + 4;
    for (y = 0; y < 160; ++y)
        pl[y] = (y & 1) ? river_dots[RANDOM & 7] : 0;
    for (r = MAP_ROW0; r < MAP_ROW0 + 20; ++r) {
        x += (RANDOM & 3) - 1;
        if (x < PLOT_X(4) + 48)
            x = PLOT_X(4) + 48;
        if (x > PLOT_X(4) + 48 + 8)
            x = PLOT_X(4) + 48 + 8;
        P3HP[r] = x;
        P3CM[r] = 0x16;
    }
    sp_dirty = 1;
}

void gfx_map_frame(void)
{
    uint8_t r;
    screen_start(SK_MAP, 0x08, 0x08, 0x00);
    for (r = MAP_ROW0; r < MAP_ROW0 + 20; ++r)
        row_setmode(r, RM_MAP);
    map_reset();
    memset(row_cm + 2 * ROWS, 0x06, ROWS);      /* the mountains are grey */
    map_river();
    dl_build();
}

/* A picture on the map (the store's fire): map characters $37-$3E, two for
 * each quarter of the store plot. */
/* ---- the town ---------------------------------------------------------- */
#define TOWN_ROW0 2

void gfx_town(void)
{
    uint8_t r, *pf;
    const uint8_t *c;
    screen_start(SK_TOWN, 0x08, 0x08, 0x00);
    memcpy(ROWADDR(TOWN_ROW0), town_scr, 18 * 40);
    for (r = 0; r < 18; ++r) {
        row_setmode(TOWN_ROW0 + r, RM_TOWN);
        pf = row_pf + TOWN_ROW0 + r;
        c = town_pal[r];
        pf[0] = c[1];
        pf[ROWS] = c[2];
        pf[2 * ROWS] = c[3];
        pf[3 * ROWS] = c[4];
    }
    dl_build();
}

/* ---- start-up ------------------------------------------------------------ */
#define SDMCTL (*(volatile uint8_t *)0x022F)
#define SDLSTL (*(volatile uint16_t *)0x0230)
#define GRACTL (*(volatile uint8_t *)0xD01D)
#define PMBASE (*(volatile uint8_t *)0xD407)

void gfx_init(void)
{
    memset(DLIST, 0, 0x300);            /* and the DLI tables (PMGFREE) */
    gfx_clear();
    dsp_init();
    PMBASE = PMBASE_PAGE;
    GRACTL = 3;
    SDLSTL = (uint16_t)DLIST;
    SDMCTL = 0x3E;                      /* normal width, players + missiles single line */
}

#endif
