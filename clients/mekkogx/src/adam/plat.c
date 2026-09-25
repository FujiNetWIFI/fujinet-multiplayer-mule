/*
 * plat.c -- the Coleco Adam: start-up, the 60 Hz clock and quitting.
 *
 * The clock is the VDP's frame interrupt, the Z80's NMI (nmi.asm): it
 * counts on while EOS waits for AdamNet, so a network call loses no time.
 */
#include "mule.h"
#include "vdp.h"
#include "fujinet-fuji.h"

uint16_t frame_count;
volatile uint8_t vdp_busy, vdp_pend;
static uint16_t seed = 0x1234;

/* The NMI came while the VDP was busy: take it now. */
void vdp_catch_up(void)
{
    uint8_t s;
    vdp_pend = 0;
    s = VDP_CTRL;               /* (the NMI may have beaten us to it) */
    if (s & 0x80) {
        vdp_seen |= s;
        ++frame_count;
    }
}

const char *plat_init(void)
{
    gfx_init();
    return 0;
}

/* Back to the FujiNet's CONFIG, as the Adam's reset switch would. */
void plat_quit(void)
{
    fuji_set_boot_config(1);
    __asm
        di
        xor     a
        out     (0x7F),a        ; SmartWriter ROM and the lower 32K of RAM
        jp      0
    __endasm;
}

/* The DCB table EOS keeps (FEC3h: how many; FEC4h: 21 bytes each). The
 * FujiNet answers as device 0Fh. */
uint8_t plat_net_probe(void)
{
    uint8_t n = *(uint8_t *)0xFEC3, i;
    const uint8_t *d = (const uint8_t *)0xFEC4;
    for (i = 0; i < n; ++i, d += 21)
        if ((d[16] & 0x0F) == 0x0F)
            return 1;
    return 0;
}

/* Wait until at least one frame has passed since the last call. */
void frame_wait(void)
{
    static uint16_t seen;
    while (*(volatile uint16_t *)&frame_count == seen)
        ;
    seen = frame_count;
    seed += frame_count;
}

void frame_tick(void)
{
}

void frame_lost(uint8_t jiffies)
{
    vdp_busy = 1;               /* (the NMI changes it too) */
    frame_count += jiffies;
    VDP_UNLOCK();
}

uint8_t plat_rand(void)
{
    /* xorshift, stirred by the frame count */
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return (uint8_t)seed;
}
