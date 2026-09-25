/*
 * plat.c -- the MSX (MSX1 and up, a FujiNet cartridge): start-up, the
 * frame clock and quitting.
 *
 * The clock is the BIOS's interrupt, through the H.TIMI hook (msxint.asm).
 * A 50 Hz machine counts 50 a second: the server's clock (main.c
 * clock_sync) keeps the game's timing right.
 */
#include "mule.h"
#include "vdp.h"

uint16_t frame_count;
static uint16_t seed = 0x1234;

void msx_hook_init(void);
void msx_quit(void);

#define IO_STATUS (*(volatile uint8_t *)0xBFFD)

const char *plat_init(void)
{
    msx_hook_init();
    gfx_init();
    return 0;
}

void plat_quit(void)
{
    msx_quit();
}

/* The FujiNet's status register shows it has a user ROM ready (us); where
 * there is no FujiNet the ROM itself is there, and its last bytes are 0. */
uint8_t plat_net_probe(void)
{
    return (IO_STATUS & 0x40) != 0;
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
    __asm__("di");              /* the interrupt changes it too */
    frame_count += jiffies;
    __asm__("ei");
}

uint8_t plat_rand(void)
{
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return (uint8_t)seed;
}
