/*
 * plat.c -- the SMS (a FujiNet cartridge): start-up, the frame clock and
 * quitting.
 *
 * The clock is the VDP's frame interrupt (smsint.asm), counting 60ths of a
 * second on a 50 Hz console too.
 */
#include "mule.h"
#include "vdp.h"
#include "fujinet-sms.h"

uint16_t frame_count;
extern uint8_t sms_pal;
static uint16_t seed = 0x1234;

void sms_int_init(void);
uint8_t sms_pal_probe(void);
void ns_close(void);            /* nsnet.c */

const char *plat_init(void)
{
    sms_int_init();
    gfx_init();
    sms_pal = sms_pal_probe();
    return 0;
}

/* Back to the cartridge's CONFIG (its loader page, out of the way of
 * slot 2's bank), having said goodbye to the server. Without a cartridge
 * there is nowhere to go. */
void plat_quit(void)
{
    ns_close();
    if (fuji_sms_present())
        fuji_sms_exit_to_config();
    gfx_clear();
    text_center(11, "YOU CAN TURN THE SMS OFF", C_WHITE);
    for (;;)
        frame_wait();
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
    (void)jiffies;              /* the interrupt kept counting */
}

uint8_t plat_rand(void)
{
    /* xorshift, stirred by the frame count */
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return (uint8_t)seed;
}
