/*
 * plat.c -- Lynx start-up, the 60 Hz clock and quitting.
 *
 * The clock is the vertical blank (irq.s): cc65's crt0 sets the display to
 * 60 Hz; the VBL counts frame_count even while a network call waits. No
 * TGI and no clock(): their VBL handlers and ours can't share the IRQ.
 */
#ifdef __LYNX__

#include "mule.h"
#include "lxhw.h"

void mikey_init(void);                  /* mikey.s */
void ns_close(void);                    /* nsnet.c */

const char *plat_init(void)
{
    gfx_init();
    mikey_init();
    vbl_init();
    return 0;
}

/* A console has nowhere to go back to: say goodbye to the server, then
 * stop. */
void plat_quit(void)
{
    ns_close();
    gfx_clear();
    text_center(11, "YOU CAN TURN THE LYNX OFF", C_WHITE);
    for (;;)
        frame_wait();
}

void frame_wait(void)
{
    static uint16_t seen;
    while (*(volatile uint16_t *)&frame_count == seen)
        ;
    seen = frame_count;
    spr_flush();                        /* for loops that don't */
}

void frame_tick(void)
{
}

void frame_lost(uint8_t jiffies)
{
    (void)jiffies;                      /* the VBL kept counting */
}

uint8_t snd_rand(void);                 /* mikey.s */

uint8_t plat_rand(void)
{
    return snd_rand() ^ MIKEY_TIM(0, 2) ^ (uint8_t)frame_count;
}

#endif
