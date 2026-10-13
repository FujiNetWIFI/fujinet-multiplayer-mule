/*
 * bank.c -- the H89 has room for the whole game (a 64K CP/M's 56K): no
 * overlays, and the strings are where they are.
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
