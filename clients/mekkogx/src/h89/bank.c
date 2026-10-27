/*
 * bank.c -- bank.h on the H89: the phases are overlays, read from disk into
 * one window (H89_OVL_WIN, src/h89/mmap.inc) as they are wanted.
 *
 * The table in front of them is tools/h89/ovl.py's: "MULE" and, for each
 * overlay, a 16-bit first unit and a number of units. A unit is a CP/M
 * record (128 bytes) or an HDOS sector (256).
 *
 * CP/M: MULE.OVL is on the drive MULE.COM ran from (the default drive), or
 * failing that on one the system has logged in (never an unlogged one: the
 * BDOS would stop with "Bdos Err on x: Select"). The table is its first
 * record.
 *
 * HDOS: HDOS leaves the program's own file open on channel -1, from SY1: or
 * FN0: alike, and the overlays follow the image in it. The table is the
 * first sector past the image HDOS loads.
 *
 * A load is disk I/O, and from an FN: drive it is the FujiNet card's, so
 * the stream's call going on the card is finished first (nt_quiesce).
 */
#include <string.h>
#include "bank.h"
#include "h89.h"
#include "nstrans.h"

#define OVERLAYS 5

static uint8_t cur;
static uint8_t tab[3 * OVERLAYS];

#ifndef __HDOS__

#define UNIT 128

static uint8_t fcb[36] = { 0, 'M', 'U', 'L', 'E', ' ', ' ', ' ', ' ', 'O', 'V', 'L' };

static uint8_t open_on(uint8_t drive)
{
    fcb[0] = drive;
    return cpm_open(fcb) != 0xFF;
}

/* The file is found (0: it isn't). */
static uint8_t overlay_file(void)
{
    uint16_t login;
    uint8_t d;

    if (open_on(0))
        return 1;
    login = cpm_login();
    for (d = 0; d < 16; ++d)
        if ((login >> d & 1) && open_on(d + 1))
            return 1;
    return 0;
}

/* The table's unit. */
static uint16_t table_unit(void)
{
    return 0;
}

/* n units from unit u into to (0: read). */
static uint8_t fetch(uint16_t u, uint8_t n, uint8_t *to)
{
    for (; n; --n) {
        cpm_dma(to);
        fcb[33] = (uint8_t)u;
        fcb[34] = (uint8_t)(u >> 8);
        fcb[35] = 0;
        if (cpm_rread(fcb))
            return 1;
        ++u;
        to += UNIT;
    }
    return 0;
}

#else /* HDOS */

#define UNIT 256

extern uint8_t _BSS_head[];
static uint16_t args[3];

static uint8_t overlay_file(void)
{
    return 1;
}

/* The sector after the image: the 8-byte ABS header and the program up to
 * its BSS, which is not in the file. */
static uint16_t table_unit(void)
{
    return ((uint16_t)_BSS_head - H89_ORG + 8 + 255) >> 8;
}

static uint8_t fetch(uint16_t u, uint8_t n, uint8_t *to)
{
    args[0] = u;
    args[1] = (uint16_t)to;
    args[2] = (uint16_t)n << 8;
    return hdos_read(args);
}

#endif /* HDOS */

const char *bank_init(void)
{
    uint8_t *win = (uint8_t *)H89_OVL_WIN;

    if (!overlay_file() || fetch(table_unit(), 1, win) || memcmp(win, "MULE", 4)) {
#ifdef __HDOS__
        os_print("M.U.L.E.: THE OVERLAYS ON THE END OF MULE.ABS WON'T LOAD\r\n$");
#else
        os_print("M.U.L.E.: NO MULE.OVL ON THE DRIVE OF MULE.COM OR ONE LOGGED IN\r\n$");
#endif
        os_exit();
    }
    memcpy(tab, win + 4, sizeof(tab));
    return 0;
}

void bank_overlay(uint8_t n)
{
    uint8_t *t;

    if (n == OV_NONE || n > OVERLAYS || n == cur)
        return;
    nt_quiesce();
    cur = 0;                    /* neither the old one nor the new, until read */
    t = tab + 3 * (n - 1);
    if (fetch(t[0] | t[1] << 8, t[2], (uint8_t *)H89_OVL_WIN))
        return;
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
