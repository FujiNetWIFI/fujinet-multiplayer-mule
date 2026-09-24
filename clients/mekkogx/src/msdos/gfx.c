/*
 * gfx.c -- gfx.h on the PC: CGA 320 x 200 in 4 colours, or on the PCjr
 * 320 x 200 in 16 (BIOS mode 9).
 *
 * 320 pixels are exactly two a colour clock, so nothing is scaled: a text
 * cell is 8 pixels, as on the Atari, and the 192 lines sit 4 down.
 *
 *   CGA   4 pixels a byte (2 bits), 80 bytes a line; even lines at B800:0000,
 *         odd at B800:2000. A clock is half a byte.
 *   PCjr  2 pixels a byte (4 bits, RGBI), 160 bytes a line, four banks
 *         (line & 3) of 8K. A clock is a byte.
 *
 * CGA shows palette 1 (cyan, magenta, white) over colour 0, the background
 * register: black, or grey behind the map as on the Atari. The four players
 * are cyan, magenta, white and cyan/magenta stripes. Where the BIOS can set
 * palette registers (EGA, VGA, the PCjr) cyan and magenta become light blue
 * and light magenta, nearer the Atari's; a real CGA ignores that.
 *
 * On the PCjr the colours are the Atari's nearest RGBI; the map's grey is
 * drawn (colour 8), so its mountains can be black.
 *
 * Sprites are drawn over the screen with what was under them saved, as on
 * the CoCo (src/coco/gfx.c), whose slot logic this is.
 */
#include <string.h>
#include <i86.h>
#include "mule.h"
#include "gfxdata.h"

extern uint8_t plat_jr;         /* plat.c: the PCjr's 16-colour mode */
extern uint8_t plat_pal;        /* plat.c: 0 CGA, 1 EGA/VGA, 2 PCjr registers */

/* The screen. CGA's is at B800. The PCjr's 16-colour one is 32K of main
 * RAM (its CPU page, 16K units): its B800 window is only 16K wide, so the
 * upper two banks would wrap onto the lower two there. */
static uint8_t __far *vram_p;
#define VRAM vram_p
#define TOP 4                   /* the Atari's line 0 is the screen's 4 */

static uint16_t rowoff[192];
/* Four colour clocks (a nibble of a 1-bit picture, MSB left) as screen
 * byte masks: on CGA two bytes (one word), on the PCjr four (two words).
 * Words are little-endian: the left byte is the low one. */
static uint16_t cga_w[16];
static uint16_t jr_w[16][2];
static uint8_t bgpat;           /* what "black" is: the background */

/* Masks: two units (CGA clocks, PCjr pixels) -> a byte; four CGA pixels
 * -> a byte. */
static const uint8_t m2[4] = { 0x00, 0x0F, 0xF0, 0xFF };
static const uint8_t m4[16] = {
    0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
    0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF,
};

/* Logical colours: CGA pixel values (4 = stripes), PCjr RGBI. */
#define STRIPES 4
static const uint8_t cga_of[16] = {
    0,          /* C_BLACK */
    3,          /* C_WHITE */
    1,          /* C_BLUE: player 0 */
    2,          /* C_PURPLE: player 1 */
    3,          /* C_GREEN: player 2 */
    2,          /* C_ORANGE */
    2,          /* C_RED */
    3,          /* C_YELLOW */
    3,          /* C_GREY */
    1,          /* C_CYAN */
    2,          /* C_BROWN */
    2,          /* C_TAN */
    1,          /* C_LTBLUE */
    1,          /* C_DKGREEN */
    2,          /* C_PINK */
    STRIPES,    /* C_P3: player 3 */
};
static const uint8_t rgbi_of[16] = {
    0,          /* C_BLACK */
    15,         /* C_WHITE */
    9,          /* C_BLUE: player 0, light blue */
    13,         /* C_PURPLE: player 1, light magenta */
    2,          /* C_GREEN: player 2 */
    12,         /* C_ORANGE */
    4,          /* C_RED */
    14,         /* C_YELLOW */
    7,          /* C_GREY */
    11,         /* C_CYAN */
    6,          /* C_BROWN */
    6,          /* C_TAN */
    11,         /* C_LTBLUE */
    2,          /* C_DKGREEN */
    13,         /* C_PINK */
    12,         /* C_P3: player 3, orange (light red) */
};
#define JR_RIVER  10            /* the river: light green (the Atari's $F4) */
#define JR_PLAINS 8             /* the map's grey ($08) */

/* The byte pattern of colour col on Atari line y. */
static uint8_t pat(uint8_t col, uint8_t y)
{
    uint8_t v;
    if (col == C_BLACK)
        return bgpat;
    if (plat_jr)
        return rgbi_of[col] * 0x11;
    v = cga_of[col];
    if (v == STRIPES)
        return (y & 1) ? 0xAA : 0x55;
    return v * 0x55;
}

static void spr_bg(uint8_t c0, uint8_t n, uint8_t y, uint8_t h);
static void bg_done(void);

/* ---- the palette -------------------------------------------------------- */
/* An RGBI colour as a palette register value: the PCjr takes it as it is,
 * EGA/VGA in a 200-line mode as rgb + intensity in bit 4. */
static uint8_t preg(uint8_t c)
{
    return plat_pal == 2 ? c : (uint8_t)((c & 7) | ((c & 8) << 1));
}

static void set_preg(uint8_t reg, uint8_t c)
{
    union REGS r;
    r.w.ax = 0x1000;
    r.h.bl = reg;
    r.h.bh = preg(c);
    int86(0x10, &r, &r);
}

/* Colour 0 (CGA: the background register). */
static void set_bg(uint8_t c)
{
    union REGS r;
    if (plat_jr)
        return;                 /* 16 colours: the background is drawn */
    if (plat_pal) {
        set_preg(0, c);
        return;
    }
    r.h.ah = 0x0B;
    r.h.bh = 0;
    r.h.bl = 0x10 | c;          /* bit 4: the bright palette */
    int86(0x10, &r, &r);
}

void gfx_init(void)
{
    union REGS r;
    uint8_t y;
    uint16_t line;

    r.w.ax = plat_jr ? 0x0009 : 0x0004;
    int86(0x10, &r, &r);
    vram_p = MK_FP(0xB800, 0);
    if (plat_jr) {
        r.w.ax = 0x0580;        /* PCjr: read the CRT/CPU page registers */
        int86(0x10, &r, &r);
        vram_p = MK_FP((uint16_t)r.h.bl << 10, 0);   /* 16K pages */
    }
    for (y = 0; y < 16; ++y) {
        cga_w[y] = (uint16_t)m2[y >> 2] | (uint16_t)m2[y & 3] << 8;
        jr_w[y][0] = (y & 8 ? 0x00FF : 0) | (y & 4 ? 0xFF00 : 0);
        jr_w[y][1] = (y & 2 ? 0x00FF : 0) | (y & 1 ? 0xFF00 : 0);
    }
    for (y = 0; y < 192; ++y) {
        line = y + TOP;
        rowoff[y] = plat_jr ? (line & 3) * 0x2000u + (line >> 2) * 160u
                            : (line & 1) * 0x2000u + (line >> 1) * 80u;
    }
    if (!plat_jr) {
        r.h.ah = 0x0B;          /* palette 1: cyan, magenta, white */
        r.h.bh = 1;
        r.h.bl = 1;
        int86(0x10, &r, &r);
        set_bg(0);
        if (plat_pal) {         /* nearer the Atari's blue and purple */
            set_preg(1, 9);
            set_preg(2, 13);
            set_preg(3, 15);
        }
    }
}

static void clear_all(uint8_t v)
{
    _fmemset(VRAM, v, plat_jr ? 0x8000u : 0x4000u);
}

void gfx_clear(void)
{
    spr_hide_all();
    spr_flush();
    bgpat = 0;
    set_bg(0);
    clear_all(0);
}

/* ---- spans -------------------------------------------------------------- */
/* Pixels px0 .. px1 - 1 of Atari line y become pattern pt. */
static void span(uint8_t y, uint16_t px0, uint16_t px1, uint8_t pt)
{
    uint8_t __far *row;
    uint16_t b0, b1;
    uint8_t lm, rm, m;

    if (px1 > 320)
        px1 = 320;
    if (px0 >= px1 || y >= 192)
        return;
    row = VRAM + rowoff[y];
    if (plat_jr) {
        b0 = px0 >> 1;
        b1 = (px1 - 1) >> 1;
        lm = (px0 & 1) ? 0x0F : 0xFF;
        rm = ((px1 - 1) & 1) ? 0xFF : 0xF0;
    } else {
        b0 = px0 >> 2;
        b1 = (px1 - 1) >> 2;
        lm = 0xFF >> ((px0 & 3) * 2);
        rm = 0xFF << ((3 - ((px1 - 1) & 3)) * 2);
    }
    if (b0 == b1) {
        m = lm & rm;
        row[b0] = (row[b0] & ~m) | (pt & m);
        return;
    }
    row[b0] = (row[b0] & ~lm) | (pt & lm);
    if (b1 > b0 + 1)
        _fmemset(row + b0 + 1, pt, b1 - b0 - 1);
    row[b1] = (row[b1] & ~rm) | (pt & rm);
}

/* A word of pattern pt through mask m. */
#define PUTW(d, m, pt) (*(d) = (*(d) & ~(m)) | ((pt) & (m)))

/* One row of a 1-bit picture, a bit a colour clock (MSB left, wb bytes),
 * its left clock at x (either edge may be off the screen): lit clocks
 * become pattern pt, the rest are left alone. */
static void clk_row(uint8_t y, int16_t x, const uint8_t *src, uint8_t wb, uint8_t pt)
{
    uint8_t __far *row = VRAM + rowoff[y];
    uint16_t pw = pt * 0x101u, m;
    int16_t b;
    uint8_t i, k, v, sh, prev, cur;

    if (plat_jr) {              /* a clock is a byte */
        if (x >= 0 && x + wb * 8 <= 160) {
            uint16_t __far *d = (uint16_t __far *)(row + x);
            for (i = 0; i < wb; ++i, d += 4) {
                v = src[i];
                if (!v)
                    continue;
                if ((m = jr_w[v >> 4][0]) != 0) PUTW(d, m, pw);
                if ((m = jr_w[v >> 4][1]) != 0) PUTW(d + 1, m, pw);
                if ((m = jr_w[v & 15][0]) != 0) PUTW(d + 2, m, pw);
                if ((m = jr_w[v & 15][1]) != 0) PUTW(d + 3, m, pw);
            }
            return;
        }
        for (i = 0; i < wb; ++i, x += 8) {
            v = src[i];
            for (k = 0; v; ++k, v <<= 1)
                if ((v & 0x80) && (uint16_t)(x + k) < 160)
                    row[x + k] = pt;
        }
        return;
    }
    /* CGA: two clocks a byte. An odd x puts a clock of nothing in front:
     * the stream's byte i is then the source's bytes i-1, i shifted one
     * right, and one byte more comes out. */
    sh = (uint8_t)x & 1;
    prev = 0;
    b = (x - sh) / 2;           /* (even: exact however it rounds) */
    if (b >= 0 && b + (wb + sh) * 4 <= 80) {
        uint16_t __far *d = (uint16_t __far *)(row + b);
        for (i = 0; i <= wb; ++i, d += 2) {
            if (i == wb && !sh)
                break;
            cur = i < wb ? src[i] : 0;
            v = sh ? (uint8_t)((prev << 7) | (cur >> 1)) : cur;
            prev = cur;
            if (!v)
                continue;
            if ((m = cga_w[v >> 4]) != 0) PUTW(d, m, pw);
            if ((m = cga_w[v & 15]) != 0) PUTW(d + 1, m, pw);
        }
        return;
    }
    for (i = 0; i <= wb; ++i, b += 4) {
        if (i == wb && !sh)
            break;
        cur = i < wb ? src[i] : 0;
        v = (uint8_t)((((uint16_t)prev << 8) | cur) >> sh);
        prev = cur;
        for (k = 0; v; ++k, v <<= 2) {
            m = m2[v >> 6];
            if (m && (uint16_t)(b + k) < 80)
                row[b + k] = (row[b + k] & ~m) | (pt & m);
        }
    }
}

/* ---- text ------------------------------------------------------------- */
static const uint8_t *glyph_of(char c)
{
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c < 32 || c > 95)
        c = '?';
    return dos_font + ((uint8_t)(c - 32) << 3);
}

/* A glyph in cell tx, line y, opaque: lit pixels in col, the rest the
 * background. */
static void glyph(uint8_t tx, uint8_t y, char c, uint8_t col)
{
    const uint8_t *g = glyph_of(c);
    uint8_t __far *p;
    uint8_t r, v, pt;
    for (r = 0; r < 8; ++r) {
        v = g[r];
        pt = pat(col, y + r);
        p = VRAM + rowoff[y + r];
        if (plat_jr) {
            p += tx * 4;
            p[0] = (bgpat & ~m2[v >> 6]) | (pt & m2[v >> 6]);
            p[1] = (bgpat & ~m2[(v >> 4) & 3]) | (pt & m2[(v >> 4) & 3]);
            p[2] = (bgpat & ~m2[(v >> 2) & 3]) | (pt & m2[(v >> 2) & 3]);
            p[3] = (bgpat & ~m2[v & 3]) | (pt & m2[v & 3]);
        } else {
            p += tx * 2;
            p[0] = (bgpat & ~m4[v >> 4]) | (pt & m4[v >> 4]);
            p[1] = (bgpat & ~m4[v & 15]) | (pt & m4[v & 15]);
        }
    }
}

void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col)
{
    if (tx > 39 || ty > 23)
        return;
    glyph(tx, ty << 3, c, col);
}

void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    if (ty > 23)
        return;
    spr_bg(tx << 2, n << 2, ty << 3, 8);
    while (n-- && *s && tx < 40)
        glyph(tx++, ty << 3, *s++, col);
    bg_done();
}

void gfx_text(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_textn(tx, ty, s, 40, col);
}

void gfx_clear_text(uint8_t tx, uint8_t ty, uint8_t w)
{
    uint8_t r;
    if (tx >= 40 || ty > 23)
        return;
    if (tx + w > 40)
        w = 40 - tx;
    spr_bg(tx << 2, w << 2, ty << 3, 8);
    for (r = 0; r < 8; ++r)
        span((ty << 3) + r, tx * 8u, (tx + w) * 8u, bgpat);
    bg_done();
}

/* Big text: each pixel of the glyph a colour clock (16 pixels a character),
 * opaque. */
void gfx_bign(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    const uint8_t *g;
    uint8_t r, y = ty << 3;
    char c;
    tx &= 0xFE;
    if (ty > 23)
        return;
    spr_bg(tx << 2, n << 3, y, 8);
    while (n-- && (c = *s++) != 0 && tx < 40) {
        g = glyph_of(c);
        for (r = 0; r < 8; ++r) {
            span(y + r, tx * 8u, tx * 8u + 16, bgpat);
            clk_row(y + r, tx * 4, g + r, 1, pat(col, y + r));
        }
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
    uint8_t r;
    spr_bg(x, w, y, h);
    for (r = 0; r < h && y + r < 192; ++r)
        span(y + r, x * 2u, (x + w) * 2u, pat(col, y + r));
    bg_done();
}

void gfx_hdots(uint8_t x, uint8_t y, uint8_t w, uint8_t col)
{
    uint8_t pt = pat(col, y), c;
    spr_bg(x, w, y, 1);
    for (c = 0; c < w; c += 2)
        span(y, (x + c) * 2u, (x + c) * 2u + 2, pt);
    bg_done();
}

void gfx_pic(uint8_t x, uint8_t y, uint8_t pic, uint8_t col)
{
    uint8_t r, pt;
    spr_bg(x, 8, y, 16);
    /* the picture replaces what was there */
    for (r = 0; r < 16; ++r) {
        pt = pat(col, y + r);
        span(y + r, x * 2u, x * 2u + 16, bgpat);
        clk_row(y + r, x, &dos_pics[pic][r >> 1], 1, pt);
    }
    bg_done();
}

/* ---- the map ---------------------------------------------------------- */
void gfx_map_frame(void)
{
    gfx_clear();
    if (plat_jr) {
        bgpat = JR_PLAINS * 0x11;
        clear_all(bgpat);
    } else {
        set_bg(8);              /* dark grey */
    }
}

static const uint8_t zeros[64];
static uint8_t dots[64];
static uint16_t pl_r, pl_w, pl_bg;      /* the river's, white's, the plains' */

/* A word of a plot: white (mw) over the owner's (mo, in pattern po) over
 * the river (mr) over the plains. */
static void plot_word(uint16_t __far *d, uint16_t mw, uint16_t mo, uint16_t mr, uint8_t po)
{
    uint16_t t;
    if (!(mw | mo | mr)) {
        *d = pl_bg;
        return;
    }
    mo &= ~mw;
    t = mw | mo;
    mr &= ~t;
    t |= mr;
    *d = (pl_bg & ~t) | (pl_w & mw) | ((po * 0x101u) & mo) | (pl_r & mr);
}

/* A plot: 32 lines of 16 clocks, opaque. On each: white (mountains, the
 * store) over the owner's (border, icon, dots) over the river, over the
 * plains. */
uint16_t gfx_nplot;              /* (tools/dossim peeks it) */
void gfx_plot(uint8_t p, const PlotLook *look)
{
    const uint8_t *o1 = zeros, *o2 = zeros, *o3 = zeros, *wh = zeros, *rv = zeros;
    uint8_t i, r, k, x0, y0, y, po, ob, rb, wb;
    uint16_t __far *d;

    if (look->store) {
        wh = plot_parts[PP_STORE];
    } else {
        if (look->owned) {
            o1 = plot_parts[PP_BORDER];
            if (look->icon)
                o2 = plot_parts[PP_ICON + look->icon - 1];
            if (look->dots) {
                memset(dots, 0, 64);
                for (i = 0; i < look->dots && i < 8; ++i) {
                    r = plot_dots[i][0] * 2 + (plot_dots[i][1] >> 3);
                    dots[r] |= 0x80 >> (plot_dots[i][1] & 7);
                    dots[r + 2] |= 0x80 >> (plot_dots[i][1] & 7);
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
    x0 = PLOT_X(p);
    y0 = PLOT_Y(p);
    spr_bg(x0, 16, y0, 32);
    if (plat_jr) {
        pl_r = JR_RIVER * 0x1111u;
        pl_w = (look->mountains && !look->store) ? 0x0000 : 0xFFFF;   /* black mountains */
    } else {
        pl_r = 0x5555;          /* cyan */
        pl_w = 0xFFFF;
    }
    pl_bg = bgpat * 0x101u;
    for (r = 0; r < 32; ++r) {
        y = y0 + r;
        k = r * 2;
        po = pat(look->col, y);
        d = (uint16_t __far *)(VRAM + rowoff[y] + (plat_jr ? x0 : x0 >> 1));
        for (i = 0; i < 2; ++i, ++k) {
            ob = o1[k] | o2[k] | o3[k];
            rb = rv[k];
            wb = wh[k];
            if (plat_jr) {
                plot_word(d++, jr_w[wb >> 4][0], jr_w[ob >> 4][0], jr_w[rb >> 4][0], po);
                plot_word(d++, jr_w[wb >> 4][1], jr_w[ob >> 4][1], jr_w[rb >> 4][1], po);
                plot_word(d++, jr_w[wb & 15][0], jr_w[ob & 15][0], jr_w[rb & 15][0], po);
                plot_word(d++, jr_w[wb & 15][1], jr_w[ob & 15][1], jr_w[rb & 15][1], po);
            } else {
                plot_word(d++, cga_w[wb >> 4], cga_w[ob >> 4], cga_w[rb >> 4], po);
                plot_word(d++, cga_w[wb & 15], cga_w[ob & 15], cga_w[rb & 15], po);
            }
        }
    }
    bg_done();
}

/* ---- the town ----------------------------------------------------------- */
/* 144 lines from line 16, clocks 32..127, unpacked as it is drawn
 * (tools/mkgfx_dos.py rle): a byte a clock, RGBI in both nibbles, as the
 * PCjr shows it. CGA takes the clocks two at a time into its palette, the
 * corral's grey inside its edge black (white would swamp the screen). */
void gfx_town(void)
{
    const uint8_t __far *src = town_jr;
    uint8_t __far *d;
    uint8_t line = 0, x = 0, n, v = 0, lit = 0, run = 0, c, cga = 0;
    uint8_t cx0 = town_corral[0] + 1, cy0 = town_corral[1] + 1;
    uint8_t cx1 = town_corral[2] - 1, cy1 = town_corral[3] - 1;
    uint16_t left = 144u * 96;

    spr_bg(32, 96, 16, 144);
    d = VRAM + rowoff[16] + (plat_jr ? 32 : 16);
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
        if (plat_jr) {
            *d++ = v;
        } else {
            c = rgbi_cga[v & 15];
            if (v == 0x88 && x > cx0 && x < cx1 && line > cy0 && line < cy1)
                c = 0;
            c |= c << 2;        /* a clock: two pixels */
            if (x & 1)
                *d++ = cga | c;
            else
                cga = c << 4;
        }
        if (++x == 96) {
            x = 0;
            if (++line < 144)
                d = VRAM + rowoff[16 + line] + (plat_jr ? 32 : 16);
        }
    }
    bg_done();
}

/* ---- sprites ----------------------------------------------------------
 * Each slot keeps what the game asked for and, once drawn, the rectangle
 * it saved from under itself: screen bytes bx..bx+n-1 of lines
 * sy..sy+sh-1, and the same in colour clocks (s_c0, s_cn) for comparing
 * with the background drawing's rectangles (spr_bg).
 */
#define SAVE_MAX 544            /* the PCjr's plot cursor: 17 x 32 */
static uint8_t save[SPR_SLOTS][SAVE_MAX];

uint8_t s_on[SPR_SLOTS], s_shape[SPR_SLOTS], s_frame[SPR_SLOTS];
static uint8_t s_col[SPR_SLOTS], s_flags[SPR_SLOTS];
static int16_t s_x[SPR_SLOTS], s_y[SPR_SLOTS];
uint8_t s_dirty[SPR_SLOTS];     /* needs (re)drawing */
uint8_t s_drawn[SPR_SLOTS];     /* on the screen: the rectangle is valid */
static uint8_t s_bx[SPR_SLOTS], s_n[SPR_SLOTS], s_sy[SPR_SLOTS], s_sh[SPR_SLOTS];
static uint8_t s_c0[SPR_SLOTS], s_cn[SPR_SLOTS];
static uint8_t any_dirty;

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

static void rect(uint8_t i, uint8_t to_screen)
{
    uint8_t r, n = s_n[i], y = s_sy[i];
    uint8_t *s = save[i];
    for (r = s_sh[i]; r; --r, ++y, s += n) {
        uint8_t __far *p = VRAM + rowoff[y] + s_bx[i];
        if (to_screen)
            _fmemcpy(p, s, n);
        else
            _fmemcpy(s, p, n);
    }
}

static void erase(uint8_t i)
{
    rect(i, 1);
    s_drawn[i] = 0;
}

/* Does drawn sprite j's rectangle meet clocks c0.. (n of them), lines y..? */
static uint8_t meets(uint8_t j, uint8_t c0, uint8_t n, uint8_t y, uint8_t h)
{
    uint16_t a = s_c0[j], e = (uint16_t)c0 + n, ae = a + s_cn[j];
    if (a >= e || c0 >= ae)
        return 0;
    a = s_sy[j];
    e = (uint16_t)y + h;
    ae = a + s_sh[j];
    return a < e && y < ae;
}

/* Take off every drawn sprite in hit[], and any drawn after one of them
 * that overlaps it (their saved backgrounds hold its pixels); mark them to
 * be drawn again. */
static uint8_t hit[SPR_SLOTS];
static void take_off(void)
{
    uint8_t i, j;
    for (i = 0; i < SPR_SLOTS; ++i)
        if (hit[i] && s_drawn[i])
            for (j = i + 1; j < SPR_SLOTS; ++j)
                if (!hit[j] && s_drawn[j] && meets(j, s_c0[i], s_cn[i], s_sy[i], s_sh[i]))
                    hit[j] = 1;
    for (i = SPR_SLOTS; i-- > 0;)
        if (hit[i]) {
            if (s_drawn[i])
                erase(i);
            s_dirty[i] = 1;
            any_dirty = 1;
        }
}

/* Background drawing is about to change a rectangle (in colour clocks):
 * take the sprites off it; bg_done puts them back over the new background. */
static uint8_t bg_took;

static void spr_bg(uint8_t c0, uint8_t n, uint8_t y, uint8_t h)
{
    uint8_t i, any = 0;
    for (i = 0; i < SPR_SLOTS; ++i) {
        hit[i] = s_drawn[i] && meets(i, c0, n, y, h);
        any |= hit[i];
    }
    if (any) {
        take_off();
        bg_took = 1;
    }
}

static void bg_done(void)
{
    if (bg_took) {
        bg_took = 0;
        spr_flush();
    }
}

/* Where slot i would be drawn: its save rectangle, and the (clipped)
 * source. Returns 0 if nothing shows. */
static const uint8_t *src_of(uint8_t i, uint8_t *y, uint8_t *h, uint8_t *wb)
{
    const ShapeDef *d = &dos_shapes[s_shape[i]];
    uint8_t frame = s_frame[i];
    uint16_t sz = (uint16_t)d->wb * d->h;
    const uint8_t *src;
    int16_t yy = s_y[i], hh = d->h, left, right;

    if (s_flags[i] & SPR_FLIPX) {
        if (s_shape[i] == SH_MULE)
            frame += 4;
        else if (s_shape[i] == SH_FIGURE && (frame % 10) < 2)
            frame += 2;
    }
    src = d->data + frame * sz;
    if (yy < 0) {
        src += (uint8_t)(-yy) * d->wb;
        hh += yy;
        yy = 0;
    }
    if (yy + hh > 192)
        hh = 192 - yy;
    left = s_x[i];
    right = left + d->w - 1;
    if (hh <= 0 || right < 0 || left > 159)
        return 0;
    if (left < 0)
        left = 0;
    if (right > 159)
        right = 159;
    if (plat_jr) {
        s_bx[i] = (uint8_t)left;
        s_n[i] = (uint8_t)(right - left + 1);
    } else {
        s_bx[i] = (uint8_t)(left >> 1);
        s_n[i] = (uint8_t)(right >> 1) - s_bx[i] + 1;
    }
    s_sy[i] = (uint8_t)yy;
    s_sh[i] = (uint8_t)hh;
    if ((uint16_t)s_n[i] * s_sh[i] > SAVE_MAX)
        s_sh[i] = SAVE_MAX / s_n[i];
    s_c0[i] = (uint8_t)left;
    s_cn[i] = (uint8_t)(right - left + 1);
    *y = (uint8_t)yy;
    *h = s_sh[i];
    *wb = d->wb;
    return src;
}

void spr_flush(void)
{
    uint8_t i, j, y, h, wb, r;
    const uint8_t *src;

    if (!any_dirty)
        return;
    any_dirty = 0;
    /* the changing sprites come off, with anything drawn after them over them */
    for (i = 0; i < SPR_SLOTS; ++i)
        hit[i] = s_dirty[i];
    take_off();
    any_dirty = 0;
    for (i = 0; i < SPR_SLOTS; ++i) {
        if (!s_dirty[i])
            continue;
        s_dirty[i] = 0;
        if (!s_on[i])
            continue;
        src = src_of(i, &y, &h, &wb);
        if (!src)
            continue;
        /* anything drawn later that is still up over the new place comes
         * off first (it is drawn again after this one) */
        for (j = i + 1; j < SPR_SLOTS; ++j)
            hit[j] = s_drawn[j] && meets(j, s_c0[i], s_cn[i], s_sy[i], s_sh[i]);
        for (j = 0; j <= i; ++j)
            hit[j] = 0;
        take_off();
        rect(i, 0);
        s_drawn[i] = 1;
        for (r = 0; r < h; ++r, src += wb)
            clk_row(y + r, s_x[i], src, wb, pat(s_col[i], y + r));
    }
    any_dirty = 0;
    for (i = 0; i < SPR_SLOTS; ++i)
        any_dirty |= s_dirty[i];
}
