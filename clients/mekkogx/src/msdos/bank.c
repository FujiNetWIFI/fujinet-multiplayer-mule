/*
 * bank.c -- bank.h on the PC: everything is addressable. The phases are
 * overlays all the same (tools/dos/wlink-mule.py), so the game fits a 128K
 * PCjr beside its 32K screen, but Watcom's overlay manager loads them when
 * they are called: here is only the bookkeeping main.c asks for.
 */
#include <string.h>
#include "bank.h"

static uint8_t cur;

const char *bank_init(void)
{
    return 0;
}

void bank_overlay(uint8_t n)
{
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
