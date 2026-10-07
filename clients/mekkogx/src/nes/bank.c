/*
 * bank.c -- bank.h on the NES: MMC3's R6 window at $8000 holds a phase's
 * overlay, or for a moment the art or the strings (src/nes/mule-nes.cfg).
 * The code that switches is outside the window, and puts the phase's bank
 * back before it returns.
 */
#include <string.h>
#include "bank.h"
#include "neshw.h"
#include "gfxdata.h"

static uint8_t cur;

static void phase_bank(void)
{
    prg_r6(BANK_OVL1 + (cur ? cur - 1 : 0));
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
        prg_r6(BANK_ART);
    else
        phase_bank();
}

void bank_get(void *dst, const void *src, uint16_t n)
{
    prg_r6(BANK_STR);
    memcpy(dst, src, n);
    phase_bank();
}
