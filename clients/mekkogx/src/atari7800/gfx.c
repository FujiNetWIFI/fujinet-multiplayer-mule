/*
 * gfx.c -- gfx.h on the Atari 7800, drawn the way the Atari 800 port draws
 * (src/atari/gfx.c): the same screen bytes, characters, colours and
 * sprites, the original's own, which maria.s turns into MARIA's display
 * lists and nmi.s's DLIs into colours and modes zone by zone.
 *
 * Nothing is scaled: gfx.h's coordinates are the Atari's, and MARIA's 160A
 * is its playfield's 160 colour clocks. The screen is 24 rows of 8 lines,
 * each row in the mode it needs (a78.inc RM_*): text (the original's font
 * in 320A, a line of it a byte), big, draw, map and town (160A). Its
 * graphics come from the ART bank into the cart RAM's character blocks
 * (a78.inc): the font (TXTB), big text (BIGB), the map's glyphs or the
 * town's characters (MTB, whichever screen is up), the drawing cells.
 */
#include <string.h>
#include "gfx.h"
#include "a78hw.h"
#include "../atari/gfxdata.h"

/* ---- colours ---------------------------------------------------------- */
const uint8_t atari_col[16] = {
    0x00, 0x0E, 0x84, 0x54, 0xC4, 0x34, 0x32, 0x1C,
    0x08, 0xA8, 0x24, 0x2A, 0x88, 0xD4, 0x5A, 0x34,
};

/* ---- rows ------------------------------------------------------------- */
#define SK_GEN  0
#define SK_MAP  1
#define SK_TOWN 2
uint8_t scr_kind;
uint8_t row_mode[ROWS];
uint8_t row_pf[4 * ROWS];               /* [i * ROWS + row]: PF0-3 */
uint8_t text_bg, text_fg;               /* the text rows' PF2 / PF1 */
uint8_t pf_dirty;
uint8_t pics_ram[4][8];                 /* gfx_pic's pictures (draw.s) */
static uint8_t mt_on;                   /* the frame's DLI runs */

/* draw.s */
void __fastcall__ row_setmode(uint8_t r, uint8_t m);
void dl_build(void);
extern uint8_t t_code;
/* cells.s */
void dyn_reset(void);
/* mapgfx.s */
extern uint8_t mbar[ROWS], expand_v[48];
void map_reset(void);
void __fastcall__ mtb_glyph(const uint8_t *src);
void __fastcall__ mtb_town(const uint8_t *chars);
/* sprite.s */
void spr_pieces(void);
/* gen/atari7800/gfxhi.s */
extern const uint8_t townchars[1024];
extern const uint8_t font[512], mapchars[512];

/* MTB holds the map's glyphs or the town's characters */
#define MTB_NONE 0
#define MTB_MAP  1
#define MTB_TOWN 2
static uint8_t mtb_kind;

/* A byte of 1-bit pixels as two 160A bytes in value v (1-3) at p, p + 1. */
static void expand2(uint8_t *p, uint8_t b, uint8_t v)
{
    const uint8_t *t = expand_v + ((v - 1) << 4);
    p[0] = t[b >> 4];
    p[1] = t[b & 15];
}

#define LINE(block, k) ((uint8_t *)(block) + ((7 - (k)) << 8))

/* A colour as this console shows it: a PAL one's hues sit one higher (grey
 * and hue 15 as they are). */
static uint8_t pal_tv(uint8_t c)
{
    if (mt_ntsc || c < 0x10 || c >= 0xF0)
        return c;
    return c + 0x10;
}

/* ---- the DLI tables ---------------------------------------------------------- */
static const uint8_t mode_cb[5] = { CB_TXT, CB_BIG, CB_DYN, CB_MT, CB_MT };
static const uint8_t mode_ct[5] = { CT_320, CT_160 | CT_CW2, CT_160, CT_160 | CT_CW2, CT_160 };

/* Each zone's CHARBASE, CTRL and colours, and a DLI where they differ from
 * what MARIA has by then (nmi.s); the frame's DLI last. A text zone shows
 * colour 2 of palette A only, so the rest is left as it is. */
static void mt_tables(void)
{
    static uint8_t cur[2][6], pal[6];
    uint8_t z, k = 0, m, p, i, cb, ct, pcb = 0xFF, pct, late, need, tc;
    const uint8_t *pf;

    memset(cur, 0xFF, sizeof(cur));
    pct = mode_ct[row_mode[ROWS - 1]];          /* the bottom's, from the last frame */
    for (z = 0; z < ROWS; ++z) {
        m = row_mode[z];
        cb = mode_cb[m];
        ct = mode_ct[m];
        p = z & 1;
        pf = row_pf + z;
        memcpy(pal, cur[p], 6);
        switch (m) {
        case RM_TEXT:
            tc = (pf[2 * ROWS] & 0xF0) | (pf[ROWS] & 0x0F);
            pal[1] = tc;
            break;
        case RM_BIG:
            pal[0] = pf[0];
            pal[1] = pf[ROWS];
            pal[3] = pf[2 * ROWS];
            pal[4] = pf[3 * ROWS];
            break;
        case RM_DRAW:
            pal[0] = pal[3] = pf[0];
            pal[1] = pal[4] = pf[ROWS];
            pal[2] = pf[2 * ROWS];
            pal[5] = pf[3 * ROWS];
            break;
        case RM_MAP:
            pal[0] = pal[4] = pf[0];
            pal[1] = pal[5] = pf[ROWS];
            pal[2] = pf[2 * ROWS];
            pal[3] = pf[3 * ROWS];
            break;
        default:                                /* town */
            pal[0] = pf[0];
            pal[1] = pf[ROWS];
            pal[2] = pf[3 * ROWS];
            break;
        }
        need = z < 2 || cb != pcb || ct != pct || memcmp(pal, cur[p], 6);
        nx_dli[z] = need;
        if (need) {
            late = (ct & 3) == 3 && (pct & 3) == 0;
            nx_cb[k] = cb;
            nx_ca[k] = late ? (uint8_t)((ct & ~3) | (pct & 3)) : ct;
            nx_cb2[k] = ct;
            nx_f[k] = (late ? 0x80 : 0) | (p ? 0x40 : 0);
            for (i = 0; i < 6; ++i)
                nx_p[i * NDLI + k] = pal_tv(pal[i]);
            if (late)
                cur[p][1] = pal[1];             /* (only that one is written) */
            else
                memcpy(cur[p], pal, 6);
            ++k;
        }
        pcb = cb;
        pct = ct;
    }
    nx_cb[k] = pcb;                             /* the frame's */
    nx_ca[k] = pct;
    nx_cb2[k] = pct;
    nx_f[k] = 0x20;
    nx_n = k + 1;
    tbl_pending = 1;
}

/* Once a frame (and from frame_wait): the sprites' pieces, the colours if
 * they changed, the zones that did; nothing while the last set waits for
 * the frame's DLI. */
void spr_flush(void)
{
    if (dsp_ready)
        return;
    spr_pieces();
    if (pf_dirty) {
        pf_dirty = 0;
        mt_tables();
    }
    mt_compose();
}

/* ---- screens ----------------------------------------------------------- */
static void screen_start(uint8_t kind, uint8_t bk, uint8_t tbg, uint8_t tfg)
{
    uint8_t r;
    while (mt_on && dsp_ready)          /* the last set is shown */
        ;
    scr_kind = kind;
    spr_hide_all();
    dsp_colbk = bk;
    mt_blank();                         /* nothing till the new one is whole */
    text_bg = tbg;
    text_fg = tfg;
    memset(SCREEN, 0, 40 * ROWS);
    memset(tr_pw, 0, sizeof(tr_pw));
    memset(mbar, 0, ROWS);
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

/* The map's glyphs that it shows in colour: borders, icons, dots, the fire */
static const uint8_t map_used[][2] = {
    { 0x11, 0x16 }, { 0x17, 0x1C }, { 0x1D, 0x22 }, { 0x23, 0x2C }, { 0x37, 0x3E },
};

static void map_glyphs(void)
{
    uint8_t i, g, n = 1;                /* (code 0 is the blank) */
    memset((void *)MTB, 0, 0x800);
    memset(mapidx, 0xFF, 64);
    bank_art_on();
    for (i = 0; i < sizeof(map_used) / 2; ++i)
        for (g = map_used[i][0]; g <= map_used[i][1]; ++g) {
            mapidx[g] = n;
            t_code = n++;
            mtb_glyph(mapchars + g * 8);
        }
    bank_art_off();
    mtb_kind = MTB_MAP;
}

/* The river: dotted, a piece of four kinds a row, wandering a little from
 * row to row (the Atari's player 3); blue, colour 1 of P6. */
static const uint8_t river_dots[8] = { 0x12, 0x14, 0x28, 0x2A, 0x48, 0x4A, 0x52, 0x54 };

static void map_river(void)
{
    uint8_t k, l, r, x = PLOT_X(4) + 4;
    for (k = 0; k < 4; ++k)
        for (l = 0; l < 8; ++l)
            expand2(LINE(TXTB, l) + COL_RIV + 2 * k, (l & 1) ? river_dots[RANDOM & 7] : 0, 1);
    for (r = MAP_ROW0; r < MAP_ROW0 + 20; ++r) {
        x += (RANDOM & 3) - 1;
        if (x < PLOT_X(4))
            x = PLOT_X(4);
        if (x > PLOT_X(4) + 8)
            x = PLOT_X(4) + 8;
        tr_lo[r * 4 + 1] = COL_RIV + 2 * (r & 3);
        tr_pw[r * 4 + 1] = (6 << 5) | ((-2) & 0x1F);
        tr_x[r * 4 + 1] = x;
    }
}

void gfx_map_frame(void)
{
    uint8_t r;
    screen_start(SK_MAP, 0x08, 0x08, 0x00);
    if (mtb_kind != MTB_MAP)
        map_glyphs();
    for (r = MAP_ROW0; r < MAP_ROW0 + 20; ++r)
        row_setmode(r, RM_MAP);
    map_reset();
    ter_pal[0] = 0x00;          /* the store: black */
    ter_pal[1] = 0x06;          /* the mountains: grey */
    ter_pal[2] = 0x96;          /* the river: blue */
    map_river();
    dl_build();
}

/* ---- the town ---------------------------------------------------------- */
#define TOWN_ROW0 2

void gfx_town(void)
{
    uint8_t r;
    const uint8_t *c;
    screen_start(SK_TOWN, 0x08, 0x08, 0x00);
    bank_art_on();
    if (mtb_kind != MTB_TOWN) {
        mtb_town(townchars);
        mtb_kind = MTB_TOWN;
    }
    memcpy(SCREEN + TOWN_ROW0 * 40, town_scr, 18 * 40);
    for (r = 0; r < 18; ++r) {
        row_setmode(TOWN_ROW0 + r, RM_TOWN);
        c = town_pal[r];
        row_pf[TOWN_ROW0 + r] = c[1];
        row_pf[ROWS + TOWN_ROW0 + r] = c[2];
        row_pf[2 * ROWS + TOWN_ROW0 + r] = c[3];
        row_pf[3 * ROWS + TOWN_ROW0 + r] = c[4];
    }
    bank_art_off();
    dl_build();
}

/* ---- start-up ------------------------------------------------------------ */
/* TXTB: the font as it is (320A: a line a byte), the mountains (grey, value
 * 1, on their row's line 6) and the store's four rows (double width). BIGB:
 * the font 8 colour clocks wide, in value 1 and 2. */
static void blocks(void)
{
    uint8_t c, k, s, b, *p;
    memset((void *)TXTB, 0, 0x800);
    bank_art_on();
    for (c = 0; c < 64; ++c)
        for (k = 0; k < 8; ++k) {
            b = font[c * 8 + k];
            LINE(TXTB, k)[c] = b;
            p = LINE(BIGB, k) + 2 * c;
            expand2(p, b, 1);
            expand2(p + 128, b, 2);
        }
    for (s = 0; s < 4; ++s) {
        for (k = 1; k < 7; ++k)
            expand2(LINE(TXTB, k) + COL_MTN + 2 * s, mtn_shape[s][k - 1], 1);
    }
    for (s = 0; s < 4; ++s)             /* the store: 4 lines a byte */
        for (k = 0; k < 8; ++k) {
            b = store_p3[2 * s + (k >> 2)];
            p = LINE(TXTB, k) + COL_STORE + 4 * s;
            p[0] = ((b & 0x80) ? 0x50 : 0) | ((b & 0x40) ? 0x05 : 0);
            p[1] = ((b & 0x20) ? 0x50 : 0) | ((b & 0x10) ? 0x05 : 0);
            p[2] = ((b & 0x08) ? 0x50 : 0) | ((b & 0x04) ? 0x05 : 0);
            p[3] = ((b & 0x02) ? 0x50 : 0) | ((b & 0x01) ? 0x05 : 0);
        }
    memcpy(pics_ram, pics, sizeof(pics_ram));
    bank_art_off();
}

/* The display: its blocks, a blank screen built, then the DLIs and DMA on
 * (the first frame's DLI shows it). */
void gfx_init(void)
{
    mt_init();
    blocks();
    mtb_kind = MTB_NONE;
    dsp_colbk = 0;
    memset(spr_pal, 0x0E, 4);
    memset(ter_pal, 0, 4);
    gfx_clear();
    dsp_ready = 0;              /* (no frame yet to have shown the last) */
    spr_flush();                /* the tables and every zone */
    mt_start();
    mt_on = 1;
}
