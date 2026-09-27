/*
 * nsnet.c -- mule.h's network calls in stream mode (the Atari): one TCP
 * connection to the server through FujiNet netstream, instead of an HTTP
 * GET a poll (net.c). docs/PROTOCOL.md "Stream mode".
 *
 * The server pushes the state as it changes: patches to a staged copy,
 * then a commit, when the staged copy becomes st -- a "reply", as a poll's
 * was. An action goes out as soon as it is queued; the commit that answers
 * it carries its sequence number, and its error and result are in that
 * state (act_sent says which action st answers, as before).
 *
 * The receive ring (nstream.s) is 256 bytes and the game drains it once a
 * frame, or not at all while it loads or draws: the server sends at most
 * STREAM_WINDOW bytes we haven't taken, and we tell it (SC_CREDIT) what we
 * took.
 */
#ifdef __ATARI__

#include <string.h>
#include "mule.h"
#include "plat.h"
#include "atari.h"

/* nstream.s */
void ns_begin(void);
void ns_end(void);
void ns_reset(void);
void __fastcall__ ns_put(uint8_t c);
uint8_t ns_frame(void);
extern uint8_t ns_on, ns_audf3, ns_skctl, ns_err, ns_taken;
extern uint8_t ns_fbuf[];

#ifndef NS_PORT
#define NS_PORT  STREAM_PORT
#endif
#define NS_AUDF3 39             /* 19200 baud (NTSC and PAL alike) */
#define NS_FLAGS 0x05           /* TCP, TX clock external (FujiNet's) */
#define NS_SKCTL 0x10           /* the same, in SKCTL: TX external, RX async */

#define PING_EVERY 600          /* frames between keepalives */
#define DEAD_AFTER 300          /* frames without a frame: the link is gone (the
                                 * server's clock alone comes every 30) */
#define SETTLE     3            /* jiffies between the motor off and SIO */
#define RESYNC_GAP 30           /* frames between RESYNCs: the whole state is
                                 * 18 frames of the line, and one bad frame in
                                 * a burst mustn't ask for it again and again */

#define DDEVIC (*(volatile uint8_t *)0x0300)
#define DUNIT  (*(volatile uint8_t *)0x0301)
#define DCOMND (*(volatile uint8_t *)0x0302)
#define DSTATS (*(volatile uint8_t *)0x0303)
#define DBUF   (*(volatile uint16_t *)0x0304)
#define DTIMLO (*(volatile uint8_t *)0x0306)
#define DBYT   (*(volatile uint16_t *)0x0308)
#define DAUX1  (*(volatile uint8_t *)0x030A)
#define DAUX2  (*(volatile uint8_t *)0x030B)

static uint8_t stage[sizeof(MuleState)];
static uint8_t link, joined;    /* connected; HELLO sent for sc_table */
static uint8_t seq, wait_seq, wait_act, credited, paused, resync_hold;
static uint16_t quiet;

typedef char mule_state_is_550_bytes[sizeof(MuleState) == 550 ? 1 : -1];
typedef char mule_table_is_33_bytes[sizeof(MuleTable) == 33 ? 1 : -1];

/* ---- sending ------------------------------------------------------------ */

static void send(uint8_t type, const uint8_t *p, uint8_t n);

/* Lost or garbled: the whole state again, once in a while at most. */
static void resync(void)
{
    if (!resync_hold) {
        send(SC_RESYNC, 0, 0);
        resync_hold = RESYNC_GAP;
    }
}

/* A frame: COBS of type, payload, Fletcher-16; then 0. */
static void send(uint8_t type, const uint8_t *p, uint8_t n)
{
    static uint8_t raw[24];
    uint8_t s1, s2, i, code, from;
    raw[0] = type;
    memcpy(raw + 1, p, n);
    ++n;
    s1 = s2 = 0;
    for (i = 0; i < n; ++i) {
        s1 = (uint8_t)(((uint16_t)s1 + raw[i]) % 255);
        s2 = (uint8_t)(((uint16_t)s2 + s1) % 255);
    }
    raw[n++] = s1;
    raw[n++] = s2;
    /* COBS: each block is its length + 1, then its bytes (none is 0) */
    from = 0;
    for (i = 0; i <= n; ++i) {
        if (i == n || raw[i] == 0) {
            code = i - from + 1;
            ns_put(code);
            for (; from < i; ++from)
                ns_put(raw[from]);
            ++from;                 /* past the 0 */
        }
    }
    ns_put(0);
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
    strncpy((char *)h + 2, sc_table, 8);
    strncpy((char *)h + 10, sc_name, 8);
    send(SC_HELLO, h, sizeof(h));
    joined = 1;
    if (wait_seq && act == ACT_NONE && wait_act != ACT_LEAVE)
        act = wait_act;
    wait_seq = 0;
}

/* ---- the link ----------------------------------------------------------- */

/* FujiNet's $70/$F0: netstream to host:port. The frame is host, 0, flags,
 * AUDF3, padded to 64. Port: DAUX1 is its high byte. */
static uint8_t ns_enable(const char *host)
{
    static uint8_t pl[64];
    uint8_t n = strlen(host);
    if (n > 60)
        n = 60;
    memset(pl, 0, sizeof(pl));
    memcpy(pl, host, n);
    pl[n + 1] = NS_FLAGS | ((PAL & 0x0E) == 0 ? 0x10 : 0);
    pl[n + 2] = NS_AUDF3;
    DDEVIC = 0x70;
    DUNIT = 1;
    DCOMND = 0xF0;
    DSTATS = 0x80;
    DBUF = (uint16_t)pl;
    DTIMLO = 15;
    DBYT = 64;
    DAUX1 = NS_PORT >> 8;
    DAUX2 = NS_PORT & 0xFF;
    asm("jsr $E459");           /* SIOV */
    return DSTATS < 128;
}

/* "http://host:9900/..." -> host */
static const char *endpoint_host(void)
{
    static char host[61];
    const char *p = strstr(sc_endpt, "://");
    uint8_t n = 0;
    p = p ? p + 3 : sc_endpt;
    while (*p && *p != ':' && *p != '/' && n < 60)
        host[n++] = *p++;
    host[n] = 0;
    return host;
}

static void drop(void)
{
    ns_end();
    link = joined = 0;
}

static uint8_t connect(void)
{
    if (link)
        return 1;
    ns_audf3 = NS_AUDF3;
    ns_skctl = NS_SKCTL;
    ns_reset();
    credited = 0;
    quiet = 0;
    if (!ns_enable(endpoint_host()))
        return 0;
    ns_begin();
    link = 1;
    joined = 0;
    return 1;
}

/* Off for plain SIO (the disk, appkeys), and back on. FujiNet must see the
 * motor go off before the next command, or it takes the command as the end
 * of the stream and closes it: give it a few jiffies. */
void ns_pause(void)
{
    uint8_t t, n;
    if (ns_on) {
        ns_end();
        paused = 1;
        for (n = 0; n < SETTLE; ++n)
            for (t = RTCLOK; t == RTCLOK; )
                ;
    }
}

void ns_resume(void)
{
    if (paused) {
        paused = 0;
        ns_begin();
    }
}

/* Leaving the program: say goodbye, then FujiNet's netstream off ("STOP"). */
void ns_close(void)
{
    if (link) {
        if (joined)
            send(SC_BYE, 0, 0);
        drop();
        ns_enable("STOP");
    }
}

/* ---- receiving ---------------------------------------------------------- */

/* The frames waiting, handled. 0: none (yet); 1: a state came in (st);
 * 2: the server refused us (st.h.err). The server sends every change (up
 * to twenty a second), more than a frame's drawing keeps up with: of the
 * states waiting only the last is kept (drawing each in turn would put us
 * ever further behind), except that the one answering our action is
 * handed on at once (its error and result are for that state alone). */
static uint8_t take(void)
{
    uint8_t n, got = 0;
    uint16_t off;
    for (;;) {
        n = ns_frame();
        if (n == 0)
            return got;
        quiet = 0;
        if (n == 255) {
            resync();                   /* lost: the whole state again */
            continue;
        }
        switch (ns_fbuf[0]) {
        case CS_PATCH:
            off = ns_fbuf[1] | ns_fbuf[2] << 8;
            n -= 3;
            if (off + n <= sizeof(stage))
                memcpy(stage + off, ns_fbuf + 3, n);
            break;
        case CS_COMMIT:
            if (!joined)
                break;
            memcpy(&st, stage, sizeof(MuleState));
            if (wait_seq && ns_fbuf[1] == wait_seq) {
                act_sent = wait_act;
                wait_seq = 0;
                return 1;
            }
            act_sent = ACT_STATE;
            got = 1;
            break;
        case CS_ERROR:
            st.h.err = ns_fbuf[1];
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
    if (act != ACT_NONE && !wait_seq) {
        if (++seq == 0)
            seq = 1;
        a[0] = seq;
        a[1] = act;
        a[2] = (uint8_t)act_arg;
        a[3] = act_arg >> 8;
        a[4] = au_traded;       /* the units we have shown (ACT_PUSH) */
        send(SC_ACT, a, 5);
        wait_seq = seq;
        wait_act = act;
        act = ACT_NONE;
    }
    r = take();
    if ((uint8_t)(ns_taken - credited) >= 48)
        send_credit();
    if (ns_err & 0xD0) {        /* framing, overrun or a full ring */
        ns_err = 0;
        resync();
    }
    if (r) {
        net_have = r;
        if (r == 2)
            drop();
        return 1;
    }
    if (++quiet % PING_EVERY == 0) {
        send_credit();          /* a keepalive, and credit it may have lost */
        send(SC_PING, 0, 0);
    }
    if (quiet > DEAD_AFTER) {
        drop();
        net_have = 0;
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

uint8_t net_detect(void)
{
    return 1;                   /* as fujinet-lib's network_init says on the Atari */
}

/* The table list: count, then MuleTables. Blocking (boot). */
uint8_t net_tables(uint8_t *buf, uint8_t max)
{
    uint16_t t;
    uint8_t n, got = 0, count = 255;
    if (!connect())
        return 0;
    send(SC_TABLES, 0, 0);
    for (t = 0; t < 300 && got < count; ++t) {
        frame_wait();
        while ((n = ns_frame()) != 0) {
            if (n == 255 || ns_fbuf[0] != CS_TABLE)
                continue;               /* a state for the old table: skip */
            count = ns_fbuf[2];
            if (ns_fbuf[1] < max && n >= 3 + sizeof(MuleTable))
                memcpy(buf + 1 + ns_fbuf[1] * sizeof(MuleTable), ns_fbuf + 3, sizeof(MuleTable));
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

#endif
