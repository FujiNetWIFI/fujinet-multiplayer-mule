/*
 * plat.c -- NES start-up, the 60 Hz clock and quitting.
 *
 * The clock is the NMI (crt0.s): it counts on while a network call waits.
 */
#include "mule.h"
#include "neshw.h"

void gfx_direct_end(void);      /* gfx.c */
void ns_close(void);            /* nsnet.c */

static uint16_t seed = 0x1234;

const char *plat_init(void)
{
    gfx_init();
    return 0;
}

/* A console has nowhere to go back to (Reset doesn't reset the cartridge:
 * CONFIG comes back at power-on): say goodbye to the server, then stop. */
void plat_quit(void)
{
    ns_close();
    gfx_clear();
    text_center(11, "YOU CAN TURN THE NES OFF", C_WHITE);
    for (;;)
        frame_wait();
}

/* Wait until at least one frame has passed since the last call. A screen
 * drawn with the display off is shown from the next frame. */
void frame_wait(void)
{
    static uint8_t seen;
    gfx_direct_end();
    while (*(volatile uint8_t *)&frame_count == seen)
        ;
    seen = (uint8_t)frame_count;
    seed += frame_count;
}

void frame_tick(void)
{
}

void frame_lost(uint8_t jiffies)
{
    (void)jiffies;              /* the NMI kept counting */
}

uint8_t plat_rand(void)
{
    /* xorshift, stirred by the frame count */
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return (uint8_t)seed;
}
