/*
 * nstrans.c -- stream mode's bytes on the Adam (src/nsnet.c): the N:TCP
 * socket's DCB driven by hand, never waited on.
 *
 * fujinet-lib's calls (src/nstrans_fn.c) set a DCB going and spin until
 * it finishes, retrying a timeout 20 times. Each AdamNet exchange takes a
 * while (an emulator answers about once a frame; a network_status is a
 * command and its reply, four or five frames), and a RECEIVE the FujiNet
 * leaves unanswered is a timeout: five seconds in FujiNet Go Adam, twenty
 * times over. Here a call is started and looked at again on the next
 * frame, so the game goes on drawing whatever the bus does.
 *
 * Each step is one of: a WRITE of what nsnet has flushed (it goes first),
 * else an AdamNet STATUS, whose device status byte (DCB+20) says whether
 * bytes wait (bit 0) and the socket is up (bit 1), and a READ only when
 * they do: the FujiNet answers nothing to a read with nothing waiting.
 * The server keeps under 256 bytes unacknowledged, so one read takes all.
 *
 * The open and the close are fujinet-lib's (a wait, once).
 */
#if defined(__ADAM__) && defined(PLAT_NETSTREAM)

#include <string.h>
#include "mule.h"
#include "nstrans.h"
#include "fujinet-network.h"
#include "fujinet-commands.h"

/* The DCB (EOS keeps FEC3h of them at FEC4h, 21 bytes each) */
#define D_STATUS  0             /* the command; 80h and up when done */
#define D_BUF     1
#define D_LEN     3             /* the length; a read's is put back */
#define D_DEV     16
#define D_DEVSTAT 20            /* the device's status byte */

#define C_STATUS  1
#define C_WRITE   3
#define C_READ    4
#define S_OK      0x80

#define NET_DEV   0x09          /* N1: */
#define DS_DATA   0x01          /* bytes wait */
#define DS_CONN   0x02          /* the socket is up */
#define DOWN_AFTER 60           /* statuses saying the socket is shut */
#define WRITE_TRIES 10

enum { T_IDLE, T_STATUS, T_READ, T_WRITE };

static char dev[72];
static volatile uint8_t *dcb;
static uint8_t state, down, wtries;
static uint8_t rbuf[256];
static uint16_t rlen, rpos, polled;
static uint8_t fbuf[32];        /* the frame being put (27 bytes at most) */
static uint8_t flen, fover;
static uint8_t qbuf[96], qlen;  /* flushed frames, not yet written */
static uint8_t tbuf[1 + 96];    /* the write going: 'W', then the frames */
static uint8_t tlen;

uint8_t nt_detect(void)
{
    return 1;                   /* plat_net_probe found the FujiNet */
}

static void start(uint8_t cmd, void *buf, uint16_t len)
{
    dcb[D_BUF] = (uint8_t)(uint16_t)buf;
    dcb[D_BUF + 1] = (uint16_t)buf >> 8;
    dcb[D_LEN] = (uint8_t)len;
    dcb[D_LEN + 1] = len >> 8;
    dcb[D_STATUS] = cmd;            /* (last: this sets it going) */
}

/* What's going finished (bounded: an unanswered call ends in a timeout). */
static void quiesce(void)
{
    volatile uint16_t *fc = (volatile uint16_t *)&frame_count;
    uint16_t t0 = *fc;
    if (!dcb || state == T_IDLE)
        return;
    while (dcb[D_STATUS] < 0x80 && (uint16_t)(*fc - t0) < 400)
        ;
}

/* Before EOS reads a block (src/adam/bank.c): an emulator lets one AdamNet
 * call go at a time, so the block would only wait behind ours anyway. */
void nt_quiesce(void)
{
    quiesce();
}

/* "N:TCP://host:port/": the connection is made here (a wait, once). */
uint8_t nt_open(const char *host, uint16_t port)
{
    uint8_t n = *(uint8_t *)0xFEC3, i;
    volatile uint8_t *d = (volatile uint8_t *)0xFEC4;
    char *p = dev;
    const char *s = "N:TCP://";

    quiesce();
    state = T_IDLE;
    dcb = 0;
    for (i = 0; i < n; ++i, d += 21)
        if ((d[D_DEV] & 0x0F) == NET_DEV) {
            dcb = d;
            break;
        }
    if (!dcb)
        return 0;
    while (*s)
        *p++ = *s++;
    while (*host && p < dev + sizeof(dev) - 8)
        *p++ = *host++;
    *p++ = ':';
    p += fmt_num(p, port);
    *p++ = '/';
    *p = 0;
    rlen = rpos = 0;
    flen = fover = qlen = tlen = 0;
    down = 0;
    polled = frame_count - 1;
    return network_open(dev, OPEN_MODE_RW, OPEN_TRANS_NONE) == FN_ERR_OK;
}

uint8_t nt_close(void)
{
    quiesce();
    state = T_IDLE;
    qlen = tlen = 0;
    network_close(dev);
    return 1;
}

static void start_write(void)
{
    start(C_WRITE, tbuf, tlen);
    state = T_WRITE;
}

/* One look at the DCB: what finished is taken, and the next call started. */
static void step(void)
{
    uint8_t s;

    if (state != T_IDLE) {
        s = dcb[D_STATUS];
        if (s < 0x80)
            return;                         /* still going */
        switch (state) {
        case T_STATUS:
            state = T_IDLE;
            if (s != S_OK)
                break;
            s = dcb[D_DEVSTAT];
            if (s & DS_CONN)
                down = 0;
            else if (down < 255)
                ++down;
            if (s & DS_DATA) {
                start(C_READ, rbuf, sizeof(rbuf));
                state = T_READ;
                return;
            }
            break;
        case T_READ:
            state = T_IDLE;
            if (s == S_OK) {
                rlen = dcb[D_LEN] | (uint16_t)dcb[D_LEN + 1] << 8;
                if (rlen > sizeof(rbuf))
                    rlen = sizeof(rbuf);
                rpos = 0;
            }
            return;                         /* (the bytes first) */
        case T_WRITE:
            state = T_IDLE;
            if (s != S_OK && ++wtries < WRITE_TRIES) {
                start_write();              /* the same again */
                return;
            }
            tlen = 0;
            break;
        }
    }
    if (qlen) {
        tbuf[0] = NETCMD_WRITE;
        memcpy(tbuf + 1, qbuf, qlen);
        tlen = qlen + 1;
        qlen = 0;
        wtries = 0;
        start_write();
        return;
    }
    if (polled != frame_count) {            /* once a frame at most */
        polled = frame_count;
        start(C_STATUS, 0, 0);
        state = T_STATUS;
    }
}

uint8_t nt_get(char *c)
{
    if (rpos >= rlen && dcb) {
        step();
        if (down >= DOWN_AFTER)
            return NT_LOST;
    }
    if (rpos < rlen) {
        *c = rbuf[rpos++];
        return 1;
    }
    return 0;
}

/* A frame is gathered, then queued for the next write. */
uint8_t nt_put(uint8_t c)
{
    if (flen >= sizeof(fbuf)) {
        fover = 1;
        return 0;
    }
    fbuf[flen++] = c;
    return 1;
}

uint8_t nt_flush(void)
{
    uint8_t ok = 0;
    if (flen && !fover && dcb && qlen + flen <= sizeof(qbuf)) {
        memcpy(qbuf + qlen, fbuf, flen);
        qlen += flen;
        ok = 1;
        if (state == T_IDLE)
            step();                         /* it goes now */
    }
    flen = fover = 0;
    return ok;
}

#endif
