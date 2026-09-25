/*
 * plat.c -- Atari start-up, the 60 Hz clock and quitting.
 *
 * The clock is RTCLOK, which the OS counts in the vertical blank even while
 * SIO is busy, so a network call loses no frames. On a PAL machine (50 Hz)
 * every fifth blank counts twice, so the game's 60 jiffies a second hold.
 */
#ifdef __ATARI__

#include <stdlib.h>
#include "plat.h"
#include "gfx.h"
#include "atari.h"

uint16_t frame_count;
static uint8_t last_rt, pal_n, is_pal;

const char *plat_init(void)
{
    is_pal = (PAL & 0x0E) == 0;         /* GTIA: 1 on NTSC, 0 on PAL */
    last_rt = RTCLOK;
    gfx_init();
    return 0;
}

void plat_quit(void)
{
    asm("jmp $E477");                   /* COLDSV */
}

static void catch_up(void)
{
    uint8_t now = RTCLOK, d = now - last_rt;
    last_rt = now;
    while (d--) {
        ++frame_count;
        if (is_pal && ++pal_n == 5) {
            pal_n = 0;
            ++frame_count;
        }
    }
}

void frame_wait(void)
{
    static uint16_t seen;
    catch_up();
    while (frame_count == seen)
        catch_up();
    seen = frame_count;
    spr_flush();                        /* the row tables too, for loops that don't */
}

void frame_tick(void)
{
    catch_up();
}

void frame_lost(uint8_t jiffies)
{
    frame_count += jiffies;
}

uint8_t plat_rand(void)
{
    return RANDOM;
}

#endif
