/*
 * plat.c -- 7800 start-up, the 60 Hz clock and quitting.
 *
 * The clock is the frame's DLI (nmi.s), 60 jiffies a second on NTSC and
 * PAL alike: it counts on while a network call waits.
 */
#include "mule.h"
#include "a78hw.h"
#include "fujinet-atari7800.h"

void ns_close(void);            /* nsnet.c */

static uint16_t seed = 0x1234;

const char *plat_init(void)
{
    gfx_init();
    return 0;
}

/* Back to CONFIG (the cartridge's loader), saying goodbye to the server. */
void plat_quit(void)
{
    ns_close();
    fuji_a7800_config();
}

/* Wait until at least one frame has passed since the last call, then put
 * what changed on the screen (the next frame shows it). */
void frame_wait(void)
{
    static uint8_t seen;
    while (*(volatile uint8_t *)&frame_count == seen)
        ;
    seen = (uint8_t)frame_count;
    seed += frame_count;
    spr_flush();
}

void frame_tick(void)
{
}

void frame_lost(uint8_t jiffies)
{
    (void)jiffies;              /* the DLI kept counting */
}

uint8_t plat_rand(void)
{
    /* xorshift, stirred by the frame count and the POKEY */
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return (uint8_t)seed ^ RANDOM;
}
