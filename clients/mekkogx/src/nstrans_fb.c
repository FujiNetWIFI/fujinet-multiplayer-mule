/*
 * nstrans_fb.c -- stream mode's bytes (src/nsnet.c) on a FujiNet that
 * speaks FujiBus (the MSX's cartridge; MS-DOS's serial port): the N:TCP
 * socket's packets put together here and never waited on.
 *
 * fujinet-lib's fuji_bus_call writes a request, then spins until the reply
 * has come in whole: the game stops for every round trip. Here a request is
 * written and its reply gathered from the wire (src/fbport.h) on the frames
 * that follow, as src/adam/nstrans.c does with its DCB.
 *
 * Each step is one of: a WRITE of what nsnet has flushed (it goes first),
 * else a STATUS, whose reply says how many bytes wait and whether the socket
 * is up, and a READ only when some do (FB_RBUF at most).
 *
 * A packet: device, command, length (16 bits, the header's 6 bytes
 * included), a checksum of it all (taken with itself 0), a field
 * descriptor, then the fields and data; SLIP-framed on the wire. The reply
 * is the same, with ACK (06h) for the command.
 *
 * The open and the close are fujinet-lib's (a wait, once). Before anything
 * else uses the bus (appkeys), nt_quiesce lets the call going finish.
 *
 * With FB_PROBE (MS-DOS) the wire is found at the first open: a STATUS on
 * each candidate (fb_try), and the first to answer is kept. If none does,
 * the same calls go through fujinet-lib instead, each one waited on (lib),
 * and the socket is asked only every LIB_EVERY frames.
 */
#if defined(PLAT_NETSTREAM) && defined(PLAT_FUJIBUS)

#include <string.h>
#include "mule.h"
#include "nstrans.h"
#include "fbport.h"
#include "fujinet-network.h"

#define DEV_NET   0x71          /* N1: */
#define C_STATUS  0x53
#define C_READ    0x52
#define C_WRITE   0x57
#define C_RESEND  0x05          /* fujinet-pc-rs232: the last reply again
                                 * (elsewhere a NAK, at once) */
#define F_REPLY   0x10          /* (fujinet-lib sends its descriptor bits) */
#define F_B12_D   0x0D          /* a 16-bit field, then data */
#define F_B12_R   0x15          /* a 16-bit field; data back */
#define ACK       0x06

#define SL_END    0xC0
#define SL_ESC    0xDB
#define SL_ESCEND 0xDC
#define SL_ESCESC 0xDD

#define WAIT_MAX   300          /* frames for a reply before it's given up */
#define DOWN_AFTER 60           /* statuses saying the socket is shut */
#define WRITE_TRIES 10
#ifndef FB_RESENDS
#define FB_RESENDS 0            /* asks for a garbled reply again */
#endif
#ifndef FB_RBUF
#define FB_RBUF 256             /* the most a read takes */
#endif
#ifndef FB_QBUF
#define FB_QBUF 96              /* flushed frames waiting (27 bytes at most each) */
#endif

enum { T_IDLE, T_STATUS, T_READ, T_WRITE };

static char dev[72];
static uint8_t state, down, wtries, resends, dropped, hold;
static uint8_t rbuf[FB_RBUF];
static uint16_t rlen, rpos, polled, started, want;
static uint8_t fbuf[32];        /* the frame being put */
static uint8_t flen, fover;
static uint8_t qbuf[FB_QBUF], qlen;     /* flushed frames, not yet written */
static uint8_t tbuf[2 + FB_QBUF];       /* the write going: its length, the frames */
static uint8_t tlen;

/* the reply coming in */
static uint8_t hdr[6], sbuf[4];
static uint8_t *dst;
static uint16_t dmax, got, sum;
static uint8_t inframe, esc;

#ifdef FB_PROBE
#define PROBE_WAIT 30           /* frames for a candidate's answer */
#define LIB_EVERY  6            /* frames between statuses through the lib */
static uint8_t found, lib, libr;
#endif

uint8_t nt_detect(void)
{
#ifdef PLAT_NET_PROBE
    if (!plat_net_probe())      /* (fujinet-lib-experimental's init says yes) */
        return 0;
#endif
    return network_init() == FN_ERR_OK;
}

static uint16_t fold(uint16_t chk, const uint8_t *p, uint8_t n)
{
    while (n--) {
        chk += *p++;
        chk = (chk >> 8) + (chk & 0xFF);
    }
    return chk;
}

static void out(uint8_t c)
{
    if (c == SL_END) {
        fb_putc(SL_ESC);
        c = SL_ESCEND;
    } else if (c == SL_ESC) {
        fb_putc(SL_ESC);
        c = SL_ESCESC;
    }
    fb_putc(c);
}

#ifdef FB_PROBE
/* No wire of our own: the call through fujinet-lib (FUJINET.SYS), done
 * when it returns. */
static void lib_call(uint8_t cmd, const uint8_t *data, uint8_t n, uint8_t *to)
{
    uint16_t k;
    libr = 2;
    got = 6;
    NET_GUARD(1);
    if (cmd == C_STATUS) {
        if (network_status(dev, (uint16_t *)to, to + 2, to + 3) == FN_ERR_OK)
            libr = 1;
    } else if (cmd == C_READ) {
        k = network_bus_read(DEV_NET, to, want);
        if (k) {
            got = 6 + k;
            libr = 1;
        }
    } else if (cmd == C_WRITE && network_bus_write(DEV_NET, data + 2, n - 2)) {
        libr = 1;
    }
    NET_GUARD(0);
}
#endif

/* A request goes, whole; its reply's data will land in to[0..max). */
static void request(uint8_t cmd, uint8_t fields, const uint8_t *data, uint8_t n,
                    uint8_t *to, uint16_t max)
{
    uint8_t h[6], i;
    dst = to;
    dmax = max;
    got = sum = 0;
    inframe = esc = 0;
    started = frame_count;
#ifdef FB_PROBE
    if (lib) {
        lib_call(cmd, data, n, to);
        return;
    }
#endif
    h[0] = DEV_NET;
    h[1] = cmd;
    h[2] = 6 + n;
    h[3] = 0;
    h[4] = 0;
    h[5] = fields;
    h[4] = (uint8_t)fold(fold(0, h, 6), data, n);
    while (fb_ready())              /* a late reply to something gone */
        (void)fb_getc();
    fb_putc(SL_END);
    for (i = 0; i < 6; ++i)
        out(h[i]);
    for (i = 0; i < n; ++i)
        out(data[i]);
    fb_putc(SL_END);
    fb_sent();
}

/* The reply's bytes, as many as wait: 0 not all here yet, 1 an ACK, 2 bad
 * (or a NAK). */
static uint8_t collect(void)
{
    uint8_t c, n = 0;
#ifdef FB_PROBE
    if (lib)
        return libr;
#endif
    while (fb_ready()) {
        c = fb_getc();
        if (c == SL_END) {
            if (!inframe || !got) {
                inframe = 1;        /* (the opening END) */
                continue;
            }
            inframe = 0;
            if (got != (hdr[2] | (uint16_t)hdr[3] << 8) || (uint8_t)sum != hdr[4]
                || hdr[0] != DEV_NET)
                return 2;
            return hdr[1] == ACK ? 1 : 2;
        }
        if (!inframe)
            continue;
        if (esc) {
            esc = 0;
            if (c == SL_ESCEND)
                c = SL_END;
            else if (c == SL_ESCESC)
                c = SL_ESC;
        } else if (c == SL_ESC) {
            esc = 1;
            continue;
        }
        if (got < 6)
            hdr[got] = c;
        else if (got - 6 < dmax)
            dst[got - 6] = c;
        sum += got == 4 ? 0 : c;    /* (taken with the checksum 0) */
        sum = (sum >> 8) + (sum & 0xFF);
        ++got;
        if (++n == 0)
            break;                  /* 255 a call is plenty */
    }
    return 0;
}

static void start_status(void)
{
    request(C_STATUS, F_REPLY, 0, 0, sbuf, sizeof(sbuf));
    state = T_STATUS;
}

static void start_read(void)
{
    uint8_t l[2];
    l[0] = (uint8_t)want;
    l[1] = want >> 8;
    request(C_READ, F_B12_R, l, 2, rbuf, sizeof(rbuf));
    state = T_READ;
}

static void start_write(void)
{
    request(C_WRITE, F_B12_D, tbuf, tlen, 0, 0);
    state = T_WRITE;
}

/* One look at the wire: what finished is taken, and the next call started. */
static void step(void)
{
    uint8_t r;

    if (state != T_IDLE) {
        r = collect();
        if (r == 0 && (uint16_t)(frame_count - started) < WAIT_MAX)
            return;                         /* still going */
#if FB_RESENDS
        if (r != 1 && resends < FB_RESENDS) {
            ++resends;                      /* garbled, or cut short */
            request(C_RESEND, 0, 0, 0, dst, dmax);
            return;
        }
        resends = 0;
#endif
        switch (state) {
        case T_STATUS:
            state = T_IDLE;
            if (r != 1)
                break;
            if (sbuf[2])                    /* connected */
                down = 0;
            else if (down < 255)
                ++down;
            want = sbuf[0] | (uint16_t)sbuf[1] << 8;
            if (want && !hold) {
                if (want > sizeof(rbuf))
                    want = sizeof(rbuf);
                start_read();
                return;
            }
            break;
        case T_READ:
            state = T_IDLE;
            if (r == 1) {
                rlen = got - 6;
                if (rlen > sizeof(rbuf))
                    rlen = sizeof(rbuf);
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
            tlen = 0;
            break;
        }
    }
    if (hold)
        return;
    if (qlen) {
        tbuf[0] = qlen;
        tbuf[1] = 0;
        memcpy(tbuf + 2, qbuf, qlen);
        tlen = qlen + 2;
        qlen = 0;
        wtries = 0;
        start_write();
        return;
    }
#ifdef FB_PROBE
    if (lib && (uint16_t)(frame_count - polled) < LIB_EVERY)
        return;
#endif
    if (polled != frame_count) {            /* once a frame at most */
        polled = frame_count;
        start_status();
    }
}

/* What's going finished (bounded: a reply that never comes times out). */
void nt_quiesce(void)
{
    hold = 1;                               /* (nothing new starts) */
    while (state != T_IDLE)
        step();
    hold = 0;
    fb_release();
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
    flen = fover = qlen = tlen = 0;
    down = resends = dropped = 0;
    polled = frame_count - 1;
    if (network_open(dev, OPEN_MODE_RW, OPEN_TRANS_NONE) != FN_ERR_OK)
        return 0;
#ifdef FB_PROBE
    if (!found && !lib) {
        uint8_t i, r = 0;
        for (i = 0; r != 1 && fb_try(i); ++i) {
            start_status();
            while (!(r = collect()) && (uint16_t)(frame_count - started) < PROBE_WAIT)
                ;
        }
        state = T_IDLE;
        found = r == 1;
        if (found)
            fb_keep();
        else
            lib = 1;                        /* no wire answered: the lib's */
        fb_release();
    }
#endif
    return 1;
}

uint8_t nt_close(void)
{
    nt_quiesce();
    qlen = tlen = 0;
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
