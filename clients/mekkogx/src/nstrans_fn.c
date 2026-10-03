/*
 * nstrans_fn.c -- stream mode's bytes (src/nsnet.c) through fujinet-lib:
 * an N:TCP socket to the server's stream port. The CoCo and the Apple II
 * use it (the Adam drives its DCB itself: src/adam/nstrans.c; the MSX and
 * MS-DOS put their packets together: src/nstrans_fb.c).
 *
 * It replaced HTTP polls (net.c), which held the machine for the whole
 * request: FujiNet makes the GET inside the first status call after the
 * open, answering nothing meanwhile, so the game stopped for the round
 * trip on every poll (and on the CoCo, the Apple II and MS-DOS lost frames
 * that clock_sync then put back in one lurch). A TCP status is quick, so
 * it is asked at most once every NT_POLL_EVERY frames, and the socket is
 * read only when the status says bytes wait. The server keeps no
 * more than STREAM_WINDOW bytes unacknowledged, so a read brings all that
 * waits. With NT_BUS_READ (MS-DOS, fujinet-lib-experimental) the read is
 * the bus call itself: fujinet-lib's network_read_nb would ask the status
 * again first, and at 9600 bps (a PCjr) a call is 25 ms.
 */
#if defined(PLAT_NETSTREAM) && !defined(__ATARI__) && !defined(__LYNX__) && !defined(__ADAM__) \
    && !defined(PLAT_FUJIBUS)

#include <string.h>
#include "mule.h"
#include "nstrans.h"
#include "fujinet-network.h"

#ifndef NT_POLL_EVERY
#define NT_POLL_EVERY 1         /* frames between status calls */
#endif
#define NT_RBUF 128             /* more waiting is read at once (more) */

static char dev[72];
static uint8_t rbuf[NT_RBUF], obuf[32];     /* our frames are 27 bytes at most */
static uint16_t rlen, rpos, polled;
static uint8_t olen, oover, more;
#ifdef NT_BUS_READ
static uint8_t unit;            /* the N: unit, for the bus read */
#endif
#ifdef PLAT_NET_GUARD
uint16_t nt_fail;               /* calls that failed (tools/dossim peeks it) */
#define NT_FAILED() (++nt_fail)
#else
#define NT_FAILED()
#endif

#ifdef __APPLE2__
/* Only boot.c calls these: they live in its overlay (main RAM is full). */
#pragma code-name (push, "OVERLAY1")
#pragma rodata-name (push, "OVERLAY1")
#endif

uint8_t nt_detect(void)
{
#ifdef PLAT_NET_PROBE
    /* a platform whose network_init() can't tell asks its own way */
    if (!plat_net_probe())
        return 0;
#endif
    return network_init() == FN_ERR_OK;
}

#ifdef __APPLE2__
#pragma rodata-name (pop)
#pragma code-name (pop)
#endif

/* "N:TCP://host:port/": the connection is made here (a wait, once). */
uint8_t nt_open(const char *host, uint16_t port)
{
    char *p = dev;
    const char *s = "N:TCP://";
    while (*s)
        *p++ = *s++;
    while (*host && p < dev + sizeof(dev) - 8)
        *p++ = *host++;
    *p++ = ':';
    p += fmt_num(p, port);
    *p++ = '/';
    *p = 0;
    rlen = rpos = 0;
    olen = oover = more = 0;
    polled = frame_count - NT_POLL_EVERY;
#ifdef NT_BUS_READ
    unit = network_unit(dev);
#endif
    if (network_open(dev, OPEN_MODE_RW, OPEN_TRANS_NONE) != FN_ERR_OK)
        return 0;
    return 1;
}

uint8_t nt_close(void)
{
    network_close(dev);
    return 1;
}

void nt_quiesce(void)
{
}                               /* (every call here is over when it returns) */

uint8_t nt_get(char *c)
{
    uint16_t bw;
    uint8_t conn, err, bad;
    int16_t n;

    if (rpos < rlen) {
        *c = rbuf[rpos++];
        return 1;
    }
    if (!more && (uint16_t)(frame_count - polled) < NT_POLL_EVERY)
        return 0;                   /* asked lately */
    polled = frame_count;
    more = 0;
    NET_GUARD(1);
    bad = network_status(dev, &bw, &conn, &err) != FN_ERR_OK;
    NET_GUARD(0);
    if (bad) {
        NT_FAILED();
        return 0;
    }
    if (!bw)
        return conn ? 0 : NT_LOST;
    if (bw > sizeof(rbuf)) {
        bw = sizeof(rbuf);
        more = 1;                   /* the rest straight after */
    }
    NET_GUARD(1);
#ifdef NT_BUS_READ                  /* fujinet-lib-experimental: the read itself */
    n = (int16_t)network_bus_read(FUJI_DEVICEID_NETWORK + unit - 1, rbuf, bw);
#else
    n = network_read_nb(dev, rbuf, bw);
#endif
    NET_GUARD(0);
    if (n <= 0) {
        /* the FujiNet took them from the socket all the same: they are
         * counted as taken, or the server's credit would be short for good */
        NT_FAILED();
        nt_dropped = bw;
        return NT_DROP;
    }
    rlen = (uint16_t)n;
    rpos = 1;
    *c = rbuf[0];
    return 1;
}

/* A frame is gathered, then sent in one write. */
uint8_t nt_put(uint8_t c)
{
    if (olen >= sizeof(obuf)) {
        oover = 1;
        return 0;
    }
    obuf[olen++] = c;
    return 1;
}

uint8_t nt_flush(void)
{
    uint8_t ok = 1;
    if (olen && !oover) {
        NET_GUARD(1);
        if (network_write(dev, obuf, olen) != FN_ERR_OK) {
            NT_FAILED();
            ok = 0;
        }
        NET_GUARD(0);
    }
    olen = oover = 0;
    return ok;
}

#endif
