/*
 * gfx.c -- gfx.h on the CoCo 1/2 in PMODE 4 (256 x 192 at $0E00), in
 * artifact colour.
 *
 * Two neighbouring pixels are one colour pixel (128 across): 00 black, 11
 * white, 01 and 10 the artifact colours, called A and B here. Which is blue
 * and which orange depends on the machine, so the player can swap them
 * (CLEAR; gfx_swap_colours). Four players get blue, orange, white and
 * stripes (A on even lines, B on odd).
 *
 * Atari colour clocks x become colour pixels x - x/4 + 2 (160 -> 120): a
 * map plot is 12 colour pixels (3 bytes) from byte 2, a text cell 6 pixels
 * from pixel 4 (40 columns), the town 72 colour pixels (tools/mkgfx_pmode4.py).
 * Sprites of characters keep their 8 colour pixels. Scanlines are 1:1.
 */
#include <string.h>
#include "mule.h"
#include "gfxdata.h"

#define SCREEN ((uint8_t *)0x0E00)
#define ROW(y) (SCREEN + ((uint16_t)(y) << 5))

/* What a colour is here. */
#define PK 0            /* black */
#define PA 1            /* artifact colour A (01): blue */
#define PB 2            /* B (10): orange */
#define PW 3            /* white */
#define PS 4            /* stripes: A on even lines, B on odd */

static const uint8_t kind[16] = {
    PK,     /* C_BLACK */
    PW,     /* C_WHITE */
    PA,     /* C_BLUE: player 0 */
    PB,     /* C_PURPLE: player 1 */
    PW,     /* C_GREEN: player 2 */
    PB,     /* C_ORANGE */
    PB,     /* C_RED */
    PW,     /* C_YELLOW */
    PW,     /* C_GREY */
    PA,     /* C_CYAN */
    PB,     /* C_BROWN */
    PB,     /* C_TAN */
    PA,     /* C_LTBLUE */
    PA,     /* C_DKGREEN */
    PB,     /* C_PINK */
    PS,     /* C_P3: player 3 */
};

static uint8_t pa = 0x55, pb = 0xAA;    /* A and B's bit patterns */
static uint8_t swapped;
static uint16_t dbl8[256];              /* a byte with each bit doubled */
static uint8_t xl[256];                 /* a byte with 01 and 10 swapped */

static uint8_t pat(uint8_t k, uint8_t y)
{
    switch (k) {
    case PA: return pa;
    case PB: return pb;
    case PW: return 0xFF;
    case PS: return (y & 1) ? pb : pa;
    }
    return 0;
}

/* x (colour clocks) -> colour pixel */
static int16_t cpx(int16_t x)
{
    return x - (x >> 2) + 2;
}

static void spr_bg(uint8_t bx, uint8_t n, uint8_t y, uint8_t h);
static void bg_done(void);

void gfx_init(void)
{
    uint16_t i, v;
    uint8_t b;
    for (i = 0; i < 256; ++i) {
        v = 0;
        for (b = 0; b < 8; ++b)
            if (i & (0x80 >> b))
                v |= 0xC000 >> (2 * b);
        dbl8[i] = v;
        b = 0;
        for (v = 0; v < 8; v += 2) {
            uint8_t p = (i >> v) & 3;
            if (p == 1 || p == 2)
                p ^= 3;
            b |= p << v;
        }
        xl[i] = b;
    }
    memset(SCREEN, 0, 6144);
    asm {
        sta     $FFC0           ; SAM V2..V0 = 110: 6K graphics
        sta     $FFC3
        sta     $FFC5
        sta     $FFC7           ; display at $0E00: F0-F2 set, F3-F6 clear
        sta     $FFC9
        sta     $FFCB
        sta     $FFCC
        sta     $FFCE
        sta     $FFD0
        sta     $FFD2
        lda     $FF22           ; VDG: RG6 (PMODE 4), CSS 1 (black/white)
        anda    #$07
        ora     #$F8
        sta     $FF22
    }
}

void gfx_clear(void)
{
    spr_hide_all();
    spr_flush();
    memset(SCREEN, 0, 6144);
}

/* Pixels px0 .. px1 - 1 of lines y .. y + h - 1 become colour kind k
 * (with dot mask dm: 0xFF solid). */
static void fill_px(uint16_t px0, uint16_t px1, uint8_t y, uint8_t h, uint8_t k, uint8_t dm)
{
    uint8_t b0, b1, lm, rm, b, m, pt;
    uint8_t *row;
    if (px1 > 256)
        px1 = 256;
    if (px0 >= px1)
        return;
    b0 = (uint8_t)(px0 >> 3);
    b1 = (uint8_t)((px1 - 1) >> 3);
    lm = 0xFF >> (px0 & 7);
    rm = 0xFF << (7 - ((px1 - 1) & 7));
    for (; h; --h, ++y) {
        if (y >= 192)
            break;
        row = ROW(y);
        pt = pat(k, y);
        for (b = b0; b <= b1; ++b) {
            m = dm;
            if (b == b0)
                m &= lm;
            if (b == b1)
                m &= rm;
            row[b] = (row[b] & ~m) | (pt & m);
        }
    }
}

/* ---- text ------------------------------------------------------------- */
static uint8_t glyph_of(char c)
{
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c < 32 || c > 95)
        c = '?';
    return (uint8_t)(c - 32);
}

/* glyph_rows's arguments: an 8-row glyph (6 pixels, bits 7..2), drawn in
 * the cell at screen byte gl_dst, sh pixels in, in the patterns of an even
 * and an odd line; gl_big doubles each pixel (a 12-pixel cell). */
const uint8_t *gl_src;
uint8_t *gl_dst;
uint8_t gl_sh, gl_pt0, gl_pt1, gl_big;
uint8_t gl_c, gl_n, gl_m0, gl_m1, gl_t;

static void glyph_rows(void)
{
    asm {
        pshs    u,y
        ldy     _gl_src
        ldu     _gl_dst
        lda     #8
        sta     _gl_n
@row
        ldb     ,y+
        tst     _gl_big
        bne     @big
        tfr     b,a
        clrb
        ldx     #$FC00
        bra     @shift
@big
        leax    _dbl8,pcr
        clra
        lslb
        rola
        ldd     d,x
        ldx     #$FFF0
@shift
        pshs    x               ; 0,s: the cell mask
        tst     _gl_sh
        beq     @put
        pshs    a
        lda     _gl_sh
        sta     _gl_c
        puls    a
@sh
        lsra
        rorb
        lsr     ,s
        ror     1,s
        dec     _gl_c
        bne     @sh
@put
        anda    _gl_pt0         ; the lit pixels in the line pattern
        andb    _gl_pt0
        std     _gl_m0          ; (gl_m0, gl_m1)
        puls    x
        tfr     x,d
        coma
        comb
        anda    ,u
        andb    1,u
        ora     _gl_m0
        orb     _gl_m1
        std     ,u
        lda     _gl_pt0         ; the next line: the other stripe
        ldb     _gl_pt1
        sta     _gl_pt1
        stb     _gl_pt0
        leau    32,u
        dec     _gl_n
        bne     @row
        puls    u,y
    }
}

static void glyph(uint8_t tx, uint8_t ty, char c, uint8_t k, uint8_t big)
{
    uint8_t px = 4 + 6 * tx, y = ty << 3;
    gl_src = pm_font + ((uint16_t)glyph_of(c) << 3);
    gl_dst = ROW(y) + (px >> 3);
    gl_sh = px & 7;
    gl_pt0 = pat(k, y);
    gl_pt1 = pat(k, y + 1);
    gl_big = big;
    glyph_rows();
}

void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col)
{
    if (tx > 39 || ty > 23)
        return;
    glyph(tx, ty, c, kind[col], 0);
}

void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    if (ty > 23)
        return;
    spr_bg(tx, n, ty << 3, 8);
    while (n-- && *s && tx < 40)
        gfx_char(tx++, ty, *s++, col);
    bg_done();
}

void gfx_text(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_textn(tx, ty, s, 40, col);
}

/* A text cell's first pixel; spr_bg's rectangles are in bytes. */
#define CELL_PX(tx) (4 + 6 * (uint16_t)(tx))

void gfx_clear_text(uint8_t tx, uint8_t ty, uint8_t w)
{
    if (tx >= 40 || ty > 23)
        return;
    if (tx + w > 40)
        w = 40 - tx;
    spr_bg(tx, w, ty << 3, 8);
    fill_px(CELL_PX(tx), CELL_PX(tx + w), ty << 3, 8, PK, 0xFF);
    bg_done();
}

void gfx_bign(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    uint8_t k = kind[col];
    char c;
    tx &= 0xFE;
    if (ty > 23)
        return;
    spr_bg(tx, n * 2, ty << 3, 8);
    while (n-- && (c = *s++) != 0 && tx < 40) {
        glyph(tx, ty, c, k, 1);
        tx += 2;
    }
    bg_done();
}

void gfx_big(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_bign(tx, ty, s, 20, col);
}

/* ---- rectangles ------------------------------------------------------- */
/* Colour pixels -> the text cells (spr_bg's units) they touch. */
static uint8_t cell_of(int16_t cp)
{
    if (cp < 2)
        return 0;
    if (cp > 129)
        return 42;
    return (uint8_t)(((uint16_t)(uint8_t)(cp - 2) * 86) >> 8);     /* / 3 */
}

void gfx_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
    int16_t c0 = cpx(x), c1 = cpx((int16_t)x + w);
    uint8_t a = cell_of(c0);
    spr_bg(a, cell_of(c1) - a + 1, y, h);
    fill_px((uint16_t)c0 * 2, (uint16_t)c1 * 2, y, h, kind[col], 0xFF);
    bg_done();
}

void gfx_hdots(uint8_t x, uint8_t y, uint8_t w, uint8_t col)
{
    int16_t c0 = cpx(x), c1 = cpx((int16_t)x + w);
    uint8_t a = cell_of(c0);
    spr_bg(a, cell_of(c1) - a + 1, y, 1);
    fill_px((uint16_t)c0 * 2, (uint16_t)c1 * 2, y, 1, kind[col], (c0 & 1) ? 0x33 : 0xCC);
    bg_done();
}

/* spr_row's arguments: one row of a 1-bit picture (wb bytes, a bit a
 * colour pixel, MSB left) drawn at screen byte bx0 (may be off either edge)
 * shifted right sh pixels, in pattern pt, on the screen row at sp_row. */
const uint8_t *sp_src;
uint8_t *sp_row;
uint8_t sp_wb, sp_bx0, sp_sh, sp_pt;
uint8_t sp_b, sp_n, sp_c, sp_m0, sp_m1, sp_m2;

static void spr_row(void)
{
    asm {
        pshs    u,y
        ldy     _sp_src
        ldu     _sp_row
        ldb     _sp_bx0
        stb     _sp_b
        lda     _sp_wb
        sta     _sp_n
@byte
        ldb     ,y+
        beq     @next
        leax    _dbl8,pcr
        clra
        lslb
        rola
        ldd     d,x             ; each pixel doubled: 16 bits
        clr     _sp_m2
        tst     _sp_sh
        beq     @put
        pshs    a
        lda     _sp_sh
        sta     _sp_c
        puls    a
@shift
        lsra
        rorb
        ror     _sp_m2
        dec     _sp_c
        bne     @shift
@put
        std     _sp_m0
        ldb     _sp_b
        lda     _sp_m0
        bsr     @one
        incb
        lda     _sp_m1
        bsr     @one
        incb
        lda     _sp_m2
        bsr     @one
@next
        inc     _sp_b
        inc     _sp_b
        dec     _sp_n
        bne     @byte
        puls    u,y
        bra     @done
; screen byte B (if 0..31) = x ^ ((x ^ pattern) & mask A)
@one
        tsta
        beq     @skip
        cmpb    #32
        bhs     @skip
        sta     _sp_c
        lda     b,u
        eora    _sp_pt
        anda    _sp_c
        eora    b,u
        sta     b,u
@skip
        rts
@done
    }
}

/* A 1-bit picture (wb bytes a row, h rows, each shown `rep` times) with its
 * left colour pixel at cx, in colour kind k. */
static void draw1(const uint8_t *src, uint8_t wb, uint8_t h, uint8_t rep, int16_t cx,
                  int16_t y, uint8_t k)
{
    uint8_t r, t;
    sp_wb = wb;
    sp_bx0 = (uint8_t)(cx >> 2);        /* (off the left: negative, skipped) */
    sp_sh = ((uint8_t)cx & 3) << 1;
    for (r = 0; r < h; ++r, src += wb) {
        for (t = 0; t < rep; ++t, ++y) {
            if (y < 0)
                continue;
            if (y >= 192)
                return;
            sp_src = src;
            sp_row = ROW(y);
            sp_pt = pat(k, (uint8_t)y);
            spr_row();
        }
    }
}

void gfx_pic(uint8_t x, uint8_t y, uint8_t pic, uint8_t col)
{
    int16_t cx = cpx(x);
    uint8_t a = cell_of(cx);
    spr_bg(a, 4, y, 16);
    /* the picture replaces what was there: clear its box first */
    fill_px((uint16_t)cx * 2, (uint16_t)cx * 2 + 16, y, 16, PK, 0xFF);
    draw1(pm_pics[pic], 1, 8, 2, cx, y, kind[col]);
    bg_done();
}

/* ---- the map ---------------------------------------------------------- */
void gfx_map_frame(void)
{
    gfx_clear();
}

/* plot_rows's arguments: five 64-byte layers (2 bytes a row: 12 colour
 * pixels, MSB left) -- the owner's three, white, the river -- the first
 * screen byte, and the owner's pattern on this line and the next. */
const uint8_t *pl_o1, *pl_o2, *pl_o3, *pl_wh, *pl_rv;
uint8_t *pl_dst;
uint8_t pl_po0, pl_po1, pl_pb;
uint16_t pl_own, pl_nown, pl_riv, pl_wv;
uint8_t pl_t;
static const uint8_t zeros[64];
static uint8_t dots[64];

/* 32 lines of 3 bytes: owner | river (where the owner isn't) | white. */
static void plot_rows(void)
{
    asm {
        pshs    u,y
        leay    _dbl8,pcr
        ldu     _pl_dst
        lda     #32
        pshs    a
@row
        ldx     _pl_o1
        ldd     ,x++
        stx     _pl_o1
        ldx     _pl_o2
        ora     ,x+
        orb     ,x+
        stx     _pl_o2
        ldx     _pl_o3
        ora     ,x+
        orb     ,x+
        stx     _pl_o3
        std     _pl_own
        coma
        comb
        std     _pl_nown
        ldx     _pl_rv
        ldd     ,x++
        stx     _pl_rv
        anda    _pl_nown
        andb    _pl_nown+1
        std     _pl_riv
        ldx     _pl_wh
        ldd     ,x++
        stx     _pl_wh
        std     _pl_wv
        ; bytes 0 and 1: the 8 colour pixels of the high bytes
        ldb     _pl_own
        clra
        lslb
        rola
        ldd     d,y
        anda    _pl_po0
        andb    _pl_po0
        std     ,u
        ldb     _pl_riv
        clra
        lslb
        rola
        ldd     d,y
        anda    _pl_pb
        andb    _pl_pb
        ora     ,u
        orb     1,u
        std     ,u
        ldb     _pl_wv
        clra
        lslb
        rola
        ldd     d,y
        ora     ,u
        orb     1,u
        std     ,u
        ; byte 2: the 4 of the low bytes
        ldb     _pl_own+1
        clra
        lslb
        rola
        lda     d,y
        anda    _pl_po0
        sta     _pl_t
        ldb     _pl_riv+1
        clra
        lslb
        rola
        lda     d,y
        anda    _pl_pb
        ora     _pl_t
        sta     _pl_t
        ldb     _pl_wv+1
        clra
        lslb
        rola
        lda     d,y
        ora     _pl_t
        sta     2,u
        ; the next line: the other stripe
        lda     _pl_po0
        ldb     _pl_po1
        sta     _pl_po1
        stb     _pl_po0
        leau    32,u
        dec     ,s
        lbne    @row
        leas    1,s
        puls    u,y
    }
}

void gfx_plot(uint8_t p, const PlotLook *look)
{
    uint8_t i, r, bx, y0, k = kind[look->col];

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
    bx = 2 + (p % 9) * 3;
    y0 = 16 + (p / 9) * 32;
    spr_bg(cell_of(bx * 4), 5, y0, 32);
    pl_dst = ROW(y0) + bx;
    pl_po0 = pat(k, y0);
    pl_po1 = pat(k, y0 + 1);
    pl_pb = pb;
    plot_rows();
    bg_done();
}

/* The town: 144 lines of 19 bytes from byte 6, unpacked as it is drawn
 * (tools/mkgfx_pmode4.py rle). */
void gfx_town(void)
{
    const uint8_t *src = pm_town;
    uint8_t *d = ROW(16) + 6;
    uint8_t col = 0, n, v, lit = 0, run = 0;
    uint16_t left = 144 * 19;
    spr_bg(7, 26, 16, 144);
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
        *d++ = swapped ? xl[v] : v;
        --left;
        if (++col == 19) {
            col = 0;
            d += 32 - 19;
        }
    }
    bg_done();
}

/* ---- sprites ----------------------------------------------------------
 * Each slot keeps what the game asked for and, once drawn, the rectangle
 * (bytes bx..bx+n-1, lines sy..sy+sh-1) it saved from under itself. The
 * rectangles given to spr_bg by background drawing are in text cells; the
 * sprites' are turned into cells for comparing (s_c0, s_cn).
 */
#define SAVE_MAX 128
/* The save-under buffers: 10 x 128 bytes at the top of RAM, above the
 * overlay window (src/coco/mule.link, tools/coco/lwlink-mule.py). */
#define SAVE ((uint8_t *)0xFA00)

uint8_t s_on[SPR_SLOTS], s_shape[SPR_SLOTS], s_frame[SPR_SLOTS];  /* (not static: cocosim peeks) */
static uint8_t s_col[SPR_SLOTS], s_flags[SPR_SLOTS];
static int16_t s_x[SPR_SLOTS], s_y[SPR_SLOTS];
uint8_t s_dirty[SPR_SLOTS];  /* needs (re)drawing */
uint8_t s_drawn[SPR_SLOTS];  /* on the screen: the rectangle is valid */
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
    uint8_t r, n = s_n[i];
    uint8_t *p = ROW(s_sy[i]) + s_bx[i], *s = SAVE + (uint16_t)i * SAVE_MAX;
    for (r = s_sh[i]; r; --r, p += 32, s += n)
        if (to_screen)
            memcpy(p, s, n);
        else
            memcpy(s, p, n);
}

static void erase(uint8_t i)
{
    rect(i, 1);
    s_drawn[i] = 0;
}

/* Does drawn sprite j's rectangle meet cells c0.. (n of them), lines y..? */
static uint8_t meets(uint8_t j, uint8_t c0, uint8_t n, uint8_t y, uint8_t h)
{
    uint8_t a = s_c0[j], e = c0 + n, ae = a + s_cn[j];
    if (a >= e || c0 >= ae)
        return 0;
    a = s_sy[j];
    e = y + h;
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

/* Background drawing is about to change a rectangle (in text cells): take
 * the sprites off it; bg_done puts them back over the new background. */
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
static const uint8_t *src_of(uint8_t i, int16_t *cx, uint8_t *y, uint8_t *h, uint8_t *wb)
{
    const ShapeDef *d = &pm_shapes[s_shape[i]];
    uint8_t frame = s_frame[i], sz = d->wb * d->h;
    const uint8_t *src;
    int16_t yy = s_y[i];
    int16_t hh = d->h;
    int16_t left, right;

    if (s_flags[i] & SPR_FLIPX) {
        if (s_shape[i] == SH_MULE)
            frame += 4;
        else if (s_shape[i] == SH_FIGURE && (frame % 10) < 2)
            frame += 2;
    }
    src = d->data + (uint16_t)frame * sz;
    if (yy < 0) {
        src += (uint8_t)(-yy) * d->wb;
        hh += yy;
        yy = 0;
    }
    if (yy + hh > 192)
        hh = 192 - yy;
    /* a character's 8 colour pixels stand for the Atari's 8 clocks (6
     * here): centred on them */
    left = cpx(s_x[i]) - (d->w == 8 ? 1 : 0);
    right = left + d->w - 1;
    if (hh <= 0 || right < 0 || left > 127)
        return 0;
    *cx = left;
    if (left < 0)
        left = 0;
    if (right > 127)
        right = 127;
    s_bx[i] = (uint8_t)(left >> 2);
    s_n[i] = (uint8_t)(right >> 2) - s_bx[i] + 1;
    s_sy[i] = (uint8_t)yy;
    s_sh[i] = (uint8_t)hh;
    if ((uint16_t)s_n[i] * s_sh[i] > SAVE_MAX)
        s_sh[i] = SAVE_MAX / s_n[i];
    s_c0[i] = cell_of(left);
    s_cn[i] = cell_of(right) - s_c0[i] + 1;
    *y = (uint8_t)yy;
    *h = s_sh[i];
    *wb = d->wb;
    return src;
}

void spr_flush(void)
{
    uint8_t i, j, y, h, wb;
    int16_t cx;
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
        src = src_of(i, &cx, &y, &h, &wb);
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
        draw1(src, wb, h, 1, cx, y, kind[s_col[i]]);
    }
    any_dirty = 0;
    for (i = 0; i < SPR_SLOTS; ++i)
        any_dirty |= s_dirty[i];
}

/* CLEAR: the artifact colours are the wrong way round on this machine. */
void gfx_swap_colours(void)
{
    uint8_t *p = SCREEN;
    uint16_t n;
    swapped ^= 1;
    pa ^= 0xFF;
    pb ^= 0xFF;
    for (n = 6144; n; --n, ++p)
        *p = xl[*p];
    for (p = SAVE, n = SPR_SLOTS * SAVE_MAX; n; --n, ++p)
        *p = xl[*p];
}
