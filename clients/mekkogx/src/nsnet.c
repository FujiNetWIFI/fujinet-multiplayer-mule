/*
 * nsnet.c -- mule.h's network calls in stream mode: one TCP connection to
 * the server instead of an HTTP GET a poll (net.c), as on the Atari
 * (src/atari/nsnet.c, docs/PROTOCOL.md "Stream mode"). Everyone but the
 * Atari and the MSX shares it; each brings the bytes its own way
 * (nstrans.h):
 *
 *   Lynx  FujiNet-Lynx's netstream: ComLynx becomes a raw pipe to the
 *         socket, for good (src/lynx/nstrans.c).
 *   Adam, CoCo, Apple II, MS-DOS
 *         an N:TCP socket, read only when its status says bytes wait: an
 *         HTTP poll held the machine for the whole request
 *         (src/nstrans_fn.c).
 *
 * The receiver's ring is small: the server sends at most STREAM_WINDOW
 * bytes we haven't credited (SC_CREDIT), so it can't overflow while the
 * game draws or loads an overlay.
 */
#if defined(PLAT_NETSTREAM) && !defined(__ATARI__)

#include <string.h>
#include "mule.h"
#include "nstrans.h"

#ifndef NS_PORT
#define NS_PORT  STREAM_PORT
#endif

#define PING_EVERY 120          /* quiet frames between keepalives: the
                                 * server hears the seat (or it stops its
                                 * line) and gets any credit it lost */
#define DEAD_AFTER 300          /* frames without a frame: the link is gone */
#define HELLO_WAIT 30           /* frames before HELLO goes again: FujiNet */
#define HELLO_MAX  10           /* answers before its socket is open */
#define FMAX       136          /* a frame, encoded (STREAM_MAX_FRAME + COBS) */
#define RESYNC_GAP 30           /* frames between RESYNCs: one bad frame (an
                                 * echo of our own, a collision on the wire)
                                 * mustn't start a storm of them */
#define RESEND_AFTER 180        /* frames before an unanswered action goes
                                 * again: its answer was lost (a bad frame,
                                 * or the RESYNC that followed emptied the
                                 * server's queue) and nothing else would
                                 * ever let the next one go */

uint8_t ns_on;                  /* the link is up */
#if defined(__SMS__) || defined(MULE_AUTOPLAY)
/* (for the harness) states taken, bad frames, RESYNCs sent, links lost */
uint8_t ns_cnt[4];
#define NS_COUNT(i) (++ns_cnt[i])
#else
#define NS_COUNT(i)
#endif
uint16_t nt_dropped;            /* (nstrans.h) */
static uint8_t ns_taken;        /* bytes taken, mod 256 (SC_CREDIT) */
static uint8_t fbuf[FMAX];
static uint8_t flen, fover;

#ifdef __APPLE2__
/* The state being built is kept in aux RAM (mule-hgr.cfg's AUXSTAGE): main
 * RAM has no 550 bytes to spare. AUXMOVE takes it there and back, the
 * whole state in two halves so the clock (frame_tick) keeps counting. */
void aux_get(void *dst, const void *src, uint16_t n);  /* lowlevel.s */
void aux_put(void *dst, const void *src, uint16_t n);
extern uint8_t _AUXSTAGE__[];
#define stage_put(off, p, n) aux_put(_AUXSTAGE__ + (off), (p), (n))
static void stage_get(void)
{
    aux_get(&st, _AUXSTAGE__, 275);
    frame_tick();
    aux_get((uint8_t *)&st + 275, _AUXSTAGE__ + 275, sizeof(MuleState) - 275);
}
#else
static uint8_t stage[sizeof(MuleState)];
#define stage_put(off, p, n) memcpy(stage + (off), (p), (n))
#define stage_get() memcpy(&st, stage, sizeof(MuleState))
#endif
static uint8_t joined, heard;
static uint8_t seq, wait_seq, wait_act, credited, hello_n, hello_t, resync_hold;
static uint8_t resync_owed;     /* a bad frame came during the hold: ask after it */
static uint8_t pending;         /* an action whose write failed: again, as it was */
static uint8_t wait_t;          /* frames its answer has been awaited */
static uint16_t quiet;

typedef char mule_state_is_550_bytes[sizeof(MuleState) == 550 ? 1 : -1];
typedef char mule_table_is_33_bytes[sizeof(MuleTable) == 33 ? 1 : -1];

/* (a + b) mod 255 for a < 255: Fletcher-16's sums, without a division
 * (a 16-bit one a byte, twice, was most of a frame's work in a burst). */
static uint8_t add255(uint8_t a, uint8_t b)
{
    uint16_t t = (uint16_t)a + b;
    if (t >= 255)
        t -= 255;
    return (uint8_t)t;
}

/* ---- bytes ---------------------------------------------------------------- */

static uint8_t put_ok, lost;

static void put(uint8_t c)
{
    if (put_ok && !nt_put(c))
        put_ok = 0;
}

/* One frame from the ring: 0 none (yet), 255 a bad one (dropped), else its
 * decoded length (type, payload; the sum left off) in fbuf. */
static uint8_t take_frame(void)
{
    char c;
    uint8_t i, o, k, code, n, s1, s2;
    for (;;) {
        k = nt_get(&c);
        if (k == NT_DROP) {
            ns_taken += (uint8_t)nt_dropped;    /* credit them all the same */
            fover = 1;              /* the frame they were in is bad */
            continue;
        }
        if (k != 1) {
            if (k == NT_LOST)
                lost = 1;
            return 0;
        }
        ++ns_taken;
        if (c) {
            if (flen < FMAX)
                fbuf[flen++] = c;
            else
                fover = 1;
            continue;
        }
        n = flen;
        flen = 0;
        if (fover) {
            fover = 0;
            return 255;
        }
        if (n == 0)
            continue;               /* an empty frame (or our own 0, echoed) */
        /* COBS, in place: a code is 1 + the bytes up to the next 0 (none
         * after the last block, nor after a full one of 254) */
        i = o = 0;
        while (i < n) {
            code = fbuf[i++];
            for (k = 1; k < code; ++k) {
                if (i >= n)
                    return 255;
                fbuf[o++] = fbuf[i++];
            }
            if (code != 0xFF && i < n)
                fbuf[o++] = 0;
        }
        if (o < 3)
            return 255;
        o -= 2;
        s1 = s2 = 0;
        for (i = 0; i < o; ++i) {
            s1 = add255(s1, fbuf[i]);
            s2 = add255(s2, s1);
        }
        if (fbuf[o] != s1 || fbuf[o + 1] != s2)
            return 255;
        return o;
    }
}

/* ---- sending ------------------------------------------------------------ */

/* A frame: COBS of type, payload, Fletcher-16; then 0. A frame given up
 * half way is ended with its 0 all the same (the server drops it). 0: the
 * write failed (it may have gone all the same: the FujiNet's answer lost). */
static uint8_t send(uint8_t type, const uint8_t *p, uint8_t n)
{
    static uint8_t raw[24];
    uint8_t s1, s2, i, code, from;
    raw[0] = type;
    memcpy(raw + 1, p, n);
    ++n;
    s1 = s2 = 0;
    for (i = 0; i < n; ++i) {
        s1 = add255(s1, raw[i]);
        s2 = add255(s2, s1);
    }
    raw[n++] = s1;
    raw[n++] = s2;
    put_ok = 1;
    from = 0;
    for (i = 0; i <= n; ++i) {
        if (i == n || raw[i] == 0) {
            code = i - from + 1;
            put(code);
            for (; from < i; ++from)
                put(raw[from]);
            ++from;                 /* past the 0 */
        }
    }
    put_ok = 1;
    put(0);
    return nt_flush();
}

/* Lost or garbled: the whole state again, once a RESYNC_GAP at most. One
 * held back is only put off: the server counts what was lost as ours, so
 * nothing else would ever bring it (the map, the auction's setup). */
static void resync(void)
{
    resync_owed = 1;
    if (!resync_hold) {
        NS_COUNT(2);
        send(SC_RESYNC, 0, 0);
        resync_hold = RESYNC_GAP;
        resync_owed = 0;
    }
}

static void send_credit(void)
{
    credited = ns_taken;
    send(SC_CREDIT, &credited, 1);
}

/* (Re)join. An action the old link never answered goes again. */
static void send_hello(void)
{
    static uint8_t h[18];
    memset(h, 0, sizeof(h));
    h[0] = STREAM_VERSION;
#ifdef PLAT_BIG_ENDIAN
    h[1] = 1;                   /* the 16- and 32-bit fields in our order */
#endif
#ifdef PLAT_NS_SLOW
    h[1] |= 2;                  /* a slow link: a push at most every 300 ms */
#endif
    strncpy((char *)h + 2, sc_table, 8);
    strncpy((char *)h + 10, sc_name, 8);
    send(SC_HELLO, h, sizeof(h));
    hello_t = 0;
    resync_owed = 0;            /* HELLO brings the whole state */
    if (!joined) {
        joined = 1;
        heard = 0;
        hello_n = 0;
        if (pending) {
            pending = 0;
            wait_seq = 1;       /* (as one sent and unanswered) */
        }
        if (wait_seq && act == ACT_NONE && wait_act != ACT_LEAVE)
            act = wait_act;
        wait_seq = 0;
    }
}

/* ---- the link ----------------------------------------------------------- */

/* "http://host:9900/..." -> host, in fbuf: nothing is being received
 * while we connect (nt_open copies it before a byte comes in). */
static const char *endpoint_host(void)
{
    char *host = (char *)fbuf;
    const char *p = strstr(sc_endpt, "://");
    uint8_t n = 0;
    p = p ? p + 3 : sc_endpt;
    while (*p && *p != ':' && *p != '/' && n < 60)
        host[n++] = *p++;
    host[n] = 0;
    return host;
}

static uint8_t connect(void)
{
    char c;
    if (ns_on)
        return 1;
    if (!nt_open(endpoint_host(), NS_PORT))
        return 0;
    while (nt_get(&c) == 1)
        ;                           /* what the command left behind */
    ns_on = 1;
    ns_taken = credited = flen = fover = 0;
    quiet = 0;
    joined = 0;
    lost = 0;
    return 1;
}

/* The link is gone (the server's silence, or the transport says so): a
 * transport that can open it again does, at the next net_begin. */
static void link_down(void)
{
    NS_COUNT(3);
    joined = 0;
    net_have = 0;
    if (nt_close())
        ns_on = 0;
}

/* Nothing else on the bus needs the stream stopped. */
/* Before plain bus calls (appkeys, net.c): the stream's call going ends. */
void ns_pause(void)
{
    nt_quiesce();
}

void ns_resume(void)
{
}

/* Leaving the program: say goodbye. */
void ns_close(void)
{
    if (ns_on && joined)
        send(SC_BYE, 0, 0);
    joined = 0;
}

/* ---- receiving ---------------------------------------------------------- */

/* The frames waiting, handled. 0: none (yet); 1: a state came in (st);
 * 2: the server refused us (st.h.err). The server sends every change (up
 * to twenty a second), more than a slow machine can draw: of the states
 * waiting only the last is kept, as a poll would have seen it, except
 * that the one answering our action is handed on at once (its error and
 * result are for that state alone). */
static uint8_t take(void)
{
    uint8_t n, got = 0;
    uint16_t off;
    for (;;) {
        n = take_frame();
        if (n == 0)
            return got;
        quiet = 0;
        heard = 1;
        if (n == 255) {
            NS_COUNT(1);
            resync();
            continue;
        }
        switch (fbuf[0]) {
        case CS_PATCH:
            off = fbuf[1] | (uint16_t)fbuf[2] << 8;    /* (CMOC shifts a byte in 8 bits) */
            n -= 3;
            if (off + n <= sizeof(MuleState))
                stage_put(off, fbuf + 3, n);
            break;
        case CS_COMMIT:
            if (!joined)
                break;
            stage_get();
            if (wait_seq && fbuf[1] == wait_seq) {
                act_sent = wait_act;
                wait_seq = pending = 0;
                return 1;
            }
            act_sent = ACT_STATE;
            got = 1;
            NS_COUNT(0);
            break;
        case CS_ERROR:
            st.h.err = fbuf[1];
            return 2;
        }
    }
}

/* ---- mule.h ------------------------------------------------------------- */

void act_send(uint8_t a, uint16_t arg)
{
    act = a;
    act_arg = arg;
    poll_wait = 0;
}

/* Busy while joined: the main loop then steps us every frame. */
uint8_t net_busy(void)
{
    return joined;
}

/* Not joined: join (connecting first). */
void net_begin(void)
{
    net_have = 0;
    if (!connect())
        return;
    if (act == ACT_LEAVE)
        act = ACT_NONE;
    send_hello();
}

uint8_t net_step(void)
{
    static uint8_t a[5];
    uint8_t r;
    if (!joined)
        return 0;
    if (resync_hold)
        --resync_hold;
    else if (resync_owed)
        resync();
    if (!heard && ++hello_t >= HELLO_WAIT && hello_n < HELLO_MAX) {
        ++hello_n;
        send_hello();               /* the socket may not have been open */
    }
    if (pending) {
        /* the write failed, or its answer never came: the same frame again
         * (the same seq: if it did go, the server answers it once more
         * rather than act twice) */
        if (send(SC_ACT, a, 5)) {
            pending = 0;
            wait_seq = a[0];
            wait_t = 0;
        }
    } else if (wait_seq) {
        if (++wait_t >= RESEND_AFTER)
            pending = 1;
    } else if (act != ACT_NONE) {
        if (++seq == 0)
            seq = 1;
        a[0] = seq;
        a[1] = act;
        a[2] = (uint8_t)act_arg;
        a[3] = act_arg >> 8;
        a[4] = au_traded;       /* the units we have shown (ACT_PUSH) */
        wait_act = act;
        act = ACT_NONE;
        wait_t = 0;
        if (send(SC_ACT, a, 5))
            wait_seq = seq;
        else
            pending = 1;
    }
    r = take();
    if ((uint8_t)(ns_taken - credited) >= 48)
        send_credit();
    if (r) {
        net_have = r;
        if (r == 2)
            joined = 0;
        return 1;
    }
    if (++quiet % PING_EVERY == 0) {
        send_credit();          /* a keepalive, and credit it may have lost */
        send(SC_PING, 0, 0);
    }
    if ((quiet > DEAD_AFTER && heard) || lost) {
        link_down();
        return 1;
    }
    return 0;
}

/* The queued action (or none) answered: wait for its commit. Leaving is
 * answered before the next table is chosen. */
uint8_t net_sync(void)
{
    uint16_t t;
    uint8_t leave = act == ACT_LEAVE;
    if (!joined)
        net_begin();
    if (!joined)
        return net_have;
    if (act == ACT_NONE)
        act = ACT_STATE;
    for (t = 0; t < 600; ++t) {
        frame_wait();
        if (net_step() && (net_have != 1 || !wait_seq))
            break;
    }
    if (leave) {
        joined = 0;             /* the next net_begin says HELLO again */
        wait_seq = 0;
    }
    return net_have;
}

#if defined(__APPLE2__) || defined(__NES__)
/* Only boot.c calls these: they live in its overlay (the Apple's main RAM,
 * the NES's fixed bank and R7's are full). */
#pragma code-name (push, "OVERLAY1")
#pragma rodata-name (push, "OVERLAY1")
#endif

/* Is there a FujiNet? (Once the stream is on, it answers for it.) */
uint8_t net_detect(void)
{
    if (ns_on)
        return 1;
    return nt_detect();
}

/* The table list: count, then MuleTables. Blocking (boot). */
uint8_t net_tables(uint8_t *buf, uint8_t max)
{
    uint16_t t;
    uint8_t n, got = 0, count = 255;
    if (!connect())
        return 0;
    for (t = 0; t < 360 && got < count; ++t) {
        if (count == 255 && t % HELLO_WAIT == 0)
            send(SC_TABLES, 0, 0);      /* again: the socket may have been shut */
        frame_wait();
        snd_tick();                     /* (the name's blip ends) */
        while ((n = take_frame()) != 0) {
            if (n == 255 || fbuf[0] != CS_TABLE)
                continue;               /* a state for the old table: skip */
            count = fbuf[2];
            if (fbuf[1] < max && n >= 3 + sizeof(MuleTable))
                memcpy(buf + 1 + fbuf[1] * sizeof(MuleTable), fbuf + 3, sizeof(MuleTable));
            if (++got >= count)
                break;
        }
        if ((uint8_t)(ns_taken - credited) >= 48)
            send_credit();
    }
    send_credit();
    if (count == 255)
        return 0;
    buf[0] = count > max ? max : count;
    return buf[0];
}

#if defined(__APPLE2__) || defined(__NES__)
#pragma rodata-name (pop)
#pragma code-name (pop)
#endif

#endif
