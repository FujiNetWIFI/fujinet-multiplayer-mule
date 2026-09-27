/*
 * bank.c -- bank.h on a 128K Apple II: the overlays and far data live in
 * auxiliary RAM (see mule-hgr.cfg). At start-up each file MULE.1 .. MULE.7
 * is read through ProDOS into the (not yet shown) HGR page and moved to
 * its place in aux RAM; the firmware's AUXMOVE brings things back.
 */
#ifdef __APPLE2__

#include <fcntl.h>
#include <unistd.h>
#include "bank.h"
#include "plat.h"

void aux_get(void *dst, const void *src, uint16_t n);  /* lowlevel.s */
void aux_put(void *dst, const void *src, uint16_t n);

extern uint8_t _OVLADDR__[], _OVLSIZE__[], _AUXSTAGE__[], _AUXGFX__[], _AUXTXT__[];

#define TEMP ((uint8_t *)0x2000)        /* HGR page 1, 8K */
#define OVL_AUX(n) ovl_at[n]

uint16_t ovl_len[6];
uint8_t *ovl_at[6];          /* where each overlay sits in aux RAM */
static uint8_t ovl_cur;

static int16_t load(uint8_t n, uint8_t *aux)
{
    static char name[] = "MULE.0";
    int fd;
    int16_t got;
    name[5] = '0' + n;
    fd = open(name, O_RDONLY);
    if (fd < 0)
        return -1;
    got = read(fd, TEMP, 0x2000);
    close(fd);
    if (got > 0)
        aux_put(aux, TEMP, got);
    return got;
}

const char *bank_init(void)
{
    uint8_t n;
    int16_t got;
    /* 80STORE would send $2000-$3FFF (hires on) and $0400-$07FF to main
     * RAM whatever AUXMOVE asks for */
    *(volatile uint8_t *)0xC000 = 0;
    ovl_at[1] = (uint8_t *)0x0800;      /* packed from there, below AUXGFX */
    for (n = 1; n <= 5; ++n) {
        got = load(n, OVL_AUX(n));
        if (got <= 0)
            return "CAN'T LOAD MULE.1 - MULE.5";
        ovl_len[n] = got;
        if (n < 5)
            ovl_at[n + 1] = ovl_at[n] + got;
    }
    if (ovl_at[5] + ovl_len[5] > _AUXSTAGE__)
        return "OVERLAYS TOO BIG FOR AUX RAM";
    if (load(6, _AUXGFX__) <= 0 || load(7, _AUXTXT__) <= 0)
        return "CAN'T LOAD MULE.6 / MULE.7";
    ovl_cur = OV_NONE;
    return 0;
}

/* A piece at a time, so the clock (frame_tick) keeps counting. */
void bank_overlay(uint8_t n)
{
    uint16_t done, k;
    if (n == ovl_cur || n == OV_NONE || n > 5)
        return;
    for (done = 0; done < ovl_len[n]; done += k) {
        k = ovl_len[n] - done;
        if (k > 512)
            k = 512;
        aux_get(_OVLADDR__ + done, OVL_AUX(n) + done, k);
        frame_tick();
    }
    ovl_cur = n;
}

uint8_t bank_current(void)
{
    return ovl_cur;
}

void bank_get(void *dst, const void *src, uint16_t n)
{
    aux_get(dst, src, n);
    frame_tick();
}

#endif
