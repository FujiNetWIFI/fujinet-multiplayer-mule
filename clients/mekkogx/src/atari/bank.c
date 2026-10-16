/*
 * bank.c -- bank.h on the Atari: the phases are overlays, PHASE.1 .. PHASE.5
 * on the boot disk, read into the window (mule-atari.cfg, __OVLADDR__)
 * when their phase comes. There is no DOS: bank_init finds the files in
 * the DOS 2 directory (sectors 361-368) and bank_overlay follows their
 * sector links, through the OS's disk handler (DSKINV).
 */
#ifdef __ATARI__

#include <string.h>
#include "bank.h"
#include "plat.h"
#include "atari.h"

#define DUNIT  (*(volatile uint8_t *)0x0301)
#define DCOMND (*(volatile uint8_t *)0x0302)
#define DSTATS (*(volatile uint8_t *)0x0303)
#define DBUF   (*(volatile uint16_t *)0x0304)
#define DAUX   (*(volatile uint16_t *)0x030A)

extern uint8_t _OVLADDR__[], _OVLSIZE__[];

void ns_pause(void);                    /* nsnet.c: the stream off for SIO */
void ns_resume(void);

static uint8_t sec[128];
static uint16_t ovl_first[6];
static uint8_t ovl_cur;

/* A sector, tried a few times: FujiNet may still be leaving the stream. */
static uint8_t read_sector(uint16_t n)
{
    uint8_t tries = 4, t;
    do {
        DUNIT = 1;
        DCOMND = 'R';
        DBUF = (uint16_t)sec;
        DAUX = n;
        asm("jsr $E453");               /* DSKINV */
        if (DSTATS < 128)
            return 1;
        for (t = RTCLOK; t == RTCLOK; )
            ;
    } while (--tries);
    return 0;
}

const char *bank_init(void)
{
    uint16_t s;
    uint8_t e, n, found = 0;
    const uint8_t *d;
    for (s = 361; s <= 368; ++s) {
        if (!read_sector(s))
            return "CAN'T READ THE DISK DIRECTORY";
        for (e = 0; e < 8; ++e) {
            d = sec + e * 16;
            /* in use, "PHASE   " + digit */
            if ((d[0] & 0xC0) != 0x40 || memcmp(d + 5, "PHASE   ", 8) || d[13] < '1' || d[13] > '5')
                continue;
            n = d[13] - '0';
            ovl_first[n] = d[3] | d[4] << 8;
            ++found;
        }
    }
    if (found < 5)
        return "CAN'T FIND PHASE.1 - PHASE.5";
    ovl_cur = OV_NONE;
    return 0;
}

void bank_overlay(uint8_t n)
{
    uint16_t s;
    uint8_t *dst = _OVLADDR__, k;
    if (n == ovl_cur || n == OV_NONE || n > 5)
        return;
    ovl_cur = OV_NONE;
    ns_pause();
    for (s = ovl_first[n]; s; ) {
        if (!read_sector(s))
            break;
        k = sec[127];
        if (k > 125)
            k = 125;
        if (dst + k > _OVLADDR__ + (uint16_t)_OVLSIZE__)
            break;
        memcpy(dst, sec, k);
        dst += k;
        s = (sec[125] & 3) << 8 | sec[126];
    }
    ns_resume();
    if (!s)
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
