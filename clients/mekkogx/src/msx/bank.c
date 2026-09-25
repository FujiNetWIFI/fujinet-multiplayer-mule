/*
 * bank.c -- the MSX's ROM banks (src/msx/mule.asm): the phases' code, the
 * art and the strings all take turns in the window at A000h, an ASCII8
 * bank switch (a write to 7800h) each. The code that switches is outside
 * the window, and puts the phase's bank back before it returns.
 */
#include <string.h>
#include "bank.h"
#include "gfxdata.h"

#define WINDOW   (*(volatile uint8_t *)0x7800)
#define B_OV     2              /* + overlay 1..5 */
#define B_ART    8
#define B_STR    9

static uint8_t cur;

static void phase_bank(void)
{
    WINDOW = B_OV + (cur ? cur : 1);
}

const char *bank_init(void)
{
    cur = 0;
    phase_bank();
    return 0;
}

void bank_overlay(uint8_t n)
{
    if (n == OV_NONE || n > 5)
        return;
    cur = n;
    phase_bank();
}

uint8_t bank_current(void)
{
    return cur;
}

void bank_art(uint8_t on)
{
    if (on)
        WINDOW = B_ART;
    else
        phase_bank();
}

void bank_get(void *dst, const void *src, uint16_t n)
{
    WINDOW = B_STR;
    memcpy(dst, src, n);
    phase_bank();
}
