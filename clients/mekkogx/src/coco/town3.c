/*
 * town3.c -- gfx_town on the CoCo 3, in the development overlay's block:
 * only develop.c draws the town.
 */
#ifdef COCO3
#include "mule.h"
#include "gfxdata3.h"

/* town_rle (in block TOWN_BLK, read through the window: $2000 below where
 * it is linked): n < 128, n + 1 bytes follow; n >= 128, the next byte
 * n - 125 times. A line is 48 bytes, two clocks each. */
static const uint8_t *rl_src;
static uint8_t rl_lit, rl_run, rl_val;

static void unpack(uint8_t *dst, uint8_t cnt)
{
    uint8_t code;
    while (cnt--) {
        if (!rl_lit && !rl_run) {
            code = *rl_src++;
            if (code >= 128) {
                rl_run = code - 125;
                rl_val = *rl_src++;
            } else {
                rl_lit = code + 1;
            }
        }
        if (rl_run) {
            --rl_run;
            *dst++ = rl_val;
        } else {
            --rl_lit;
            *dst++ = *rl_src++;
        }
    }
}

void gfx_town(void)
{
    uint8_t buf[48];
    uint8_t line, idx, val;
    uint8_t *dst;
    rl_src = (const uint8_t *)((uint16_t)town_rle - 0x2000);
    rl_lit = rl_run = 0;
    spr_bg(32, 96, 16, 144);
    for (line = 16; line < 16 + 144; ++line) {
        MMU6 = TOWN_BLK;
        unpack(buf, 48);
        dst = gfx3_row(line) + 32;
        for (idx = 0; idx < 48; ++idx) {
            val = buf[idx];
            *dst++ = CC(val >> 4);
            *dst++ = CC(val & 15);
        }
    }
    bg_done();
    gfx3_done();
}
#endif
