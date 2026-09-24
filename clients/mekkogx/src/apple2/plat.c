/*
 * plat.c -- Apple II start-up, the 60 Hz clock and quitting.
 *
 * The clock is the vertical blank (lowlevel.s frame_tick: //e $C019, IIc
 * VBL flag, IIgs inverted), so a //e, //c or IIgs is required. Drawing
 * calls frame_tick as it goes, so a slow frame still counts every blank;
 * network calls block the CPU for longer, and frame_lost() lets the
 * network layer add the frames they took.
 */
#ifdef __APPLE2__

#include <apple2.h>
#include <stdlib.h>
#include "plat.h"
#include "gfx.h"

uint16_t frame_count;
static uint16_t seed = 0x1234;

void vbl_init(uint8_t mode);        /* lowlevel.s */

const char *plat_init(void)
{
    uint8_t t = get_ostype();
    if (t < APPLE_IIE)
        return "M.U.L.E. NEEDS AN APPLE //E, //C OR IIGS";
    vbl_init(t >= APPLE_IIGS ? 1 : (t >= APPLE_IIC && t < APPLE_IIGS) ? 2 : 0);
    gfx_init();
    return 0;
}

void plat_quit(void)
{
    exit(0);
}

/* Wait until at least one frame has passed since the last call. */
void frame_wait(void)
{
    static uint16_t seen;
    while (frame_count == seen)
        frame_tick();
    seen = frame_count;
    seed += frame_count;
}

void frame_lost(uint8_t jiffies)
{
    frame_count += jiffies;
}

uint8_t plat_rand(void)
{
    /* xorshift, stirred by the frame count */
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return (uint8_t)seed;
}

#endif
