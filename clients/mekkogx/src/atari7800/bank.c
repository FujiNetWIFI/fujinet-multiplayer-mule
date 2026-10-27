/*
 * bank.c -- bank.h on the 7800: the SuperGame window at $8000 holds, above
 * the REPL block every bank has, a phase's overlay, or for a moment the art
 * or the strings (src/atari7800/mule-a78.cfg). A store anywhere in the
 * window picks the bank; this file is the only code that stores there. The
 * code that switches is outside the window (or in REPL), and puts the
 * phase's bank back before it returns.
 */
#include <string.h>
#include "bank.h"
#include "a78hw.h"

#define BANKSEL  (*(volatile uint8_t *)0x8000)
#define BANK_OVL1 0             /* OVERLAY1-5: banks 0-4 */
#define BANK_ART 5
#define BANK_STR 6

static uint8_t cur;

static void phase_bank(void)
{
    BANKSEL = BANK_OVL1 + (cur ? cur - 1 : 0);
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

/* For the display's code (fixed bank): the shapes, the font, the town. */
void bank_art_on(void)
{
    BANKSEL = BANK_ART;
}

void bank_art_off(void)
{
    phase_bank();
}

void bank_get(void *dst, const void *src, uint16_t n)
{
    BANKSEL = BANK_STR;
    memcpy(dst, src, n);
    phase_bank();
}
