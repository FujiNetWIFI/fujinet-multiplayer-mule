/*
 * auction3.c -- gfx_hdots on the CoCo 3, in the auction overlay's block:
 * only auction.c draws the dotted lines.
 */
#ifdef COCO3
#include "mule.h"
#include "gfxdata3.h"

/* A dot every other clock. */
void gfx_hdots(uint8_t x, uint8_t y, uint8_t w, uint8_t col)
{
    uint8_t clk = gfx3_cc(col), idx;
    uint8_t *dst;
    if (y >= 192 || x >= LINE)
        return;
    if (x + w > LINE)
        w = LINE - x;
    spr_bg(x, w, y, 1);
    dst = gfx3_row(y) + x;
    for (idx = 0; idx < w; idx += 2, dst += 2)
        *dst = clk;
    bg_done();
    gfx3_done();
}
#endif
