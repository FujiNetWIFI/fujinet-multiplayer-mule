/*
 * bank.c -- bank.h on the Lynx: the phases are overlays, entries 1..5 of
 * the cart's directory (src/lynx/directory.s), read into the window
 * (mule-lynx.cfg, __OVLADDR__) by cc65's lynx_load when their phase comes.
 * A cart read takes a few milliseconds; ComLynx keeps receiving meanwhile.
 */
#ifdef __LYNX__

#include <string.h>
#include <lynx.h>
#include "bank.h"

void snd_overlay(void);                 /* sound.c */

static uint8_t ovl_cur;

const char *bank_init(void)
{
    ovl_cur = OV_NONE;
    return 0;
}

void bank_overlay(uint8_t n)
{
    if (n == ovl_cur || n == OV_NONE || n > 5)
        return;
    snd_overlay();                      /* its tunes and scripts go with it */
    lynx_load(n);
    ovl_cur = n;
}

uint8_t bank_current(void)
{
    return ovl_cur;
}

void bank_get(void *dst, const void *src, uint16_t n)
{
    memcpy(dst, src, n);
}

#endif
