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
 * same:
 *
 *   - t_hash, b_head, t_next: the tiles by a hash of their 32 bytes; a
 *     candidate is compared with what VRAM holds (tile_same).
 *   - m_pc, m_t: the cell stored last and its tile, for the next that is
 *     the same (a fill, the plains, blanks): nothing to work out.
 *   - t_ref: the cells showing each tile. A tile no cell shows any more
 *     keeps its pixels and its place among the hashes, to be shown again
 *     for nothing, until it is wanted for another. 255 sticks (the cleared
 *     cell, tile 0, and any tile on that many cells), until the next clear.
 *   - the name table holds each cell's tile: what a cell had is read back
 *     from there.
 *
 * Reading a cell back (tile_load) turns its tile into patterns and colours
 * again: each row's left pixel's colour is the background, the rest the
 * foreground. If the 255 tiles run out, a cell keeps what it had and
 * tile_full counts it (the harnesses look).
 *
 * The tile is made, hashed and compared in tileconv.asm (in C that was most of
 * a screen's drawing).
 *
 * VRAM: tiles 0..255 at 0000h (the cells'), sprite tiles from 2000h
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

static uint8_t t_ref[NTILES], t_hash[NTILES], t_next[NTILES];
static uint8_t b_head[256];
static uint8_t t_scan;
uint8_t tw[32];                 /* the tile being stored (tileconv.asm) */
static uint8_t tr[32];          /* one read back */
static uint8_t m_pc[16], m_t;   /* the last cell stored, its tile */
uint8_t tile_full;              /* cells left as they were: no tile free */
uint8_t tile_live, tile_most;   /* tiles on screen, and the most yet */

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

static void vset_r(uint16_t a)
{
    VDP_CTRL = (uint8_t)a;
    VDP_CTRL = (uint8_t)(a >> 8) & 0x3F;
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

static uint8_t name_of(uint16_t cell)
{
    uint8_t t;
    VDP_LOCK();
    vset_r(SV_NAME + (cell << 1));
    t = VDP_DATA;
    VDP_UNLOCK();
    return t;
}

static void name_set(uint16_t cell, uint8_t t)
{
    VDP_LOCK();
    vset_w(SV_NAME + (cell << 1));
    VDP_DATA = t;
    VDP_UNLOCK();
}

uint8_t tile_same(uint8_t t) __z88dk_fastcall;  /* tileconv.asm: tile t holds tw? */
uint8_t tile_conv(const uint8_t *pat, const uint8_t *col);   /* -> tw, its hash */

/* ---- the hashes --------------------------------------------------------- */
static void unlink_tile(uint8_t t)
{
    uint8_t *p = &b_head[t_hash[t]];
    while (*p != NONE) {
        if (*p == t) {
            *p = t_next[t];
            return;
        }
        p = &t_next[*p];
    }
}

static void link_tile(uint8_t t, uint8_t h)
{
    t_hash[t] = h;
    t_next[t] = b_head[h];
    b_head[h] = t;
}

/* A tile no cell shows (a fresh one if there is one), or NONE. */
static uint8_t tile_free(void)
{
    uint8_t n = NTILES, t = t_scan;
    do {
        if (++t >= NTILES)
            t = 1;
        if (!t_ref[t]) {
            t_scan = t;
            return t;
        }
    } while (--n);
    return NONE;
}

/* ---- a cell's patterns and colours <-> a tile --------------------------- */
static void ref_up(uint8_t t)
{
    if (t_ref[t] == 255)
        return;
    if (!t_ref[t]++ && ++tile_live > tile_most)
        tile_most = tile_live;
}

static void ref_down(uint8_t t)
{
    if (t_ref[t] == 255)
        return;
    if (!--t_ref[t])
        --tile_live;
}

/* Tile t (in tw) is the one for these patterns and colours. */
static void remember(uint8_t t, const uint8_t *pat, const uint8_t *col)
{
    memcpy(m_pc, pat, 8);
    memcpy(m_pc + 8, col, 8);
    m_t = t;
}

void tile_store(uint16_t cell, const uint8_t *pat, const uint8_t *col)
{
    uint8_t h, old, t;

    old = name_of(cell);
    if (m_t != NONE && !memcmp(m_pc, pat, 8) && !memcmp(m_pc + 8, col, 8)) {
        t = m_t;                        /* as the last one */
        if (t == old)
            return;
        goto found;
    }
    h = tile_conv(pat, col);
    if (t_hash[old] == h && tile_same(old)) {
        remember(old, pat, col);
        return;                         /* as it was */
    }
    for (t = b_head[h]; t != NONE; t = t_next[t])
        if (t != old && tile_same(t))
            goto known;
    if (t_ref[old] == 1) {              /* this cell's own: made over */
        unlink_tile(old);
        link_tile(old, h);
        vram_out((uint16_t)old << 5, tw, 32);
        remember(old, pat, col);
        return;
    }
    t = tile_free();
    if (t == NONE) {
        ++tile_full;
        return;
    }
    unlink_tile(t);
    link_tile(t, h);
    vram_out((uint16_t)t << 5, tw, 32);
known:
    remember(t, pat, col);
found:
    ref_up(t);
    name_set(cell, t);
    ref_down(old);
}

void tile_load(uint16_t cell, uint8_t *pat, uint8_t *col)
{
    uint8_t r, i, b, m0, m1, m2, m3, d, x, f;
    const uint8_t *s = tr;
    uint16_t a = (uint16_t)name_of(cell) << 5;
    VDP_LOCK();
    vset_r(a);
    for (i = 0; i < 32; ++i)
        tr[i] = VDP_DATA;
    VDP_UNLOCK();
    for (r = 0; r < 8; ++r, s += 4) {
        /* the left pixel's colour is the background */
        b = (s[0] >> 7) | ((s[1] >> 6) & 2) | ((s[2] >> 5) & 4) | ((s[3] >> 4) & 8);
        m0 = b & 1 ? 0xFF : 0;
        m1 = b & 2 ? 0xFF : 0;
        m2 = b & 4 ? 0xFF : 0;
        m3 = b & 8 ? 0xFF : 0;
        d = (s[0] ^ m0) | (s[1] ^ m1) | (s[2] ^ m2) | (s[3] ^ m3);
        f = b;
        if (d) {
            for (x = 0x80; !(d & x); x >>= 1)
                ;
            f = (s[0] & x ? 1 : 0) | (s[1] & x ? 2 : 0) | (s[2] & x ? 4 : 0) | (s[3] & x ? 8 : 0);
        }
        pat[r] = d;
        col[r] = (uint8_t)(f << 4) | b;
    }
}

/* Every cell the cleared one, in colour bg. */
void tiles_clear(uint8_t bg)
{
    uint8_t pc[16], h;
    memset(t_ref, 0, sizeof(t_ref));
    memset(t_hash, 0, sizeof(t_hash));
    memset(b_head, NONE, sizeof(b_head));
    tile_live = 0;
    t_scan = 0;
    vram_fill(SV_NAME, 0, 24 * 64);
    memset(pc, 0, 8);
    memset(pc + 8, (uint8_t)(bg << 4) | bg, 8);
    h = tile_conv(pc, pc + 8);
    vram_out(0, tw, 32);
    link_tile(0, h);
    t_ref[0] = 255;
    remember(0, pc, pc + 8);
}

void tiles_init(void)
{
    uint8_t i;
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
