/*
 * vram_tms.h -- gfx.c's screen on the TMS9918A itself (the Adam, the MSX):
 * Graphics II, a cell's 8 pattern bytes at its place in the pattern table
 * and its 8 colour bytes 2000h further on; sprites 16 x 16, one colour.
 *
 * Included by gfx.c only (its cell, cp/cc/ca, bgc and text buffer are
 * this file's to use); the SMS has its own (src/sms/vram_sms.h).
 */
#include "vdp.h"

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

/* Line r of the cell alone (op_line: the auction's dots). */
static void row_load(uint8_t r)
{
    VDP_LOCK();
    VDP_RADDR(ca + r);
    cp[r] = VDP_DATA;
    VDP_RADDR(ca + r + VR_COL);
    cc[r] = VDP_DATA;
    VDP_UNLOCK();
}

static void row_store(uint8_t r)
{
    VDP_LOCK();
    VDP_WADDR(ca + r);
    VDP_DATA = cp[r];
    VDP_WADDR(ca + r + VR_COL);
    VDP_DATA = cc[r];
    VDP_UNLOCK();
}

/* A whole cell from o: 8 pattern bytes, then 8 colour bytes. */
static void cell_put(const uint8_t *o)
{
    vwrite(ca, o, 8);
    vwrite(ca + VR_COL, o + 8, 8);
}

/* ---- set-up ------------------------------------------------------------- */
static void vram_init(void)
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
}

static void vram_clear(void)
{
    vfill(VR_PAT, 0, 0x1800);
    vfill(VR_COL, (T_WHITE << 4) | bgc, 0x1800);
}

/* ---- text --------------------------------------------------------------- */
/* k cells from (tx, ty): tbuf's characters (or blanks, when !chars) in
 * colour c on the background. The cells of a row are one run of VRAM, so
 * a string is two bursts. */
static void text_cells(uint8_t tx, uint8_t ty, uint8_t k, uint8_t c, uint8_t chars)
{
    const uint8_t *g;
    uint16_t a = ((uint16_t)ty << 8) | ((uint16_t)tx << 3);
    uint8_t i, r, v;
    ART_BEGIN();
    VDP_LOCK();
    VDP_WADDR(a);
    for (i = 0; i < k; ++i) {
        if (chars) {
            g = glyph_of(tbuf[i]);
            for (r = 0; r < 8; ++r)
                VDP_DATA = g[r];
        } else {
            for (r = 0; r < 8; ++r)
                VDP_DATA = 0;
        }
    }
    VDP_UNLOCK();
    ART_END();
    v = (uint8_t)(c << 4) | bgc;
    VDP_LOCK();
    VDP_WADDR(a + VR_COL);
    for (i = 0; i < k; ++i)
        for (r = 0; r < 8; ++r)
            VDP_DATA = v;
    VDP_UNLOCK();
}

/* ---- the town ----------------------------------------------------------- */
/* Cell row cy of the town: TOWN_CW cells' pattern bytes, then their colour
 * bytes (each in VRAM order). */
static void town_row(uint8_t cy, const uint8_t *pat, const uint8_t *col)
{
    uint16_t a = ((uint16_t)(TOWN_CY + cy) << 8) | (TOWN_CX << 3);
    vwrite(a, pat, TOWN_CW * 8);
    vwrite(a + VR_COL, col, TOWN_CW * 8);
}

/* ---- sprites ------------------------------------------------------------ */
/* VDP sprite n's 16 x 16 pattern (the left column's 16 lines, then the
 * right's); its colour is in the attributes. */
#define SPR_COL_IN_PAT 0
static void spr_pattern(uint8_t n, const uint8_t *pat, uint8_t col)
{
    (void)col;
    vwrite(VR_SPAT + (uint16_t)n * 32, pat, 32);
}

static uint8_t sat[SPR_SLOTS * 4 * 4 + 1], sat_n;

static void sat_begin(void)
{
    sat_n = 0;
}

/* VDP sprite n at pixel (x, y), in TMS colour c. Left of the screen is the
 * early clock: 32 to the left. */
static void sat_put(int16_t x, int16_t y, uint8_t n, uint8_t c)
{
    uint8_t ec = 0;
    if (sat_n >= 32)
        return;
    if (x < 0) {
        x += 32;
        ec = 0x80;
    }
    sat[sat_n * 4] = (uint8_t)(y - 1);
    sat[sat_n * 4 + 1] = (uint8_t)x;
    sat[sat_n * 4 + 2] = n << 2;
    sat[sat_n * 4 + 3] = ec | c;
    ++sat_n;
}

static void sat_end(void)
{
    if (sat_n < 32)
        sat[sat_n * 4] = 0xD0;      /* the end of the list */
    vwrite(VR_SAT, sat, sat_n < 32 ? sat_n * 4 + 1 : 128);
}
