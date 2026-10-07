/*
 * bank.c -- the SMS's ROM banks (src/sms/mule.asm): the phases' code, the
 * art and the strings all take turns in slot 2 at 8000h, a Sega mapper
 * bank switch (a store to FFFFh) each. The code that switches is outside
 * the window, and puts the phase's bank back before it returns.
 *
 * The cartridge tells the mapper from the stores it finds in the image's
 * first 32K (tools/sms/mkrom.py makes sure it still sees a Sega one): the
 * switches themselves are in mule.asm, each a store it counts.
 */
#include <string.h>
#include "bank.h"
#include "gfxdata.h"

#define CART_RAM (*(volatile uint8_t *)0xFFFC)
#define SLOT0    (*(volatile uint8_t *)0xFFFD)
#define SLOT1    (*(volatile uint8_t *)0xFFFE)
#define B_OV     1              /* + overlay 1..5 (B_ART 7, B_STR 8: mule.asm) */

void sms_slot2(uint8_t bank) __z88dk_fastcall;
void sms_slot2_art(void);
void sms_slot2_str(void);

static uint8_t cur;

static void phase_bank(void)
{
    sms_slot2(B_OV + (cur ? cur : 1));
}

const char *bank_init(void)
{
    CART_RAM = 0;               /* slot 2 is ROM */
    SLOT0 = 0;
    SLOT1 = 1;
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
        sms_slot2_art();
    else
        phase_bank();
}

void bank_get(void *dst, const void *src, uint16_t n)
{
    sms_slot2_str();
    memcpy(dst, src, n);
    phase_bank();
}
