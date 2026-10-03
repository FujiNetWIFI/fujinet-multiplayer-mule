/*
 * gfx.c -- gfx.h on the Lynx: 160 x 102, 16 colours, drawn by Suzy.
 *
 * Across, an Atari colour clock is a Lynx pixel. Down, 192 lines go into
 * 102 (ly()):
 *   rows 0-1     (lines 0-15)    5 lines each: 0, 5
 *   rows 2-21    (lines 16-175)  the map and the town, at half height: a
 *                                plot is 16 x 16, the town 96 x 72, from
 *                                line 10; text rows alternate 5 and 3
 *                                lines (an even row's text has room, an
 *                                odd row's runs 2 lines into the next --
 *                                no screen writes both), but 18 and 19
 *                                move up a line (the town's corral price
 *                                is on 19, its money on 20)
 *   rows 22-23   (lines 176-191) 5 lines each: 92, 97
 * Text is a 3 x 5 font in 4-pixel cells (40 across), big text the same
 * twice as wide.
 *
 * Everything is drawn into BACK, the background; what changed is copied
 * to SCREEN (the one shown) and the sprites are drawn over it again, so a
 * sprite never needs what was under it saved. Suzy does the drawing; while
 * she does, the CPU and its interrupts wait, so nothing large is drawn in
 * one go (ComLynx has one byte of buffering).
 */
#ifdef __LYNX__

#include <string.h>
#include "gfx.h"
#include "gfxdata.h"
#include "lxhw.h"

#define PEN_BLACK 14            /* black that shows in a sprite (mkgfx_lynx.py) */

/* gfx.h colour -> pen: the colours in order, C_P3 in orange's */
static const uint8_t pen_of[16] = {
    PEN_BLACK, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, PEN_BLACK, 5,
};

const uint8_t row_y[24] = {
    0, 5,
    10, 15, 18, 23, 26, 31, 34, 39, 42, 47, 50, 55, 58, 63, 66, 71, 72, 77, 82, 87,
    92, 97,
};

static uint16_t lineoff[LX_H];
static uint8_t bg_pen;          /* the screen's background */
static uint8_t grey;            /* the map or the town: white text is black */

/* An Atari line (0..192) -> a Lynx line (0..102). */
uint8_t __fastcall__ ly(int16_t y)
{
    if (y <= 0)
        return 0;
    if (y >= 192)
        return LX_H;
    if (y >= 16 && y < 176)
        return 10 + ((uint8_t)(y - 16) >> 1);
    return row_y[(uint8_t)y >> 3] + ((((uint8_t)y & 7) * 5) >> 3);
}

/* The same for a sprite, which may be off the screen. */
static int16_t lys(int16_t y)
{
    if (y < 0)
        return y >> 1;
    if (y > 191)
        return LX_H + ((y - 192) >> 1);
    return ly(y);
}

/* ---- Suzy --------------------------------------------------------------- */
typedef struct {
    uint8_t ctl0, ctl1, coll;
    uint16_t next;
    const uint8_t *data;
    int16_t x, y;
    uint16_t hs, vs;
    uint8_t pal[8];
} Scb;

static Scb scb;

/* A packed sprite of one pixel of pen index 1 (cc65's TGI clears with it). */
static const uint8_t pixel[] = { 3, 0x84, 0x00, 0 };

/* A rectangle of pen in the buffer suzy_dst, a few lines at a time. */
static void fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pen)
{
    uint8_t n;
    if (!w || !h)
        return;
    scb.ctl0 = 0x01;            /* 1 bit, background non-colliding: opaque */
    scb.ctl1 = 0x10;            /* packed, reload h/v size */
    scb.coll = 0x20;
    scb.next = 0;
    scb.data = pixel;
    scb.x = x;
    scb.hs = (uint16_t)w << 8;
    scb.pal[0] = pen * 0x11;
    while (h) {
        n = h > 8 ? 8 : h;
        scb.y = y;
        scb.vs = (uint16_t)n << 8;
        suzy_draw(&scb);
        y += n;
        h -= n;
    }
}

/* A 1-bit picture (gfxdata.h: h lines of wb bytes): lit pixels in pen, the
 * rest seen through. hs: $100 as it is, $200 twice as wide. Suzy gets it as
 * a literal sprite: each line its length + 1, its bytes, and a 0 (she
 * drops a literal line's last bits), then a 0. */
static uint8_t lit[72];

static void spr1(const uint8_t *src, uint8_t wb, uint8_t h, int16_t x, int16_t y,
                 uint8_t pen, uint16_t hs)
{
    uint8_t *p = lit, k;
    while (h--) {
        *p++ = wb + 2;
        for (k = wb; k; --k)
            *p++ = *src++;
        *p++ = 0;
    }
    *p = 0;
    scb.ctl0 = 0x05;            /* 1 bit, non-colliding: index 0 transparent */
    scb.ctl1 = 0x90;            /* literal, reload h/v size */
    scb.coll = 0x20;
    scb.next = 0;
    scb.data = lit;
    scb.x = x;
    scb.y = y;
    scb.hs = hs;
    scb.vs = 0x100;
    scb.pal[0] = pen;           /* index 0 -> 0, index 1 -> pen */
    suzy_draw(&scb);
}

/* ---- what changed: copied from BACK to SCREEN --------------------------- */
static uint8_t dlo[LX_H], dhi[LX_H];    /* byte columns, per line */
static uint8_t dirty;

static void mark(int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint8_t b0, b1, yy, ye;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > LX_W) w = LX_W - x;
    if (y + h > LX_H) h = LX_H - y;
    if (w <= 0 || h <= 0)
        return;
    b0 = (uint8_t)x >> 1;
    b1 = (uint8_t)(x + w - 1) >> 1;
    ye = (uint8_t)(y + h);
    for (yy = (uint8_t)y; yy < ye; ++yy) {
        if (b0 < dlo[yy]) dlo[yy] = b0;
        if (b1 > dhi[yy]) dhi[yy] = b1;
    }
    dirty = 1;
}

/* ---- sprites -------------------------------------------------------------
 * Each slot keeps what the game asked for (s_*), and what is on the screen
 * (d_*: drawn from, where, the rectangle it covers).
 */
static uint8_t s_on[SPR_SLOTS], s_shape[SPR_SLOTS], s_frame[SPR_SLOTS];
static uint8_t s_col[SPR_SLOTS], s_flags[SPR_SLOTS], s_dirty[SPR_SLOTS];
static int16_t s_x[SPR_SLOTS], s_y[SPR_SLOTS];
static uint8_t any_dirty;

static uint8_t d_on[SPR_SLOTS], d_pen[SPR_SLOTS], d_w[SPR_SLOTS], d_h[SPR_SLOTS], d_wb[SPR_SLOTS];
static const uint8_t *d_data[SPR_SLOTS];
static int16_t d_x[SPR_SLOTS], d_y[SPR_SLOTS];

/* Copy what changed to SCREEN with the sprites over it: a band of lines at
 * a time is made in band[] -- the background from BACK, then Suzy draws
 * the sprites' rows in it -- and copied out, so each byte of SCREEN is
 * written once, as it ends up: a sprite is never off the screen. */
#define BAND 32
static uint8_t band[BAND * LX_BPL];

static void commit(void)
{
    uint8_t y, y0, y1, t, i, lo;
    int16_t top, bot;
    if (!dirty)
        return;
    dirty = 0;
    for (y = 0; y < LX_H;) {
        if (dlo[y] > dhi[y]) {
            ++y;
            continue;
        }
        y0 = y;
        while (y < LX_H && dlo[y] <= dhi[y] && (uint8_t)(y - y0) < BAND)
            ++y;
        y1 = y;
        for (t = y0; t < y1; ++t) {
            lo = dlo[t];
            memcpy(band + (uint16_t)(t - y0) * LX_BPL + lo, BACK + lineoff[t] + lo, dhi[t] - lo + 1);
        }
        suzy_dst = (uint16_t)band - lineoff[y0];
        for (i = 0; i < SPR_SLOTS; ++i) {
            if (!d_on[i])
                continue;
            top = d_y[i] > y0 ? d_y[i] : y0;
            bot = d_y[i] + d_h[i] < y1 ? d_y[i] + d_h[i] : y1;
            if (top < bot)
                spr1(d_data[i] + (uint8_t)(top - d_y[i]) * d_wb[i], d_wb[i], (uint8_t)(bot - top),
                     d_x[i], top, d_pen[i], 0x100);
        }
        suzy_dst = (uint16_t)BACK;
        for (t = y0; t < y1; ++t) {
            lo = dlo[t];
            memcpy(SCREEN + lineoff[t] + lo, band + (uint16_t)(t - y0) * LX_BPL + lo, dhi[t] - lo + 1);
            dlo[t] = 255;
            dhi[t] = 0;
        }
    }
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

void spr_flush(void)
{
    const ShapeDef *d;
    uint8_t i, frame;
    if (any_dirty) {
        any_dirty = 0;
        for (i = 0; i < SPR_SLOTS; ++i) {
            if (!s_dirty[i])
                continue;
            s_dirty[i] = 0;
            if (d_on[i])
                mark(d_x[i], d_y[i], d_w[i], d_h[i]);   /* its old place */
            d_on[i] = 0;
            if (!s_on[i])
                continue;
            d = &lynx_shapes[s_shape[i]];
            frame = s_frame[i];
            if (s_flags[i] & SPR_FLIPX) {   /* the mirrored frames */
                if (s_shape[i] == SH_MULE)
                    frame += 4;
                else if (s_shape[i] == SH_FIGURE && (frame % 10) < 2)
                    frame += 2;
            }
            d_data[i] = d->data + frame * d->fsize;
            d_x[i] = s_x[i];
            d_y[i] = lys(s_y[i]);
            d_w[i] = d->w;
            d_wb[i] = d->wb;
            d_h[i] = d->h;
            d_pen[i] = pen_of[s_col[i] & 15];
            d_on[i] = 1;
            mark(d_x[i], d_y[i], d_w[i], d_h[i]);
        }
    }
    commit();
}

/* ---- screens ------------------------------------------------------------- */
static void clear_to(uint8_t pen, uint8_t is_grey)
{
    uint8_t i;
    for (i = 0; i < SPR_SLOTS; ++i) {
        s_on[i] = s_dirty[i] = d_on[i] = 0;
    }
    any_dirty = 0;
    bg_pen = pen;
    grey = is_grey;
    memset(BACK, pen * 0x11, LX_H * LX_BPL);
    mark(0, 0, LX_W, LX_H);
    commit();
}

void gfx_clear(void)
{
    clear_to(0, 0);
}

void gfx_map_frame(void)
{
    clear_to(pen_of[C_GREY], 1);
}

void gfx_init(void)
{
    uint8_t i;
    for (i = 0; i < 16; ++i) {
        MIKEY_GREEN[i] = lynx_pal[i];
        MIKEY_BLUERED[i] = lynx_pal[16 + i];
    }
    for (i = 0; i < LX_H; ++i) {
        lineoff[i] = (uint16_t)i * LX_BPL;
        dlo[i] = 255;
        dhi[i] = 0;
    }
    suzy_dst = (uint16_t)BACK;
    memset(SCREEN, 0, LX_H * LX_BPL);
    MIKEY_DISPADR = (uint16_t)SCREEN;
    gfx_clear();
}

/* ---- text --------------------------------------------------------------- */
static uint8_t text_pen(uint8_t col)
{
    if (grey && col == C_WHITE)
        return PEN_BLACK;       /* as the Atari prints on the grey screens */
    return pen_of[col & 15];
}

static const uint8_t *glyph_of(char c)
{
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c < 32 || c > 95)
        c = '?';
    return lynx_font[(uint8_t)(c - 32)];
}

/* n characters from cell tx on row ty, opaque; cw: 4 small, 8 big */
static void text_at(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col, uint8_t cw)
{
    uint8_t x = tx << 2, y, m = 0, pen = text_pen(col);
    const char *p = s;
    if (ty > 23 || tx > 39)
        return;
    while (m < n && p[m] && x + (m + 1) * cw <= LX_W)
        ++m;
    if (!m)
        return;
    y = row_y[ty];
    fill(x, y, m * cw, 5, bg_pen);
    while (m--) {
        spr1(glyph_of(*p++), 1, 5, x, y, pen, cw == 8 ? 0x200 : 0x100);
        x += cw;
    }
    mark(tx << 2, y, (uint16_t)(x - (tx << 2)), 5);
    commit();
}

void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    text_at(tx, ty, s, n, col, 4);
}

void gfx_text(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    text_at(tx, ty, s, 40, col, 4);
}

void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col)
{
    char s[2];
    s[0] = c;
    s[1] = 0;
    text_at(tx, ty, s, 1, col, 4);
}

void gfx_bign(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    text_at(tx & 0xFE, ty, s, n, col, 8);
}

void gfx_big(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    text_at(tx & 0xFE, ty, s, 20, col, 8);
}

void gfx_clear_text(uint8_t tx, uint8_t ty, uint8_t w)
{
    uint8_t y;
    if (tx >= 40 || ty > 23)
        return;
    if (tx + w > 40)
        w = 40 - tx;
    y = row_y[ty];
    fill(tx << 2, y, w << 2, 5, bg_pen);
    mark(tx << 2, y, w << 2, 5);
    commit();
}

/* ---- rectangles ------------------------------------------------------- */
void gfx_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
    uint8_t y0 = ly(y), y1 = ly((int16_t)y + h);
    if (h && y1 == y0)
        ++y1;
    if (x >= LX_W)
        return;
    if (x + w > LX_W)
        w = LX_W - x;
    fill(x, y0, w, y1 - y0, col == C_BLACK ? bg_pen : pen_of[col & 15]);
    mark(x, y0, w, y1 - y0);
    commit();
}

/* A dotted line: every other pixel, by the CPU. */
void gfx_hdots(uint8_t x, uint8_t y, uint8_t w, uint8_t col)
{
    uint8_t c, p = pen_of[col & 15], yy = ly(y), *row;
    if (yy >= LX_H)
        return;
    row = BACK + lineoff[yy];
    for (c = 0; c < w && x + c < LX_W; c += 2) {
        if ((x + c) & 1)
            row[(x + c) >> 1] = (row[(x + c) >> 1] & 0xF0) | p;
        else
            row[(x + c) >> 1] = (row[(x + c) >> 1] & 0x0F) | (p << 4);
    }
    mark(x, yy, w, 1);
    commit();
}

void gfx_pic(uint8_t x, uint8_t y, uint8_t pic, uint8_t col)
{
    uint8_t yy = ly(y);
    fill(x, yy, 8, 8, bg_pen);
    spr1(lynx_pics[pic & 3], 1, 8, x, yy, pen_of[col & 15], 0x100);
    mark(x, yy, 8, 8);
    commit();
}

/* ---- the map ---------------------------------------------------------- */
/* A plot, 16 x 16: over the plains the river, the owner's border, icon and
 * dots, then the mountains (black) or the store (white). */
void gfx_plot(uint8_t p, const PlotLook *look)
{
    uint8_t x0 = PLOT_X(p), y0 = ly(PLOT_Y(p)), po = pen_of[look->col & 15], i;
    fill(x0, y0, 16, 16, bg_pen);
    if (look->store) {
        spr1(plot_spr[PP_STORE], 2, 16, x0, y0, pen_of[C_WHITE], 0x100);
    } else {
        if (look->river)
            spr1(plot_spr[PP_RIVER], 2, 16, x0, y0, PEN_RIVER, 0x100);
        if (look->owned) {
            spr1(plot_spr[PP_BORDER], 2, 16, x0, y0, po, 0x100);
            if (look->icon)
                spr1(plot_spr[PP_ICON + look->icon - 1], 2, 16, x0, y0, po, 0x100);
            for (i = 0; i < look->dots && i < 8; ++i)
                fill(x0 + plot_dots[i][1], y0 + plot_dots[i][0], 1, 1, po);
        }
        if (look->mountains)
            spr1(plot_spr[PP_MOUNT + look->mountains - 1], 2, 16, x0, y0, PEN_BLACK, 0x100);
    }
    mark(x0, y0, 16, 16);
    commit();
}

/* ---- the town ----------------------------------------------------------- */
/* 72 lines of 48 bytes from line 10, pixel 32, unpacked as it is drawn
 * (tools/mkgfx_dos.py rle). */
void gfx_town(void)
{
    const uint8_t *src = town_lx;
    uint8_t *d;
    uint8_t line = 0, x = 0, n, v = 0, lit = 0, run = 0;
    uint16_t left = 72u * 48;

    clear_to(pen_of[C_GREY], 1);
    d = BACK + lineoff[10] + 16;
    while (left) {
        if (!lit && !run) {
            n = *src++;
            if (n >= 128)
                run = n - 125, v = *src++;
            else
                lit = n + 1;
        }
        if (run) {
            --run;
        } else {
            --lit;
            v = *src++;
        }
        --left;
        *d++ = v;
        if (++x == 48) {
            x = 0;
            if (++line < 72)
                d = BACK + lineoff[10 + line] + 16;
        }
    }
    mark(32, 10, 96, 72);
    commit();
}

#endif
