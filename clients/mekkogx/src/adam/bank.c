/*
 * bank.c -- the Adam's overlays: each phase's code is read from the boot
 * medium (the disk or tape the FujiNet serves, or a real one) into the
 * window when the phase changes, 1K blocks with EOS.
 *
 * tools/adam/mkimage.py puts the overlays after the program's blocks and
 * fills ovl_table in the image: where each starts, and how many blocks.
 * Everything else is in memory all along: bank_get is a copy.
 */
#include <string.h>
#include "bank.h"
#include "eoscall.h"

#define CURRENT_DEV (*(uint8_t *)0xFD6F)    /* EOS: the device we booted from */

extern uint8_t ovl_window[];                 /* mule.asm */

/* "OVLT", then for overlays 1..5 the first block (low, high) and the count.
 * Filled in by tools/adam/mkimage.py. */
const uint8_t ovl_table[4 + 5 * 3] = { 'O', 'V', 'L', 'T' };

static uint8_t dev, cur;

const char *bank_init(void)
{
    dev = CURRENT_DEV;
    return 0;
}

void bank_overlay(uint8_t n)
{
    const uint8_t *t;
    uint16_t blk;
    uint8_t k, tries;
    uint8_t *dst = ovl_window;

    if (n == cur || n == OV_NONE || n > 5)
        return;
    t = ovl_table + 4 + (n - 1) * 3;
    blk = t[0] | (uint16_t)t[1] << 8;
    eos_dev = dev;
    for (k = 0; k < t[2]; ++k, ++blk, dst += 1024) {
        eos_blk = blk;
        eos_dst = dst;
        for (tries = 0; tries < 5; ++tries)
            if (eos_read_blk() == 0)
                break;
    }
    cur = n;
}

uint8_t bank_current(void)
{
    return cur;
}

void bank_get(void *dst, const void *src, uint16_t n)
{
    memcpy(dst, src, n);
}
