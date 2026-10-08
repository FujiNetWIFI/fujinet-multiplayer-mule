/*
 * vram_sms.h -- gfx.c's screen on the SMS: the cells are Mode 4 tiles
 * (tiles.c), the sprites the SMS's own.
 *
 * Included by gfx.c only, as src/tms9918/vram_tms.h is on the TMS9918A:
 * the same functions, on gfx.c's cell (cp/cc, ca = its number), bgc and
 * text buffer. A line across cells (op_line) is tileline.asm's.
 *
 * Sprites: a VDP sprite of the TMS9918A's (16 x 16, one colour) is two of
 * the SMS's side by side, each 8 x 16 (two tiles), the colour drawn into
 * the tiles: TMS sprite n is sprite tiles n * 4 .. n * 4 + 3 (left top,
 * left bottom, right top, right bottom). Off the left edge a half isn't
 * shown (the SMS can only shift all its sprites).
 */
#include "tiles.h"

static void cell_at(uint8_t cx, uint8_t cy)
{
    ca = ((uint16_t)cy << 5) | cx;
}

static void cell_load(void)
{
    tile_load(ca, cp, cc);
}

static void cell_store(void)
{
    tile_store(ca, cp, cc);
}

/* op_line's cells cx0..cx1 of cell row cy, their line r: the pixels in
 * pic[cx] (masked by lm in the first, rm in the last) become colour c. */
#define VRAM_LINE 1
static void line_cells(uint8_t cx0, uint8_t cx1, uint8_t cy, uint8_t r,
                       const uint8_t *pic, uint8_t lm, uint8_t rm, uint8_t c)
{
    if (!tiles_ok)
        return;
    hl_cell = ((uint16_t)cy << 5) | cx0;
    hl_n = cx1 - cx0 + 1;
    hl_r = r;
    hl_pic = pic + cx0;
    hl_lm = lm;
    hl_rm = rm;
    hl_c = c;
    tile_hline();
}

static void cell_put(const uint8_t *o)
{
    tile_store(ca, o, o + 8);
}

static void vram_init(void)
{
    tiles_init();
}

static void vram_clear(void)
{
    tiles_clear(bgc);
}

/* A plot drawn before is drawn by its tiles' names (tiles.c). */
#define PLOT_MEMO 1

/* ---- text --------------------------------------------------------------- */
static void text_cells(uint8_t tx, uint8_t ty, uint8_t k, uint8_t c, uint8_t chars)
{
    uint8_t i;
    memset(cc, (uint8_t)(c << 4) | bgc, 8);
    memset(cp, 0, 8);
    if (!chars) {
        tile_run(((uint16_t)ty << 5) + tx, k, cp, cc);
        return;
    }
    for (i = 0; i < k; ++i) {
        if (chars) {
            ART_BEGIN();
            memcpy(cp, glyph_of(tbuf[i]), 8);
            ART_END();
        }
        tile_store(((uint16_t)ty << 5) + tx + i, cp, cc);
    }
}

/* ---- the town ----------------------------------------------------------- */
static void town_row(uint8_t cy, const uint8_t *pat, const uint8_t *col)
{
    uint8_t i;
    uint16_t cell = ((uint16_t)(TOWN_CY + cy) << 5) + TOWN_CX;
    for (i = 0; i < TOWN_CW; ++i, pat += 8, col += 8)
        tile_store(cell + i, pat, col);
}

/* ---- sprites ------------------------------------------------------------ */
#define SPR_COL_IN_PAT 1

/* TMS sprite n's pattern (the left column's 16 lines, then the right's) in
 * colour c: its four tiles, which follow one another in VRAM in the same
 * order (left top, left bottom, right top, right bottom). sprout.asm. */
static void spr_pattern(uint8_t n, const uint8_t *pat, uint8_t col)
{
    so_addr = SV_SPR + ((uint16_t)n << 7);
    so_pat = pat;
    so_col = col;
    spr_out();
}

/* A frame's pattern, made once (gfx.c's send: tiles.c keeps them). */
#define SPR_CACHE 1

#define HW_SPR 64
static uint8_t sat_y[HW_SPR], sat_xt[HW_SPR * 2], sat_n;

static void sat_begin(void)
{
    sat_n = 0;
}

static void sat_half(int16_t x, uint8_t y, uint8_t tile)
{
    if (x < 0 || x > 255 || sat_n >= HW_SPR)
        return;
    sat_y[sat_n] = y;
    sat_xt[sat_n * 2] = (uint8_t)x;
    sat_xt[sat_n * 2 + 1] = tile;
    ++sat_n;
}

/* TMS sprite n at pixel (x, y) (its colour is in its tiles). */
static void sat_put(int16_t x, int16_t y, uint8_t n, uint8_t c)
{
    uint8_t yy = (uint8_t)(y - 1);
    (void)c;
    if (yy == 0xD0)             /* would end the list */
        return;
    sat_half(x, yy, n << 2);
    sat_half(x + 8, yy, (n << 2) + 2);
}

static void sat_end(void)
{
    if (sat_n < HW_SPR)
        sat_y[sat_n] = 0xD0;    /* the end of the list */
    vram_out(SV_SAT, sat_y, sat_n < HW_SPR ? sat_n + 1 : HW_SPR);
    if (sat_n)
        vram_out(SV_SAT + 128, sat_xt, sat_n * 2);
}
