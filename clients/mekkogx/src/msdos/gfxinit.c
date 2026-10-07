/*
 * gfxinit.c -- gfx_init (gfx.h), once at the start: it lives in the boot
 * overlay (tools/dos/wlink-mule.py), as a 128K PCjr has no room to keep it.
 */
#include <i86.h>
#include "mule.h"

extern uint8_t plat_jr, plat_pal;       /* plat.c */
extern uint8_t __far *vram_p;           /* gfx.c */
extern uint16_t rowoff[192], cga_w[16], jr_w[16][2];
void set_bg(uint8_t c);
void set_preg(uint8_t reg, uint8_t c);
#define TOP 4                   /* the Atari's line 0 is the screen's 4 */

/* Two units (CGA clocks, PCjr pixels) -> a byte. */
static const uint8_t m2[4] = { 0x00, 0x0F, 0xF0, 0xFF };

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
