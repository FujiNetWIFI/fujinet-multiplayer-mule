/*
 * gfx.c -- gfx.h on the TMS9918A (the Adam, the MSX): Graphics II, a
 * 256 x 192 bitmap in which each 8-pixel row of a cell has two colours.
 *
 * x is scaled 3/2 from the Atari's colour clocks, 4 pixels in: 160 clocks
 * are pixels 4..243, and a plot (16 clocks from clock 8 + 16k) is exactly
 * the three cells from pixel 16 + 24k. A text cell is 6 pixels (40 across
 * in 240); lines are the Atari's own.
 *
 * Drawing reads the 8 pattern and 8 colour bytes of each cell it touches,
 * paints them (paint(): a colour the cell hasn't got takes the place of
 * the one with fewer pixels left, which then show in the new colour) and
 * writes them back. Plots and the town are cell-aligned and made whole.
 *
 * The sprites are the VDP's (16 x 16, one colour each): a slot is up to
 * four of them side by side and one above the other, with its own four
 * pattern names; a pattern is sent when its frame changes. With five on a
 * line the VDP drops the fifth: the order is rotated a frame at a time so
 * that they flicker instead.
 */
#include <string.h>
#include "mule.h"
#include "gfxdata.h"
#include "vdp.h"

#define PX(x) ((uint16_t)(x) + ((uint8_t)(x) >> 1) + 4)

/* Logical colours -> TMS9918 colours (1 black, 2 medium green, 3 light
 * green, 5 light blue, 6 dark red, 7 cyan, 8 medium red, 9 light red,
 * 10 dark yellow, 11 light yellow, 12 dark green, 13 magenta, 14 grey,
 * 15 white). */
static const uint8_t tms_of[16] = {
    1,          /* C_BLACK */
    15,         /* C_WHITE */
    5,          /* C_BLUE: player 0 */
    13,         /* C_PURPLE: player 1 */
    2,          /* C_GREEN: player 2 */
    9,          /* C_ORANGE */
    8,          /* C_RED */
    11,         /* C_YELLOW */
    14,         /* C_GREY */
    7,          /* C_CYAN */
    6,          /* C_BROWN */
    10,         /* C_TAN */
    5,          /* C_LTBLUE */
    12,         /* C_DKGREEN */
    9,          /* C_PINK */
    9,          /* C_P3: player 3, orange */
};
#define T_BLACK  1
#define T_WHITE  15
#define T_PLAINS 14             /* the map's grey */
#define T_RIVER  3              /* light green (the Atari's $F4) */

static uint8_t bgc = T_BLACK;   /* what "black" is: the background */
uint8_t vdp_seen;

static uint8_t tcol(uint8_t c)
{
    return c == C_BLACK ? bgc : tms_of[c & 15];
}

/* ---- VRAM --------------------------------------------------------------- */
static void vfill(uint16_t a, uint8_t v, uint16_t n)
{
    uint8_t k;
    while (n) {                 /* a page at a time: the MSX's interrupt */
        k = n > 255 ? 255 : (uint8_t)n;     /* gets a look in between */
        n -= k;
        VDP_LOCK();
        VDP_WADDR(a);
        do {
            VDP_DATA = v;
        } while (--k);
        VDP_UNLOCK();
        a += 255;
    }
}

static void vwrite(uint16_t a, const uint8_t *s, uint8_t n)
{
    VDP_LOCK();
    VDP_WADDR(a);
    do {
        VDP_DATA = *s++;
    } while (--n);
    VDP_UNLOCK();
}

/* One cell: its patterns and colours while it is painted. */
static uint8_t cp[8], cc[8];
static uint16_t ca;

static void cell_at(uint8_t cx, uint8_t cy)
{
    ca = ((uint16_t)cy << 8) | ((uint16_t)cx << 3);
}

static void cell_load(void)
{
    uint8_t i;
    VDP_LOCK();
    VDP_RADDR(ca);
    for (i = 0; i < 8; ++i)
        cp[i] = VDP_DATA;
    VDP_RADDR(ca + VR_COL);
    for (i = 0; i < 8; ++i)
        cc[i] = VDP_DATA;
    VDP_UNLOCK();
}

static void cell_store(void)
{
    vwrite(ca, cp, 8);
    vwrite(ca + VR_COL, cc, 8);
}

static uint8_t nbits(uint8_t v)
{
    uint8_t n = 0;
    while (v) {
        v &= v - 1;
        ++n;
    }
    return n;
}

/* Line r of the cell: the pixels in m become colour c. */
static void paint(uint8_t r, uint8_t m, uint8_t c)
{
    uint8_t p = cp[r], f = cc[r] >> 4, b = cc[r] & 15, rest;
    if (!m)
        return;
    if (m == 0xFF) {
        cp[r] = 0;
        cc[r] = (uint8_t)(c << 4) | c;
        return;
    }
    if (f == b)
        p = 0;                  /* one colour: it is all background */
    if (c == f && f != b) {
        cp[r] = p | m;
        return;
    }
    if (c == b) {
        cp[r] = p & ~m;
        return;
    }
    rest = ~m;
    if (nbits(p & rest) <= nbits(~p & rest)) {
        cc[r] = (uint8_t)(c << 4) | b;      /* the foreground becomes c */
        cp[r] = p | m;
    } else {
        cc[r] = (uint8_t)(f << 4) | c;      /* the background becomes c */
        cp[r] = p & ~m;
    }
}

/* ---- painting rectangles ------------------------------------------------ */
/* A line picture for op(): up to 8 lines of the screen's 256 pixels. */
static uint8_t tb[8][32];
static uint8_t op_col, op_bg;   /* op_bg 0: leave the rest */
static uint8_t op_pic;          /* 1: paint tb's pixels, else all */

/* Pixels px0 .. px1 - 1 of lines y .. y + h - 1 (h <= 8 with a picture,
 * whose line 0 is line y). */
static void op(uint16_t px0, uint16_t px1, uint8_t y, uint8_t h)
{
    uint8_t cx, cx0, cx1, cy, cy1, r, r0, r1, m, lm, rm, ye;
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
        for (cx = cx0; cx <= cx1; ++cx) {
            m = 0xFF;
            if (cx == cx0)
                m &= lm;
            if (cx == cx1)
                m &= rm;
            cell_at(cx, cy);
            if (!op_pic && m == 0xFF && r0 == 0 && r1 == 8) {
                memset(cp, 0, 8);   /* all of it: no need to look */
                memset(cc, (uint8_t)(op_col << 4) | op_col, 8);
            } else {
                cell_load();
                for (r = r0; r < r1; ++r) {
                    if (!op_pic) {
                        paint(r, m, op_col);
                        continue;
                    }
                    if (op_bg)
                        paint(r, m, op_bg);
                    paint(r, m & tb[(cy << 3) + r - y][cx], op_col);
                }
            }
            cell_store();
        }
    }
}

static void fill_px(uint16_t px0, uint16_t px1, uint8_t y, uint8_t h, uint8_t col)
{
    op_col = col;
    op_pic = 0;
    op(px0, px1, y, h);
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

/* ---- set-up ------------------------------------------------------------- */
void spr_hide_all(void);
static void spr_init(void);

void gfx_init(void)
{
    uint16_t i;
    VDP_LOCK();
    VDP_REG(0, 0x02);           /* Graphics II */
    VDP_REG(1, 0x82 | VDP_R1_IE);   /* 16K, blank, 16 x 16 sprites */
    VDP_REG(2, VR_NAME >> 10);
    VDP_REG(3, 0xFF);           /* colours at 2000h, all three thirds */
    VDP_REG(4, 0x03);           /* patterns at 0000h, all three thirds */
    VDP_REG(5, VR_SAT >> 7);
    VDP_REG(6, VR_SPAT >> 11);
    VDP_REG(7, T_BLACK);        /* the border */
    VDP_WADDR(VR_NAME);
    for (i = 0; i < 768; ++i)
        VDP_DATA = (uint8_t)i;
    VDP_WADDR(VR_SAT);
    VDP_DATA = 0xD0;            /* no sprites */
    VDP_UNLOCK();
    vfill(VR_PAT, 0, 0x1800);
    vfill(VR_COL, (T_WHITE << 4) | T_BLACK, 0x1800);
    VDP_LOCK();
    VDP_REG(1, 0xC2 | VDP_R1_IE);   /* on */
    VDP_UNLOCK();
    spr_init();
}

static void clear_all(void)
{
    vfill(VR_PAT, 0, 0x1800);
    vfill(VR_COL, (T_WHITE << 4) | bgc, 0x1800);
}

void gfx_clear(void)
{
    spr_hide_all();
    spr_flush();
    bgc = T_BLACK;
    clear_all();
}

void gfx_map_frame(void)
{
    spr_hide_all();
    spr_flush();
    bgc = T_PLAINS;
    clear_all();
}

/* ---- text --------------------------------------------------------------- */
static char tbuf[41];

/* s, at most n characters, cut at column 40 from tx (step columns each):
 * into tbuf (it may be in the overlay window, which ART_BEGIN changes). */
static uint8_t take(const char *s, uint8_t n, uint8_t tx, uint8_t step)
{
    uint8_t k = 0;
    while (k < n && s[k] && tx < 40) {
        tbuf[k] = s[k];
        ++k;
        tx += step;
    }
    return k;
}

static const uint8_t *glyph_of(char c)
{
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c < 32 || c > 95)
        c = '?';
    return tms_font + ((uint8_t)(c - 32) << 3);
}

/* tbuf's k characters as small (6-pixel) or big (12-pixel) glyphs from
 * pixel px0, into tb. */
static void tb_glyphs(uint16_t px0, uint8_t k, uint8_t big)
{
    const uint8_t *g;
    uint8_t i, r;
    memset(tb, 0, sizeof(tb));
    ART_BEGIN();
    for (i = 0; i < k; ++i) {
        g = glyph_of(tbuf[i]);
        for (r = 0; r < 8; ++r) {
            if (big)
                tb_put(r, px0, wide12[g[r]] << 4);
            else
                tb_put(r, px0, (uint16_t)g[r] << 8);
        }
        px0 += big ? 12 : 6;
    }
    ART_END();
}

static void text_op(uint16_t px0, uint16_t px1, uint8_t y, uint8_t col)
{
    op_col = tcol(col);
    op_bg = bgc;
    op_pic = 1;
    op(px0, px1, y, 8);
}

void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    uint8_t k;
    uint16_t px0 = tx * 6 + 4;
    if (ty > 23 || tx > 39)
        return;
    k = take(s, n, tx, 1);
    if (!k)
        return;
    tb_glyphs(px0, k, 0);
    text_op(px0, px0 + k * 6, ty << 3, col);
}

void gfx_text(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_textn(tx, ty, s, 40, col);
}

void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col)
{
    char s[2];
    s[0] = c;
    s[1] = 0;
    gfx_textn(tx, ty, s, 1, col);
}

void gfx_clear_text(uint8_t tx, uint8_t ty, uint8_t w)
{
    if (tx >= 40 || ty > 23)
        return;
    if (tx + w > 40)
        w = 40 - tx;
    fill_px(tx * 6 + 4, (tx + w) * 6 + 4, ty << 3, 8, bgc);
}

void gfx_bign(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    uint8_t k;
    uint16_t px0;
    tx &= 0xFE;
    if (ty > 23 || tx > 39)
        return;
    px0 = tx * 6 + 4;
    k = take(s, n, tx, 2);
    if (!k)
        return;
    tb_glyphs(px0, k, 1);
    text_op(px0, px0 + k * 12, ty << 3, col);
}

void gfx_big(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_bign(tx, ty, s, 20, col);
}

/* ---- rectangles --------------------------------------------------------- */
void gfx_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
    fill_px(PX(x), PX(x + w), y, h, tcol(col));
}

void gfx_hdots(uint8_t x, uint8_t y, uint8_t w, uint8_t col)
{
    uint8_t c;
    memset(tb[0], 0, 32);
    for (c = 0; c < w; c += 2)
        tb_put(0, PX(x + c), PX(x + c + 1) - PX(x + c) > 1 ? 0xC000 : 0x8000);
    op_col = tcol(col);
    op_bg = 0;
    op_pic = 1;
    op(PX(x), PX(x + w), y, 1);
}

void gfx_pic(uint8_t x, uint8_t y, uint8_t pic, uint8_t col)
{
    uint8_t half, r;
    uint16_t px0 = PX(x);
    for (half = 0; half < 2; ++half) {
        memset(tb, 0, sizeof(tb));
        ART_BEGIN();
        for (r = 0; r < 8; ++r)
            tb_put(r, px0, wide12[tms_pics[pic][half * 4 + (r >> 1)]] << 4);
        ART_END();
        op_col = tcol(col);
        op_bg = bgc;
        op_pic = 1;
        op(px0, px0 + 12, y + half * 8, 8);
    }
}

/* ---- the map ------------------------------------------------------------ */
static const uint8_t zeros[96];
static uint8_t dots[96];
uint16_t gfx_nplot;             /* (the harnesses peek it) */

/* A plot: three cells by four, made whole. Each cell line shows the first
 * of the owner's (border, icon, dots), white or black (mountains, the
 * store) and the river that it has, over the plains or over the next one
 * of them, whichever has more pixels there. */
void gfx_plot(uint8_t p, const PlotLook *look)
{
    const uint8_t *o1 = zeros, *o2 = zeros, *o3 = zeros, *wh = zeros, *rv = zeros;
    uint8_t lm[3], lc[3];
    uint8_t i, j, t, k, cx, cy, cx0, cy0, l, mask, fg, bg, best, n;
    uint8_t co = tms_of[look->col & 15];

    ART_BEGIN();
    if (look->store) {
        wh = plot_parts[PP_STORE];
    } else {
        if (look->owned) {
            o1 = plot_parts[PP_BORDER];
            if (look->icon)
                o2 = plot_parts[PP_ICON + look->icon - 1];
            if (look->dots) {
                memset(dots, 0, sizeof(dots));
                for (i = 0; i < look->dots && i < 8; ++i) {
                    k = plot_dots[i][0] * 3 + (plot_dots[i][1] >> 3);
                    t = 0xC0 >> (plot_dots[i][1] & 7);
                    dots[k] |= t;
                    dots[k + 3] |= t;
                }
                o3 = dots;
            }
        }
        if (look->mountains)
            wh = plot_parts[PP_MOUNT + look->mountains - 1];
        if (look->river)
            rv = plot_parts[PP_RIVER];
    }
    ++gfx_nplot;
    lc[0] = co;
    lc[1] = (look->mountains && !look->store) ? T_BLACK : T_WHITE;
    lc[2] = T_RIVER;
    cx0 = 2 + 3 * (p % 9);
    cy0 = 2 + 4 * (p / 9);
    for (cy = 0; cy < 4; ++cy) {
        for (cx = 0; cx < 3; ++cx) {
            for (l = 0; l < 8; ++l) {
                k = (uint8_t)((cy * 8 + l) * 3 + cx);
                lm[0] = o1[k] | o2[k] | o3[k];
                lm[1] = wh[k] & ~lm[0];
                lm[2] = rv[k] & ~(lm[0] | lm[1]);
                for (t = 0; t < 3 && !lm[t]; ++t)
                    ;
                if (t == 3) {
                    cp[l] = 0;
                    cc[l] = (uint8_t)(bgc << 4) | bgc;
                    continue;
                }
                mask = lm[t];
                fg = lc[t];
                bg = bgc;
                best = nbits(~(lm[0] | lm[1] | lm[2]));
                for (j = t + 1; j < 3; ++j) {
                    n = nbits(lm[j]);
                    if (n > best) {
                        best = n;
                        bg = lc[j];
                    }
                }
                cp[l] = mask;
                cc[l] = (uint8_t)(fg << 4) | bg;
            }
            cell_at(cx0 + cx, cy0 + cy);
            cell_store();
        }
    }
    ART_END();
}

/* ---- the town ----------------------------------------------------------- */
/* town_rle: n < 128, n + 1 bytes follow; n >= 128, the next byte n - 125
 * times. Unpacked a half cell row (152 bytes) at a time. */
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
    static uint8_t buf[TOWN_CW * 8];
    uint8_t cy;
    uint16_t a;
    rl_src = town_rle;
    rl_lit = rl_run = 0;
    for (cy = 0; cy < TOWN_CH; ++cy) {
        a = ((uint16_t)(TOWN_CY + cy) << 8) | (TOWN_CX << 3);
        ART_BEGIN();
        unpack(buf, sizeof(buf));
        ART_END();
        vwrite(a, buf, sizeof(buf));
        ART_BEGIN();
        unpack(buf, sizeof(buf));
        ART_END();
        vwrite(a + VR_COL, buf, sizeof(buf));
    }
}

/* ---- sprites ------------------------------------------------------------ */
#define PARTS 4                 /* VDP sprites (and pattern names) a slot */

static uint8_t s_on[SPR_SLOTS], s_shape[SPR_SLOTS], s_frame[SPR_SLOTS];
static uint8_t s_col[SPR_SLOTS], s_flags[SPR_SLOTS];
static int16_t s_x[SPR_SLOTS], s_y[SPR_SLOTS];
static uint8_t s_sent_shape[SPR_SLOTS], s_sent_frame[SPR_SLOTS];
static uint8_t any_dirty, rot;

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

/* Send slot i's frame f as its patterns: part = column + 2 * row... in
 * order along the rows of VDP sprites. */
static void send(uint8_t i, const ShapeDef *d, uint8_t f, uint8_t cols, uint8_t rows)
{
    static uint8_t pat[32];
    uint8_t line[6];
    const uint8_t *src;
    uint8_t part, gc, gr, l, row, b;
    uint16_t v;

    ART_BEGIN();
    src = d->data + (uint16_t)f * d->wb * d->h;
    part = 0;
    for (gr = 0; gr < rows; ++gr) {
        for (gc = 0; gc < cols; ++gc, ++part) {
            for (l = 0; l < 16; ++l) {
                row = (gr << 4) + l;
                pat[l] = pat[16 + l] = 0;
                if (row >= d->h)
                    continue;
                memset(line, 0, sizeof(line));
                for (b = 0; b < d->wb && b < 4; ++b) {
                    /* 8 clocks -> 12 pixels, from pixel 12b */
                    v = wide12[src[row * d->wb + b]];
                    if (b & 1) {
                        line[(b >> 1) * 3 + 1] |= (uint8_t)(v >> 8);
                        line[(b >> 1) * 3 + 2] = (uint8_t)v;
                    } else {
                        line[(b >> 1) * 3] = (uint8_t)(v >> 4);
                        line[(b >> 1) * 3 + 1] = (uint8_t)(v << 4);
                    }
                }
                pat[l] = line[gc * 2];
                pat[16 + l] = line[gc * 2 + 1];
            }
            vwrite(VR_SPAT + (uint16_t)(i * PARTS + part) * 32, pat, 32);
        }
    }
    ART_END();
}

void spr_flush(void)
{
    static uint8_t sat[SPR_SLOTS * PARTS * 4 + 1];
    const ShapeDef *d;
    uint8_t i, k, f, cols, rows, gc, gr, part, n, ec, shape, h;
    int16_t x, y;

    if (!any_dirty && !(vdp_seen & 0x40))
        return;
    if (vdp_seen & 0x40)            /* a sprite was dropped: take turns */
        rot = rot + 1 < SPR_SLOTS ? rot + 1 : 0;
    vdp_seen = 0;
    any_dirty = 0;
    n = 0;
    for (k = 0; k < SPR_SLOTS; ++k) {
        i = k + rot;
        if (i >= SPR_SLOTS)
            i -= SPR_SLOTS;
        if (!s_on[i])
            continue;
        shape = s_shape[i];
        ART_BEGIN();
        d = &tms_shapes[shape];
        h = d->h;
        cols = (uint8_t)(((d->w * 3 + 1) / 2 + 15) >> 4);
        ART_END();
        rows = (h + 15) >> 4;
        if (cols * rows > PARTS)
            rows = PARTS / cols;
        f = frame_of(i);
        if (s_sent_shape[i] != shape || s_sent_frame[i] != f) {
            send(i, &tms_shapes[shape], f, cols, rows);
            s_sent_shape[i] = shape;
            s_sent_frame[i] = f;
        }
        part = 0;
        for (gr = 0; gr < rows; ++gr) {
            for (gc = 0; gc < cols; ++gc, ++part) {
                y = s_y[i] + (gr << 4);
                x = s_x[i];
                x = x + (x >> 1) + 4 + (gc << 4);   /* PX, signed */
                if (y <= -16 || y >= 192 || x <= -16 || x >= 256 || n >= 32)
                    continue;
                ec = 0;
                if (x < 0) {
                    x += 32;            /* the early clock: 32 to the left */
                    ec = 0x80;
                }
                sat[n * 4] = (uint8_t)(y - 1);
                sat[n * 4 + 1] = (uint8_t)x;
                sat[n * 4 + 2] = (uint8_t)((i * PARTS + part) << 2);
                sat[n * 4 + 3] = ec | tms_of[s_col[i] & 15];
                ++n;
            }
        }
    }
    if (n < 32)
        sat[n * 4] = 0xD0;          /* the end of the list */
    vwrite(VR_SAT, sat, n < 32 ? n * 4 + 1 : 128);
}
