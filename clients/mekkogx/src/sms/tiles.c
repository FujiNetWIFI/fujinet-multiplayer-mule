/*
 * tiles.c -- the SMS's screen: the TMS9918A's cells (src/tms9918/gfx.c)
 * shown in Mode 4 (vram_sms.h is gfx.c's side of it).
 *
 * Graphics II gives every one of the 768 cells its own pattern; Mode 4 has
 * room for about 450 tiles of 4 bits a pixel, and the game's screens use a
 * few hundred different cells at most (text is the same glyph again and
 * again, the plains one cell over and over). So a cell's 8 pattern and 8
 * colour bytes become a tile (each row its two colours spread over the
 * four bit planes), and the tile is shared by every cell that looks the
 * same.
 *
 * The bookkeeping is in the cartridge's 32K of RAM (the Sega mapper's,
 * FFFCh bit 3 puts it in slot 2 in place of the ROM bank there), which
 * holds what VRAM can only give back slowly:
 *
 *   - T_SRC: each tile's 16 bytes as patterns and colours, in one form
 *     (tc_canon: each row's left pixel the background), so a cell is read
 *     back (tile_load) by copying them, and a cell is found among the tiles
 *     by comparing them, not by reading and decoding VRAM.
 *   - T_NAME: each cell's tile (the name table's, which is written too).
 *   - T_HASH, B_HEAD, T_NEXT: the tiles by a hash of their 16 bytes.
 *   - T_INRM, T_INPM: the row memo's and the plot memo's entries naming
 *     each tile (a tile made again is taken out of them).
 *   - T_REF: the cells showing each tile. A tile no cell shows any more
 *     keeps its pixels and its place among the hashes, to be shown again
 *     for nothing, until it is wanted for another. 255 sticks (the cleared
 *     cell's tile, and any tile on that many cells), until the next clear.
 *
 * A clear keeps the tiles (their pixels, bytes and hashes; only who shows
 * them is forgotten), so a screen drawn again finds its tiles made, and the
 * plot memo (gfx.c's PLOT_MEMO: a plot's look -> its 12 tiles) draws a map
 * seen before with names alone. A tile wanted for new pixels is one no
 * memo names, if there is one.
 *
 * m_pc, m_t: the cell stored last and its tile, for the next that is the
 * same (a fill, the plains, blanks): nothing to work out. If the 255 tiles
 * run out, a cell keeps what it had and tile_full counts it (the harnesses
 * look).
 *
 * The cartridge's RAM is in slot 2 only inside the functions here (they and
 * the interrupt are outside the window), and none of the bytes passed in
 * are in the window. Nothing here stores to 8000h or A000h by address: the
 * cartridge's mapper heuristic would count it (tools/sms/mkrom.py).
 *
 * VRAM: tiles 0..254 at 0000h (the cells'), sprite tiles from 2000h
 * (256 + 4 a VDP sprite pair: vram_sms.h), the name table at 3800h, the
 * sprite table at 3F00h. Palette: the TMS9918A's colours as near as the
 * SMS has them (sprites the same, 0 transparent).
 */
#include <string.h>
#include "mule.h"
#include "vdp.h"
#include "tiles.h"

#define NONE 255
#define NTILES 255              /* 0..254; 255 is NONE */

#define CART_RAM (*(volatile uint8_t *)0xFFFC)
#define CR_ON()  (CART_RAM = 0x08)
#define CR_OFF() (CART_RAM = 0)

#define T_SRC  ((uint8_t *)0x8000)      /* 16 a tile (tileconv.asm) */
#define T_NAME ((uint8_t *)0x9000)      /* 768 */
#define T_REF  ((uint8_t *)0x9300)
#define T_HASH ((uint8_t *)0x9400)
#define T_NEXT ((uint8_t *)0x9500)      /* t itself: in no chain */
#define B_HEAD ((uint8_t *)0x9600)
#define T_INRM ((uint8_t *)0x9700)      /* row memo entries naming it */
#define S_TAG  ((uint8_t *)0x9800)      /* sprite patterns: 128 x (shape, */
#define S_PAT  ((uint8_t *)0x9A00)      /* frame, part), 128 x 32 bytes */
#define P_MEMO ((uint8_t *)0xAA00)      /* plots: PM x PM_SIZE */
#define T_INPM ((uint8_t *)0xAC00)      /* plot memo entries naming it */
#define T_TEST ((volatile uint8_t *)0xAF00)

static uint8_t t_scan;
uint8_t tw[32];                 /* the tile being made (tileconv.asm) */
uint8_t ns[16];                 /* the cell being stored, in a tile's form */
uint8_t m_pc[16];               /* the last cell stored (tileconv.asm), */
static uint8_t m_t;             /* its tile */
uint8_t made;                   /* the tile tile_make made last (tileline.asm) */

/* A line of a cell (tileline.asm, gfx.c's op_line: the auction's dotted
 * lines): a tile with row r made (p, c) is always the same tile, so it is
 * kept, by a hash of the four, in a few of these; a tile made again is
 * taken out (its old pixels are no longer what either side says). */
#define RM 64                   /* (tileline.asm too) */
uint8_t rm_old[RM], rm_r[RM], rm_p[RM], rm_c[RM], rm_new[RM];

/* tile_hline's line: hl_n cells from hl_cell, row hl_r, the pixels from
 * hl_pic (the first cell's masked by hl_lm, the last's by hl_rm), colour
 * hl_c; for tile_row_miss, the cell's row painted (hl_p, hl_cc) and the
 * tile it had (hl_old). */
uint16_t hl_cell;
const uint8_t *hl_pic;
uint8_t hl_n, hl_r, hl_lm, hl_rm, hl_c, hl_p, hl_cc, hl_old;
uint8_t tile_full;              /* cells left as they were: no tile free */
uint8_t tile_live, tile_most;   /* tiles on screen, and the most yet */
uint8_t tiles_ok;               /* the cartridge's RAM answered */
uint16_t so_addr;               /* spr_out's (sprout.asm) */
const uint8_t *so_pat;
uint8_t so_col;

/* The TMS9918A's colours: --BBGGRR. */
static const uint8_t pal[16] = {
    0x00, 0x00, 0x08, 0x1D, 0x35, 0x3A, 0x06, 0x3D,
    0x17, 0x1B, 0x1A, 0x1F, 0x04, 0x26, 0x2A, 0x3F,
};

/* ---- VRAM --------------------------------------------------------------- */
static void vset_w(uint16_t a)
{
    VDP_CTRL = (uint8_t)a;
    VDP_CTRL = (uint8_t)(a >> 8) | 0x40;
}

void vram_out(uint16_t a, const uint8_t *s, uint8_t n)
{
    VDP_LOCK();
    vset_w(a);
    do {
        VDP_DATA = *s++;
    } while (--n);
    VDP_UNLOCK();
}

static void vram_fill(uint16_t a, uint8_t v, uint16_t n)
{
    uint8_t k;
    while (n) {                 /* a page at a time: the interrupt gets in */
        k = n > 255 ? 255 : (uint8_t)n;
        n -= k;
        VDP_LOCK();
        vset_w(a);
        do {
            VDP_DATA = v;
        } while (--k);
        VDP_UNLOCK();
        a += 255;
    }
}

/* Every name tile t (its attributes 0). */
static void names_fill(uint8_t t)
{
    uint8_t r, k;
    for (r = 0; r < 24; ++r) {  /* a row at a time: the interrupt gets in */
        VDP_LOCK();
        vset_w(SV_NAME + ((uint16_t)r << 6));
        k = 32;
        do {
            VDP_DATA = t;
            VDP_DATA = 0;
        } while (--k);
        VDP_UNLOCK();
    }
}

static void name_set(uint16_t cell, uint8_t t)
{
    VDP_LOCK();
    vset_w(SV_NAME + (cell << 1));
    VDP_DATA = t;
    VDP_UNLOCK();
}

uint8_t tc_canon(const uint8_t *pat, const uint8_t *col);  /* -> ns, its hash */
uint8_t tc_same(uint8_t t) __z88dk_fastcall;   /* tile t holds ns? */
uint8_t tc_memo(const uint8_t *pat, const uint8_t *col);    /* = m_pc? */
void tile_conv(const uint8_t *pat, const uint8_t *col);     /* -> tw */

/* ---- the hashes --------------------------------------------------------- */
static void unlink_tile(uint8_t t)
{
    uint8_t *p;
    if (T_NEXT[t] == t)
        return;                         /* in none */
    p = &B_HEAD[T_HASH[t]];
    while (*p != NONE) {
        if (*p == t) {
            *p = T_NEXT[t];
            break;
        }
        p = &T_NEXT[*p];
    }
    T_NEXT[t] = t;
}

static void link_tile(uint8_t t, uint8_t h)
{
    T_HASH[t] = h;
    T_NEXT[t] = B_HEAD[h];
    B_HEAD[h] = t;
}

/* A tile no cell shows, or NONE: one no memo names, if there is one. */
static uint8_t tile_free(void)
{
    uint8_t n = NTILES, t = t_scan, any = NONE;
    if (tile_live >= NTILES - 1)
        return NONE;                    /* (the cleared cell's isn't counted) */
    do {
        if (++t >= NTILES)
            t = 0;
        if (!T_REF[t]) {
            if (!T_INPM[t] && !T_INRM[t]) {
                t_scan = t;
                return t;
            }
            if (any == NONE)
                any = t;
        }
    } while (--n);
    return any;
}

/* ---- a cell's patterns and colours <-> a tile --------------------------- */
static void ref_up(uint8_t t)
{
    uint8_t *r = &T_REF[t];
    if (*r == 255)
        return;
    if (!(*r)++ && ++tile_live > tile_most)
        tile_most = tile_live;
}

static void ref_down(uint8_t t)
{
    uint8_t *r = &T_REF[t];
    if (*r == 255)
        return;
    if (!--(*r))
        --tile_live;
}

/* Tile t (as ns) is the one for these patterns and colours. */
static void remember(uint8_t t, const uint8_t *pat, const uint8_t *col)
{
    memcpy(m_pc, pat, 8);
    memcpy(m_pc + 8, col, 8);
    m_t = t;
}

/* Row memo entry k forgotten. */
static void rm_drop(uint8_t k)
{
    --T_INRM[rm_old[k]];
    --T_INRM[rm_new[k]];
    rm_new[k] = NONE;
}

/* ---- the plot memo -------------------------------------------------- */
#define PM 16
#define PM_SIZE 24              /* look (7), bgc, 12 tiles, in use, spare */
#define PM_IN 20
static uint8_t pm_next, pm_full;

static void pm_drop(uint8_t *e)
{
    uint8_t i;
    for (i = 8; i < 20; ++i)
        --T_INPM[e[i]];
    e[PM_IN] = 0;
}

/* Tile t is being made again: the plots that have it are forgotten. */
static void pm_forget(uint8_t t)
{
    uint8_t k, i;
    uint8_t *e = P_MEMO;
    for (k = 0; k < PM; ++k, e += PM_SIZE) {
        if (!e[PM_IN])
            continue;
        for (i = 8; i < 20; ++i)
            if (e[i] == t) {
                pm_drop(e);
                break;
            }
    }
}

/* Plot p's top left cell. */
static uint16_t pm_cell(uint8_t p)
{
    uint8_t cy = 0;
    while (p >= 9) {
        p -= 9;
        ++cy;
    }
    return ((uint16_t)(2 + 4 * cy) << 5) + 2 + 3 * p;
}

/* Tile t on cell (it is the right one: t is in a memo). */
static void show(uint16_t cell, uint8_t t)
{
    uint8_t old = T_NAME[cell];
    if (t == old)
        return;
    ref_up(t);
    T_NAME[cell] = t;
    name_set(cell, t);
    ref_down(old);
}

/* gfx.c's gfx_plot: a plot that looks like one drawn before (look, the
 * background bgc) is drawn by its tiles' names. 1: done. */
uint8_t plot_memo_show(uint8_t p, const uint8_t *look, uint8_t bgc)
{
    uint8_t k, i, cx;
    uint8_t *e;
    uint16_t cell;
    if (!tiles_ok)
        return 0;
    CR_ON();
    e = P_MEMO;
    for (k = 0; k < PM; ++k, e += PM_SIZE)
        if (e[PM_IN] && e[7] == bgc && !memcmp(e, look, 7))
            break;
    if (k == PM) {
        CR_OFF();
        pm_full = tile_full;
        return 0;
    }
    cell = pm_cell(p);
    for (i = 8; i < 20; cell += 32 - 3)
        for (cx = 0; cx < 3; ++cx, ++i, ++cell)
            show(cell, e[i]);
    CR_OFF();
    return 1;
}

/* ... and one drawn the long way is remembered (unless a cell of it found
 * no tile). */
void plot_memo_keep(uint8_t p, const uint8_t *look, uint8_t bgc)
{
    uint8_t i, cx, t;
    uint8_t *e;
    uint16_t cell;
    if (!tiles_ok || tile_full != pm_full)
        return;
    CR_ON();
    e = P_MEMO + pm_next * PM_SIZE;
    if (++pm_next >= PM)
        pm_next = 0;
    if (e[PM_IN])
        pm_drop(e);
    memcpy(e, look, 7);
    e[7] = bgc;
    cell = pm_cell(p);
    for (i = 8; i < 20; cell += 32 - 3)
        for (cx = 0; cx < 3; ++cx, ++i, ++cell) {
            t = T_NAME[cell];
            e[i] = t;
            ++T_INPM[t];
        }
    e[PM_IN] = 1;
    CR_OFF();
}

/* Tile t becomes ns, hash h: its pixels sent, its bytes kept. */
static void tile_make(uint8_t t, uint8_t h)
{
    uint8_t k = 0;
    if (T_INRM[t]) {
        do {
            if (rm_new[k] != NONE && (rm_old[k] == t || rm_new[k] == t))
                rm_drop(k);
        } while (++k < RM);
    }
    if (T_INPM[t])
        pm_forget(t);
    made = t;
    unlink_tile(t);
    link_tile(t, h);
    memcpy(T_SRC + ((uint16_t)t << 4), ns, 16);
    tile_conv(ns, ns + 8);
    vram_out((uint16_t)t << 5, tw, 32);
}

void tile_store(uint16_t cell, const uint8_t *pat, const uint8_t *col)
{
    uint8_t h, old, t;

    if (!tiles_ok) {
        /* no cartridge RAM: a tile of its own, enough for plat_init's
         * line of text on a cleared screen */
        t = (uint8_t)cell | 1;
        tile_conv(pat, col);
        vram_out((uint16_t)t << 5, tw, 32);
        name_set(cell, t);
        return;
    }
    CR_ON();
    old = T_NAME[cell];
    if (m_t != NONE && tc_memo(pat, col)) {
        t = m_t;                        /* as the last one */
        if (t == old)
            goto out;
        goto found;
    }
    h = tc_canon(pat, col);
    if (tc_same(old)) {
        remember(old, pat, col);
        goto out;                       /* as it was */
    }
    for (t = B_HEAD[h]; t != NONE; t = T_NEXT[t])
        if (t != old && tc_same(t))
            goto known;
    if (T_REF[old] == 1) {              /* this cell's own: made over */
        tile_make(old, h);
        remember(old, pat, col);
        goto out;
    }
    t = tile_free();
    if (t == NONE) {
        ++tile_full;
        goto out;
    }
    tile_make(t, h);
known:
    remember(t, pat, col);
found:
    ref_up(t);
    T_NAME[cell] = t;
    name_set(cell, t);
    ref_down(old);
out:
    CR_OFF();
}

void tile_load(uint16_t cell, uint8_t *pat, uint8_t *col)
{
    const uint8_t *s;
    if (!tiles_ok) {
        memset(pat, 0, 8);
        memset(col, 0x11, 8);
        return;
    }
    CR_ON();
    s = T_SRC + ((uint16_t)T_NAME[cell] << 4);
    memcpy(pat, s, 8);
    memcpy(col, s + 8, 8);
    CR_OFF();
}

/* Cells cell .. cell + k - 1 all the same (a row of blanks): the first is
 * stored, the rest take its tile by name. */
void tile_run(uint16_t cell, uint8_t k, const uint8_t *pat, const uint8_t *col)
{
    uint8_t t, full = tile_full;
    tile_store(cell, pat, col);
    if (!--k)
        return;
    if (!tiles_ok || tile_full != full) {
        do {
            tile_store(++cell, pat, col);       /* (the long way) */
        } while (--k);
        return;
    }
    CR_ON();
    t = T_NAME[cell];
    do {
        show(++cell, t);
    } while (--k);
    CR_OFF();
}

/* tile_hline's cell that the row memo hasn't seen: tile hl_old with row
 * hl_r made (hl_p, hl_cc), stored the long way, and the answer noted. */
void tile_row_miss(void)
{
    static uint8_t pc[16];
    uint8_t r = hl_r, old = hl_old, k, t, full;
    CR_ON();
    memcpy(pc, T_SRC + ((uint16_t)old << 4), 16);
    CR_OFF();
    pc[r] = hl_p;
    pc[8 + r] = hl_cc;
    full = tile_full;
    made = NONE;
    tile_store(hl_cell, pc, pc + 8);
    if (tile_full != full || made == old)
        return;                         /* no tile, or the old one made over */
    k = (uint8_t)(old + (r << 3) + hl_p + (hl_cc << 1) + (hl_cc >> 3)) & (RM - 1);
    CR_ON();
    if (rm_new[k] != NONE)
        rm_drop(k);
    t = T_NAME[hl_cell];
    rm_old[k] = old;
    rm_r[k] = r;
    rm_p[k] = hl_p;
    rm_c[k] = hl_cc;
    rm_new[k] = t;
    ++T_INRM[old];
    ++T_INRM[t];
    CR_OFF();
}

/* Every cell the cleared one, in colour bg: a tile of its own, which no
 * cell's drawing takes away (255 shows). The tiles keep their pixels. */
void tiles_clear(uint8_t bg)
{
    uint8_t pc[16], h, t;
    memset(pc, 0, 8);
    memset(pc + 8, (uint8_t)(bg << 4) | bg, 8);
    if (!tiles_ok) {
        vram_fill(SV_NAME, 0, 24 * 64);
        tile_conv(pc, pc + 8);
        vram_out(0, tw, 32);
        return;
    }
    memset(rm_new, NONE, RM);
    CR_ON();
    memset(T_INRM, 0, 256);
    memset(T_REF, 0, NTILES);
    tile_live = 0;
    h = tc_canon(pc, pc + 8);
    for (t = B_HEAD[h]; t != NONE; t = T_NEXT[t])
        if (tc_same(t))
            break;
    if (t == NONE) {
        t = tile_free();
        tile_make(t, h);
    }
    T_REF[t] = 255;
    memset(T_NAME, t, 768);
    names_fill(t);
    remember(t, pc, pc + 8);
    CR_OFF();
}

/* No tiles made yet, no memos. */
static void tiles_reset(void)
{
    uint8_t t = 0;
    CR_ON();
    memset(B_HEAD, NONE, 256);
    do {
        T_NEXT[t] = t;
    } while (++t);
    memset(T_INPM, 0, 256);
    memset(P_MEMO, 0, PM * PM_SIZE);
    memset(S_TAG, 0xFF, 128 * 3);       /* no sprite patterns kept */
    memset(T_REF, 0, NTILES);
    CR_OFF();
    t_scan = NTILES - 1;                /* (tile 0 first) */
    pm_next = 0;
}

/* ---- sprites' patterns ------------------------------------------------- */
/* gfx.c's send() works out a frame's 32 bytes from the shape a line at a
 * time; walking, a figure changes frame every step: they are kept, by a
 * hash of shape, frame and part, in the cartridge's RAM (a shape's frames
 * never change). */
static uint8_t *s_tag(uint8_t shape, uint8_t f, uint8_t part, uint8_t *k)
{
    *k = (uint8_t)((f << 2) + part + shape * 29) & 127;
    return S_TAG + *k * 3;
}

uint8_t spr_cache_get(uint8_t shape, uint8_t f, uint8_t part, uint8_t *pat)
{
    uint8_t k, ok = 0;
    uint8_t *t;
    if (!tiles_ok)
        return 0;
    CR_ON();
    t = s_tag(shape, f, part, &k);
    if (t[0] == shape && t[1] == f && t[2] == part) {
        memcpy(pat, S_PAT + ((uint16_t)k << 5), 32);
        ok = 1;
    }
    CR_OFF();
    return ok;
}

void spr_cache_put(uint8_t shape, uint8_t f, uint8_t part, const uint8_t *pat)
{
    uint8_t k;
    uint8_t *t;
    if (!tiles_ok)
        return;
    CR_ON();
    t = s_tag(shape, f, part, &k);
    t[0] = shape;
    t[1] = f;
    t[2] = part;
    memcpy(S_PAT + ((uint16_t)k << 5), pat, 32);
    CR_OFF();
}

/* The cartridge's RAM is there: a byte written reads back, and it isn't
 * the ROM's (which is back in slot 2 once the RAM is put away). */
static uint8_t cart_ram_test(void)
{
    uint8_t ok, rom;
    CR_ON();
    *T_TEST = 0x5A;
    CR_OFF();
    rom = *T_TEST;
    CR_ON();
    ok = *T_TEST == 0x5A;
    *T_TEST = 0xA5;
    ok = ok && *T_TEST == 0xA5 && rom != 0x5A;
    CR_OFF();
    return ok;
}

void tiles_init(void)
{
    uint8_t i;
    tiles_ok = cart_ram_test();
    if (tiles_ok)
        tiles_reset();
    VDP_LOCK();
    VDP_REG(1, 0xA2);           /* blank, the frame interrupt, 8 x 16 sprites */
    VDP_REG(0, 0x06);           /* Mode 4 */
    VDP_REG(2, 0xFF);           /* names at 3800h */
    VDP_REG(3, 0xFF);
    VDP_REG(4, 0xFF);
    VDP_REG(5, 0xFF);           /* sprite table at 3F00h */
    VDP_REG(6, 0xFF);           /* sprite tiles from 2000h */
    VDP_REG(7, 0x00);           /* the border: sprite colour 0, black */
    VDP_REG(8, 0x00);
    VDP_REG(9, 0x00);
    VDP_REG(10, 0xFF);          /* no line interrupts */
    VDP_CTRL = 0x00;            /* the palette: */
    VDP_CTRL = 0xC0;
    for (i = 0; i < 32; ++i)    /* the cells', then the sprites' */
        VDP_DATA = pal[i & 15];
    VDP_UNLOCK();
    vram_fill(SV_SAT, 0xD0, 1); /* no sprites */
    tiles_clear(1);
    VDP_LOCK();
    VDP_REG(1, 0xE2);           /* on */
    VDP_UNLOCK();
}
