/*
 * nstrans.c -- stream mode's bytes on the Adam (src/nsnet.c): an N:TCP
 * socket to the server's stream port.
 *
 * An HTTP poll held the Z80 for the whole request: FujiNet makes the GET
 * inside its AdamNet handler, answering nothing meanwhile, and EOS waits
 * it out. A TCP status is quick, so it is asked at most once a frame, and
 * the socket is read only when the status says bytes wait (a read with
 * none gets no answer at all: 20 timeouts in fujinet-lib). A read brings
 * all that waits; the server keeps no more than STREAM_WINDOW of them
 * unacknowledged, so the buffer holds it.
 */
#ifdef __ADAM__

#include <string.h>
#include "mule.h"
#include "nstrans.h"
#include "fujinet-network.h"

static char dev[80];
static uint8_t rbuf[256], obuf[160];
static uint16_t rlen, rpos, polled;
static uint8_t olen, oover;

uint8_t nt_detect(void)
{
    return plat_net_probe();
}

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
    olen = oover = 0;
    polled = frame_count - 1;
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
    if (polled == frame_count)
        return 0;                   /* asked this frame already */
    polled = frame_count;
    if (network_status(dev, &bw, &conn, &err) != FN_ERR_OK)
        return 0;
    if (!bw)
        return conn ? 0 : NT_LOST;
    if (bw > sizeof(rbuf))
        return NT_LOST;             /* more than the server may send */
    n = network_read_nb(dev, rbuf, sizeof(rbuf));
    if (n <= 0)
        return 0;
    if ((uint16_t)n >= sizeof(rbuf))
        n = bw;                     /* the length wasn't put back in the DCB
                                     * (ADAMEm): no reply fills the buffer */
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
