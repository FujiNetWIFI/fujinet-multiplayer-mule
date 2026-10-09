/*
 * gfx.c -- gfx.h on the NES: the TMS9918A clients' screen (src/tms9918/
 * gfx.c), 256 x 192, x scaled 3/2 from the Atari's colour clocks, text in
 * whole 8 x 8 cells, 32 across.
 *
 * The background is a bitmap: each of the 768 cells is its own tile, a
 * third of the screen to each of three pattern tables that the scanline
 * interrupt switches between (crt0.s). A pixel is one of four colours: 0
 * the background, 1-3 from the palette of its 16 x 16 block (the
 * attribute). Each screen has a set of four palettes (pset): white is 1 in
 * all of them, so white text goes anywhere; a colour lives in one palette,
 * and drawing in it gives the block that palette (the last colour drawn in
 * a block wins).
 *
 * The PPU can only be written while it shows nothing, so drawing goes into
 * a queue the interrupts drain in the blank (neshw.h). gfx_clear and
 * gfx_map_frame turn the display off ("direct": the NMI leaves the PPU
 * alone and the queue is drained as it fills) until the next frame_wait,
 * so a screen is drawn whole before it shows.
 *
 * The sprites are the NES's (8 x 16, one colour each from four palettes):
 * a slot is up to six of them, with its own patterns, sent when its frame or
 * colour changes. The river on the map is sprites too, behind the
 * background (the plots' palettes have no room for its blue): it shows
 * where the plains do.
 */
#include <string.h>
#include "mule.h"
#include "neshw.h"
#include "gfxdata.h"

#define PX(x) ((uint16_t)(x) + ((uint8_t)(x) >> 1) + 4)

/* ---- palettes ------------------------------------------------------------ */
#define PS_NORMAL 0
#define PS_MAP    1
#define PS_TOWN   2

/* Each set: the background, then four palettes' colours 1-3. */
static const uint8_t pal_sets[3][13] = {
    /* black screens: white with blue/purple, green/orange, red/yellow,
     * cyan/grey */
    { 0x0F, 0x30, 0x21, 0x24, 0x30, 0x2A, 0x27, 0x30, 0x16, 0x28, 0x30, 0x2C, 0x10 },
    /* the map: the plains, white, black, and a player's colour */
    { 0x10, 0x30, 0x0F, 0x21, 0x30, 0x0F, 0x24, 0x30, 0x0F, 0x2A, 0x30, 0x0F, 0x27 },
    /* the town: as black screens for the players' colours, then the
     * buildings' (grey with green and light green, tan and red) */
    { 0x0F, 0x30, 0x21, 0x24, 0x30, 0x2A, 0x27, 0x10, 0x2A, 0x2B, 0x10, 0x38, 0x26 },
};
/* The sprites' palettes: one colour each, so twelve. */
static const uint8_t spr_pal[16] = {
    0x0F, 0x21, 0x24, 0x30,     /* blue, purple, white */
    0x0F, 0x2A, 0x27, 0x12,     /* green, orange, the river */
    0x0F, 0x16, 0x28, 0x2C,     /* red, yellow, cyan */
    0x0F, 0x0F, 0x10, 0x38,     /* black, grey, tan */
};

/* A logical colour in a set: palette << 2 | colour, or ANY | colour (in
 * every palette: 0 the background, 1 white). */
#define ANY 0x10
#define PC(p, i) (((p) << 2) | (i))
static const uint8_t col_tab[3][16] = {
    {   /* black screens */
        ANY | 0, ANY | 1, PC(0, 2), PC(0, 3), PC(1, 2), PC(1, 3), PC(2, 2), PC(2, 3),
        PC(3, 3), PC(3, 2), PC(2, 2), PC(2, 3), PC(0, 2), PC(1, 2), PC(0, 3), PC(1, 3),
    },
    {   /* the map: the players' colours; others white */
        ANY | 0, ANY | 1, PC(0, 3), PC(1, 3), PC(2, 3), PC(3, 3), PC(3, 3), ANY | 1,
        ANY | 0, PC(0, 3), PC(3, 3), ANY | 1, PC(0, 3), PC(2, 3), PC(1, 3), PC(3, 3),
    },
    {   /* the town */
        ANY | 0, ANY | 1, PC(0, 2), PC(0, 3), PC(1, 2), PC(1, 3), PC(3, 3), PC(3, 2),
        PC(2, 1), PC(0, 2), PC(3, 3), PC(3, 2), PC(0, 2), PC(2, 2), PC(0, 3), PC(1, 3),
    },
};
#define MAP_BLACK 2             /* the map's true black (mountains) */

/* Logical colour -> a sprite's palette << 2 | colour. */
static const uint8_t spr_col[16] = {
    PC(3, 1), PC(0, 3), PC(0, 1), PC(0, 2), PC(1, 1), PC(1, 2), PC(2, 1), PC(2, 2),
    PC(3, 2), PC(2, 3), PC(2, 1), PC(3, 3), PC(0, 1), PC(1, 1), PC(0, 2), PC(1, 2),
};
#define RIVER_SPR PC(1, 3)

static uint8_t pset;

/* ---- the queue ------------------------------------------------------------ */
/* Room for n bytes (the interrupts make it, or, direct, the drain). */
static void q_room(uint8_t n)
{
    while ((uint8_t)(qtail - qhead - 1) < n)
        if (ppu_direct)
            ppu_drain(255);
}

static uint8_t qh;              /* where the command being made goes */

static void q_begin(uint8_t op, uint8_t third, uint16_t a, uint8_t n)
{
    q_room(n);
    qh = qhead;
    ppuq[qh++] = op;
    ppuq[qh++] = third;
    ppuq[qh++] = (uint8_t)(a >> 8);
    ppuq[qh++] = (uint8_t)a;
}

#define Q_PUT(v) (ppuq[qh++] = (v))

static void q_end(void)
{
    qhead = qh;
    if (ppu_direct)
        ppu_drain(255);
}

static void q_copy(uint8_t third, uint16_t a, const uint8_t *s, uint8_t n)
{
    q_begin(Q_COPY, third, a, n + 5);
    Q_PUT(n);
    do {
        Q_PUT(*s++);
    } while (--n);
    q_end();
}

/* (a fill costs the drain 9/16 of a unit a byte: 120 is within the NMI's
 * budget; with the display off, the drain takes all there is) */
static void q_fill(uint8_t third, uint16_t a, uint16_t n, uint8_t v)
{
    uint8_t k, most = ppu_direct ? 240 : 120;
    while (n) {
        k = n > most ? most : (uint8_t)n;
        q_begin(Q_FILL, third, a, 6);
        Q_PUT(k);
        Q_PUT(v);
        q_end();
        n -= k;
        a += k;
    }
}

/* The cell (cx, cy): its third and its address in that third's table. */
#define THIRD(cy) ((uint8_t)(cy) >> 3)
static uint16_t cell_addr(uint8_t cx, uint8_t cy)
{
    return ((uint16_t)(cy & 7) << 9) | ((uint16_t)cx << 4);
}

/* ---- attributes ------------------------------------------------------------ */
static uint8_t attr[64];
static uint8_t attr_dirty[8];   /* a bit an attribute byte */

/* Cell (cx, cy)'s block gets palette p. (The screen is on name table rows
 * 2..25: cell row cy is row cy + 2.) */
static void set_pal(uint8_t cx, uint8_t cy, uint8_t p)
{
    uint8_t by = (uint8_t)(cy + 2) >> 1;
    uint8_t i = ((by >> 1) << 3) | (cx >> 2);
    uint8_t sh = ((by & 1) << 2) | ((cx >> 1) & 1) << 1;
    uint8_t v = (attr[i] & ~(3 << sh)) | (p << sh);
    if (v != attr[i]) {
        attr[i] = v;
        attr_dirty[i >> 3] |= 1 << (i & 7);
    }
}

static void attr_flush(void)
{
    uint8_t r, b, i, n;
    for (r = 0; r < 8; ++r) {
        b = attr_dirty[r];
        if (!b)
            continue;
        attr_dirty[r] = 0;
        for (i = 0; i < 8; ) {
            if (!(b & (1 << i))) {
                ++i;
                continue;
            }
            for (n = 1; i + n < 8 && (b & (1 << (i + n))); ++n)
                ;
            q_copy(Q_NOBANK, 0x23C0 + (r << 3) + i, attr + (r << 3) + i, n);
            i += n;
        }
    }
}

/* The colour (0-3) that col is in cell (cx, cy): its block takes col's
 * palette if it must. */
static uint8_t cidx(uint8_t cx, uint8_t cy, uint8_t col)
{
    uint8_t e = col_tab[pset][col & 15];
    if (!(e & ANY))
        set_pal(cx, cy, e >> 2);
    return e & 3;
}

/* The masks a colour's pixels are in the two planes. */
static uint8_t m0, m1;
static void planes(uint8_t idx)
{
    m0 = (idx & 1) ? 0xFF : 0;
    m1 = (idx & 2) ? 0xFF : 0;
}

/* ---- the display ------------------------------------------------------------ */
static void palettes(void)
{
    static uint8_t p[32];
    const uint8_t *s = pal_sets[pset];
    uint8_t i;
    for (i = 0; i < 16; ++i)
        p[i] = (i & 3) ? s[1 + (i >> 2) * 3 + (i & 3) - 1] : s[0];
    memcpy(p + 16, spr_pal, 16);
    p[16] = s[0];
    q_copy(Q_NOBANK, 0x3F00, p, 32);
}

/* Off, until the next frame_wait: the NMI says when it has done it. */
static void direct(void)
{
    if (!ppu_direct) {
        ppu_off = 0;
        ppu_direct = 1;
        while (!ppu_off)
            ;
    }
    ppu_drain(255);
}

void gfx_direct_end(void)       /* plat.c frame_wait */
{
    if (ppu_direct) {
        ppu_drain(255);
        ppu_direct = 0;
    }
}

static void clear_all(void)
{
    uint8_t t;
    for (t = 0; t < 3; ++t)
        q_fill(t, 0, 0x1000, 0);
    memset(attr, 0, sizeof(attr));
    memset(attr_dirty, 0, sizeof(attr_dirty));
    q_fill(Q_NOBANK, 0x23C0, 64, 0);
    palettes();
}

static void river_init(void);
static void spr_init(void);
static uint8_t river_on[45];
static uint8_t river_n;

void gfx_init(void)
{
    static uint8_t row[32];
    uint8_t r, c;
    direct();
    q_fill(Q_NOBANK, 0x2000, 64, 0x80);         /* rows 0-1: R1's first tile
                                                 * (crt0.s: the blank one) */
    for (r = 0; r < 24; ++r) {
        for (c = 0; c < 32; ++c)
            row[c] = (uint8_t)(((r & 7) << 5) | c);
        q_copy(Q_NOBANK, 0x2040 + ((uint16_t)r << 5), row, 32);
    }
    q_fill(Q_NOBANK, 0x2340, 128, 0);           /* rows 26-29: R0's first */
    q_fill(Q_NOBANK, 0x1000, 0x1000, 0);        /* the sprites' patterns (and
                                                 * the margins' blank tile) */
    pset = PS_NORMAL;
    clear_all();
    river_init();
    spr_init();
}

static void clear_screen(uint8_t set)
{
    spr_hide_all();
    memset(river_on, 0, sizeof(river_on));
    river_n = 0;
    spr_flush();
    direct();
    pset = set;
    clear_all();
}

void gfx_clear(void)
{
    clear_screen(PS_NORMAL);
}

void gfx_map_frame(void)
{
    clear_screen(PS_MAP);
}

/* ---- text ---------------------------------------------------------------- */
static char tbuf[TCOLS + 1];
static uint8_t cbuf[64];

/* s, at most n characters, cut at the right edge from tx: into tbuf (it may
 * be in the overlay window, which ART_BEGIN changes). */
static uint8_t take(const char *s, uint8_t n, uint8_t tx)
{
    uint8_t k = 0;
    while (k < n && s[k] && tx < TCOLS) {
        tbuf[k] = s[k];
        ++k;
        ++tx;
    }
    return k;
}

static const uint8_t *glyph_of(char c)
{
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c < 32 || c > 95)
        c = '?';
    return nes_font + ((uint8_t)(c - 32) << 3);
}

/* k cells from (tx, ty): tbuf's characters in colour col. Four cells (64
 * bytes) a command. */
static void text_cells(uint8_t tx, uint8_t ty, uint8_t k, uint8_t col)
{
    const uint8_t *g;
    uint16_t a = cell_addr(tx, ty);
    uint8_t t = THIRD(ty), i, r, n = 0, v;
    ART_BEGIN();
    for (i = 0; i < k; ++i) {
        planes(cidx(tx + i, ty, col));
        g = glyph_of(tbuf[i]);
        for (r = 0; r < 8; ++r) {
            v = g[r];
            cbuf[n + r] = v & m0;
            cbuf[n + 8 + r] = v & m1;
        }
        n += 16;
        if (n == 64 || i + 1 == k) {
            q_copy(t, a, cbuf, n);
            a += n;
            n = 0;
        }
    }
    ART_END();
    attr_flush();
}

void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    uint8_t k;
    if (ty > 23 || tx >= TCOLS)
        return;
    k = take(s, n, tx);
    if (k)
        text_cells(tx, ty, k, col);
}

void gfx_text(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_textn(tx, ty, s, TCOLS, col);
}

void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col)
{
    tbuf[0] = c;
    if (ty > 23 || tx >= TCOLS)
        return;
    text_cells(tx, ty, 1, col);
}

void gfx_clear_text(uint8_t tx, uint8_t ty, uint8_t w)
{
    if (tx >= TCOLS || ty > 23)
        return;
    if (tx + w > TCOLS)
        w = TCOLS - tx;
    if (w)
        q_fill(THIRD(ty), cell_addr(tx, ty), (uint16_t)w << 4, 0);
}

void gfx_bign(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    gfx_textn(tx, ty, s, n, col);
}

void gfx_big(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_textn(tx, ty, s, TCOLS, col);
}

/* ---- painting rectangles ------------------------------------------------ */
/* A line picture for paint_rect: up to 16 lines of the screen's 256 pixels. */
static uint8_t tb[16][32];

/* Pixels px0 .. px1 - 1 of lines y .. y + h - 1 become colour col; with a
 * picture (pic: tb, whose line 0 is line y), only its pixels, the rest of
 * the rectangle the background. A whole cell is written; a part of one is
 * painted (the PPU reads it back). */
static void paint_rect(uint16_t px0, uint16_t px1, uint8_t y, uint8_t h, uint8_t col,
                       uint8_t pic)
{
    uint8_t cx, cx0, cx1, cy, cy1, r, r0, r1, m, lm, rm, ye, f, t;
    uint16_t a;
    if (px1 > 256)
        px1 = 256;
    if (y >= 192 || px0 >= px1 || !h)
        return;
    ye = (y + h > 192) ? 192 : (uint8_t)(y + h);
    cx0 = (uint8_t)(px0 >> 3);
    cx1 = (uint8_t)((px1 - 1) >> 3);
    lm = 0xFF >> (px0 & 7);
    rm = 0xFF << (7 - ((px1 - 1) & 7));
    cy1 = (uint8_t)((ye - 1) >> 3);
    for (cy = y >> 3; cy <= cy1; ++cy) {
        r0 = (cy << 3) < y ? (y & 7) : 0;
        r1 = ((cy << 3) + 8 > ye) ? (uint8_t)(ye - (cy << 3)) : 8;
        t = THIRD(cy);
        for (cx = cx0; cx <= cx1; ++cx) {
            m = 0xFF;
            if (cx == cx0)
                m &= lm;
            if (cx == cx1)
                m &= rm;
            planes(cidx(cx, cy, col));
            a = cell_addr(cx, cy);
            if (!pic && m == 0xFF && r0 == 0 && r1 == 8) {
                if (m0 == m1) {
                    q_fill(t, a, 16, m0);
                } else {
                    q_fill(t, a, 8, m0);
                    q_fill(t, a + 8, 8, m1);
                }
                continue;
            }
            q_begin(Q_PAINT, t, a, 28);
            for (r = 0; r < 8; ++r) {
                if (r < r0 || r >= r1) {
                    Q_PUT(0);
                    Q_PUT(0);
                    Q_PUT(0);
                    continue;
                }
                f = pic ? (m & tb[(cy << 3) + r - y][cx]) : m;
                Q_PUT(m);
                Q_PUT(f & m0);
                Q_PUT(f & m1);
            }
            q_end();
        }
    }
    attr_flush();
}

/* tb line r gets the pixels of v (bit 15 left) from pixel px. */
static void tb_put(uint8_t r, uint16_t px, uint16_t v)
{
    uint8_t *d, sh, i;
    if (px >= 256 || !v)
        return;
    i = (uint8_t)(px >> 3);
    d = &tb[r][i];
    sh = px & 7;
    d[0] |= (uint8_t)((v >> sh) >> 8);
    if (i < 31)
        d[1] |= (uint8_t)(v >> sh);
    if (sh && i < 30)
        d[2] |= (uint8_t)(v << (8 - sh));
}

void gfx_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
    paint_rect(PX(x), PX(x + w), y, h, col, 0);
}

/* (Only the auction draws them: in its overlay, out of the full fixed
 * banks.) */
#pragma code-name (push, "OVERLAY4")
/* A dotted line: one line of each cell, read and written back (Q_ROW).
 * The dots are made again only for another x or width (the auction's
 * lines move a step every few frames, always the same dots). */
void gfx_hdots(uint8_t x, uint8_t y, uint8_t w, uint8_t col)
{
    static uint8_t dots[32], dx = 255, dw;
    uint16_t px0, px1;
    uint8_t c, cx, cx0, cx1, n, m, t, cy;
    if (x != dx || w != dw) {
        dx = x;
        dw = w;
        memset(tb[0], 0, 32);
        for (c = 0; c < w; c += 2)
            tb_put(0, PX(x + c), PX(x + c + 1) - PX(x + c) > 1 ? 0xC000 : 0x8000);
        memcpy(dots, tb[0], 32);
    }
    px0 = PX(x);
    px1 = PX(x + w);
    if (px1 > 256)
        px1 = 256;
    if (y >= 192 || px0 >= px1)
        return;
    cx0 = (uint8_t)(px0 >> 3);
    cx1 = (uint8_t)((px1 - 1) >> 3);
    cy = y >> 3;
    t = THIRD(cy);
    while (cx0 <= cx1) {
        n = cx1 - cx0 + 1;
        if (n > 16)
            n = 16;
        q_begin(Q_ROW, t, cell_addr(cx0, cy) + (y & 7), 5 + 3 * n);
        Q_PUT(n);
        for (cx = cx0; cx < cx0 + n; ++cx) {
            m = dots[cx];
            if (cx == (uint8_t)(px0 >> 3))
                m &= 0xFF >> (px0 & 7);
            if (cx == (uint8_t)((px1 - 1) >> 3))
                m &= 0xFF << (7 - ((px1 - 1) & 7));
            planes(cidx(cx, cy, col));
            Q_PUT(m);
            Q_PUT(m & m0);
            Q_PUT(m & m1);
        }
        q_end();
        cx0 += n;
    }
    attr_flush();
}
#pragma code-name (pop)

void gfx_pic(uint8_t x, uint8_t y, uint8_t pic, uint8_t col)
{
    uint8_t r;
    uint16_t px0 = PX(x);
    memset(tb, 0, sizeof(tb));
    ART_BEGIN();
    for (r = 0; r < 16; ++r)
        tb_put(r, px0, wide12[nes_pics[pic][r >> 1]] << 4);
    ART_END();
    paint_rect(px0, px0 + 12, y, 16, col, 1);
}

/* ---- the map ------------------------------------------------------------ */
uint16_t gfx_nplot;             /* (the harnesses peek it) */

/* A plot: three cells by four, made whole. A pixel is the owner's (border,
 * icon, dots) in their colour, else white or black (the store, the
 * mountains), else the plains; the river is sprites under it. */
static uint8_t own[96], wh[96];

void gfx_plot(uint8_t p, const PlotLook *look)
{
    uint8_t i, k, t, cx, cy, cx0, cy0, r, o, w, oi, wi, o0, o1, w0, w1;
    uint8_t *d;

    memset(own, 0, 96);
    memset(wh, 0, 96);
    ART_BEGIN();
    if (look->store) {
        memcpy(wh, plot_parts[PP_STORE], 96);
    } else {
        if (look->owned) {
            memcpy(own, plot_parts[PP_BORDER], 96);
            if (look->icon)
                for (i = 0; i < 96; ++i)
                    own[i] |= plot_parts[PP_ICON + look->icon - 1][i];
            for (i = 0; i < look->dots && i < 8; ++i) {
                k = plot_dots[i][0] * 3 + (plot_dots[i][1] >> 3);
                t = 0xC0 >> (plot_dots[i][1] & 7);
                own[k] |= t;
                own[k + 3] |= t;
            }
        }
        if (look->mountains)
            memcpy(wh, plot_parts[PP_MOUNT + look->mountains - 1], 96);
    }
    ART_END();
    ++gfx_nplot;

    cx0 = 2 + 3 * (p % 9);
    cy0 = 2 + 4 * (p / 9);
    wi = (look->mountains && !look->store) ? (pset == PS_MAP ? MAP_BLACK : 0) : 1;
    w0 = (wi & 1) ? 0xFF : 0;
    w1 = (wi & 2) ? 0xFF : 0;
    for (cy = 0; cy < 4; ++cy) {
        d = cbuf;
        for (cx = 0; cx < 3; ++cx) {
            oi = look->owned ? cidx(cx0 + cx, cy0 + cy, look->col) : 0;
            o0 = (oi & 1) ? 0xFF : 0;
            o1 = (oi & 2) ? 0xFF : 0;
            for (r = 0; r < 8; ++r) {
                k = ((cy << 3) + r) * 3 + cx;
                o = own[k];
                w = wh[k] & ~o;
                d[r] = (o & o0) | (w & w0);
                d[8 + r] = (o & o1) | (w & w1);
            }
            d += 16;
        }
        q_copy(THIRD(cy0 + cy), cell_addr(cx0, cy0 + cy), cbuf, 48);
    }
    attr_flush();

    if (p < 45) {
        k = look->river && !look->store;
        if (k != river_on[p]) {
            river_on[p] = k;
            river_n += k ? 1 : -1;
        }
    }
}

/* ---- the town ----------------------------------------------------------- */
/* town_rle: n < 128, n + 1 bytes follow; n >= 128, the next byte n - 125
 * times. Unpacked a cell row (304 bytes) at a time, sent 64 at a time.
 * Only the development phase draws it: this and the town's tables are in
 * its overlay (OVERLAY5), not the art's bank. */
#pragma code-name (push, "OVERLAY5")
static const uint8_t *rl_src;
static uint8_t rl_lit, rl_run, rl_v;

static void unpack(uint8_t *d, uint8_t n)
{
    uint8_t c;
    while (n--) {
        if (!rl_lit && !rl_run) {
            c = *rl_src++;
            if (c >= 128) {
                rl_run = c - 125;
                rl_v = *rl_src++;
            } else {
                rl_lit = c + 1;
            }
        }
        if (rl_run) {
            --rl_run;
            *d++ = rl_v;
        } else {
            --rl_lit;
            *d++ = *rl_src++;
        }
    }
}

void gfx_town(void)
{
    uint8_t cy, k, bx, by;
    uint16_t a, left;
    if (pset != PS_TOWN) {
        pset = PS_TOWN;
        palettes();
    }
    rl_src = town_rle;
    rl_lit = rl_run = 0;
    for (cy = 0; cy < TOWN_CH; ++cy) {
        a = cell_addr(TOWN_CX, TOWN_CY + cy);
        for (left = TOWN_CW * 16; left; left -= k, a += k) {
            k = left > 64 ? 64 : (uint8_t)left;
            unpack(cbuf, k);
            q_copy(THIRD(TOWN_CY + cy), a, cbuf, k);
        }
    }
    for (by = 0; by < 9; ++by)
        for (bx = 0; bx < 10; ++bx)
            set_pal((bx + 3) << 1, (by << 1) + TOWN_CY, town_pal[by][bx]);
    attr_flush();
}
#pragma code-name (pop)

/* ---- sprites ------------------------------------------------------------ */
#define PAIRS 6                 /* 8 x 16 sprites a slot */
#define RIVER_PAIR 1            /* the river's: 2 across, 2 down (pair 0 is
                                 * the margins' blank tile, crt0.s) */
#define RIVER_PAIRS 4
#define PAIR_OF(i) (RIVER_PAIR + RIVER_PAIRS + (i) * PAIRS)

static uint8_t s_on[SPR_SLOTS], s_shape[SPR_SLOTS], s_frame[SPR_SLOTS];
static uint8_t s_col[SPR_SLOTS], s_flags[SPR_SLOTS];
static int16_t s_x[SPR_SLOTS], s_y[SPR_SLOTS];
static uint8_t s_sent_shape[SPR_SLOTS], s_sent_frame[SPR_SLOTS], s_sent_col[SPR_SLOTS];
static uint8_t any_dirty, rot, river_shown;
static uint8_t river_x0;        /* the river's first pixel in its plot */

static void spr_init(void)
{
    memset(s_sent_shape, 0xFF, sizeof(s_sent_shape));   /* nothing sent */
}

void spr_set(uint8_t i, uint8_t shape, uint8_t frame, int16_t x, int16_t y,
             uint8_t col, uint8_t flags)
{
    if (s_on[i] && s_x[i] == x && s_y[i] == y && s_frame[i] == frame &&
        s_shape[i] == shape && s_col[i] == col && s_flags[i] == flags)
        return;
    s_on[i] = 1;
    s_shape[i] = shape;
    s_frame[i] = frame;
    s_x[i] = x;
    s_y[i] = y;
    s_col[i] = col;
    s_flags[i] = flags;
    any_dirty = 1;
}

void spr_hide(uint8_t i)
{
    if (!s_on[i])
        return;
    s_on[i] = 0;
    any_dirty = 1;
}

void spr_hide_all(void)
{
    uint8_t i;
    for (i = 0; i < SPR_SLOTS; ++i)
        spr_hide(i);
}

/* The frame slot i shows (mirrored ones are frames of their own). */
static uint8_t frame_of(uint8_t i)
{
    uint8_t f = s_frame[i];
    if (s_flags[i] & SPR_FLIPX) {
        if (s_shape[i] == SH_MULE)
            f += 4;
        else if (s_shape[i] == SH_FIGURE && (f % 10) < 2)
            f += 2;
    }
    return f;
}

/* A shape's lines, widened to pixels: line[] for line l of frame f. */
static uint8_t line[6];
static void widen(const ShapeDef *d, const uint8_t *src, uint8_t l)
{
    uint8_t b;
    uint16_t v;
    memset(line, 0, sizeof(line));
    if (l >= d->h)
        return;
    src += l * d->wb;
    for (b = 0; b < d->wb && b < 4; ++b) {
        /* 8 clocks -> 12 pixels, from pixel 12b */
        v = wide12[src[b]];
        if (b & 1) {
            line[(b >> 1) * 3 + 1] |= (uint8_t)(v >> 8);
            line[(b >> 1) * 3 + 2] = (uint8_t)v;
        } else {
            line[(b >> 1) * 3] = (uint8_t)(v >> 4);
            line[(b >> 1) * 3 + 1] = (uint8_t)(v << 4);
        }
    }
}

/* Send slot i's frame f in colour ci (1-3) as its patterns: pair
 * gc + cols * gr, two tiles of 16 bytes (planes 0 and 1). */
static void send(uint8_t i, uint8_t shape, uint8_t f, uint8_t ci, uint8_t cols, uint8_t rows)
{
    static uint8_t pat[32];
    const ShapeDef *d;
    const uint8_t *src;
    uint8_t pair, gc, gr, l, v, k;

    planes(ci);
    ART_BEGIN();
    d = &nes_shapes[shape];
    src = d->data + (uint16_t)f * d->wb * d->h;
    pair = PAIR_OF(i);
    for (gr = 0; gr < rows; ++gr) {
        for (gc = 0; gc < cols; ++gc, ++pair) {
            for (l = 0; l < 16; ++l) {
                widen(d, src, (gr << 4) + l);
                v = line[gc];
                k = (l & 8) ? 16 + (l & 7) : l;
                pat[k] = v & m0;
                pat[k + 8] = v & m1;
            }
            q_copy(Q_NOBANK, 0x1000 + ((uint16_t)pair << 5), pat, 32);
        }
    }
    ART_END();
}

/* 8 pixels of a 24-pixel line (3 bytes, MSB left) from pixel s. */
static uint8_t river_byte(const uint8_t *l, uint8_t s)
{
    uint8_t i = s >> 3, sh = s & 7, v = l[i] << sh;
    if (sh && i < 2)
        v |= l[i + 1] >> (8 - sh);
    return v;
}

/* The river's patterns: the river part's pixels from river_x0, two tiles
 * across (16 pixels), 32 lines, in colour 3. */
static void river_init(void)
{
    static uint8_t pat[32];
    uint8_t pair, gc, gr, l, k, c, x0 = 24, b;
    ART_BEGIN();
    for (l = 0; l < 32; ++l)
        for (c = 0; c < 24; ++c)
            if ((plot_parts[PP_RIVER][l * 3 + (c >> 3)] & (0x80 >> (c & 7))) && c < x0)
                x0 = c;
    if (x0 > 8)
        x0 = 8;
    river_x0 = x0;
    pair = RIVER_PAIR;
    for (gr = 0; gr < 2; ++gr) {
        for (gc = 0; gc < 2; ++gc, ++pair) {
            for (l = 0; l < 16; ++l) {
                b = river_byte(&plot_parts[PP_RIVER][((gr << 4) + l) * 3], x0 + (gc << 3));
                k = (l & 8) ? 16 + (l & 7) : l;
                pat[k] = b;
                pat[k + 8] = b;
            }
            q_copy(Q_NOBANK, 0x1000 + ((uint16_t)pair << 5), pat, 32);
        }
    }
    ART_END();
}

void spr_flush(void)
{
    const ShapeDef *d;
    uint8_t *o, *page;
    uint8_t i, k, f, cols, rows, gc, gr, n, shape, h, w, sc, ci, at, pair, p, ovf;
    int16_t x, y;

    ovf = spr_ovf;
    spr_ovf = 0;
    if (!any_dirty && !ovf && river_shown == river_n)
        return;
    if (ovf)                        /* a line dropped one: take turns */
        rot = rot + 1 < SPR_SLOTS ? rot + 1 : 0;
    any_dirty = 0;
    n = 0;
    /* the page the NMI isn't sending (and won't, until it is named) */
    oam_next = 0;
    page = o = oam_show == (uint8_t)((uint16_t)oam >> 8) ? oam2 : oam;
    for (k = 0; k < SPR_SLOTS; ++k) {
        i = k + rot;
        if (i >= SPR_SLOTS)
            i -= SPR_SLOTS;
        if (!s_on[i])
            continue;
        shape = s_shape[i];
        ART_BEGIN();
        d = &nes_shapes[shape];
        h = d->h;
        w = d->w;
        ART_END();
        cols = (uint8_t)(((w * 3 + 1) / 2 + 7) >> 3);
        rows = (h + 15) >> 4;
        if (cols * rows > PAIRS)
            rows = PAIRS / cols;
        f = frame_of(i);
        sc = spr_col[s_col[i] & 15];
        ci = sc & 3;
        if (s_sent_shape[i] != shape || s_sent_frame[i] != f || s_sent_col[i] != ci) {
            send(i, shape, f, ci, cols, rows);
            s_sent_shape[i] = shape;
            s_sent_frame[i] = f;
            s_sent_col[i] = ci;
        }
        at = sc >> 2;
        pair = PAIR_OF(i);
        for (gr = 0; gr < rows; ++gr) {
            for (gc = 0; gc < cols; ++gc, ++pair) {
                y = s_y[i] + (gr << 4);
                x = s_x[i];
                x = x + (x >> 1) + 4 + (gc << 3);   /* PX, signed */
                if (y <= -16 || y >= 192 || x < 0 || x >= 256 || n >= 64)
                    continue;
                o[0] = (uint8_t)(y + 16 - 1);
                o[1] = (pair << 1) | 1;
                o[2] = at;
                o[3] = (uint8_t)x;
                o += 4;
                ++n;
            }
        }
    }
    /* the river, last: under everything */
    river_shown = river_n;
    if (river_n) {
        for (p = 0; p < 45; ++p) {
            if (!river_on[p])
                continue;
            for (gr = 0; gr < 2; ++gr)
                for (gc = 0; gc < 2; ++gc) {
                    if (n >= 64)
                        break;
                    o[0] = (uint8_t)((2 + 4 * (p / 9)) * 8 + (gr << 4) + 16 - 1);
                    o[1] = ((RIVER_PAIR + gr * 2 + gc) << 1) | 1;
                    o[2] = 0x20 | (RIVER_SPR >> 2);
                    o[3] = (uint8_t)((2 + 3 * (p % 9)) * 8 + river_x0 + (gc << 3));
                    o += 4;
                    ++n;
                }
        }
    }
    for (; n < 64; ++n, o += 4)
        o[0] = 0xF8;
    oam_next = (uint8_t)((uint16_t)page >> 8);
}
