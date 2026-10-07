/*
 * gfx.c -- gfx.h on the Apple II's HGR page 1 (hires.s does the pixels).
 *
 * Atari colour clocks become HGR colour pixels at 7/8 (160 -> 140): a text
 * cell or 4 colour clocks is one screen byte, and a map plot (16 clocks) is
 * four. Scanlines are kept 1:1.
 */
#ifdef __APPLE2__

#include <string.h>
#include "gfx.h"
#include "gfxdata.h"
#include "bank.h"
#include "plat.h"

void aux_get(void *dst, const void *src, uint16_t n);     /* lowlevel.s */
extern const uint8_t hgr_rowlo[192], hgr_rowhi[192];

/* hires.s */
extern uint8_t hg_bx, hg_y, hg_h, hg_n, hg_mask, hg_mask1, hg_hi, hg_v0, hg_v1;
extern uint8_t hg_x, hg_wb, hg_col;
extern const uint8_t *hg_src;
#pragma zpsym ("hg_src")
extern const uint8_t hgr_cxb0[140], hgr_cxb1[140];
void hgr_init(void);
void hgr_clear(void);
void hgr_fill(void);
void hgr_glyph(void);
void hgr_glyph2(void);
void hgr_blit(void);
void hgr_save(void);
void hgr_spr(void);
extern uint8_t sl_cl[SPR_SLOTS], sl_nc, sl_y0, sl_y1, sl_pass;
void spr_lines(void);
void spr_one(void);

/* HGR colours as hires.s numbers them. */
#define H_BLACK  0
#define H_WHITE  1
#define H_VIOLET 2
#define H_GREEN  3
#define H_BLUE   4
#define H_ORANGE 5

const uint8_t hcol[16] = {     /* (hires.s too) */
    H_BLACK,    /* C_BLACK */
    H_WHITE,    /* C_WHITE */
    H_BLUE,     /* C_BLUE */
    H_VIOLET,   /* C_PURPLE */
    H_GREEN,    /* C_GREEN */
    H_ORANGE,   /* C_ORANGE */
    H_ORANGE,   /* C_RED */
    H_ORANGE,   /* C_YELLOW */
    H_WHITE,    /* C_GREY */
    H_BLUE,     /* C_CYAN */
    H_ORANGE,   /* C_BROWN */
    H_ORANGE,   /* C_TAN */
    H_BLUE,     /* C_LTBLUE */
    H_GREEN,    /* C_DKGREEN */
    H_VIOLET,   /* C_PINK */
    H_ORANGE,   /* C_P3 */
};

/* Per HGR colour: the bits of an even / odd screen byte that show it, and
 * the palette bit. (A byte in an even column starts on an even pixel.) */
static const uint8_t mask_even[6] = { 0x00, 0x7F, 0x55, 0x2A, 0x55, 0x2A };
static const uint8_t mask_odd[6]  = { 0x00, 0x7F, 0x2A, 0x55, 0x2A, 0x55 };
static const uint8_t pal_hi[6]    = { 0x00, 0x00, 0x00, 0x00, 0x80, 0x80 };

static void spr_bg(uint8_t bx, uint8_t n, uint8_t y, uint8_t h);
static void bg_done(void);

void gfx_init(void)
{
    hgr_init();
}

void gfx_clear(void)
{
    spr_hide_all();
    spr_flush();
    hg_y = 0;
    hg_h = 192;
    hgr_clear();
}

/* ---- text ------------------------------------------------------------- */
void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col)
{
    uint8_t h = hcol[col];
    if (tx > 39 || ty > 23)
        return;
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c < 32 || c > 95)
        c = '?';
    hg_src = hgr_font + ((uint8_t)(c - 32) << 3);
    hg_bx = tx;
    hg_y = ty << 3;
    hg_mask = mask_even[h];
    hg_mask1 = mask_odd[h];
    hg_hi = pal_hi[h];
    hgr_glyph();
}

void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    if (ty > 23)
        return;
    spr_bg(tx, n, ty << 3, 8);
    while (n-- && *s && tx < 40) {
        gfx_char(tx++, ty, *s++, col);
        frame_tick();
    }
    bg_done();
}

void gfx_text(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_textn(tx, ty, s, 40, col);
}

void gfx_clear_text(uint8_t tx, uint8_t ty, uint8_t w)
{
    if (tx >= 40 || ty > 23)
        return;
    if (tx + w > 40)
        w = 40 - tx;
    spr_bg(tx, w, ty << 3, 8);
    hg_bx = tx;
    hg_n = w;
    hg_y = ty << 3;
    hg_h = 8;
    hg_v0 = hg_v1 = 0;
    hgr_fill();
    bg_done();
}

void gfx_bign(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    uint8_t h = hcol[col];
    char c;
    tx &= 0xFE;
    if (ty > 23)
        return;
    spr_bg(tx, n * 2, ty << 3, 8);
    hg_mask = mask_even[h];
    hg_mask1 = mask_odd[h];
    hg_hi = pal_hi[h];
    hg_y = ty << 3;
    while (n-- && (c = *s++) != 0 && tx < 40) {
        if (c >= 'a' && c <= 'z')
            c -= 32;
        if (c < 32 || c > 95)
            c = '?';
        hg_src = hgr_font + ((uint8_t)(c - 32) << 3);
        hg_bx = tx;
        hgr_glyph2();
        tx += 2;
    }
    bg_done();
}

void gfx_big(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_bign(tx, ty, s, 20, col);
}

/* ---- rectangles ------------------------------------------------------- */
void gfx_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
    uint8_t c = hcol[col];
    uint8_t bx = x >> 2, n = ((uint8_t)(x + w + 3) >> 2) - bx;
    if (bx + n > 40)
        n = 40 - bx;
    /* sprites off first: taking one off sets hg_ (its own rectangle) */
    spr_bg(bx, n, y, h);
    hg_bx = bx;
    hg_n = n;
    hg_y = y;
    hg_h = h;
    /* hgr_fill picks v0 or v1 by each column's parity */
    hg_v0 = mask_even[c] | pal_hi[c];
    hg_v1 = mask_odd[c] | pal_hi[c];
    hgr_fill();
    bg_done();
}

/* x (colour clocks, may be off the left) -> hires.s's biased colour pixel */
static uint8_t cx_bias(int16_t x)
{
    if (x < -64)
        x = -64;
    if (x > 210)
        x = 210;
    return (uint8_t)(((uint16_t)(x + 64) * 7) >> 3) + 8;
}

static const uint8_t dots_row[20] = {
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
};

void gfx_hdots(uint8_t x, uint8_t y, uint8_t w, uint8_t col)
{
    uint8_t cx = cx_bias(x);
    uint8_t wb = (uint8_t)(((uint16_t)w * 7 / 8 + 7) >> 3);
    spr_bg(hgr_cxb0[cx - 64], (uint8_t)((uint16_t)w * 7 / 32) + 2, y, 1);
    hg_src = dots_row;
    hg_wb = wb;
    hg_h = 1;
    hg_x = cx;
    hg_y = y;
    hg_col = hcol[col];
    hgr_spr();
    bg_done();
}

void gfx_pic(uint8_t x, uint8_t y, uint8_t pic, uint8_t col)
{
    static uint8_t tall[16];
    uint8_t i;
    uint8_t cx = cx_bias(x);
    for (i = 0; i < 8; ++i)
        tall[i * 2] = tall[i * 2 + 1] = hgr_pics[pic][i];
    spr_bg(hgr_cxb0[cx - 64], 3, y, 16);
    /* the picture replaces what was there: clear its box first */
    hg_bx = hgr_cxb0[cx - 64];
    hg_n = 3;
    if (hg_bx > 37)
        hg_n = 40 - hg_bx;
    hg_y = y;
    hg_h = 16;
    hg_v0 = hg_v1 = 0;
    hgr_fill();
    hg_src = tall;
    hg_wb = 1;
    hg_h = 16;
    hg_x = cx;
    hg_y = y;
    hg_col = hcol[col];
    hgr_spr();
    bg_done();
}

/* ---- the map ----------------------------------------------------------
 * A plot is built by hires.s hgr_plot from 128-byte layers (gfxdata.h
 * plot_parts): the owner's border, icon and production dots in the owner's
 * colour, mountains (or the store) in white, the river in blue. */
extern const uint8_t *pl_own1, *pl_own2, *pl_own3, *pl_white, *pl_river;
#pragma zpsym ("pl_own1")
#pragma zpsym ("pl_own2")
#pragma zpsym ("pl_own3")
#pragma zpsym ("pl_white")
#pragma zpsym ("pl_river")
extern uint8_t pl_om[4], pl_hi;
void hgr_plot(void);

static const uint8_t zeros[128];
static uint8_t dots[128];

void gfx_map_frame(void)
{
    gfx_clear();
}

void gfx_plot(uint8_t p, const PlotLook *look)
{
    uint8_t i, b, c = hcol[look->col];

    pl_own1 = pl_own2 = pl_own3 = pl_white = pl_river = zeros;
    if (look->store) {
        pl_white = plot_parts[PP_STORE];
    } else {
        if (look->owned) {
            pl_own1 = plot_parts[PP_BORDER];
            if (look->icon)
                pl_own2 = plot_parts[PP_ICON + look->icon - 1];
            if (look->dots) {
                memset(dots, 0, 128);
                for (i = 0; i < look->dots && i < 8; ++i) {
                    b = plot_dots[i][0] * 4;
                    dots[b + plot_dots[i][1]] |= plot_dots[i][2];
                    dots[b + plot_dots[i][3]] |= plot_dots[i][4];
                    dots[b + 4 + plot_dots[i][1]] |= plot_dots[i][2];
                    dots[b + 4 + plot_dots[i][3]] |= plot_dots[i][4];
                }
                pl_own3 = dots;
            }
        }
        if (look->mountains)
            pl_white = plot_parts[PP_MOUNT + look->mountains - 1];
        if (look->river)
            pl_river = plot_parts[PP_RIVER];
    }
    pl_om[0] = pl_om[2] = mask_even[c];
    pl_om[1] = pl_om[3] = mask_odd[c];
    pl_hi = pal_hi[c];
    /* sprites off first: taking one off sets hg_ (its own rectangle) */
    b = 2 + (p % 9) * 4;
    i = 16 + (p / 9) * 32;
    spr_bg(b, 4, i, 32);
    hg_bx = b;
    hg_y = i;
    hgr_plot();
    bg_done();
}

void gfx_town(void)
{
    uint8_t y;
    const uint8_t *src = hgr_town;
    spr_bg(8, 24, 16, 144);
    for (y = 16; y < 160; ++y) {
        aux_get((uint8_t *)((hgr_rowhi[y] << 8) | hgr_rowlo[y]) + 8, src, 24);
        src += 24;
    }
    bg_done();
}

/* ---- sprites ----------------------------------------------------------
 * Each slot keeps what the game asked for and, once drawn, the rectangle
 * (bytes bx..bx+n-1, lines sy..sy+sh-1) it saved from under itself.
 * Parallel byte arrays: cc65 indexes those without multiplying.
 */
#define SAVE_MAX 170

uint8_t s_on[SPR_SLOTS], s_shape[SPR_SLOTS], s_frame[SPR_SLOTS];  /* (not static: tools/a2sim peeks) */
uint8_t s_col[SPR_SLOTS], s_flags[SPR_SLOTS];             /* (hires.s src_of too) */
uint8_t s_xl[SPR_SLOTS], s_xh[SPR_SLOTS], s_yl[SPR_SLOTS], s_yh[SPR_SLOTS];
uint8_t s_dirty[SPR_SLOTS];  /* needs (re)drawing */
uint8_t s_drawn[SPR_SLOTS];  /* on the screen: the rectangle is valid */
uint8_t s_bx[SPR_SLOTS], s_n[SPR_SLOTS], s_sy[SPR_SLOTS], s_sh[SPR_SLOTS];   /* (hires.s too) */
/* The save buffers: above the overlay window (SPRSAVE, $B620: no overlay
 * reaches it, see mule-hgr.cfg). */
#pragma bss-name (push, "SPRSAVE")
static uint8_t save_hi[SPR_SLOTS * SAVE_MAX];
#pragma bss-name (pop)
uint8_t *const save_at[SPR_SLOTS] = {
    save_hi, save_hi + SAVE_MAX, save_hi + 2 * SAVE_MAX, save_hi + 3 * SAVE_MAX,
    save_hi + 4 * SAVE_MAX, save_hi + 5 * SAVE_MAX, save_hi + 6 * SAVE_MAX,
    save_hi + 7 * SAVE_MAX, save_hi + 8 * SAVE_MAX, save_hi + 9 * SAVE_MAX,
};
static uint8_t any_dirty;

void spr_set(uint8_t i, uint8_t shape, uint8_t frame, int16_t x, int16_t y,
             uint8_t col, uint8_t flags)
{
    /* compared a byte at a time: cc65's 16-bit compares are slow */
    uint8_t xl = (uint8_t)x, xh = (uint8_t)((uint16_t)x >> 8);
    uint8_t yl = (uint8_t)y, yh = (uint8_t)((uint16_t)y >> 8);
    if (s_on[i] && s_xl[i] == xl && s_yl[i] == yl && s_frame[i] == frame &&
        s_shape[i] == shape && s_xh[i] == xh && s_yh[i] == yh && s_col[i] == col &&
        s_flags[i] == flags)
        return;
    s_on[i] = 1;
    s_shape[i] = shape;
    s_frame[i] = frame;
    s_xl[i] = xl;
    s_xh[i] = xh;
    s_yl[i] = yl;
    s_yh[i] = yh;
    s_col[i] = col;
    s_flags[i] = flags;
    s_dirty[i] = 1;
    any_dirty = 1;
}

void spr_hide(uint8_t i)
{
    if (!s_on[i])
        return;
    s_on[i] = 0;
    s_dirty[i] = 1;
    any_dirty = 1;
}

void spr_hide_all(void)
{
    uint8_t i;
    for (i = 0; i < SPR_SLOTS; ++i)
        spr_hide(i);
}

/* The save buffers are rings of rows: line y's bytes are row y % rows
 * (rows = SAVE_MAX / the shape's widest row, st), so a sprite that moves a
 * little finds its old row for a line just where its new one goes. A
 * flush then does each line at once, top to bottom -- put the old rows
 * back (last drawn first), save the new ones, draw -- and no sprite is
 * ever off the screen while the beam goes by. */
extern uint8_t d_oo[SPR_SLOTS], d_oend[SPR_SLOTS], d_show[SPR_SLOTS];
/* hires.s spr_groups: the flush's sprites (g_cl) drawn, alone or together */
extern uint8_t g_cl[SPR_SLOTS], g_nc, g_y0, g_y1, g_pass;
void spr_groups(void);
uint8_t s_st[SPR_SLOTS];                /* the ring's row length when drawn */
static uint8_t s_oo[SPR_SLOTS], s_oend[SPR_SLOTS];  /* its first line's row, its end */
extern uint8_t d_no0[SPR_SLOTS];        /* (hires.s src_of: d_no before drawing) */
uint8_t lift_y0, lift_y1;               /* lines spr_bg took sprites off */

static uint8_t ring(uint8_t st, uint8_t y)
{
    return (uint8_t)(y % (uint8_t)(SAVE_MAX / st)) * st;
}

/* Mark every drawn slot after i that meets a rectangle (hires.s): slot
 * i's drawn one, or its new one; or every slot meeting co_* (hit[] cleared
 * first). (All 8-bit: a rectangle's right and bottom edges are at most 40
 * and 192.) */
uint8_t hit[SPR_SLOTS];                 /* (bit 1: taken off by spr_bg) */
extern uint8_t co_bx, co_n, co_y, co_h;
void __fastcall__ close_s(uint8_t i);
void __fastcall__ close_d(uint8_t i);
void close_hits(void);

/* Background drawing is about to change a rectangle: the sprites on it,
 * and any drawn after one of them over it (their saved backgrounds hold
 * its pixels), come off -- only its lines; bg_done puts them back over the
 * new background. */
static uint8_t bg_took;

static void spr_bg(uint8_t bx, uint8_t n, uint8_t y, uint8_t h)
{
    uint8_t i, nc = 0, y1 = y + h;
    co_bx = bx;
    co_n = n;
    co_y = y;
    co_h = h;
    close_hits();
    for (i = 0; i < SPR_SLOTS; ++i)
        if (hit[i]) {
            frame_tick();
            close_s(i);
            g_cl[nc++] = i;
            d_show[i] = 0;      /* (only the old place counts here) */
            d_oo[i] = s_sy[i] >= y ? s_oo[i] : ring(s_st[i], y);
            d_oend[i] = s_oend[i];
            s_dirty[i] |= 2;
            bg_took = any_dirty = 1;
        }
    lift_y0 = y;
    lift_y1 = y1;
    if (nc) {
        g_nc = nc;
        g_y0 = y;
        g_y1 = y1;
        g_pass = 4;             /* only put back */
        spr_groups();
    }
}

/* After background drawing: the sprites it took off go straight back, so
 * they are only missing (a few lines of them) while it draws. */
static void bg_done(void)
{
    if (bg_took) {
        bg_took = 0;
        spr_flush();
    }
}

/* Each sprite spr_flush draws: where (as s_bx.. will say once it is), the
 * picture's next row (in aux RAM), where its rows go in the rings, and how
 * hires.s draws it (colour; first colour pixel + 64; all of it on the
 * screen): set by hires.s src_of. */
const uint8_t *d_src[SPR_SLOTS];
uint8_t d_bx[SPR_SLOTS], d_n[SPR_SLOTS], d_sy[SPR_SLOTS];
uint8_t d_sh[SPR_SLOTS], d_st[SPR_SLOTS], d_wb[SPR_SLOTS], d_show[SPR_SLOTS];
uint8_t d_no[SPR_SLOTS], d_nend[SPR_SLOTS], d_oo[SPR_SLOTS], d_oend[SPR_SLOTS];
uint8_t d_col[SPR_SLOTS], d_cx[SPR_SLOTS], d_fast[SPR_SLOTS], d_rows[SPR_SLOTS];

/* Where slot i would be drawn now (d_*[i]): hires.s src_of. */
void src_of(uint8_t i);

void spr_flush(void)
{
    uint8_t i, k, y0, y1, pass, e, nc, part;

    if (!any_dirty)
        return;
    any_dirty = 0;
    for (i = 0; i < SPR_SLOTS; ++i)
        hit[i] = s_dirty[i];
    /* the changing sprites, and anything drawn after one of them over its
     * old place or its new one (drawn again: its saved background would be
     * wrong otherwise). s_bx.. are still the old places. */
    y0 = 255;
    y1 = 0;
    nc = 0;
    pass = 2;                   /* each line at once */
    part = 1;                   /* only spr_bg's lines (it put those back) */
    for (i = 0; i < SPR_SLOTS; ++i) {
        if (!hit[i])
            continue;
        frame_tick();           /* (planning a flush takes a while) */
        if (hit[i] != 2)
            part = 0;
        g_cl[nc++] = i;
        src_of(i);
        if (d_show[i]) {
            if (d_sy[i] < y0)
                y0 = d_sy[i];
            if ((e = d_sy[i] + d_sh[i]) > y1)
                y1 = e;
        }
        if (s_drawn[i]) {
            d_oo[i] = s_oo[i];
            d_oend[i] = s_oend[i];
            if (s_sy[i] < y0)
                y0 = s_sy[i];
            if ((e = s_sy[i] + s_sh[i]) > y1)
                y1 = e;
            /* a line's new row must not land where an old row not put back
             * yet is: if it could, the old all come off first */
            if (d_show[i] && (d_st[i] != s_st[i] || e > (uint16_t)d_sy[i] + d_rows[i]))
                pass = 0;
            close_s(i);
        }
        if (d_show[i])
            close_d(i);
    }
    if (part && nc) {
        /* nothing moved: save and draw again only what spr_bg took off */
        pass = 1;
        if (y0 < lift_y0)
            y0 = lift_y0;
        if (y1 > lift_y1)
            y1 = lift_y1;
        for (k = 0; k < nc; ++k) {
            i = g_cl[k];
            if (d_show[i] && d_sy[i] < y0) {
                d_src[i] += (uint8_t)(y0 - d_sy[i]) * d_wb[i];
                d_no[i] = ring(d_st[i], y0);
            }
        }
    }
    if (y0 < y1) {
        g_nc = nc;
        g_y0 = y0;
        g_y1 = y1;
        g_pass = pass;
        spr_groups();
    }
    for (k = 0; k < nc; ++k) {
        i = g_cl[k];
        s_dirty[i] = 0;
        if ((s_drawn[i] = d_show[i]) != 0) {
            s_bx[i] = d_bx[i];
            s_n[i] = d_n[i];
            s_sy[i] = d_sy[i];
            s_sh[i] = d_sh[i];
            s_st[i] = d_st[i];
            s_oo[i] = d_no0[i];
            s_oend[i] = d_nend[i];
        }
    }
    lift_y1 = 0;
}

#endif /* __APPLE2__ */
