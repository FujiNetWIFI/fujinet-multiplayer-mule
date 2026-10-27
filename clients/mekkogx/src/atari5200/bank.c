/*
 * bank.c -- bank.h on the 5200: a Super Cart's bank is the whole cart
 * window, $4000-$BFFF, so every bank holds the same REPL (the program) and
 * a part of its own: a phase's overlay or two, the strings, the boot's
 * art (src/atari5200/mule-a52.cfg). A read of $BFD0 + 4n picks bank n;
 * this file and the crt0 are the only code that reads there. The switch
 * takes effect on the next fetch, which REPL makes the same instruction
 * in every bank. The interrupts never switch.
 */
#include <string.h>
#include "bank.h"

#define BANK_STR 2              /* src/atari5200/mule-a52.cfg */

/* the bank of OVERLAY1-5 */
static const uint8_t ovl_bank[5] = { 3, 2, 0, 1, 0 };
static uint8_t cur, sel;

/* (the optimizer drops a load whose value goes unused: not here) */
#pragma optimize (push, off)
static void pick(uint8_t b)
{
    sel = b << 2;               /* one read, no page crossed: $BFD0 + 4b */
    __asm__("ldx %v", sel);
    __asm__("lda $BFD0,x");
}
#pragma optimize (pop)

static void phase_bank(void)
{
    pick(ovl_bank[cur ? cur - 1 : 0]);
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

void bank_get(void *dst, const void *src, uint16_t n)
{
    pick(BANK_STR);
    memcpy(dst, src, n);
    phase_bank();
}
