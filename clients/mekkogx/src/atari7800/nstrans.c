/*
 * nstrans.c -- stream mode's bytes on the 7800 (src/nsnet.c): the N:TCP
 * socket's calls put to the FujiNet cartridge's mailbox by hand, never
 * waited on (the NES's src/nes/nstrans.c, at the 7800 cart's addresses).
 *
 * fujinet-lib's fuji_bus_call (fujinet-lib-experimental bus/atari7800) writes a
 * request into the mailbox, launches it and spins until the cartridge has
 * the reply: the game would stop for every round trip. Here a request is
 * launched and the mailbox looked at again on the frames that follow, as
 * src/adam/nstrans.c does with its DCB and src/nstrans_fb.c with its wire.
 *
 * The mailbox (fujinet-firmware pico/atari-7800/firmware/include/
 * fuji_mailbox.h): a register is set by a store to $0D00 + its number; each
 * store to $0F00 adds a byte to the request (each parameter its size, then
 * its value, low byte first; then the data). A sequence number different
 * from the last one acknowledged launches it; when ACKSEQ ($0C00) has
 * caught up, the reply is in the 1K window at $0800, its length at $0C04.
 * Only plain stores may touch $0D00-$0FFF (a read-modify-write's dummy
 * write would be an event of its own), and MARIA must never read there.
 *
 * Each step is one of: a WRITE of what nsnet has flushed (it goes first),
 * else a STATUS, whose reply says how many bytes wait and whether the
 * socket is up, and a READ only when some do. The open and the close are
 * fujinet-lib's (a wait, once). Before anything else uses the mailbox
 * (appkeys), nt_quiesce lets the call going finish.
 */
#if defined(__ATARI7800__) && defined(PLAT_NETSTREAM)

#include <string.h>
#include "mule.h"
#include "nstrans.h"
#include "fujinet-network.h"
#include "fujinet-atari7800.h"
#include "fujinet-fuji.h"

#define MB_REPLY    ((volatile uint8_t *)0x0800)
#define MB_ACKSEQ   (*(volatile uint8_t *)0x0C00)
#define MB_ERR      (*(volatile uint8_t *)0x0C02)
#define MB_REPLYCMD (*(volatile uint8_t *)0x0C03)
#define MB_RXLEN_LO (*(volatile uint8_t *)0x0C04)
#define MB_RXLEN_HI (*(volatile uint8_t *)0x0C05)
#define MB_REG(r)   (*(volatile uint8_t *)(0x0D00 + (r)))
#define MB_TX       (*(volatile uint8_t *)0x0F00)

#define R_DEVICE   0x00
#define R_CMD      0x01
#define R_NPARAM   0x02
#define R_DATA_RST 0x05
#define R_SEQ      0x10

#define DEV_NET   0x71          /* N1: */
#define C_STATUS  0x53
#define C_READ    0x52
#define C_WRITE   0x57
#define ACK       0x06

#define WAIT_MAX   330          /* frames for a reply (the cart's own is 5 s) */
#define DOWN_AFTER 60           /* statuses saying the socket is shut */
#define WRITE_TRIES 10

enum { T_IDLE, T_STATUS, T_READ, T_WRITE };

static char dev[72];
static uint8_t state, down, wtries, seq, hold, dropped;
static uint8_t rbuf[256];
static uint16_t rlen, rpos, polled, started, want;
static uint8_t fbuf[32];        /* the frame being put */
static uint8_t flen, fover;
static uint8_t qbuf[96], qlen;  /* flushed frames, not yet written */
static uint8_t tlen;            /* how many of them the write going has */

/* The cartridge, and a FujiNet answering through it (the cartridge alone
 * answers "no link" after a wait). Only boot asks: its overlay. */
#pragma code-name (push, "OVERLAY1")
uint8_t nt_detect(void)
{
    uint8_t s;
    return fuji_a7800_present() && fuji_get_wifi_status(&s);
}
#pragma code-name (pop)

/* A request goes: the registers, the parameter (a 16-bit one, when n16),
 * the data; then the launch. */
static void request(uint8_t cmd, uint8_t n16, uint16_t v, const uint8_t *data, uint8_t n)
{
    MB_REG(R_DATA_RST) = 0;
    MB_REG(R_DEVICE) = DEV_NET;
    MB_REG(R_CMD) = cmd;
    MB_REG(R_NPARAM) = 0;
    if (n16) {
        MB_TX = 2;
        MB_TX = (uint8_t)v;
        MB_TX = (uint8_t)(v >> 8);
        MB_REG(R_NPARAM) = 1;
    }
    while (n--)
        MB_TX = *data++;
    /* the cart's sequence, not ours: a console Reset doesn't reset it */
    seq = MB_ACKSEQ + 1;
    if (!seq)
        seq = 1;
    MB_REG(R_SEQ) = seq;
    started = frame_count;
}

/* 0 not done yet, 1 an ACK, 2 failed (or no answer in time). */
static uint8_t collect(void)
{
    if (MB_ACKSEQ != seq)
        return (uint16_t)(frame_count - started) < WAIT_MAX ? 0 : 2;
    return (MB_ERR == 0 && MB_REPLYCMD == ACK) ? 1 : 2;
}

static uint16_t reply_len(void)
{
    return MB_RXLEN_LO | (uint16_t)MB_RXLEN_HI << 8;
}

static void start_write(void)
{
    request(C_WRITE, 1, tlen, qbuf, tlen);
    state = T_WRITE;
}

/* One look at the mailbox: what finished is taken, and the next call
 * started. */
static void step(void)
{
    uint8_t r;
    uint16_t k;
    volatile uint8_t *s;

    if (state != T_IDLE) {
        r = collect();
        if (r == 0)
            return;                         /* still going */
        switch (state) {
        case T_STATUS:
            state = T_IDLE;
            if (r != 1)
                break;
            if (MB_REPLY[2])                /* connected */
                down = 0;
            else if (down < 255)
                ++down;
            want = MB_REPLY[0] | (uint16_t)MB_REPLY[1] << 8;
            if (want && !hold) {
                if (want > sizeof(rbuf))
                    want = sizeof(rbuf);
                request(C_READ, 1, want, 0, 0);
                state = T_READ;
                return;
            }
            break;
        case T_READ:
            state = T_IDLE;
            if (r == 1) {
                k = reply_len();
                if (k > sizeof(rbuf))
                    k = sizeof(rbuf);
                s = MB_REPLY;
                for (rlen = 0; rlen < k; ++rlen)
                    rbuf[rlen] = s[rlen];
                rpos = 0;
            } else {
                /* the FujiNet took them from the socket all the same: they
                 * are counted as taken, or the server's credit would be
                 * short for good */
                dropped = 1;
            }
            return;                         /* (the bytes first) */
        case T_WRITE:
            state = T_IDLE;
            if (r != 1 && ++wtries < WRITE_TRIES) {
                start_write();              /* the same again */
                return;
            }
            qlen -= tlen;               /* (frames flushed meanwhile stay) */
            memmove(qbuf, qbuf + tlen, qlen);
            break;
        }
    }
    if (hold)
        return;
    if (qlen) {
        tlen = qlen;
        wtries = 0;
        start_write();
        return;
    }
    if (polled != frame_count) {            /* once a frame at most */
        polled = frame_count;
        request(C_STATUS, 0, 0, 0, 0);
        state = T_STATUS;
    }
}

/* What's going finished (bounded: a reply that never comes times out). */
void nt_quiesce(void)
{
    hold = 1;                               /* (nothing new starts) */
    while (state != T_IDLE)
        step();
    hold = 0;
}

/* "N:TCP://host:port/": the connection is made here (a wait, once). */
uint8_t nt_open(const char *host, uint16_t port)
{
    char *p = dev;
    const char *s = "N:TCP://";

    nt_quiesce();
    while (*s)
        *p++ = *s++;
    while (*host && p < dev + sizeof(dev) - 8)
        *p++ = *host++;
    *p++ = ':';
    p += fmt_num(p, port);
    *p++ = '/';
    *p = 0;
    rlen = rpos = 0;
    flen = fover = qlen = 0;
    down = dropped = 0;
    polled = frame_count - 1;
    return network_open(dev, OPEN_MODE_RW, OPEN_TRANS_NONE) == FN_ERR_OK;
}

uint8_t nt_close(void)
{
    nt_quiesce();
    qlen = 0;
    network_close(dev);
    return 1;
}

uint8_t nt_get(char *c)
{
    if (rpos >= rlen) {
        step();
        if (dropped) {
            dropped = 0;
            nt_dropped = want;
            return NT_DROP;
        }
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
    if (flen && !fover && qlen + flen <= sizeof(qbuf)) {
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
