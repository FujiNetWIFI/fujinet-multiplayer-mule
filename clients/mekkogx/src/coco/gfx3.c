/*
 * gfx3.c -- gfx.h on the CoCo 3: 320 x 192 x 16, one byte per Atari color
 * clock, so the Atari's tables are used unscaled. A pixel's nibble is its
 * C_* color; CLEAR switches between the RGB and composite palettes.
 *
 * The screen is physical blocks 0-3. Drawing maps a line's block at $C000
 * (MMU slot 6) and puts the Disk ROM back after, for DriveWire. Lines 51,
 * 102 and 153 cross a block boundary and go through a shadow line
 * (gfx3_row).
 *
 * The town is read from block 4 (town3.c); the font, palettes and plot
 * tables from the end of block 3, behind the screen (ART).
 */
#ifdef COCO3
#include <string.h>
#include "mule.h"
#include "gfxdata3.h"

#define ROM_BLK  0xFE           /* $C000's block at boot (the GIME keeps 6 bits: $3E) */

/* Over Disk BASIC's buffers and graphics page, unused once loading is
 * done; the low core starts at $1F00. */
#define SHADOW ((uint8_t *)0x1A00)
#define JOBS   ((SprJob *)0x1AA0)
#define LB     0x1C80           /* the line buffer: a line's sprites change at once */
#define SCRATCH ((uint8_t *)0x1AA0)  /* the jobs: free from spr_bg's return to bg_done */
static uint8_t sh_on, sh_blk, sh_len;
static uint16_t sh_off;

static void sh_flush(void)
{
    sh_on = 0;
    MMU6 = sh_blk;
    memcpy(WINDOW + sh_off, SHADOW, sh_len);
    MMU6 = sh_blk + 1;
    memcpy(WINDOW, SHADOW + sh_len, LINE - sh_len);
}

/* The line's first byte, its block brought to the window. */
uint8_t *gfx3_row(uint8_t line)
{
    uint16_t off;
    uint8_t blk;
    if (sh_on)
        sh_flush();
    off = (uint16_t)line << 5;
    off += off << 2;                        /* line * 160 */
    blk = (uint8_t)(off >> 13);
    off &= 0x1FFF;
    MMU6 = blk;
    if (off > 8192 - LINE) {
        sh_len = (uint8_t)(8192 - off);
        memcpy(SHADOW, WINDOW + off, sh_len);
        MMU6 = blk + 1;
        memcpy(SHADOW + sh_len, WINDOW, LINE - sh_len);
        sh_on = 1;
        sh_blk = blk;
        sh_off = off;
        return SHADOW;
    }
    return WINDOW + off;
}

#define DONE() do { if (sh_on) sh_flush(); MMU6 = ROM_BLK; } while (0)

void gfx3_done(void)
{
    DONE();
}

static void spr_reset(void);
static void unrotate_all(void);
void gfx_swap_colours(void);

static uint8_t bgc;             /* what "black" is: the background */

static uint8_t tcol(uint8_t col)
{
    return col == C_BLACK ? bgc : (col & 15);
}

uint8_t gfx3_cc(uint8_t col)
{
    return CC(tcol(col));
}

static void clear_all(uint8_t col)
{
    uint8_t blk;
    for (blk = 0; blk < 4; ++blk) {
        MMU6 = blk;
        memset(WINDOW, CC(col), blk == 3 ? 6144 : 8192);
    }
    MMU6 = ROM_BLK;
}

/* ---- set-up ------------------------------------------------------------- */
void gfx_init(void)
{
    spr_reset();
    bgc = C_BLACK;
    clear_all(C_BLACK);
    gfx_swap_colours();
    *(volatile uint8_t *)0xFF90 = 0x4C;     /* GIME modes, MMU on, vectors at $FE00 */
    *(volatile uint8_t *)0xFF98 = 0x80;     /* graphics */
    *(volatile uint8_t *)0xFF99 = 0x1E;     /* 192 lines, 160 bytes a line, 16 colors */
    *(volatile uint8_t *)0xFF9A = 0;        /* border black */
    *(volatile uint8_t *)0xFF9C = 0;
    *(volatile uint16_t *)0xFF9D = 0;       /* the screen: physical 0 */
    *(volatile uint8_t *)0xFF9F = 0;
}

void gfx_clear(void)
{
    spr_hide_all();
    spr_flush();
    bgc = C_BLACK;
    clear_all(bgc);
}

void gfx_map_frame(void)
{
    spr_hide_all();
    spr_flush();
    bgc = C_GREY;
    clear_all(bgc);
}

uint8_t gfx3_cmp = 1;           /* the composite palette is up (gfx_init's call makes it RGB) */

/* CLEAR: the other monitor's palette. */
void gfx_swap_colours(void)
{
    gfx3_cmp ^= 1;
    MMU6 = ART_BLK;
    memcpy((void *)0xFFB0, ART(gfx3_cmp ? c3_pal_cmp : c3_pal), 16);
    MMU6 = ROM_BLK;
}

/* ---- text --------------------------------------------------------------- */
static const uint8_t *glyph_of(char chr)
{
    if (chr >= 'a' && chr <= 'z')
        chr -= 32;
    if (chr < 32 || chr > 95)
        chr = '?';
    return c3_font + ((uint16_t)(uint8_t)(chr - 32) << 3);
}

/* cnt characters from cell (tx, ty), a line at a time: a pixel a
 * bit (wide 0, 4 bytes a character) or a clock a bit (wide 1, 8 bytes). */
static void glyphs(uint8_t tx, uint8_t ty, const char *str, uint8_t cnt, uint8_t fgcol,
                   uint8_t bgcol, uint8_t wide)
{
    uint8_t *gly = SCRATCH;
    uint8_t tbl[4], row, idx, bits;
    uint8_t *dst;
    uint8_t line = ty << 3;
    MMU6 = ART_BLK;
    for (idx = 0; idx < cnt; ++idx)
        memcpy(gly + ((uint16_t)idx << 3), ART(glyph_of(str[idx])), 8);
    tbl[0] = CC(bgcol);
    tbl[1] = (uint8_t)(bgcol << 4) | fgcol;
    tbl[2] = (uint8_t)(fgcol << 4) | bgcol;
    tbl[3] = CC(fgcol);
    for (row = 0; row < 8; ++row, ++line) {
        dst = gfx3_row(line) + ((uint16_t)tx << 2);
        for (idx = 0; idx < cnt; ++idx) {
            bits = gly[((uint16_t)idx << 3) + row];
            if (wide) {
                uint8_t bit;
                for (bit = 0x80; bit; bit >>= 1)
                    *dst++ = (bits & bit) ? tbl[3] : tbl[0];
            } else {
                *dst++ = tbl[bits >> 6];
                *dst++ = tbl[(bits >> 4) & 3];
                *dst++ = tbl[(bits >> 2) & 3];
                *dst++ = tbl[bits & 3];
            }
        }
    }
}

void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    uint8_t len = 0;
    if (ty > 23 || tx >= TCOLS)
        return;
    while (len < n && s[len] && tx + len < TCOLS)
        ++len;
    if (len) {
        spr_bg(tx << 2, len << 2, ty << 3, 8);
        glyphs(tx, ty, s, len, tcol(col), bgc, 0);
        bg_done();
        DONE();
    }
}

void gfx_text(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_textn(tx, ty, s, TCOLS, col);
}

void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col)
{
    gfx_textn(tx, ty, &c, 1, col);
}

void gfx_clear_text(uint8_t tx, uint8_t ty, uint8_t w)
{
    if (tx >= TCOLS || ty > 23)
        return;
    if (tx + w > TCOLS)
        w = TCOLS - tx;
    gfx_fill(tx << 2, ty << 3, w << 2, 8, C_BLACK);
}

/* Big text: each pixel of the glyph a color clock, two cells a character. */
void gfx_bign(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    uint8_t len = 0;
    tx &= 0xFE;
    if (ty > 23 || tx >= TCOLS)
        return;
    while (len < n && s[len] && tx + 2 * len < TCOLS)
        ++len;
    if (len) {
        spr_bg(tx << 2, len << 3, ty << 3, 8);
        glyphs(tx, ty, s, len, tcol(col), bgc, 1);
        bg_done();
        DONE();
    }
}

void gfx_big(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_bign(tx, ty, s, TCOLS / 2, col);
}

/* ---- rectangles --------------------------------------------------------- */
void gfx_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
    uint8_t clk = CC(tcol(col));
    if (x >= LINE || y >= 192 || !w || !h)
        return;
    if (x + w > LINE)
        w = LINE - x;
    if (y + h > 192)
        h = 192 - y;
    spr_bg(x, w, y, h);
    for (; h; --h, ++y)
        memset(gfx3_row(y) + x, clk, w);
    bg_done();
    DONE();
}

/* 8 x 8 clocks, each row shown on two lines: the picture's clocks in col,
 * the rest cleared. */
void gfx_pic(uint8_t x, uint8_t y, uint8_t pic, uint8_t col)
{
    uint8_t fgclk = CC(tcol(col)), bgclk = CC(bgc), row, bits, bit;
    uint8_t *dst, *pic8 = SCRATCH;
    if (x > LINE - 8 || y > 192 - 16)
        return;
    spr_bg(x, 8, y, 16);
    MMU6 = ART_BLK;
    memcpy(pic8, ART(c3_pics[pic]), 8);
    for (row = 0; row < 16; ++row, ++y) {
        dst = gfx3_row(y) + x;
        bits = pic8[row >> 1];
        for (bit = 0x80; bit; bit >>= 1)
            *dst++ = (bits & bit) ? fgclk : bgclk;
    }
    bg_done();
    DONE();
}

/* ---- the map ------------------------------------------------------------ */
static const uint8_t zeros[64];

/* plot_parts[idx], fetched from behind the screen into dst */
static const uint8_t *part(uint8_t idx, uint8_t *dst)
{
    MMU6 = ART_BLK;
    memcpy(dst, ART(plot_parts[idx]), 64);
    return dst;
}

/* A plot: 16 clocks x 32 lines from three 1-bit layers (32 lines of 2
 * bytes), each clock the first it is in: the owner's (border, icon,
 * dots), white or black (the store, mountains), the river; else the
 * plains. */
void gfx_plot(uint8_t p, const PlotLook *look)
{
    const uint8_t *own1 = zeros, *own2 = zeros, *own3 = zeros, *white = zeros, *river = zeros;
    uint8_t dots[64], pdots[16];
    uint8_t *dst;
    uint8_t idx, pos, msk, row, half, bit, own, wht, riv, wcol, ocol, rcol, bgclk, left, line;

    left = PLOT_X(p);
    line = PLOT_Y(p);
    spr_bg(left, 16, line, 32);         /* (before the parts: its jobs use SCRATCH) */
    if (look->store) {
        white = part(PP_STORE, SCRATCH);
    } else {
        if (look->owned) {
            own1 = part(PP_BORDER, SCRATCH + 64);
            if (look->icon)
                own2 = part(PP_ICON + look->icon - 1, SCRATCH + 128);
            if (look->dots) {
                memset(dots, 0, 64);
                MMU6 = ART_BLK;
                memcpy(pdots, ART(plot_dots), 16);
                for (idx = 0; idx < look->dots && idx < 8; ++idx) {
                    pos = pdots[idx << 1] * 2 + (pdots[(idx << 1) + 1] >> 3);
                    msk = (uint8_t)(0x80 >> (pdots[(idx << 1) + 1] & 7));
                    dots[pos] |= msk;
                    dots[pos + 2] |= msk;
                }
                own3 = dots;
            }
        }
        if (look->mountains)
            white = part(PP_MOUNT + look->mountains - 1, SCRATCH);
        if (look->river)
            river = part(PP_RIVER, SCRATCH + 192);
    }
    wcol = (look->mountains && !look->store) ? CC(C_BLACK) : CC(C_WHITE);
    ocol = CC(tcol(look->col));
    rcol = CC(C_LTBLUE);
    bgclk = CC(bgc);
    for (row = 0; row < 32; ++row, ++line) {
        dst = gfx3_row(line) + left;
        for (half = 0; half < 2; ++half) {
            own = *own1++ | *own2++ | *own3++;
            wht = *white++;
            riv = *river++;
            for (bit = 0x80; bit; bit >>= 1)
                *dst++ = (own & bit) ? ocol : (wht & bit) ? wcol : (riv & bit) ? rcol : bgclk;
        }
    }
    bg_done();
    DONE();
}

/* ---- sprites -------------------------------------------------------------
 * The PCjr's engine (src/msdos/gfx.c). A drawn slot has saved bytes
 * byt..byt+len-1 of lines top..top+hgt-1 in a ring of rows (line y is row
 * y % rows), so a small move finds its old row in place. A flush does each
 * line at once, so the beam never sees half a sprite: one sprite straight
 * to the screen (sl3_solo), several through a line buffer. A flush that
 * only moves sprites one clock left slides them (slide_flush).
 */
#define SAVE_MAX 512            /* the plot cursor: 16 x 32; a hull: 32 x 16 */
#define SAVE ((uint8_t *)0x0600)    /* 10 x 512, to $1A00 */

static uint8_t s_on[SPR_SLOTS], s_shape[SPR_SLOTS], s_frame[SPR_SLOTS];
static uint8_t s_col[SPR_SLOTS], s_flags[SPR_SLOTS];
static int16_t s_xpos[SPR_SLOTS], s_ypos[SPR_SLOTS];
static uint8_t s_dirty[SPR_SLOTS];  /* 1: needs (re)drawing, 2: spr_bg took it off */
static uint8_t s_drawn[SPR_SLOTS];  /* on the screen: the rectangle is valid */
static uint8_t s_byt[SPR_SLOTS], s_len[SPR_SLOTS], s_top[SPR_SLOTS], s_hgt[SPR_SLOTS];
static uint8_t s_stride[SPR_SLOTS];         /* the ring's row length when drawn */
static uint8_t s_same[SPR_SLOTS];           /* asked for as drawn but a clock to the left: a slide */
static const uint8_t *s_dlist[SPR_SLOTS];   /* its slide list from its first line, or 0 */
static uint16_t s_dnoff[SPR_SLOTS];         /* its first line's ring offset */
static uint8_t s_anc[SPR_SLOTS];            /* slides keep column p at (p - anchor) & (stride - 1) */
static uint8_t any_dirty, bg_took;
static uint8_t lift_y0, lift_y1;        /* lines spr_bg took sprites off */
static uint8_t drawn_lo, drawn_hi;  /* the lines drawn sprites are on */
static uint8_t hit[SPR_SLOTS];

typedef struct {
    uint8_t *save;              /* its save buffer */
    const uint8_t *src;         /* the picture's next row */
    int16_t xpos;               /* its left clock (either edge may be off) */
    uint16_t noff, nstr, nend;  /* the new rows' ring: offset, row, end */
    uint16_t ooff, ostr, oend;  /* the old rows' */
    uint8_t show, top, hgt, byt, len, rowb, pix;    /* where it goes */
    uint8_t drawn, otop, ohgt, obyt, olen, lifted;  /* where it was */
    uint8_t same;               /* alone, in the same bytes: a line put back
                                 * (put: the last) is already saved */
    uint8_t put, slot;
    /* sl3_solo: the old row's background moves within the save row
     * (ovd <- ovs, ovn bytes, forwards or not); uncovered bytes: left ulx,
     * uln, right urx from save offset urs, urn; newly covered: left byt,
     * cln, right crx to save offset crd, crn; solo: the rows match.
     * slide_flush reuses several of these, and ooff. */
    uint8_t solo, ulx, uln, urx, urs, urn, ovd, ovs, ovn, fwd, cln, crx, crd, crn;
} SprJob;
/* JOBS: SPR_SLOTS of them (spr_bg's and spr_flush's are never live together) */
static SprJob *jobp[SPR_SLOTS];

static void spr_reset(void)
{
    memset(s_on, 0, SPR_SLOTS);
    memset(s_dirty, 0, SPR_SLOTS);
    memset(s_drawn, 0, SPR_SLOTS);
    any_dirty = bg_took = 0;
    drawn_lo = 255;
    drawn_hi = 0;
    sh_on = 0;
}

void spr_set(uint8_t slot, uint8_t shape, uint8_t frame, int16_t x, int16_t y,
             uint8_t col, uint8_t flags)
{
    if (s_on[slot] && s_xpos[slot] == x && s_ypos[slot] == y && s_frame[slot] == frame &&
        s_shape[slot] == shape && s_col[slot] == col && s_flags[slot] == flags)
        return;
    s_same[slot] = !s_dirty[slot] && s_on[slot] && s_xpos[slot] == x + 1 && s_ypos[slot] == y &&
                   s_frame[slot] == frame && s_shape[slot] == shape && s_col[slot] == col &&
                   s_flags[slot] == flags;
    s_on[slot] = 1;
    s_shape[slot] = shape;
    s_frame[slot] = frame;
    s_xpos[slot] = x;
    s_ypos[slot] = y;
    s_col[slot] = col;
    s_flags[slot] = flags;
    s_dirty[slot] |= 1;
    any_dirty = 1;
}

void spr_hide(uint8_t slot)
{
    if (!s_on[slot])
        return;
    s_on[slot] = 0;
    s_dirty[slot] |= 1;
    any_dirty = 1;
}

void spr_hide_all(void)
{
    uint8_t slot;
    for (slot = 0; slot < SPR_SLOTS; ++slot)
        spr_hide(slot);
}

/* A line's offset in a ring of stride-byte rows: every shape is 8, 16 or 32
 * wide, so the rows divide SAVE_MAX and the ring ends there. */
static uint16_t ring(uint8_t stride, uint8_t line)
{
    return ((uint16_t)line * stride) & (SAVE_MAX - 1);
}

static uint8_t rows_of(uint8_t stride)
{
    return stride == 8 ? 64 : stride == 16 ? 32 : 16;
}

/* Lines top..bot-1 of the njob jobs at jobp[]: pass 0 puts old rows back
 * (last drawn first), pass 1 saves and draws new ones, pass 2 does both.
 * The asm uses SprJob's field offsets; rows are copied six bytes at a time
 * with PULU/PSHS, interrupts off. */
uint8_t sl_nc, sl_y0, sl_y1, sl_pass, sl_cur, sl_pt, sl_wb, sl_tmp, sl_ctr, sl_cc;
uint8_t sl_lb, sl_lo, sl_hi, sl_len, sl_fn, sl_fo, sl_adv, sl_cnt, sl_hit, sl_anc, sl_msk;
uint8_t *sl_row, *sl_dst, *sl_end, *sl_scr, *sl_rr, *sl_rb, *sl_job, *sl_job0;
uint16_t sl_off, sl_stk;
int16_t sl_xpos, sl_px;
typedef char sprjob_is_48_bytes[sizeof(SprJob) == 48 ? 1 : -1];

static void solo_prep(SprJob *job)
{
    uint8_t newl = job->byt, newr = newl + job->len, oldl = job->obyt, oldr = oldl + job->olen, ovl, ovr;
    job->solo = job->show && job->drawn && job->nstr == job->ostr;
    if (!job->solo)
        return;
    job->ulx = oldl;
    job->uln = oldl < newl ? (newl < oldr ? newl : oldr) - oldl : 0;
    job->urx = newr > oldl ? newr : oldl;
    job->urs = job->urx - oldl;
    job->urn = newr < oldr ? oldr - job->urx : 0;
    ovl = newl > oldl ? newl : oldl;
    ovr = newr < oldr ? newr : oldr;
    job->ovn = ovr > ovl && newl != oldl ? ovr - ovl : 0;
    job->ovd = ovl - newl;
    job->ovs = ovl - oldl;
    job->fwd = newl > oldl;
    job->cln = newl < oldl ? (oldl < newr ? oldl : newr) - newl : 0;
    job->crx = oldr > newl ? oldr : newl;
    job->crd = job->crx - newl;
    job->crn = oldr < newr ? newr - job->crx : 0;
}


#ifdef MULE_GFXTEST
/* gfxtest.c: lines slid, and GIME timer ticks (4 scan lines each) in the
 * flushes */
uint16_t ct_slide, pf_flush;
uint16_t pf_now(void);
#define COUNT(var) lbsr cnt##var
#else
#define COUNT(var) ;    /* (not empty: a blank line ends lwasm's @label scope) */
#endif

static void lines(uint8_t njob, uint8_t top, uint8_t bot, uint8_t pass)
{
    sl_nc = njob;
    sl_y0 = top;
    sl_y1 = bot;
    sl_pass = pass;
    sl_end = (uint8_t *)&jobp[njob];
    if (pass == 2)
        for (sl_cur = 0; sl_cur < njob; ++sl_cur)
            solo_prep(jobp[sl_cur]);
    asm {
        pshs    u
        lda     _sl_y0
        sta     _sl_cur
        lbra    sl3_line
sl3_next
        tst     _sh_on          ; a straddling line goes back in its turn
        beq     @nosh
        lbsr    _sh_flush
@nosh
        tst     _sl_lb          ; the line buffer back to the screen
        beq     @nx
        ldb     _sl_lo
        clra
        ldu     #LB
        leau    d,u
        addd    _sl_scr
        std     _sl_dst
        ldb     _sl_len
        lbsr    sl3_copy
@nx
        inc     _sl_cur
sl3_line
        lda     _sl_cur
        cmpa    _sl_y1
        blo     @go
        puls    u
        lbra    sl3_end
@go
        clr     _sl_lb
        ldb     _sl_cur         ; the row: y * 160, its block to the window
        lda     #160
        mul
        std     _sl_off
        lsra
        lsra
        lsra
        lsra
        lsra
        sta     $FFA6
        ldd     _sl_off
        anda    #$1F
        cmpd    #8192-160
        bhi     @xblk           ; it crosses into the next block: gfx3_row
        addd    #$C000
        std     _sl_row
        bra     @rowok
@xblk
        ldb     _sl_cur
        clra
        pshs    b,a
        lbsr    _gfx3_row
        leas    2,s
        std     _sl_row
        lbra    @work           ; (a straddling line is in the shadow already)
@rowok
        std     _sl_scr
        lda     _sl_pass        ; one job, each line at once: straight onto the line
        cmpa    #2
        bne     @span
        lda     _sl_nc
        cmpa    #1
        bne     @span
        ldx     _jobp
        tst     34,x            ; solo
        beq     @span
        lbsr    sl3_solo
        lbra    sl3_next
@span                           ; the jobs on this line: their bytes lo..hi-1
        lda     #160
        sta     _sl_lo
        clr     _sl_hi
        clr     _sl_cnt
        ldy     #_jobp
@sp
        cmpy    _sl_end
        bhs     @spdone
        ldx     ,y++
        clr     _sl_hit
        lda     _sl_pass
        cmpa    #1
        beq     @spnew
        tst     25,x            ; drawn
        beq     @spnew
        lda     _sl_cur
        suba    26,x
        cmpa    27,x
        bhs     @spnew
        inc     _sl_hit
        lda     28,x            ; obyt
        cmpa    _sl_lo
        bhs     @o1
        sta     _sl_lo
@o1
        adda    29,x            ; obyt + olen
        cmpa    _sl_hi
        bls     @spnew
        sta     _sl_hi
@spnew
        tst     _sl_pass
        beq     @spcnt
        tst     18,x            ; show
        beq     @spcnt
        lda     _sl_cur
        suba    19,x
        cmpa    20,x
        bhs     @spcnt
        inc     _sl_hit
        lda     21,x            ; byt
        cmpa    _sl_lo
        bhs     @n1
        sta     _sl_lo
@n1
        adda    22,x            ; byt + len
        cmpa    _sl_hi
        bls     @spcnt
        sta     _sl_hi
@spcnt
        tst     _sl_hit
        beq     @sp
        inc     _sl_cnt
        stx     _sl_job
        bra     @sp
@spdone
        lda     _sl_hi
        suba    _sl_lo
        lbls    sl3_next        ; nothing on this line
        sta     _sl_len
        lda     _sl_pass        ; one job on the line: straight onto it
        cmpa    #2
        bne     @spcopy
        lda     _sl_cnt
        cmpa    #1
        bne     @spcopy
        ldx     _sl_job
        tst     34,x            ; solo
        beq     @spcopy
        lbsr    sl3_solo
        lbra    sl3_next
@spcopy
        ldb     _sl_lo          ; the span into the line buffer
        clra
        ldu     _sl_scr
        leau    d,u
        addd    #LB
        std     _sl_dst
        ldb     _sl_len
        lbsr    sl3_copy
        ldd     #LB
        std     _sl_row
        lda     #1
        sta     _sl_lb
@work
        lda     _sl_pass
        cmpa    #1
        lbeq    sl3_draws
        ldy     _sl_end         ; put back, the last job first
sl3_put
        cmpy    #_jobp
        lbls    sl3_draws
        ldx     ,--y
        tst     25,x            ; drawn
        beq     sl3_put
        lda     _sl_cur
        suba    26,x            ; otop
        cmpa    27,x            ; ohgt
        bhs     sl3_put
        tst     30,x            ; lifted: back already on the lines spr_bg did
        beq     @do
        lda     _sl_cur
        cmpa    _lift_y0
        blo     @do
        cmpa    _lift_y1
        blo     @adv
@do
        lda     _sl_cur
        sta     32,x            ; put
        ldu     ,x              ; from save + ooff
        ldd     12,x
        leau    d,u
        ldd     _sl_row         ; to row + obyt
        addb    28,x
        adca    #0
        std     _sl_dst
        ldb     29,x            ; olen
        lbsr    sl3_copy
@adv
        ldd     12,x
        addd    14,x            ; ooff += ostr, round the ring
        cmpd    16,x
        blo     @st
        ldd     #0
@st
        std     12,x
        lbra    sl3_put
sl3_draws
        tst     _sl_pass
        lbeq    sl3_next
        ldy     #_jobp
sl3_drw
        cmpy    _sl_end
        lbhs    sl3_next
        ldx     ,y++
        tst     18,x            ; show
        beq     sl3_drw
        lda     _sl_cur
        suba    19,x            ; top
        cmpa    20,x            ; hgt
        bhs     sl3_drw
        tst     31,x            ; same: a line just put back is saved already
        beq     @save
        lda     _sl_cur
        cmpa    32,x
        beq     @saved
@save
        ldu     _sl_row         ; from row + byt
        ldb     21,x
        clra
        leau    d,u
        ldd     ,x              ; to save + noff
        addd    6,x
        std     _sl_dst
        ldb     22,x            ; len
        lbsr    sl3_copy
@saved
        ldd     6,x
        addd    8,x             ; noff += nstr, round the ring
        cmpd    10,x
        blo     @dst
        ldd     #0
@dst
        std     6,x
        lbsr    sl3_draw
        lbra    sl3_drw
; Job X alone on the line, pass 2 (solo_prep): move the old background within
; the save row, restore uncovered bytes, save newly covered ones, then draw
; once with the background. Y and U are free.
sl3_solo
        clr     _sl_fn
        tst     18,x            ; show, on this line
        beq     @so1
        lda     _sl_cur
        suba    19,x
        cmpa    20,x
        bhs     @so1
        inc     _sl_fn
@so1
        clr     _sl_fo
        clr     _sl_adv
        tst     25,x            ; drawn, on this line
        beq     @so2
        lda     _sl_cur
        suba    26,x
        cmpa    27,x
        bhs     @so2
        inc     _sl_adv
        inc     _sl_fo
        tst     30,x            ; lifted: spr_bg put it back, the background is new
        beq     @so2
        lda     _sl_cur
        cmpa    _lift_y0
        blo     @so2
        cmpa    _lift_y1
        bhs     @so2
        clr     _sl_fo
@so2
        tst     _sl_fo
        lbeq    @nold
        tst     _sl_fn
        lbeq    @putonly
        ldu     ,x              ; R = save + ooff (= save + noff)
        ldd     12,x
        leau    d,u
        stu     _sl_rr
        clra                    ; (offsets in D: A as an offset is signed)
        ldb     36,x          ; uncovered on the left
        beq     @sv1
        ldy     _sl_row
        ldb     35,x
        leay    d,y
        ldb     36,x
        lbsr    sl3_few
@sv1
        clra
        ldb     39,x          ; uncovered on the right
        beq     @sv2
        ldu     _sl_rr
        ldb     38,x
        leau    d,u
        ldy     _sl_row
        ldb     37,x
        leay    d,y
        ldb     39,x
        lbsr    sl3_few
@sv2
        clra
        ldb     42,x          ; the overlap moves
        beq     @sv3
        ldu     _sl_rr
        ldb     41,x
        leau    d,u
        ldy     _sl_rr
        ldb     40,x
        leay    d,y
        ldb     42,x
        tst     43,x
        bne     @sv2f
        sty     _sl_dst
        lbsr    sl3_copy        ; (from the end back)
        bra     @sv3
@sv2f
        lbsr    sl3_fwd
@sv3
        clra
        ldb     44,x         ; newly covered on the left
        beq     @sv3b
        ldu     _sl_row
        ldb     21,x
        leau    d,u
        ldy     _sl_rr
        ldb     44,x
        lbsr    sl3_few
@sv3b
        clra
        ldb     47,x         ; newly covered on the right
        beq     @sv4
        ldu     _sl_row
        ldb     45,x
        leau    d,u
        ldy     _sl_rr
        ldb     46,x
        leay    d,y
        ldb     47,x
        lbsr    sl3_few
@sv4
        lbsr    sl3_drawbg
        lbra    sl3_soadv
; B (1..) bytes from U to Y (X kept)
sl3_few
        lda     ,u+
        sta     ,y+
        decb
        bne     sl3_few
        rts
@putonly
        ldu     ,x              ; gone from this line: its old row back
        ldd     12,x
        leau    d,u
        ldd     _sl_row
        addb    28,x
        adca    #0
        std     _sl_dst
        ldb     29,x
        lbsr    sl3_copy
        lbra    sl3_soadv
@nold
        tst     _sl_fn
        lbeq    sl3_soadv
        ldu     _sl_row         ; new here: save what is under it, draw over
        ldb     21,x
        clra
        leau    d,u
        ldd     ,x
        addd    6,x
        std     _sl_dst
        ldb     22,x
        lbsr    sl3_copy
        lbsr    sl3_draw
sl3_soadv
        tst     _sl_adv         ; the rings move on
        beq     @sa1
        ldd     12,x
        addd    14,x
        cmpd    16,x
        blo     @sa0
        ldd     #0
@sa0
        std     12,x
@sa1
        tst     _sl_fn
        beq     @sa3
        ldd     6,x
        addd    8,x
        cmpd    10,x
        blo     @sa2
        ldd     #0
@sa2
        std     6,x
@sa3
        rts
; B bytes from U to Y, forwards (X kept)
sl3_fwd
        lsrb
        bcc     @fw2
        lda     ,u+
        sta     ,y+
@fw2
        tstb
        beq     @fwd
        stb     _sl_ctr
@fwl
        ldd     ,u++
        std     ,y++
        dec     _sl_ctr
        bne     @fwl
@fwd
        rts
; the picture row of job X onto the line, the background from the save
; row at sl_rr where a clock is not lit (X kept)
sl3_drawbg
        ldu     2,x             ; src
        lda     24,x
        sta     _sl_pt
        ldb     23,x
        stb     _sl_wb
        ldd     4,x
        std     _sl_xpos
        lbmi    @dbslow
        lda     #8
        ldb     23,x
        mul
        addd    _sl_xpos
        cmpd    #160
        lbhi    @dbslow
        pshs    x               ; all on the screen: Y the line, X the background
        ldy     _sl_row
        ldd     _sl_xpos
        leay    d,y
        ldx     _sl_rr
@dbb
        ldb     ,u+
        lbeq    @dbz
        cmpb    #$FF
        lbeq    @dbf
        lda     _sl_pt
        lslb
        bcc     @u0
        sta     ,y
        bra     @l1
@u0
        lda     ,x
        sta     ,y
        lda     _sl_pt
@l1
        lslb
        bcc     @u1
        sta     1,y
        bra     @l2
@u1
        lda     1,x
        sta     1,y
        lda     _sl_pt
@l2
        lslb
        bcc     @u2
        sta     2,y
        bra     @l3
@u2
        lda     2,x
        sta     2,y
        lda     _sl_pt
@l3
        lslb
        bcc     @u3
        sta     3,y
        bra     @l4
@u3
        lda     3,x
        sta     3,y
        lda     _sl_pt
@l4
        lslb
        bcc     @u4
        sta     4,y
        bra     @l5
@u4
        lda     4,x
        sta     4,y
        lda     _sl_pt
@l5
        lslb
        bcc     @u5
        sta     5,y
        bra     @l6
@u5
        lda     5,x
        sta     5,y
        lda     _sl_pt
@l6
        lslb
        bcc     @u6
        sta     6,y
        bra     @l7
@u6
        lda     6,x
        sta     6,y
        lda     _sl_pt
@l7
        lslb
        bcc     @u7
        sta     7,y
        bra     @dbn
@u7
        lda     7,x
        sta     7,y
        bra     @dbn
@dbz                            ; nothing lit: the background
        ldd     ,x
        std     ,y
        ldd     2,x
        std     2,y
        ldd     4,x
        std     4,y
        ldd     6,x
        std     6,y
        bra     @dbn
@dbf                            ; all lit: the color
        lda     _sl_pt
        sta     ,y
        sta     1,y
        sta     2,y
        sta     3,y
        sta     4,y
        sta     5,y
        sta     6,y
        sta     7,y
@dbn
        leax    8,x
        leay    8,y
        dec     _sl_wb
        lbne    @dbb
        puls    x
        stu     2,x
        rts
@dbslow                         ; an edge off: a clock at a time
        pshs    x
        ldd     _sl_rr          ; the background, indexed by the clock
        subb    21,x
        sbca    #0
        std     _sl_rb
        ldd     _sl_xpos
        std     _sl_px
@dsb
        ldb     ,u+
        stb     _sl_tmp
        lda     #8
        sta     _sl_ctr
@dsbit
        ldd     _sl_px
        bmi     @dsn
        cmpd    #160
        bhs     @dsn
        ldy     _sl_row
        leay    d,y
        ldx     _sl_rb
        lsl     _sl_tmp
        bcc     @dsbg
        lda     _sl_pt
        bra     @dsst
@dsbg
        lda     d,x
@dsst
        sta     ,y
        bra     @dsn2
@dsn
        lsl     _sl_tmp
@dsn2
        ldd     _sl_px
        addd    #1
        std     _sl_px
        dec     _sl_ctr
        bne     @dsbit
        dec     _sl_wb
        bne     @dsb
        puls    x
        stu     2,x
        rts
; B bytes from U to sl_dst (X and Y kept)
sl3_copy
        pshs    x,y
        cmpb    #8
        lbeq    @b8
        lblo    @few
        cmpb    #32
        lbeq    @b32
        stb     _sl_tmp           ; any count: 6 bytes a go, then the rest
        tfr     cc,a
        sta     _sl_cc
        orcc    #$50
        sts     _sl_stk
        clra
        leau    d,u
        addd    _sl_dst
        tfr     d,s
        lda     #171            ; count / 6 (exact up to 160)
        ldb     _sl_tmp
        mul
        lsra
        lsra
        sta     _sl_ctr
        ldb     #6
        mul
        negb
        addb    _sl_tmp
        stb     _sl_tmp           ; the rest, under 6
        tst     _sl_ctr
        beq     @c1
        leau    -6,u
@c6
        pulu    d,x,y
        pshs    d,x,y
        leau    -12,u
        dec     _sl_ctr
        bne     @c6
        leau    6,u
@c1
        tst     _sl_tmp
        lbeq    @bdone
@cl
        lda     ,-u
        pshs    a
        dec     _sl_tmp
        bne     @cl
        lbra    @bdone
@b32                            ; whole rows: from the end back, 6 bytes a go
        tfr     cc,a
        sta     _sl_cc
        orcc    #$50
        sts     _sl_stk
        ldd     _sl_dst
        addd    #32
        tfr     d,s
        leau    26,u
        pulu    d,x,y
        pshs    d,x,y
        leau    -12,u
        pulu    d,x,y
        pshs    d,x,y
        leau    -12,u
        pulu    d,x,y
        pshs    d,x,y
        leau    -12,u
        pulu    d,x,y
        pshs    d,x,y
        leau    -12,u
        pulu    d,x,y
        pshs    d,x,y
        leau    -8,u
        pulu    d
        pshs    d
        lbra    @bdone
@b8
        tfr     cc,a
        sta     _sl_cc
        orcc    #$50
        sts     _sl_stk
        ldd     _sl_dst
        addd    #8
        tfr     d,s
        leau    2,u
        pulu    d,x,y
        pshs    d,x,y
        leau    -8,u
        pulu    d
        pshs    d
@bdone
        lds     _sl_stk
        lda     _sl_cc
        tfr     a,cc
        puls    x,y,pc
@few                            ; under 8: a byte at a time, from the end
        ldy     _sl_dst
        leay    b,y
        leau    b,u
@fl
        lda     ,-u
        sta     ,-y
        decb
        bne     @fl
        puls    x,y,pc
; the picture row of job X on the line: lit clocks become its color (Y kept)
sl3_draw
        pshs    y
        ldu     2,x             ; src
        lda     24,x
        sta     _sl_pt
        ldb     23,x
        stb     _sl_wb
        ldd     4,x
        std     _sl_xpos
        lbmi    @slow
        lda     #8
        ldb     23,x
        mul
        addd    _sl_xpos
        cmpd    #160
        lbhi    @slow
        ldy     _sl_row         ; all on the screen: 8 clocks a source byte
        ldd     _sl_xpos
        leay    d,y
@fb
        ldb     ,u+
        beq     @fnext
        lda     _sl_pt
        lslb
        bcc     @f1
        sta     ,y
@f1
        lslb
        bcc     @f2
        sta     1,y
@f2
        lslb
        bcc     @f3
        sta     2,y
@f3
        lslb
        bcc     @f4
        sta     3,y
@f4
        lslb
        bcc     @f5
        sta     4,y
@f5
        lslb
        bcc     @f6
        sta     5,y
@f6
        lslb
        bcc     @f7
        sta     6,y
@f7
        lslb
        bcc     @fnext
        sta     7,y
@fnext
        leay    8,y
        dec     _sl_wb
        bne     @fb
        bra     @dend
@slow                           ; an edge off: a clock at a time
        pshs    x
        ldd     _sl_xpos
        std     _sl_px
@sb
        ldb     ,u+
        stb     _sl_tmp
        lda     #8
        sta     _sl_ctr
@sbit
        lsl     _sl_tmp
        bcc     @snext
        ldd     _sl_px
        bmi     @snext
        cmpd    #160
        bhs     @snext
        ldx     _sl_row
        leax    d,x
        lda     _sl_pt
        sta     ,x
@snext
        ldd     _sl_px
        addd    #1
        std     _sl_px
        dec     _sl_ctr
        bne     @sbit
        dec     _sl_wb
        bne     @sb
        puls    x
@dend
        puls    y
        stu     2,x             ; the next row
        rts
#ifdef MULE_GFXTEST
cnt_ct_slide
        ldd     _ct_slide
        addd    #1
        std     _ct_slide
        rts
#endif
sl3_end
    }
}

/* Into job: where the drawn sprite in slot is, from line on. */
static void old_job(uint8_t slot, uint8_t line, SprJob *job)
{
    job->save = SAVE + (uint16_t)slot * SAVE_MAX;
    job->drawn = 1;
    job->otop = s_top[slot];
    job->ohgt = s_hgt[slot];
    job->obyt = s_byt[slot];
    job->olen = s_len[slot];
    job->ostr = s_stride[slot];
    job->oend = SAVE_MAX;
    job->ooff = ring(s_stride[slot], line > job->otop ? line : job->otop);
    job->lifted = 0;
    job->same = 0;
    job->put = 255;
}

/* Does drawn sprite slot's rectangle meet clocks left.. (wid of them), lines top..? */
static uint8_t meets(uint8_t slot, uint8_t left, uint8_t wid, uint8_t top, uint8_t hgt)
{
    /* (no sum passes 255: 160 clocks, 192 lines) */
    if (s_byt[slot] >= (uint8_t)(left + wid) || left >= (uint8_t)(s_byt[slot] + s_len[slot]))
        return 0;
    return s_top[slot] < (uint8_t)(top + hgt) && top < (uint8_t)(s_top[slot] + s_hgt[slot]);
}

/* Mark every drawn slot after slot that meets the rectangle. */
static void close_over(uint8_t slot, uint8_t left, uint8_t wid, uint8_t top, uint8_t hgt)
{
    while (++slot < SPR_SLOTS)
        if (!hit[slot] && s_drawn[slot] && meets(slot, left, wid, top, hgt))
            hit[slot] |= 1;
}

/* Before background is drawn in a rectangle: take off the sprites on it,
 * and those drawn later over them, on its lines only; bg_done puts them back. */
void spr_bg(uint8_t left, uint8_t wid, uint8_t top, uint8_t hgt)
{
    uint8_t slot, njob = 0;
    uint16_t bot = (uint16_t)top + hgt;
    if (top >= drawn_hi || bot <= drawn_lo)
        return;                 /* (most text: nowhere near one) */
    unrotate_all();
    for (slot = 0; slot < SPR_SLOTS; ++slot)
        hit[slot] = s_drawn[slot] && meets(slot, left, wid, top, hgt);
    for (slot = 0; slot < SPR_SLOTS; ++slot)
        if (hit[slot]) {
            close_over(slot, s_byt[slot], s_len[slot], s_top[slot], s_hgt[slot]);
            jobp[njob] = &JOBS[njob];
            old_job(slot, top, &JOBS[njob++]);
            s_dirty[slot] |= 2;
            bg_took = any_dirty = 1;
        }
    lift_y0 = top;
    lift_y1 = bot > 255 ? 255 : (uint8_t)bot;
    if (njob)
        lines(njob, top, lift_y1, 0);
}

void bg_done(void)
{
    if (bg_took) {
        bg_took = 0;
        spr_flush();
    }
}

/* The frame slot shows (flipped: the mirrored frames). */
static uint8_t frame_of(uint8_t slot)
{
    uint8_t frame = s_frame[slot];
    if (s_flags[slot] & SPR_FLIPX) {
        if (s_shape[slot] == SH_MULE)
            frame += 4;
        else if (s_shape[slot] == SH_FIGURE && (frame % 10) < 2)
            frame += 2;
    }
    return frame;
}

static const uint8_t *pic_of(uint8_t slot)
{
    const ShapeDef *def = &c3_shapes[s_shape[slot]];
    return def->data + frame_of(slot) * ((uint16_t)def->rowb * def->hgt);
}

/* Slot's slide list (c3_slide) from its first line on the screen, or 0. */
static const uint8_t *list_of(uint8_t slot)
{
    const uint8_t *list = c3_slide[s_shape[slot]];
    uint8_t rows;
    if (!list)
        return 0;
    rows = frame_of(slot) * c3_shapes[s_shape[slot]].hgt;
    if (s_ypos[slot] < 0)
        rows += (uint8_t)-s_ypos[slot];
    while (rows--)
        while (*list++ != 0x7F)
            ;
    return list;
}

/* Into job: where the sprite in slot would be drawn now. */
static void src_of(uint8_t slot, SprJob *job)
{
    const ShapeDef *def = &c3_shapes[s_shape[slot]];
    uint16_t rows;
    const uint8_t *src;
    int16_t ypos = s_ypos[slot], hgt = def->hgt, left, right;

    job->save = SAVE + (uint16_t)slot * SAVE_MAX;
    job->slot = slot;
    job->show = 0;
    job->drawn = 0;
    job->lifted = 0;
    job->same = 0;
    job->put = 255;
    if (!s_on[slot])
        return;
    src = pic_of(slot);
    if (ypos < 0) {
        src += (uint8_t)(-ypos) * def->rowb;
        hgt += ypos;
        ypos = 0;
    }
    if (ypos + hgt > 192)
        hgt = 192 - ypos;
    left = s_xpos[slot];
    right = left + def->wid - 1;
    if (hgt <= 0 || right < 0 || left > LINE - 1)
        return;
    job->show = 1;
    job->src = src;
    job->xpos = left;
    if (left < 0)
        left = 0;
    if (right > LINE - 1)
        right = LINE - 1;
    job->byt = (uint8_t)left;
    job->len = (uint8_t)(right - left + 1);
    job->nstr = def->wid;
    job->nend = SAVE_MAX;
    rows = rows_of(def->wid);
    if (hgt > (int16_t)rows)
        hgt = rows;
    job->top = (uint8_t)ypos;
    job->hgt = (uint8_t)hgt;
    job->rowb = def->rowb;
    job->pix = CC(tcol(s_col[slot]));
    job->noff = ring(def->wid, job->top);
}

/* Each job's rectangle, old and new together: lines rtop..rbot-1 (within
 * top..bot-1), bytes rleft..rright-1. */
static uint8_t rtop[SPR_SLOTS], rbot[SPR_SLOTS], rleft[SPR_SLOTS], rright[SPR_SLOTS], grp[SPR_SLOTS];

static void rect_of(uint8_t idx, SprJob *job, uint8_t top, uint8_t bot)
{
    uint8_t ymin = 255, ymax = 0, xmin = 255, xmax = 0, end;
    if (job->show) {
        ymin = job->top;
        ymax = job->top + job->hgt;
        xmin = job->byt;
        xmax = job->byt + job->len;
    }
    if (job->drawn) {
        if (job->otop < ymin)
            ymin = job->otop;
        if ((end = job->otop + job->ohgt) > ymax)
            ymax = end;
        if (job->obyt < xmin)
            xmin = job->obyt;
        if ((end = job->obyt + job->olen) > xmax)
            xmax = end;
    }
    rtop[idx] = ymin < top ? top : ymin;
    rbot[idx] = ymax > bot ? bot : ymax;
    rleft[idx] = xmin;
    rright[idx] = xmax;
}

/* Slot's save rows back in line order, column byt + i at i: the slides
 * (slide_jobs) leave column p at (p - anchor) & mask. */
static void unrotate(uint8_t slot)
{
    uint8_t stride = s_stride[slot], rows, rest;
    uint8_t turn = (uint8_t)(s_byt[slot] - s_anc[slot]) & (stride - 1);
    uint8_t *row, *tmp = (uint8_t *)LB;
    uint16_t off;
    if (!turn)
        return;
    rest = stride - turn;
    off = ring(stride, s_top[slot]);
    for (rows = s_hgt[slot]; rows; --rows) {
        row = SAVE + (uint16_t)slot * SAVE_MAX + off;
        memcpy(tmp, row, stride);
        memcpy(row, tmp + turn, rest);
        memcpy(row + rest, tmp, turn);
        off += stride;
        if (off >= SAVE_MAX)
            off = 0;
    }
    s_anc[slot] = s_byt[slot];
}

static void unrotate_all(void)
{
    uint8_t slot;
    for (slot = 0; slot < SPR_SLOTS; ++slot)
        if (s_drawn[slot])
            unrotate(slot);
}

/* The lines drawn sprites are on, for spr_bg. */
static void drawn_range(void)
{
    uint8_t slot, end;
    lift_y1 = 0;
    drawn_lo = 255;
    drawn_hi = 0;
    for (slot = 0; slot < SPR_SLOTS; ++slot)
        if (s_drawn[slot]) {
            if (s_top[slot] < drawn_lo)
                drawn_lo = s_top[slot];
            if ((end = s_top[slot] + s_hgt[slot]) > drawn_hi)
                drawn_hi = end;
        }
}

/* Slide each job at jobp[] one clock left, line by line: restore the byte
 * it leaves, save the byte it covers, redraw only the clocks in its slide
 * list. Save rows stay put, column p at (p - anchor) & mask (unrotate puts
 * them back in order). A line crossing a block uses the block holding its
 * bytes, else the shadow. */
static void slide_jobs(uint8_t njob)
{
    sl_end = (uint8_t *)&jobp[njob];
    asm {
        pshs    u
        ldy     #_jobp
sj_job
        cmpy    _sl_end
        lbhs    sj_done
        ldx     ,y++
        sty     _sl_job0
        stx     _sl_job
        ldd     4,x             ; xpos
        std     _sl_xpos
        lda     24,x            ; pix
        sta     _sl_pt
        lda     37,x            ; anchor - 128, the mask
        sta     _sl_anc
        lda     38,x
        sta     _sl_msk
        lda     19,x            ; top .. top + hgt
        sta     _sl_cur
        adda    20,x
        sta     _sl_y1
        ldb     19,x            ; the first row: y * 160 as block, offset
        lda     #160
        mul
        std     _sl_off
        lsra
        lsra
        lsra
        lsra
        lsra
        sta     _sl_hi
        ldd     _sl_off
        anda    #$1F
        std     _sl_off
sj_line
        COUNT(_ct_slide)
        clr     _sl_lb
        ldd     _sl_off
        cmpd    #8192-160
        bhi     @across
@blk
        lda     _sl_hi
        sta     $FFA6
        ldd     _sl_off
        addd    #$C000
        std     _sl_row
        bra     @row
@across                         ; its bytes all before the break, or all after it?
        ldd     #8192
        subd    _sl_off         ; B: the first byte past the break
        cmpb    36,x            ; hi
        bhs     @blk
        cmpb    35,x            ; lo
        bhi     @shadow
        lda     _sl_hi
        inca
        sta     $FFA6
        ldd     _sl_off
        addd    #$A000          ; $C000 + off - 8192
        std     _sl_row
        bra     @row
@shadow
        pshs    x
        ldb     _sl_cur
        clra
        pshs    b,a
        lbsr    _gfx3_row
        leas    2,s
        puls    x
        std     _sl_row
        inc     _sl_lb
@row
        ldu     ,x              ; R = save + noff
        ldd     6,x
        leau    d,u
        stu     _sl_rr
        ldy     _sl_row
        leay    128,y           ; (positions as signed byte offsets)
        tst     39,x            ; the byte left behind: its background back
        beq     @nvac
        ldb     41,x
        lda     b,u
        ldb     46,x
        sta     b,y
@nvac
        tst     44,x            ; the byte covered: from the screen
        beq     @ncov
        ldb     47,x
        lda     b,y
        ldb     40,x
        sta     b,u
@ncov
        ldu     12,x            ; the list for the row
        ldx     _sl_rr
@tr
        ldb     ,u+
        cmpb    #$7F
        beq     @trend
        stb     _sl_tmp         ; lit in bit 7
        andb    #$7F
        clra
        addd    _sl_xpos        ; p
        bmi     @tr
        cmpd    #160
        bhs     @tr
        subb    #128
        tst     _sl_tmp
        bpl     @trbg
        lda     _sl_pt
        sta     b,y
        bra     @tr
@trbg
        tfr     b,a
        suba    _sl_anc         ; (p - anchor) & mask
        anda    _sl_msk
        lda     a,x
        sta     b,y
        bra     @tr
@trend
        ldx     _sl_job
        stu     12,x            ; (the next row follows)
@adv
        ldd     6,x             ; noff += nstr, round the ring
        addd    8,x
        cmpd    10,x
        blo     @st
        ldd     #0
@st
        std     6,x
        tst     _sl_lb
        beq     @nsh
        pshs    x
        lbsr    _sh_flush
        puls    x
@nsh
        ldd     _sl_off         ; the next row
        addd    #160
        cmpd    #8192
        blo     @same
        subd    #8192
        inc     _sl_hi
@same
        std     _sl_off
        inc     _sl_cur
        lda     _sl_cur
        cmpa    _sl_y1
        lblo    sj_line
        ldy     _sl_job0
        lbra    sj_job
sj_done
        puls    u
    }
}

/* The ship's flush: every changed sprite is its drawn picture one clock
 * left, still on the screen, with nothing else near. 0 if it does not apply. */
static uint8_t slide_flush(void)
{
    uint8_t slot, oth, idx, njob = 0, lo, hi, others = 0, anc, mask;
    int16_t xpos, right;
    const ShapeDef *def;
    SprJob *job, *nxt;

    for (slot = 0; slot < SPR_SLOTS; ++slot) {
        if (!s_dirty[slot]) {
            if (s_drawn[slot])
                others = 1;
            continue;
        }
        def = &c3_shapes[s_shape[slot]];
        if (s_dirty[slot] == 1 && s_on[slot] && !s_drawn[slot] &&
            (s_xpos[slot] + def->wid <= 0 || s_xpos[slot] >= LINE)) {
            s_dirty[slot] = 0;                  /* off the screen: nothing to draw */
            continue;
        }
        if (s_dirty[slot] != 1 || !s_same[slot] || !s_drawn[slot] || !s_dlist[slot])
            return 0;
        job = &JOBS[njob];
        jobp[njob++] = job;
        job->slot = slot;
        job->save = SAVE + (uint16_t)slot * SAVE_MAX;
        job->ooff = (uint16_t)s_dlist[slot];
        xpos = s_xpos[slot];
        job->xpos = xpos;
        job->top = s_top[slot];
        job->hgt = s_hgt[slot];
        job->pix = CC(tcol(s_col[slot]));
        job->obyt = s_byt[slot];
        job->olen = s_len[slot];
        job->nstr = s_stride[slot];
        job->nend = SAVE_MAX;
        job->noff = s_dnoff[slot];
        anc = s_anc[slot];
        mask = (uint8_t)job->nstr - 1;
        job->urx = anc - 128;
        job->urs = mask;
        right = xpos + def->wid;
        if (right <= 0)
            return 0;                           /* gone off the left: the usual way */
        job->byt = xpos < 0 ? 0 : (uint8_t)xpos;
        job->len = (uint8_t)((right > LINE ? LINE : right) - job->byt);
        job->urn = job->len != job->olen + 1;       /* a byte left behind: byt + len */
        job->ovs = (uint8_t)(job->byt + job->len - anc) & mask;
        job->crd = job->byt + job->len - 128;
        job->cln = xpos >= 0;                       /* a byte newly covered: byt */
        job->ovd = (uint8_t)(job->byt - anc) & mask;
        job->crn = job->byt - 128;
        lo = job->byt < job->obyt ? job->byt : job->obyt;
        hi = job->obyt + job->olen;
        if (job->byt + job->len > hi)
            hi = job->byt + job->len;
        job->ulx = lo;
        job->uln = hi;
    }
    if (!njob)
        return 1;
    for (idx = 1; idx < njob; ++idx) {          /* by x: the leftmost first */
        job = jobp[idx];
        for (oth = idx; oth && jobp[oth - 1]->xpos > job->xpos; --oth)
            jobp[oth] = jobp[oth - 1];
        jobp[oth] = job;
    }
    for (idx = 0; idx < njob; ++idx) {
        job = jobp[idx];
        for (oth = idx + 1; oth < njob; ++oth) {    /* apart from those on its lines, old and new */
            nxt = jobp[oth];
            if (job->top < nxt->top + nxt->hgt && nxt->top < job->top + job->hgt &&
                (job->byt + job->len > nxt->byt || job->obyt + job->olen > nxt->obyt))
                return 0;
        }
        if (others)                             /* nothing else drawn on its lines */
            for (oth = 0; oth < SPR_SLOTS; ++oth)
                if (s_drawn[oth] && !s_dirty[oth] &&
                    meets(oth, job->ulx, job->uln - job->ulx, job->top, job->hgt))
                    return 0;
    }
    slide_jobs(njob);
    for (idx = 0; idx < njob; ++idx) {
        job = jobp[idx];
        slot = job->slot;
        s_dirty[slot] = 0;
        s_byt[slot] = job->byt;
        s_len[slot] = job->len;
    }
    DONE();
    return 1;
}

static void flush_full(void)
{
    uint8_t slot, oth, idx, top, bot, pass, end, njob, part, cnt;
    SprJob *job;

    unrotate_all();
    for (slot = 0; slot < SPR_SLOTS; ++slot)
        hit[slot] = s_dirty[slot];
    /* the changing sprites, and anything drawn after one of them over its
     * old place or its new one (drawn again: its saved background would be
     * wrong otherwise). s_byt.. are still the old places. */
    top = 255;
    bot = 0;
    njob = 0;
    pass = 2;                   /* each line at once */
    part = 1;                   /* only spr_bg's lines (it put those back) */
    for (slot = 0; slot < SPR_SLOTS; ++slot) {
        if (!hit[slot])
            continue;
        if (hit[slot] != 2)
            part = 0;
        job = &JOBS[njob++];
        src_of(slot, job);
        if (job->show) {
            if (job->top < top)
                top = job->top;
            if ((end = job->top + job->hgt) > bot)
                bot = end;
        }
        if (s_drawn[slot]) {
            old_job(slot, 0, job);
            job->lifted = hit[slot] & 2;
            if (s_top[slot] < top)
                top = s_top[slot];
            if ((end = s_top[slot] + s_hgt[slot]) > bot)
                bot = end;
            /* a line's new row must not land where an old row not put back
             * yet is: if it could, the old all come off first */
            if (job->show && (job->nstr != job->ostr || end > (uint16_t)job->top + rows_of((uint8_t)job->nstr)))
                pass = 0;
            close_over(slot, s_byt[slot], s_len[slot], s_top[slot], s_hgt[slot]);
        }
        if (job->show)
            close_over(slot, job->byt, job->len, job->top, job->hgt);
    }
    if (part && njob) {
        /* nothing moved: save and draw again only what spr_bg took off */
        pass = 1;
        if (top < lift_y0)
            top = lift_y0;
        if (bot > lift_y1)
            bot = lift_y1;
        for (idx = 0; idx < njob; ++idx) {
            job = &JOBS[idx];
            if (job->show && job->top < top) {
                job->src += (uint8_t)(top - job->top) * job->rowb;
                job->noff = ring((uint8_t)job->nstr, top);
            }
        }
    }
    /* Those whose rectangles (old and new) meet are done together, a line
     * at a time, in order; the rest each alone, on its lines (a line then
     * looks only at it). */
    for (idx = 0; idx < njob; ++idx) {
        rect_of(idx, &JOBS[idx], top, bot);
        grp[idx] = idx;
        for (oth = 0; oth < idx; ++oth)
            if (rtop[oth] < rbot[idx] && rtop[idx] < rbot[oth] && rleft[oth] < rright[idx] && rleft[idx] < rright[oth] &&
                grp[oth] != grp[idx]) {
                cnt = grp[idx];
                for (slot = 0; slot <= idx; ++slot)
                    if (grp[slot] == cnt)
                        grp[slot] = grp[oth];
            }
    }
    for (idx = 0; idx < njob; ++idx) {
        if (grp[idx] != idx)
            continue;           /* (a group's first is its own) */
        cnt = 0;
        top = 255;
        bot = 0;
        for (oth = idx; oth < njob; ++oth)
            if (grp[oth] == idx && rtop[oth] < rbot[oth]) {
                jobp[cnt++] = &JOBS[oth];
                if (rtop[oth] < top)
                    top = rtop[oth];
                if (rbot[oth] > bot)
                    bot = rbot[oth];
            }
        if (cnt) {
            job = jobp[0];
            if (cnt == 1)
                job->same = job->show && job->drawn && job->byt == job->obyt && job->len == job->olen &&
                          job->nstr == job->ostr;
            lines(cnt, top, bot, pass);
            if (!pass)
                lines(cnt, top, bot, 1);
        }
    }
    for (idx = 0; idx < njob; ++idx) {
        job = &JOBS[idx];
        slot = job->slot;
        s_dirty[slot] = 0;
        if ((s_drawn[slot] = job->show) != 0) {
            s_byt[slot] = job->byt;
            s_len[slot] = job->len;
            s_top[slot] = job->top;
            s_hgt[slot] = job->hgt;
            s_stride[slot] = (uint8_t)job->nstr;
            s_dlist[slot] = list_of(slot);
            s_dnoff[slot] = ring((uint8_t)job->nstr, job->top);
            s_anc[slot] = job->byt;
        }
    }
    drawn_range();
    DONE();
}

void spr_flush(void)
{
#ifdef MULE_GFXTEST
    uint16_t start = pf_now();
#endif
    if (!any_dirty)
        return;
    any_dirty = 0;
    if (!slide_flush())
        flush_full();
#ifdef MULE_GFXTEST
    pf_flush += pf_now() - start;
#endif
}
#endif
