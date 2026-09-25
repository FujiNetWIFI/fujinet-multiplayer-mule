/*
 * msx-portio.c -- fujinet-lib-experimental's bus/msx/portio.s, in C, for
 * the zsdcc build (tools/z80/mkfnlib.sh puts it in the library in place
 * of portio.s and timeout.s). The asm uses IX and IY freely, which zsdcc
 * keeps (IX is its frame pointer), and port_get_until returned with a byte
 * pair still on the stack when it timed out.
 *
 * The FujiNet cartridge's registers are at BFFCh-BFFFh (page 2 in its
 * slot); timeouts count the BIOS's JIFFY.
 */
#include <stdint.h>
#include "portio.h"

#define IO_GETC   (*(volatile uint8_t *)0xBFFC)
#define IO_STATUS (*(volatile uint8_t *)0xBFFD)
#define IO_PUTC   (*(volatile uint8_t *)0xBFFE)
#define JIFFY     (*(volatile uint16_t *)0xFC9E)

int port_getc(void)
{
    if (!(IO_STATUS & 0x80))
        return -1;
    return IO_GETC;
}

int port_getc_timeout(uint16_t timeout)
{
    uint16_t t0 = JIFFY;
    int c;
    while ((c = port_getc()) < 0)
        if ((uint16_t)(JIFFY - t0) >= timeout)
            return -1;
    return c;
}

int port_discard_until(uint8_t c, uint16_t timeout)
{
    uint16_t t0 = JIFFY;
    int v;
    for (;;) {
        v = port_getc();
        if (v == c)
            return c;
        if ((uint16_t)(JIFFY - t0) >= timeout)
            return -1;
    }
}

uint16_t port_getbuf(void *buf, uint16_t len, uint16_t timeout)
{
    uint8_t *p = buf;
    uint16_t n = 0;
    int c;
    while (n < len) {
        c = port_getc_timeout(timeout);
        if (c < 0)
            break;
        p[n++] = (uint8_t)c;
    }
    return n;
}

uint16_t port_get_until(void *buf, uint16_t maxlen, uint8_t c, uint16_t timeout)
{
    uint8_t *p = buf;
    uint16_t n = 0;
    int v;
    while (n < maxlen) {
        v = port_getc_timeout(timeout);
        if (v < 0)
            break;
        p[n++] = (uint8_t)v;
        if (v == c)
            break;
    }
    return n;
}

void port_putc(uint8_t c)
{
    IO_PUTC = c;
}

uint16_t port_putbuf(void *buf, uint16_t len)
{
    const uint8_t *p = buf;
    uint16_t n;
    for (n = 0; n < len; ++n)
        IO_PUTC = p[n];
    return len;
}
