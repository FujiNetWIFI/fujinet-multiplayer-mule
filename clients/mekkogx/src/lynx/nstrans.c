/*
 * nstrans.c -- stream mode's bytes on the Lynx (src/nsnet.c):
 * fujinet-lib-experimental's fuji_enable_netstream() turns FujiNet-Lynx's
 * netstream on; from then on ComLynx is a raw pipe to the socket (cc65's
 * serial driver, the one the library opened: 62500 baud, 8 bits, odd
 * parity). It can't be turned off again, so nothing else may go over the
 * bus after that (net.c's appkey calls look at ns_on).
 *
 * The driver's receive ring is 256 bytes, filled by its interrupt. ComLynx
 * is one wire: while we send, the driver doesn't receive (FujiNet waits
 * for the line to go quiet), so frames going out are kept short.
 */
#ifdef __LYNX__

#include <serial.h>
#include "mule.h"
#include "nstrans.h"
#include "fujinet-fuji.h"

#define PUT_TRIES  2000         /* a full send ring: give up on the frame */

uint8_t nt_detect(void)
{
    uint8_t s;
    return fuji_get_wifi_status(&s);
}

uint8_t nt_open(const char *host, uint16_t port)
{
    return fuji_enable_netstream(port, host, NETSTREAM_TCP);
}

/* The stream stays on for good. */
uint8_t nt_close(void)
{
    return 0;
}

void nt_quiesce(void)
{
}

uint8_t nt_get(char *c)
{
    return ser_get(c) == SER_ERR_OK;
}

uint8_t nt_put(uint8_t c)
{
    uint16_t n;
    for (n = PUT_TRIES; ser_put(c) != SER_ERR_OK; )
        if (--n == 0)
            return 0;
    return 1;
}

uint8_t nt_flush(void)
{
    return 1;
}

#endif
