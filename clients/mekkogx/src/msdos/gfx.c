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

#include "machine.h"

/* The screen. CGA's is at B800. The PCjr's 16-colour one is 32K of main
 * RAM (its CPU page, 16K units): its B800 window is only 16K wide, so the
 * upper two banks would wrap onto the lower two there. */
uint8_t __far *vram_p;          /* (gfxa.asm too) */
#define VRAM vram_p
#define TOP 4                   /* the Atari's line 0 is the screen's 4 */

uint16_t rowoff[192];           /* each line's first byte (gfxa.asm too) */
/* Four colour clocks (a nibble of a 1-bit picture, MSB left) as screen
 * byte masks: on CGA two bytes (one word), on the PCjr four (two words).
 * Words are little-endian: the left byte is the low one. cga_w is also
 * four PCjr pixels. Made by gfx_init (gfxinit.c). */
uint16_t cga_w[16];
uint16_t jr_w[16][2];
static uint8_t bgpat;           /* what "black" is: the background */

#ifndef PLAT_JRONLY             /* (MULEJR.EXE: the PCjr only, machine.h) */
/* Masks: four CGA pixels -> a byte. */
const uint8_t m4[16] = {
    0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
    0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF,
};

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
#endif

/* Logical colours: PCjr RGBI (and CGA pixel values above, 4 = stripes). */
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
#define JR_RIVER  1             /* the river: blue (player 0 is light blue) */
#define JR_PLAINS 8             /* the map's grey ($08) */

/* The byte pattern of colour col on Atari line y. */
static uint8_t pat(uint8_t col, uint8_t y)
{
    if (col == C_BLACK)
        return bgpat;
#ifndef PLAT_JRONLY
    if (!plat_jr) {
        uint8_t v = cga_of[col];
        if (v == STRIPES)
            return (y & 1) ? 0xAA : 0x55;
        return v * 0x55;
    }
#endif
    return rgbi_of[col] * 0x11;
}

static void spr_bg(uint8_t c0, uint8_t n, uint8_t y, uint8_t h);
static void bg_done(void);
static uint8_t spr_under(uint8_t y);

/* ---- the palette -------------------------------------------------------- */
/* An RGBI colour as a palette register value: the PCjr takes it as it is,
 * EGA/VGA in a 200-line mode as rgb + intensity in bit 4. */
static uint8_t preg(uint8_t c)
{
    return plat_pal == 2 ? c : (uint8_t)((c & 7) | ((c & 8) << 1));
}

void set_preg(uint8_t reg, uint8_t c)
{
    union REGS r;
    r.w.ax = 0x1000;
    r.h.bl = reg;
    r.h.bh = preg(c);
    int86(0x10, &r, &r);
}

/* Colour 0 (CGA: the background register). */
void set_bg(uint8_t c)
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

/* ---- gfxa.asm's drawing ------------------------------------------------- */
/* gx_cells: gx_n cells of a 1-bit picture, opaque, from byte gx_off of
 * line gx_y on, gx_rows lines (16: each picture row twice). Cell i's rows
 * are at gx_gb + 8 * gx_gc[i]; gx_kind 0 draws a bit a pixel (text), 1 a
 * colour clock (big text, pictures). Lit pixels take gx_fg0 (even lines)
 * or gx_fg1 (odd), the rest gx_bgp. */
uint8_t gx_gc[40];
uint16_t gx_gb, gx_off;
uint8_t gx_n, gx_rows, gx_y, gx_fg0, gx_fg1, gx_bgp, gx_kind;
void __near gx_cells(void);
#pragma aux gx_cells "*_" parm [] modify [ax bx cx dx si di es]

/* gx_rect: gx_rows lines from gx_y on, from byte gx_off: through mask
 * gx_lm (0: none), gx_mid whole bytes, through gx_rm (0: none), in gx_fg0
 * or gx_fg1. */
uint8_t gx_mid, gx_lm, gx_rm;
void __near gx_rect(void);
#pragma aux gx_rect "*_" parm [] modify [ax bx cx dx si di es]

static void fg(uint8_t col)
{
    gx_fg0 = pat(col, 0);
    gx_fg1 = pat(col, 1);
}

/* Pixels px0 .. px1 - 1 of lines y .. y + h - 1 in col. */
static void rect(uint16_t px0, uint16_t px1, uint8_t y, uint8_t h, uint8_t col)
{
    uint16_t b0, b1;
    uint8_t lm, rm, mid;

    if (px1 > 320)
        px1 = 320;
    if (px0 >= px1 || y >= 192 || !h)
        return;
    if ((uint16_t)y + h > 192)
        h = 192 - y;
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
    mid = 0;
    if (b0 == b1) {
        lm &= rm;
        rm = 0;
    } else {
        mid = (uint8_t)(b1 - b0 - 1);
        if (rm == 0xFF) {       /* whole bytes go in the middle */
            rm = 0;
            ++mid;
        }
    }
    if (lm == 0xFF) {
        lm = 0;
        ++mid;
    }
    gx_off = b0;
    gx_y = y;
    gx_rows = h;
    gx_lm = lm;
    gx_rm = rm;
    gx_mid = mid;
    fg(col);
    gx_rect();
}

/* ---- text ------------------------------------------------------------- */
static uint8_t glyph_ix(char c)
{
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c < 32 || c > 95)
        c = '?';
    return (uint8_t)(c - 32);
}

/* gx_cells over gx_n cells: column tx (big: two a cell), line y. */
static void cells(uint8_t kind, uint8_t tx, uint8_t y, uint8_t col)
{
    gx_kind = kind;
    gx_gb = (uint16_t)dos_font;
    gx_off = plat_jr ? tx * 4 : tx * 2;
    gx_y = y;
    gx_rows = 8;
    gx_bgp = bgpat;
    fg(col);
    gx_cells();
}

void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col)
{
    if (tx > 39 || ty > 23)
        return;
    gx_gc[0] = glyph_ix(c);
    gx_n = 1;
    cells(0, tx, ty << 3, col);
}

void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    uint8_t k = 0;
    if (ty > 23)
        return;
    spr_bg(tx << 2, n << 2, ty << 3, 8);
    while (n-- && *s && tx + k < 40)
        gx_gc[k++] = glyph_ix(*s++);
    gx_n = k;
    cells(0, tx, ty << 3, col);
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
    spr_bg(tx << 2, w << 2, ty << 3, 8);
    rect(tx * 8u, (tx + w) * 8u, ty << 3, 8, C_BLACK);
    bg_done();
}

/* Big text: each pixel of the glyph a colour clock (16 pixels a character),
 * opaque. */
void gfx_bign(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    uint8_t k = 0;
    char c;
    tx &= 0xFE;
    if (ty > 23)
        return;
    spr_bg(tx << 2, n << 3, ty << 3, 8);
    while (n-- && (c = *s++) != 0 && tx + 2 * k < 40)
        gx_gc[k++] = glyph_ix(c);
    gx_n = k;
    cells(1, tx, ty << 3, col);
    bg_done();
}

void gfx_big(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_bign(tx, ty, s, 20, col);
}

/* ---- rectangles ------------------------------------------------------- */
void gfx_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
    spr_bg(x, w, y, h);
    rect(x * 2u, (x + w) * 2u, y, h, col);
    bg_done();
}

/* A dot every other colour clock (gfxa.asm's gx_hdots: gx_n of them from
 * byte gx_off of line gx_y, in gx_fg0): on the PCjr every other byte, on
 * CGA the same half (gx_lm) of each byte. gx_hdclip draws the line hd_n
 * dots from byte hd_off on only where it falls in bytes a .. b - 1.
 *
 * Between the sprites on its line it is drawn at once; under them it
 * waits for the next flush (hq_*), which draws it there between their old
 * rows going back and their new ones (gx_sprlines): the auction's price
 * lines step with the players, and a slow PC planned the sprites again
 * for each of the four lines. */
void __near gx_hdots(void);
#pragma aux gx_hdots "*_" parm [] modify [ax bx cx dx di es]
uint16_t hd_off;
uint8_t hd_n;
void __near gx_hdclip(uint8_t a, uint8_t b);
#pragma aux gx_hdclip "*_" parm [ax] [dx] modify [ax bx cx dx di es]

#define HQ_MAX 6
uint8_t hq_n, hq_lo = 255, hq_hi;       /* (gfxa.asm too) */
uint8_t hq_y[HQ_MAX], hq_c[HQ_MAX], hq_fg[HQ_MAX], hq_lm[HQ_MAX];
uint16_t hq_off[HQ_MAX];

void gfx_hdots(uint8_t x, uint8_t y, uint8_t w, uint8_t col)
{
    uint8_t n, k;
    if (y >= 192 || x >= 160)
        return;
    if (hq_n == HQ_MAX)
        spr_flush();
    n = (w + 1) >> 1;
    if (n > (uint8_t)((161 - x) >> 1))
        n = (161 - x) >> 1;
    gx_y = y;
    hd_n = n;
    gx_fg0 = pat(col, y);
    hd_off = plat_jr ? x : x >> 1;
    gx_lm = (x & 1) ? 0x0F : 0xF0;
    if (spr_under(y)) {
        k = hq_n++;
        hq_y[k] = y;
        hq_c[k] = n;
        hq_fg[k] = gx_fg0;
        hq_lm[k] = gx_lm;
        hq_off[k] = hd_off;
        if (y < hq_lo)
            hq_lo = y;
        if (y > hq_hi)
            hq_hi = y;
    }
}

void gfx_pic(uint8_t x, uint8_t y, uint8_t pic, uint8_t col)
{
    spr_bg(x, 8, y, 16);
    /* the picture replaces what was there: a cell of 8 rows, each twice */
    gx_kind = 1;
    gx_gc[0] = 0;
    gx_n = 1;
    gx_gb = (uint16_t)dos_pics[pic];
    gx_off = plat_jr ? x : x >> 1;
    gx_y = y;
    gx_rows = 16;
    gx_bgp = bgpat;
    fg(col);
    gx_cells();
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

/* gx_plot: a plot's 32 lines from line pl_y0, byte pl_x on, opaque. Its
 * layers are 64 bytes, 2 a line (16 clocks): white (pl_wh, in pl_w) over
 * the owner's (pl_o1 | pl_o2 | pl_o3, in pl_p0 / pl_p1 by line) over the
 * river (pl_rv, in pl_r) over the plains (pl_bg). */
const uint8_t *pl_o1, *pl_o2, *pl_o3, *pl_wh, *pl_rv;
uint16_t pl_r, pl_w, pl_bg, pl_x;
uint8_t pl_y0, pl_p0, pl_p1;
void __near gx_plot(void);
#pragma aux gx_plot "*_" parm [] modify [ax bx cx dx si di es]

uint16_t gfx_nplot;              /* (tools/dossim peeks it) */
void gfx_plot(uint8_t p, const PlotLook *look)
{
    uint8_t i, r, x0;

    pl_o1 = pl_o2 = pl_o3 = pl_wh = pl_rv = zeros;
    if (look->store) {
        pl_wh = plot_parts[PP_STORE];
    } else {
        if (look->owned) {
            pl_o1 = plot_parts[PP_BORDER];
            if (look->icon)
                pl_o2 = plot_parts[PP_ICON + look->icon - 1];
            if (look->dots) {
                memset(dots, 0, 64);
                for (i = 0; i < look->dots && i < 8; ++i) {
                    r = plot_dots[i][0] * 2 + (plot_dots[i][1] >> 3);
                    dots[r] |= 0x80 >> (plot_dots[i][1] & 7);
                    dots[r + 2] |= 0x80 >> (plot_dots[i][1] & 7);
                }
                pl_o3 = dots;
            }
        }
        if (look->mountains)
            pl_wh = plot_parts[PP_MOUNT + look->mountains - 1];
        if (look->river)
            pl_rv = plot_parts[PP_RIVER];
    }
    ++gfx_nplot;
    x0 = PLOT_X(p);
    pl_y0 = PLOT_Y(p);
    spr_bg(x0, 16, pl_y0, 32);
    if (plat_jr) {
        pl_r = JR_RIVER * 0x1111u;
        pl_w = (look->mountains && !look->store) ? 0x0000 : 0xFFFF;   /* black mountains */
        pl_x = x0;
    } else {
        pl_r = 0x5555;          /* cyan (light blue on EGA/VGA), the nearest to blue */
        pl_w = 0xFFFF;
        pl_x = x0 >> 1;
    }
    pl_bg = bgpat * 0x101u;
    pl_p0 = pat(look->col, 0);
    pl_p1 = pat(look->col, 1);
    gx_plot();
    bg_done();
}

/* ---- the town ----------------------------------------------------------- */
/* 144 lines from line 16, clocks 32..127, unpacked as it is drawn
 * (tools/mkgfx_dos.py rle): a byte a clock, RGBI in both nibbles, as the
 * PCjr shows it. CGA takes the clocks two at a time into its palette, the
 * corral's grey inside its edge black (white would swamp the screen):
 * gx_town, with the corral's inside in gx_cx0..gx_cy1 (exclusive) and
 * cga2[], an RGBI's CGA pixel pair. */
const uint8_t __far *gx_tsrc;
uint8_t gx_cx0, gx_cy0, gx_cx1, gx_cy1;
#ifndef PLAT_JRONLY
uint8_t cga2[16];
#endif
void __near gx_town(void);
#pragma aux gx_town "*_" parm [] modify [ax bx cx dx si di es]

void gfx_town(void)
{
#ifndef PLAT_JRONLY
    uint8_t i;
    for (i = 0; i < 16; ++i)
        cga2[i] = rgbi_cga[i] | rgbi_cga[i] << 2;
#endif
    gx_tsrc = town_jr;
    gx_cx0 = town_corral[0] + 1;
    gx_cy0 = town_corral[1] + 1;
    gx_cx1 = town_corral[2] - 1;
    gx_cy1 = town_corral[3] - 1;
    spr_bg(32, 96, 16, 144);
    gx_town();
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

/* The save buffers are rings of rows: line y's bytes are row y % rows
 * (rows = SAVE_MAX / the shape's widest row, st), so a sprite that moves a
 * little finds its old row for a line just where its new one goes. A
 * flush then does each line at once, top to bottom -- put the old rows
 * back (last drawn first), save the new ones, draw -- and no sprite is
 * ever off the screen while the beam goes by. */
static uint16_t s_st[SPR_SLOTS];        /* the ring's row length when drawn */
uint8_t lift_y0, lift_y1;               /* lines spr_bg took sprites off */

static uint16_t ring(uint16_t st, uint8_t y)
{
    return (y % (SAVE_MAX / st)) * st;
}

/* A sprite as gfxa.asm's gx_sprlines puts it back, saves and draws it, a
 * line at a time (its J_* offsets: keep the two in step). */
typedef struct {
    uint8_t *save;              /* 0  its save buffer */
    const uint8_t *src;         /* 2  the picture's next row */
    int16_t x;                  /* 4  its left clock (either edge may be off) */
    uint16_t no, nst, nend;     /* 6  the new rows' ring: offset, row, end */
    uint16_t oo, ost, oend;     /* 12 the old rows' */
    uint8_t show, sy, sh, bx, n, wb, pt0, pt1;  /* 18 where it goes */
    uint8_t drawn, osy, osh, obx, on, lifted;   /* 26 where it was; lifted
                                                 * 2: by spr_bg, 4: only the
                                                 * waiting lines change */
    uint8_t same, put;          /* 32 same 1: alone, in the same bytes: a
                                 * line put back (put: the last) is already
                                 * saved; 2: as lifted 4 */
    uint8_t c0, cn, slot;       /* 34 (C's) */
} SprJob;
typedef char sprjob_is_37_bytes[sizeof(SprJob) == 37 ? 1 : -1];

SprJob *gx_jp[SPR_SLOTS];       /* gx_sprlines: the sprites, in slot order */
uint8_t gx_nc, gx_y0, gx_y1, gx_pass;   /* how many; lines y0..y1-1; 0 put
                                         * back, 1 save and draw, 2 both */
void __near gx_sprlines(void);
#pragma aux gx_sprlines "*_" parm [] modify [ax bx cx dx si di es]

static void lines(uint8_t nc, uint8_t y0, uint8_t y1, uint8_t pass)
{
    gx_nc = nc;
    gx_y0 = y0;
    gx_y1 = y1;
    gx_pass = pass;
    gx_sprlines();
}

/* Where drawn slot i is, from line y on, into t. */
static void old_job(uint8_t i, uint8_t y, SprJob *t)
{
    t->save = save[i];
    t->drawn = 1;
    t->osy = s_sy[i];
    t->osh = s_sh[i];
    t->obx = s_bx[i];
    t->on = s_n[i];
    t->ost = s_st[i];
    t->oend = SAVE_MAX / t->ost * t->ost;
    t->oo = ring(t->ost, y > t->osy ? y : t->osy);
    t->lifted = 0;
    t->same = 0;
    t->put = 255;
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

/* Mark every drawn slot after i that meets the rectangle. */
static uint8_t hit[SPR_SLOTS];          /* (bit 1: taken off by spr_bg; bit 2:
                                         * under a waiting line) */
static void close_over(uint8_t i, uint8_t c0, uint8_t n, uint8_t y, uint8_t h)
{
    while (++i < SPR_SLOTS)
        if (!(hit[i] & 3) && s_drawn[i] && meets(i, c0, n, y, h))
            hit[i] |= 1;
}

/* Background drawing is about to change a rectangle (in colour clocks):
 * the sprites on it, and any drawn after one of them over it (their saved
 * backgrounds hold its pixels), come off -- only its lines; bg_done puts
 * them back over the new background. */
static uint8_t bg_took;

static uint8_t d_lo = 255, d_hi;        /* the lines drawn sprites are on */

static void spr_lift(uint8_t c0, uint8_t n, uint8_t y, uint8_t h);

static void spr_bg(uint8_t c0, uint8_t n, uint8_t y, uint8_t h)
{
    uint16_t y1 = (uint16_t)y + h;
    if (y >= d_hi || y1 <= d_lo)
        return;                 /* (most text: nowhere near one) */
    /* the lines that wait go first: here, not under spr_lift's jobs (a
     * flush's on the stack as well would nearly fill it) */
    if (hq_n && y <= hq_hi && y1 > hq_lo)
        spr_flush();
    spr_lift(c0, n, y, h);
}

static void spr_lift(uint8_t c0, uint8_t n, uint8_t y, uint8_t h)
{
    uint8_t i, nc = 0;
    uint16_t y1 = (uint16_t)y + h;
    SprJob jb[SPR_SLOTS];
    for (i = 0; i < SPR_SLOTS; ++i)
        hit[i] = s_drawn[i] && meets(i, c0, n, y, h);
    for (i = 0; i < SPR_SLOTS; ++i)
        if (hit[i]) {
            close_over(i, s_c0[i], s_cn[i], s_sy[i], s_sh[i]);
            gx_jp[nc] = &jb[nc];
            old_job(i, y, &jb[nc++]);
            s_dirty[i] |= 2;
            bg_took = any_dirty = 1;
        }
    lift_y0 = y;
    lift_y1 = y1 > 255 ? 255 : (uint8_t)y1;
    if (nc)
        lines(nc, y, lift_y1, 0);
}

static void bg_done(void)
{
    if (bg_took) {
        bg_took = 0;
        spr_flush();
    }
}

/* gfx_hdots' line (hd_*, on line y) between the sprites' rows on it, now;
 * does it cross any? Those are drawn again at the flush, with it under
 * them: only on the waiting lines, if nothing else changes them. */
static uint8_t spr_under(uint8_t y)
{
    uint8_t i, a, e, b, c = 0, r = 0;
    if (y >= d_lo && y < d_hi) {
        b = plat_jr ? hd_off + 2 * hd_n - 1 : hd_off + hd_n;
        for (i = 0; i < SPR_SLOTS; ++i)
            if (s_drawn[i] && (uint8_t)(y - s_sy[i]) < s_sh[i] &&
                s_bx[i] < b && (uint8_t)(s_bx[i] + s_n[i]) > (uint8_t)hd_off) {
                s_dirty[i] |= 4;
                r = any_dirty = 1;
            }
        /* the gaps, left to right */
        while (r) {
            a = 255;
            for (i = 0; i < SPR_SLOTS; ++i)
                if (s_drawn[i] && (uint8_t)(y - s_sy[i]) < s_sh[i] &&
                    (b = s_bx[i] + s_n[i]) > c && s_bx[i] < a) {
                    a = s_bx[i];
                    e = b;
                }
            if (a == 255)
                break;
            gx_hdclip(c, a);
            c = e;
        }
    }
    gx_hdclip(c, 255);
    return r;
}

/* Where slot i would be drawn now, into t. */
static void src_of(uint8_t i, SprJob *t)
{
    const ShapeDef *d = &dos_shapes[s_shape[i]];
    uint8_t frame = s_frame[i];
    uint16_t sz = (uint16_t)d->wb * d->h, rows;
    const uint8_t *src;
    int16_t yy = s_y[i], hh = d->h, left, right;

    t->save = save[i];
    t->slot = i;
    t->show = 0;
    t->drawn = 0;
    t->lifted = 0;
    t->same = 0;
    t->put = 255;
    if (!s_on[i])
        return;
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
        return;
    t->show = 1;
    t->src = src;
    t->x = left;
    if (left < 0)
        left = 0;
    if (right > 159)
        right = 159;
    if (plat_jr) {
        t->bx = (uint8_t)left;
        t->n = (uint8_t)(right - left + 1);
        t->nst = d->w;
    } else {
        t->bx = (uint8_t)(left >> 1);
        t->n = (uint8_t)(right >> 1) - t->bx + 1;
        t->nst = (d->w >> 1) + 1;
    }
    rows = SAVE_MAX / t->nst;
    t->nend = rows * t->nst;
    if (hh > (int16_t)rows)
        hh = rows;
    t->sy = (uint8_t)yy;
    t->sh = (uint8_t)hh;
    t->c0 = (uint8_t)left;
    t->cn = (uint8_t)(right - left + 1);
    t->wb = d->wb;
    t->pt0 = pat(s_col[i], 0);
    t->pt1 = pat(s_col[i], 1);
    t->no = ring(t->nst, t->sy);
}

/* Each job's rectangle, old and new together: lines lo..hi-1 (within
 * y0..y1-1), bytes xl..xh-1. */
static uint8_t lo[SPR_SLOTS], hi[SPR_SLOTS], xl[SPR_SLOTS], xh[SPR_SLOTS], grp[SPR_SLOTS];

static void rect_of(uint8_t k, SprJob *t, uint8_t y0, uint8_t y1)
{
    uint8_t a = 255, b = 0, l = 255, r = 0, e;
    if (t->show) {
        a = t->sy;
        b = t->sy + t->sh;
        l = t->bx;
        r = t->bx + t->n;
    }
    if (t->drawn) {
        if (t->osy < a)
            a = t->osy;
        if ((e = t->osy + t->osh) > b)
            b = e;
        if (t->obx < l)
            l = t->obx;
        if ((e = t->obx + t->on) > r)
            r = e;
    }
    lo[k] = a < y0 ? y0 : a;
    hi[k] = b > y1 ? y1 : b;
    xl[k] = l;
    xh[k] = r;
}

void spr_flush(void)
{
    uint8_t i, j, k, y0, y1, pass, e, nc, part, n;
    SprJob jb[SPR_SLOTS], *t;

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
        if (hit[i] != 2)
            part = 0;
        t = &jb[nc++];
        src_of(i, t);
        if (t->show) {
            if (t->sy < y0)
                y0 = t->sy;
            if ((e = t->sy + t->sh) > y1)
                y1 = e;
        }
        if (s_drawn[i]) {
            old_job(i, 0, t);
            t->lifted = hit[i] & 2;
            if (hit[i] == 4) {          /* only under waiting lines */
                t->lifted = 4;
                t->same = 2;
            }
            if (s_sy[i] < y0)
                y0 = s_sy[i];
            if ((e = s_sy[i] + s_sh[i]) > y1)
                y1 = e;
            /* a line's new row must not land where an old row not put back
             * yet is: if it could, the old all come off first */
            if (t->show && (t->nst != t->ost || e > (uint16_t)t->sy + t->nend / t->nst))
                pass = 0;
            close_over(i, s_c0[i], s_cn[i], s_sy[i], s_sh[i]);
        }
        if (t->show)
            close_over(i, t->c0, t->cn, t->sy, t->sh);
    }
    if (part && nc) {
        /* nothing moved: save and draw again only what spr_bg took off */
        pass = 1;
        if (y0 < lift_y0)
            y0 = lift_y0;
        if (y1 > lift_y1)
            y1 = lift_y1;
        for (k = 0; k < nc; ++k) {
            t = &jb[k];
            if (t->show && t->sy < y0) {
                t->src += (uint8_t)(y0 - t->sy) * t->wb;
                t->no = ring(t->nst, y0);
            }
        }
    }
    /* Those whose rectangles (old and new) meet are done together, a line
     * at a time, in order; the rest each alone, on its lines (a line then
     * looks only at it). */
    for (k = 0; k < nc; ++k) {
        rect_of(k, &jb[k], y0, y1);
        grp[k] = k;
        for (j = 0; j < k; ++j)
            if (lo[j] < hi[k] && lo[k] < hi[j] && xl[j] < xh[k] && xl[k] < xh[j] &&
                grp[j] != grp[k]) {
                n = grp[k];
                for (i = 0; i <= k; ++i)
                    if (grp[i] == n)
                        grp[i] = grp[j];
            }
    }
    for (k = 0; k < nc; ++k) {
        if (grp[k] != k)
            continue;           /* (a group's first is its own) */
        n = 0;
        y0 = 255;
        y1 = 0;
        for (j = k; j < nc; ++j)
            if (grp[j] == k && lo[j] < hi[j]) {
                gx_jp[n++] = &jb[j];
                if (lo[j] < y0)
                    y0 = lo[j];
                if (hi[j] > y1)
                    y1 = hi[j];
            }
        if (n) {
            t = gx_jp[0];
            if (n == 1 && !hq_n)
                t->same = t->show && t->drawn && t->bx == t->obx && t->n == t->on &&
                          t->nst == t->ost;
            lines(n, y0, y1, pass);
            if (!pass)
                lines(n, y0, y1, 1);
        }
    }
    for (k = 0; k < nc; ++k) {
        t = &jb[k];
        i = t->slot;
        s_dirty[i] = 0;
        if ((s_drawn[i] = t->show) != 0) {
            s_bx[i] = t->bx;
            s_n[i] = t->n;
            s_sy[i] = t->sy;
            s_sh[i] = t->sh;
            s_c0[i] = t->c0;
            s_cn[i] = t->cn;
            s_st[i] = t->nst;
        }
    }
    lift_y1 = 0;
    hq_n = 0;
    hq_lo = 255;
    hq_hi = 0;
    d_lo = 255;
    d_hi = 0;
    for (i = 0; i < SPR_SLOTS; ++i)
        if (s_drawn[i]) {
            if (s_sy[i] < d_lo)
                d_lo = s_sy[i];
            if ((e = s_sy[i] + s_sh[i]) > d_hi)
                d_hi = e;
        }
}
