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
 * map plot is 12 colour pixels (3 bytes) from byte 2, the town 72 colour
 * pixels (tools/mkgfx_pmode4.py). A text cell is a byte (32 columns, the
 * Namco font, as on the TMS9918).
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

/* A character: the 8 x 8 Namco glyph fills the byte at text cell tx, in
 * colour kind k. White keeps every pixel. A colour has 4 colour pixels a
 * cell: each is lit where the glyph's odd pixel is, so a character has the
 * same shape in blue, in orange and in the stripes (A alone would take the
 * odd pixels and B the even ones: two shapes, mixed line by line). */
static void glyph(uint8_t tx, uint8_t ty, char c, uint8_t k)
{
    const uint8_t *src = pm_font + ((uint16_t)glyph_of(c) << 3);
    uint8_t *d = ROW(ty << 3) + tx;
    uint8_t p0 = pat(k, 0), p1 = pat(k, 1), r, m;
    for (r = 0; r < 8; ++r, d += 32) {
        m = *src++;
        if (k != PW) {
            m &= 0x55;
            m |= m << 1;
        }
        *d = m & ((r & 1) ? p1 : p0);
    }
}

void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col)
{
    if (tx >= TCOLS || ty > 23)
        return;
    glyph(tx, ty, c, kind[col]);
}

void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    uint8_t k = kind[col];
    if (ty > 23 || tx >= TCOLS)
        return;
    spr_bg(tx, n, ty << 3, 8);
    while (n-- && *s && tx < TCOLS)
        glyph(tx++, ty, *s++, k);
    bg_done();
}

void gfx_text(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_textn(tx, ty, s, TCOLS, col);
}

void gfx_clear_text(uint8_t tx, uint8_t ty, uint8_t w)
{
    if (tx >= TCOLS || ty > 23)
        return;
    if (tx + w > TCOLS)
        w = TCOLS - tx;
    spr_bg(tx, w, ty << 3, 8);
    fill_px((uint16_t)tx << 3, (uint16_t)(tx + w) << 3, ty << 3, 8, PK, 0xFF);
    bg_done();
}

/* Big text is the same font (BIGW 1), as on the TMS9918. */
void gfx_bign(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    gfx_textn(tx, ty, s, n, col);
}

void gfx_big(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_textn(tx, ty, s, TCOLS, col);
}

/* ---- rectangles ------------------------------------------------------- */
/* Colour pixels -> the text cells (bytes: spr_bg's units) they touch. */
static uint8_t cell_of(int16_t cp)
{
    if (cp < 0)
        return 0;
    if (cp > 127)
        return 32;
    return (uint8_t)cp >> 2;
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
    spr_bg(bx, 3, y0, 32);
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
    spr_bg(6, 19, 16, 144);
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
 * rectangles given to spr_bg by background drawing are in text cells
 * (bytes); the sprites' are turned into cells for comparing (s_c0, s_cn).
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

/* The save buffers are rings of rows: screen line y's bytes are row
 * y % rows (rows = SAVE_MAX / the shape's widest row, st), so a sprite that
 * moves a little finds its old row for a line just where its new one goes.
 * A flush then does each line at once, top to bottom -- put the old rows
 * back (last drawn first), save the new ones, draw -- and no sprite is
 * ever off the screen while the beam goes by. (In assembly: CMOC's code
 * for it would not fit.) */
static uint8_t s_st[SPR_SLOTS];         /* the ring's row length when drawn */
static uint8_t lift_y0, lift_y1;        /* lines spr_bg took sprites off */
static uint8_t hit[SPR_SLOTS];          /* (bit 1: taken off by spr_bg) */
static uint8_t bg_took;

void spr_asm(void)
{
    asm {
        rts
; does drawn slot B's rectangle meet the one at X (cells c0, n, lines y, h)?
; A: 0 if not
sprMeets
        pshs    b,y
        leay    _s_c0,pcr
        lda     b,y
        cmpa    ,x
        bhs     @m1
        leay    _s_cn,pcr
        adda    b,y
        cmpa    ,x
        bls     @no
        bra     @mv
@m1
        suba    ,x
        cmpa    1,x
        bhs     @no
@mv
        leay    _s_sy,pcr
        lda     b,y
        cmpa    2,x
        bhs     @m2
        leay    _s_sh,pcr
        adda    b,y
        cmpa    2,x
        bls     @no
        bra     @yes
@m2
        suba    2,x
        cmpa    3,x
        bhs     @no
@yes
        lda     #1
        puls    b,y,pc
@no
        clra
        puls    b,y,pc
; mark every drawn slot after B that meets the rectangle at X
sprClose
        pshs    b,y
@c
        incb
        cmpb    #10
        bhs     @cx
        leay    _hit,pcr
        tst     b,y
        bne     @c
        leay    _s_drawn,pcr
        tst     b,y
        beq     @c
        lbsr    sprMeets
        tsta
        beq     @c
        leay    _hit,pcr
        sta     b,y
        bra     @c
@cx
        puls    b,y,pc
; slot B's drawn rectangle -> X (c0, n, y, h)
sprOld
        pshs    y
        leay    _s_c0,pcr
        lda     b,y
        sta     ,x
        leay    _s_cn,pcr
        lda     b,y
        sta     1,x
        leay    _s_sy,pcr
        lda     b,y
        sta     2,x
        leay    _s_sh,pcr
        lda     b,y
        sta     3,x
        puls    y,pc
; A: row length st, B: line -> A: where the line's row is in the ring,
; B: the ring's length (128 / st * st)
sprRing
        pshs    a
        mul
        pshs    d
        lda     #128
@r1
        suba    2,s
        bcc     @r1
        adda    2,s
        nega
        adda    #128
        sta     2,s
        puls    d
@r2
        subb    ,s
        sbca    #0
        bcc     @r2
        addb    ,s
        tfr     b,a
        puls    b,pc
; put back slot B's saved row for line A; X: its place in the ring (then
; the ring's length), moved on to the next line's
sprPut
        pshs    d,x,y,u
        ldb     #32
        mul
        addd    #$0E00
        tfr     d,u
        ldb     1,s
        leay    _s_bx,pcr
        ldb     b,y
        leau    b,u
        ldb     1,s
        lda     #128
        mul
        addd    #$FA00
        ldy     2,s
        addb    ,y
        adca    #0
        tfr     d,y
        ldb     1,s
        leax    _s_n,pcr
        ldb     b,x
@p
        lda     ,y+
        sta     ,u+
        decb
        bne     @p
        puls    d,x,y,u
sprAdv
        pshs    d,y
        leay    _s_st,pcr
        lda     b,y
        adda    ,x
        cmpa    1,x
        blo     @a
        clra
@a
        sta     ,x
        puls    d,y,pc
    }
}

/* Background drawing is about to change a rectangle (in text cells): the
 * sprites on it, and any drawn after one of them over it (their saved
 * backgrounds hold its pixels), come off -- only its lines; bg_done puts
 * them back over the new background. */
static void spr_bg(uint8_t bc0, uint8_t bn, uint8_t by, uint8_t bh)
{
    uint8_t ii, ll, le, rc[4], ol[2];
    asm {
        leax    rc
        lda     bc0
        sta     ,x
        lda     bn
        sta     1,x
        lda     by
        sta     2,x
        lda     bh
        sta     3,x
        ldb     #9
@b1
        leay    _hit,pcr
        clr     b,y
        leay    _s_drawn,pcr
        tst     b,y
        beq     @b2
        lbsr    sprMeets
        leay    _hit,pcr
        sta     b,y
@b2
        decb
        bpl     @b1
        clrb
@b3
        leay    _hit,pcr
        tst     b,y
        beq     @b4
        leax    rc
        lbsr    sprOld
        lbsr    sprClose
@b4
        incb
        cmpb    #10
        blo     @b3
; their lines in by.. go back, the last drawn first
        ldb     #10
        stb     ii
@b5
        dec     ii
        lbmi    @b9
        ldb     ii
        leay    _hit,pcr
        tst     b,y
        beq     @b5
        lda     #1
        sta     _bg_took,pcr
        sta     _any_dirty,pcr
        leay    _s_dirty,pcr
        lda     b,y
        ora     #2
        sta     b,y
        leay    _s_sy,pcr
        lda     b,y
        cmpa    by
        bhs     @b6
        lda     by
@b6
        sta     ll
        lda     b,y
        leay    _s_sh,pcr
        adda    b,y
        pshs    a
        lda     by
        adda    bh
        cmpa    ,s+
        bls     @b7
        lda     -1,s
@b7
        sta     le
        lda     ll
        cmpa    le
        bhs     @b5
        leay    _s_st,pcr
        lda     b,y
        ldb     ll
        lbsr    sprRing
        leax    ol
        std     ,x
        ldb     ii
@b8
        lda     ll
        lbsr    sprPut
        inc     ll
        lda     ll
        cmpa    le
        blo     @b8
        lbra    @b5
@b9
        lda     by
        sta     _lift_y0,pcr
        adda    bh
        sta     _lift_y1,pcr
    }
}

static void bg_done(void)
{
    if (bg_took) {
        bg_took = 0;
        spr_flush();
    }
}

/* A sprite being drawn by spr_flush (whose assembly knows the offsets). */
typedef struct {
    const uint8_t *src;         /* 0: the picture's next row */
    int16_t cx;                 /* 2: its left colour pixel */
    uint8_t c0, cn, sy, sh;     /* 4: where (as s_c0.. will say) */
    uint8_t bx, n, st, wb;      /* 8 */
    uint8_t pt0, pt1;           /* 12: the pattern on even lines, odd */
    uint8_t no, nlim, oo, olim; /* 14: new rows' place in the ring, old */
    uint8_t show;               /* 18: 0: nothing (hidden, or off the screen) */
    uint8_t rows;               /* 19: the ring's rows */
} Draw;

/* Where slot si would be drawn now, into t (ShapeDef: data, wb, w, h). */
void coco_src_of(uint8_t si, Draw *t)
{
    asm {
        ldb     si
        ldy     t
        pshs    u
        leas    -12,s           ; 0 yy, 2 hh, 4 slot, 5 frame, 6 shape, 8 left, 10 right
        stb     4,s
        clr     18,y
        leax    _s_on,pcr
        tst     b,x
        lbeq    @ret
        leax    _s_shape,pcr
        lda     b,x
        sta     6,s
        leax    _s_frame,pcr
        lda     b,x
        leax    _s_flags,pcr
        ldb     b,x
        andb    #1              ; SPR_FLIPX: the mirrored frames
        beq     @nf
        ldb     6,s
        cmpb    #2              ; SH_MULE
        bne     @f1
        adda    #4
        bra     @nf
@f1
        cmpb    #1              ; SH_FIGURE, frames 0-1 of 10
        bne     @nf
        tfr     a,b
@f2
        subb    #10
        bcc     @f2
        addb    #10
        cmpb    #2
        bhs     @nf
        adda    #2
@nf
        sta     5,s
        ldb     6,s
        lda     #5
        mul
        leau    _pm_shapes,pcr
        leau    d,u
        lda     2,u
        ldb     4,u
        mul
        lda     5,s
        mul
        addd    ,u
        std     ,y
        clra
        ldb     4,u
        std     2,s
        ldb     4,s
        lslb
        leax    _s_y,pcr
        ldd     b,x
        std     ,s
        bpl     @yp
        addd    2,s             ; off the top: the rows below it
        std     2,s
        ldb     1,s
        negb
        lda     2,u
        mul
        addd    ,y
        std     ,y
        clr     ,s
        clr     1,s
@yp
        ldd     ,s
        addd    2,s
        cmpd    #192
        ble     @yh
        ldd     #192
        subd    ,s
        std     2,s
@yh
        ldd     2,s
        lble    @ret
; a character's 8 colour pixels stand for the Atari's 8 clocks (6 here):
; centred on them. cpx(x) = x - x / 4 + 2
        ldb     4,s
        lslb
        leax    _s_x,pcr
        ldd     b,x
        std     8,s
        asra
        rorb
        asra
        rorb
        std     10,s
        ldd     8,s
        subd    10,s
        addd    #2
        tfr     d,x
        lda     3,u
        cmpa    #8
        bne     @w8
        leax    -1,x
@w8
        stx     8,s
        ldb     3,u
        decb
        clra
        addd    8,s
        std     10,s
        lbmi    @ret
        ldd     8,s
        cmpd    #127
        lbgt    @ret
        lda     #1
        sta     18,y
        ldd     8,s
        std     2,y
        bpl     @l0
        clra
        clrb
        std     8,s
@l0
        ldd     10,s
        cmpd    #127
        ble     @r0
        ldd     #127
        std     10,s
@r0
        ldb     9,s
        lsrb
        lsrb
        stb     8,y
        ldb     11,s
        lsrb
        lsrb
        subb    8,y
        incb
        stb     9,y
        ldb     1,s
        stb     6,y
        ldb     3,u
        addb    #6
        lsrb
        lsrb
        stb     10,y
        lda     #128
        clrb
@dv
        incb
        suba    10,y
        bcc     @dv
        decb
        stb     19,y
        lda     3,s
        cmpa    19,y
        bls     @hs
        lda     19,y
@hs
        sta     7,y
        lda     2,u
        sta     11,y
        pshs    y
        ldd     10,s
        pshs    d
        lbsr    _cell_of
        leas    2,s
        ldy     ,s
        stb     4,y
        ldd     12,s
        pshs    d
        lbsr    _cell_of
        leas    2,s
        ldy     ,s
        subb    4,y
        incb
        stb     5,y
        ldb     6,s
        leax    _s_col,pcr
        ldb     b,x
        leax    _kind,pcr
        ldb     b,x
        clra
        tfr     d,x
        ldd     #1
        pshs    d
        pshs    x
        lbsr    _pat
        ldy     4,s
        stb     13,y
        ldx     ,s
        leas    4,s
        clra
        clrb
        pshs    d
        pshs    x
        lbsr    _pat
        leas    4,s
        puls    y
        stb     12,y
@ret
        leas    12,s
        puls    u
    }
}

void spr_flush(void)
{
    uint8_t y0, y1, pass, nc, ll, kk, ii;
    uint8_t cl[SPR_SLOTS];      /* the sprites redrawn, in order */
    uint8_t rc[4];
    Draw dr[SPR_SLOTS];

    if (!any_dirty)
        return;
    any_dirty = 0;
    asm {
; the changing sprites, and anything drawn after one of them over its old
; place or its new one (drawn again: its saved background would be wrong
; otherwise); s_bx.. are still the old places
        ldb     #9
@h
        leax    _s_dirty,pcr
        lda     b,x
        leax    _hit,pcr
        sta     b,x
        decb
        bpl     @h
        lda     #255
        sta     y0
        clr     y1
        clr     nc
        lda     #2              ; 2: each line at once
        sta     pass
        clr     ii
@s
        ldb     ii
        cmpb    #10
        lbhs    @pass
        leax    _hit,pcr
        tst     b,x
        lbeq    @snext
        leax    cl
        lda     nc
        stb     a,x
        inc     nc
        lda     #20
        mul
        leay    dr
        leay    d,y
        pshs    y
        ldb     ii
        clra
        pshs    d
        lbsr    _coco_src_of
        leas    2,s
        puls    y
        tst     18,y
        beq     @s2
        lda     10,y
        ldb     6,y
        lbsr    sprRing
        std     14,y
        lda     6,y
        cmpa    y0
        bhs     @s1
        sta     y0
@s1
        adda    7,y
        cmpa    y1
        bls     @s2
        sta     y1
@s2
        ldb     ii
        leax    _s_drawn,pcr
        tst     b,x
        beq     @s5
        leax    _s_st,pcr
        lda     b,x
        leax    _s_sy,pcr
        ldb     b,x
        lbsr    sprRing
        std     16,y
        ldb     ii
        lda     b,x
        cmpa    y0
        bhs     @s3
        sta     y0
@s3
        leax    _s_sh,pcr
        adda    b,x
        cmpa    y1
        bls     @s4
        sta     y1
@s4
; a line's new row must not land where an old row not put back yet is: if
; it could, the old all come off first
        tst     18,y
        beq     @s5
        leax    _s_st,pcr
        ldb     b,x
        cmpb    10,y
        bne     @s4u
        suba    6,y
        bcs     @s5
        cmpa    19,y
        bls     @s5
@s4u
        clr     pass
@s5
        ldb     ii
        leax    _s_drawn,pcr
        tst     b,x
        beq     @s6
        leax    rc
        lbsr    sprOld
        lbsr    sprClose
@s6
        tst     18,y
        beq     @snext
        leax    4,y
        lbsr    sprClose
@snext
        inc     ii
        lbra    @s
; pass 2: each line at once; 0: put back, then 1: draw
@pass
        lda     y0
        sta     ll
@line
        lda     ll
        cmpa    y1
        lbhs    @pdone
        lda     pass
        cmpa    #1
        beq     @draw
        ldb     nc
        stb     kk
@rl
        dec     kk
        bmi     @draw
        ldb     kk
        leax    cl
        ldb     b,x
        stb     ii
        leax    _s_drawn,pcr
        tst     b,x
        beq     @rl
        lda     ll
        leax    _s_sy,pcr
        suba    b,x
        leax    _s_sh,pcr
        cmpa    b,x
        bhs     @rl
        lda     #20
        mul
        leay    dr
        leay    d,y
        leax    16,y
        ldb     ii
        leay    _hit,pcr
        lda     b,y
        anda    #2              ; taken off by spr_bg: not these lines
        beq     @put
        lda     ll
        cmpa    _lift_y0,pcr
        blo     @put
        cmpa    _lift_y1,pcr
        blo     @adv
@put
        lda     ll
        lbsr    sprPut
        bra     @rl
@adv
        lbsr    sprAdv
        bra     @rl
@draw
        lda     pass
        lbeq    @next
        clr     kk
@dl
        ldb     kk
        cmpb    nc
        lbhs    @next
        inc     kk
        leax    cl
        ldb     b,x
        stb     ii
        lda     #20
        mul
        leay    dr
        leay    d,y
        tst     18,y
        beq     @dl
        lda     ll
        suba    6,y
        cmpa    7,y
        bhs     @dl
        ldb     ll
        lda     #32
        mul
        addd    #$0E00
        std     _sp_row,pcr
        addb    8,y
        adca    #0
        tfr     d,x             ; the screen at bx
        lda     9,y
        pshs    a,y
        ldb     ii
        lda     #128
        mul
        addd    #$FA00
        addb    14,y
        adca    #0
        tfr     d,y
        ldb     ,s
@c2
        lda     ,x+
        sta     ,y+
        decb
        bne     @c2
        puls    a,y
        lda     14,y
        adda    10,y
        cmpa    15,y
        blo     @w2
        clra
@w2
        sta     14,y
        ldd     ,y
        std     _sp_src,pcr
        addb    11,y
        adca    #0
        std     ,y
        lda     11,y
        sta     _sp_wb,pcr
        ldd     2,y
        andb    #3
        lslb
        stb     _sp_sh,pcr
        ldd     2,y
        asra
        rorb
        asra
        rorb
        stb     _sp_bx0,pcr
        lda     ll
        lsra
        bcs     @odd
        lda     12,y
        bra     @pt
@odd
        lda     13,y
@pt
        sta     _sp_pt,pcr
        lbsr    _spr_row
        lbra    @dl
@next
        inc     ll
        lbra    @line
@pdone
        lda     pass
        bne     @fin
        inc     pass
        lbra    @pass
; what is drawn now
@fin
        ldb     nc
        stb     kk
@f
        dec     kk
        bmi     @end
        ldb     kk
        leax    cl
        ldb     b,x
        stb     ii
        lda     #20
        mul
        leay    dr
        leay    d,y
        ldb     ii
        leax    _s_dirty,pcr
        clr     b,x
        lda     18,y
        leax    _s_drawn,pcr
        sta     b,x
        beq     @f
        lda     4,y
        leax    _s_c0,pcr
        sta     b,x
        lda     5,y
        leax    _s_cn,pcr
        sta     b,x
        lda     6,y
        leax    _s_sy,pcr
        sta     b,x
        lda     7,y
        leax    _s_sh,pcr
        sta     b,x
        lda     8,y
        leax    _s_bx,pcr
        sta     b,x
        lda     9,y
        leax    _s_n,pcr
        sta     b,x
        lda     10,y
        leax    _s_st,pcr
        sta     b,x
        bra     @f
@end
        clr     _lift_y1,pcr
    }
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
