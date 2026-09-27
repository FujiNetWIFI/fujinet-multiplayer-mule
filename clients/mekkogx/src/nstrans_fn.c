/*
 * nstrans_fn.c -- stream mode's bytes (src/nsnet.c) through fujinet-lib:
 * an N:TCP socket to the server's stream port. The Adam, the CoCo, the
 * Apple II and MS-DOS use it.
 *
 * It replaced HTTP polls (net.c), which held the machine for the whole
 * request: FujiNet makes the GET inside the first status call after the
 * open, answering nothing meanwhile, so the game stopped for the round
 * trip on every poll (and on the CoCo, the Apple II and MS-DOS lost frames
 * that clock_sync then put back in one lurch). A TCP status is quick, so
 * it is asked at most once every NT_POLL_EVERY frames, and the socket is
 * read only when the status says bytes wait (on the Adam a read with none
 * gets no answer at all: 20 timeouts in fujinet-lib). The server keeps no
 * more than STREAM_WINDOW bytes unacknowledged, so a read brings all that
 * waits.
 */
#if defined(PLAT_NETSTREAM) && !defined(__ATARI__) && !defined(__LYNX__)

#include <string.h>
#include "mule.h"
#include "nstrans.h"
#include "fujinet-network.h"

#ifndef NT_POLL_EVERY
#define NT_POLL_EVERY 1         /* frames between status calls */
#endif
#ifdef __ADAM__
#define NT_RBUF 256             /* ADAMEm: see nt_get */
#else
#define NT_RBUF 128             /* more waiting is read at once (more) */
#endif

static char dev[72];
static uint8_t rbuf[NT_RBUF], obuf[32];     /* our frames are 27 bytes at most */
static uint16_t rlen, rpos, polled;
static uint8_t olen, oover, more;

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
#ifdef __ADAM__
    return 1;
#else
    return network_init() == FN_ERR_OK;
#endif
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
    if (network_open(dev, OPEN_MODE_RW, OPEN_TRANS_NONE) != FN_ERR_OK)
        return 0;
    return 1;
}

uint8_t nt_close(void)
{
    network_close(dev);
    return 1;
}

uint8_t nt_get(char *c)
{
    uint16_t bw;
    uint8_t conn, err;
    int16_t n;

    if (rpos < rlen) {
        *c = rbuf[rpos++];
        return 1;
    }
    if (!more && (uint16_t)(frame_count - polled) < NT_POLL_EVERY)
        return 0;                   /* asked lately */
    polled = frame_count;
    more = 0;
    if (network_status(dev, &bw, &conn, &err) != FN_ERR_OK)
        return 0;
    if (!bw)
        return conn ? 0 : NT_LOST;
#ifdef __ADAM__
    if (bw > sizeof(rbuf))
        return NT_LOST;             /* more than the server may send */
#else
    if (bw > sizeof(rbuf)) {
        bw = sizeof(rbuf);
        more = 1;                   /* the rest straight after */
    }
#endif
#ifdef __ADAM__
    n = network_read_nb(dev, rbuf, sizeof(rbuf));
#else
    n = network_read_nb(dev, rbuf, bw);
#endif
    if (n <= 0)
        return 0;
#ifdef __ADAM__
    if ((uint16_t)n >= sizeof(rbuf))
        n = bw;                     /* the length wasn't put back in the DCB
                                     * (ADAMEm): no reply fills the buffer */
#endif
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

void nt_flush(void)
{
    if (olen && !oover)
        network_write(dev, obuf, olen);
    olen = oover = 0;
}

#endif
