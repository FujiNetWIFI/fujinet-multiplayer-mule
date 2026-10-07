/*
 * nstrans_fn.c -- stream mode's bytes (src/nsnet.c) through fujinet-lib:
 * an N:TCP socket to the server's stream port. The CoCo and the Apple II
 * use it (the Adam drives its DCB itself: src/adam/nstrans.c; the NES and
 * the SMS their cartridge's mailbox: src/nes/nstrans.c, src/sms/nstrans.c;
 * the MSX and MS-DOS put their packets together: src/nstrans_fb.c).
 *
 * It replaced HTTP polls (net.c), which held the machine for the whole
 * request: FujiNet makes the GET inside the first status call after the
 * open, answering nothing meanwhile, so the game stopped for the round
 * trip on every poll (and on the CoCo, the Apple II and MS-DOS lost frames
 * that clock_sync then put back in one lurch). A TCP status is quick, so
 * it is asked at most once every NT_POLL_EVERY frames, and the socket is
 * read only when the status says bytes wait. The server keeps no
 * more than STREAM_WINDOW bytes unacknowledged, so a read brings all that
 * waits.
 *
 * The bus calls hold the machine (a SmartPort call, a DriveWire exchange
 * with the IRQ masked), so there are as few as can be:
 *  - the Apple II reads with sp_read itself: fujinet-lib's network_read_nb
 *    would ask the status again first (two SmartPort calls, not one);
 *  - the CoCo talks DriveWire itself (dw_call): the FujiNet answers its
 *    commands in order and keeps a reply until it is asked for, so a
 *    command and the fetch of its reply go in one write and one read takes
 *    the answer. fujinet-lib asks "ready?" before each part: three round
 *    trips for a status, five for a read, two for a write; here one each.
 */
#if defined(PLAT_NETSTREAM) && !defined(__ATARI__) && !defined(__LYNX__) && !defined(__ADAM__) && !defined(__NES__) \
    && !defined(__SMS__) && !defined(PLAT_FUJIBUS)

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
#ifdef _CMOC_VERSION_
/* fujinet-lib's DriveWire (through the Disk ROM's vectors) */
unsigned char dwread(unsigned char *s, int l);
unsigned char dwwrite(unsigned char *s, int l);
void bus_ready(void);
#define OP_NET          0xE3
#define DW_GET_RESPONSE 0x01
#define DW_SEND_ERROR   0x02
static uint8_t unit;            /* the N: unit */
static uint8_t dwq[10];
#endif
#ifdef __APPLE2__
extern uint8_t sp_network;      /* fujinet-lib's SmartPort side */
extern uint8_t sp_payload[];
extern uint16_t sp_count;       /* what the call brought (X/Y) */
int8_t sp_read(uint8_t dest, uint16_t len);
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
#ifdef _CMOC_VERSION_
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

#ifdef _CMOC_VERSION_
/* An N: command (its aux, big-endian as the CoCo is; a write's data after
 * it), then the fetch of its answer (GET_RESPONSE, len bytes, or
 * SEND_ERROR, one), all written at once; the answer in one read. 0: it
 * didn't come (the FujiNet is asked "ready?" until it is: in step again). */
static uint8_t dw_call(uint8_t cmd, uint16_t aux, uint8_t *data, uint8_t fetch,
                       uint8_t *reply, uint16_t len)
{
    dwq[0] = dwq[5] = OP_NET;
    dwq[1] = dwq[6] = unit;
    dwq[2] = cmd;
    *(uint16_t *)(dwq + 3) = aux;
    dwq[7] = fetch;
    *(uint16_t *)(dwq + 8) = len;
    dwwrite(dwq, 5);
    if (data)
        dwwrite(data, (int)aux);
    dwwrite(dwq + 5, 5);
    if (dwread(reply, (int)len))
        return 1;
    bus_ready();
    return 0;
}
#endif

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
#ifdef _CMOC_VERSION_
    {
        uint8_t sr[4];          /* bytes waiting (big-endian), connected, error */
        bad = !dw_call('S', 0, 0, DW_GET_RESPONSE, sr, 4);
        bw = *(uint16_t *)sr;
        conn = sr[2];
    }
#else
    bad = network_status(dev, &bw, &conn, &err) != FN_ERR_OK;
#endif
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
#if defined(_CMOC_VERSION_)
    n = dw_call('R', bw, 0, DW_GET_RESPONSE, rbuf, bw) ? (int16_t)bw : 0;
#elif defined(__APPLE2__)
    n = 0;
    if (sp_read(sp_network, bw) == 0) {
        /* only the bytes it says it brought (the rest of sp_payload is an
         * older reply's, and would garble the frame); a firmware that
         * says nothing brought them all */
        if (sp_count && sp_count < bw)
            bw = sp_count;
        memcpy(rbuf, sp_payload, bw);
        n = (int16_t)bw;
    }
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
#ifdef _CMOC_VERSION_
        {
            uint8_t e = 0;          /* how it went: 1, the FujiNet's SUCCESS */
            if (!dw_call('W', olen, obuf, DW_SEND_ERROR, &e, 1) || e != 1)
                ok = 0;
        }
#else
        if (network_write(dev, obuf, olen) != FN_ERR_OK) {
            NT_FAILED();
            ok = 0;
        }
#endif
        NET_GUARD(0);
    }
    olen = oover = 0;
    return ok;
}

#endif
