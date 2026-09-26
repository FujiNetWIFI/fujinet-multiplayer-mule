/*
 * net.c -- talking to the M.U.L.E. server through fujinet-lib.
 *
 * Every call is an HTTP GET of
 *     N:<endpoint><route>[<arg>]?table=<t>&player=<name>&bin=1
 * and every call, action or poll, returns the whole 550-byte MuleState.
 *
 * One request is in flight at a time. An action (act_send) takes the place
 * of the next /state poll. A request is spread over frames so the game
 * keeps moving: open, then one status call a frame until the reply has
 * settled (two equal non-zero byte counts in a row: a status can report
 * before all of it has arrived), then reads of up to 512 bytes. Only a
 * whole reply is copied into st. (clients/intv/net.bas, fujinet.bas)
 */
#include <string.h>
#include "mule.h"
#include "fujinet-network.h"
#include "fujinet-fuji.h"
#include "config.h"

uint8_t act, act_sent, net_have, net_fail, poll_wait, test_mode;
uint16_t act_arg;
uint8_t au_traded;
char sc_name[9], sc_endpt[66], sc_table[9];

static char *add(char *p, const char *s)
{
    while (*s)
        *p++ = *s++;
    return p;
}

#ifndef PLAT_NETSTREAM          /* the Atari talks stream mode: src/atari/nsnet.c */
static char url[200];
static uint8_t rx[sizeof(MuleState)];
static uint16_t rx_got, bw_last;
static uint8_t nstate, nwait;

/* The reply is read straight into the struct: it must be the server's size
 * (../mule_proto.h; the compiler packs structs). */
typedef char mule_state_is_550_bytes[sizeof(MuleState) == 550 ? 1 : -1];
typedef char mule_table_is_33_bytes[sizeof(MuleTable) == 33 ? 1 : -1];

#define NS_IDLE   0
#define NS_STATUS 1
#define NS_READ   2

#define TIMEOUT   240   /* frames without progress before giving up */

static const char *const routes[] = {
    "", "/state", "/ready/1", "/ready/0", "/setup/species/", "/claim/", "/auction/push/",
    "/declare/", "/collude", "/turn/begin", "/turn/buymule", "/turn/outfit/", "/turn/install/",
    "/turn/return", "/turn/assay/", "/turn/sell/", "/turn/unsell/", "/turn/wampus", "/turn/pub",
    "/turn/end", "/leave", "/auction/push/hold/",
};
static const char *const push_word[] = { "hold", "up", "down" };
static const char *const declare_word[] = { "n", "b", "s" };

static char *add_num(char *p, uint16_t v)
{
    return p + fmt_num(p, v);
}

static void compose(uint8_t a)
{
    char *p = add(url, "N:");
    const char *r = routes[a];
    p = add(p, sc_endpt);
    p = add(p, r);
    if (r[strlen(r) - 1] == '/') {
        if (a == ACT_PUSH) {
            p = add(p, push_word[act_arg < 3 ? act_arg : 0]);
            *p++ = '/';
            p = add_num(p, au_traded);      /* the units we have shown */
        } else if (a == ACT_HOLD) {
            p = add_num(p, au_traded);      /* the units we have shown */
            *p++ = '/';
            p = add_num(p, act_arg);
        } else if (a == ACT_DECLARE) {
            p = add(p, declare_word[act_arg < 3 ? act_arg : 0]);
        } else {
            p = add_num(p, act_arg);
        }
    }
    p = add(p, "?table=");
    p = add(p, sc_table);
    p = add(p, "&player=");
    p = add(p, sc_name);
    p = add(p, "&bin=1");
#ifdef PLAT_BIG_ENDIAN
    p = add(p, "&be=1");            /* the 16- and 32-bit fields in our order */
#endif
    *p = 0;
}

void act_send(uint8_t a, uint16_t arg)
{
    act = a;
    act_arg = arg;
    poll_wait = 0;
}

uint8_t net_busy(void)
{
    return nstate != NS_IDLE;
}

static void finish(uint8_t have)
{
    network_close(url);
    nstate = NS_IDLE;
    net_have = have;
}

void net_begin(void)
{
    if (nstate != NS_IDLE)
        return;
    net_have = 0;
    if (act == ACT_NONE)
        act = ACT_STATE;
    compose(act);
    act_sent = act;
    act = ACT_NONE;
    if (network_open(url, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE) != FN_ERR_OK) {
        finish(0);
        return;
    }
    rx_got = 0;
    bw_last = 0;
    nwait = 0;
    nstate = NS_STATUS;
}

/* One step. Returns 1 when the request is over (net_have: 1 a state,
 * 2 an error header only, 0 failed). */
uint8_t net_step(void)
{
    uint16_t bw;
    uint8_t conn, err;
    int16_t n;

    if (nstate == NS_IDLE)
        return 0;
    if (++nwait > TIMEOUT) {
        finish(0);
        return 1;
    }
    if (nstate == NS_STATUS) {
        if (network_status(url, &bw, &conn, &err) != FN_ERR_OK) {
            finish(0);
            return 1;
        }
        if (err != 1 && err != 136) {       /* 1 OK, 136 EOF */
            if (bw == 0) {
                finish(0);
                return 1;
            }
        }
        if (bw == 0 || bw != bw_last) {
            bw_last = bw;
            return 0;
        }
        nstate = NS_READ;
        nwait = 0;
    }
    /* NS_READ */
    n = network_read_nb(url, rx + rx_got, sizeof(MuleState) - rx_got);
    if (n < 0) {
        finish(0);
        return 1;
    }
    if (n > 0)
        nwait = 0;
    rx_got += n;
    if (rx_got >= sizeof(MuleHeader)) {
        if (rx[0] != 'M') {
            finish(0);
            return 1;
        }
        if (rx[2] != MSG_STATE) {
            /* an error header only: keep it (st.h.err says why) */
            memcpy(&st.h, rx, sizeof(MuleHeader));
            finish(2);
            return 1;
        }
    }
    if (rx_got < sizeof(MuleState))
        return 0;
    memcpy(&st, rx, sizeof(MuleState));
    finish(1);
    return 1;
}

/* Send the queued action now and wait for its answer (finishing any poll
 * already on its way first). */
uint8_t net_sync(void)
{
    uint8_t pass;
    for (pass = 0; pass < 2; ++pass) {
        while (net_busy()) {
            frame_wait();
            net_step();
        }
        if (pass == 0)
            net_begin();
    }
    return net_have;
}

uint8_t net_detect(void)
{
#ifdef PLAT_NET_PROBE
    /* a platform whose network_init() can't tell (fujinet-lib-experimental's
     * says yes always) asks its own way */
    if (!plat_net_probe())
        return 0;
#endif
    return network_init() == FN_ERR_OK;
}

/* /tables?bin=1: a count, then 33-byte MuleTables. Blocking (boot). */
uint8_t net_tables(uint8_t *buf, uint8_t max)
{
    char *p = add(url, "N:");
    uint16_t want = 1 + (uint16_t)max * sizeof(MuleTable), got = 0, bw = 0, last = 0xFFFF;
    uint8_t conn, err, tries = 0;
    int16_t n;
    p = add(p, sc_endpt);
    p = add(p, "/tables?bin=1");
    *p = 0;
    if (network_open(url, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE) != FN_ERR_OK)
        return 0;
    /* settle */
    while (tries++ < 180) {
        frame_wait();
        if (network_status(url, &bw, &conn, &err) != FN_ERR_OK)
            break;
        if (bw && bw == last)
            break;
        last = bw;
    }
    tries = 0;
    while (got < want && tries++ < 120) {
        n = network_read_nb(url, buf + got, want - got);
        if (n < 0)
            break;
        if (n == 0) {
            if (got)
                break;
            frame_wait();
            continue;
        }
        got += n;
    }
    network_close(url);
    if (got == 0)
        return 0;
    n = buf[0];
    if (n > max)
        n = max;
    if (1 + (uint16_t)n * sizeof(MuleTable) > got)
        n = (got - 1) / sizeof(MuleTable);
    return (uint8_t)n;
}

#endif /* PLAT_NETSTREAM */

/* ---- appkeys: creator 1, app 1 (the FujiNet lobby's) ------------------- */
#define AK_NAME   0
#define AK_SERVER 10
static uint8_t akbuf[66];

#ifdef PLAT_NETSTREAM
/* plain SIO while the stream is on: off, then on again (nsnet.c) */
void ns_pause(void);
void ns_resume(void);
#else
#define ns_pause()
#define ns_resume()
#endif

#ifdef PLAT_NS_ONEWAY
/* The Lynx: once the stream is on, the bus is the server's for good, so
 * appkeys are read and written only before it (the name at boot); the
 * table isn't kept (it would go stale with no way to clear it). */
extern uint8_t ns_on;
#define AK_NO_BUS(r) if (ns_on) return r
#else
#define AK_NO_BUS(r)
#endif

static uint8_t ak_read(uint8_t key)
{
    uint16_t count = 0;
    uint8_t ok;
    memset(akbuf, 0, sizeof(akbuf));
    AK_NO_BUS(0);
    ns_pause();
    fuji_set_appkey_details(1, 1, DEFAULT);
    ok = fuji_read_appkey(key, &count, akbuf);
    ns_resume();
    if (!ok)
        return 0;
    if (count > 64)
        count = 64;
    akbuf[count] = 0;
    return (uint8_t)count;
}

static void endpoint_default(void)
{
    strncpy(sc_endpt, DEFAULT_ENDPOINT, 64);
    sc_endpt[64] = 0;
}

/* "https://host/?table=xyz" -> sc_endpt "https://host/", sc_table "xyz"
 * (letters and digits only, else no table). */
static void split_room_url(void)
{
    char *q = strchr(sc_endpt, '?');
    uint8_t k = 0;
    char c;
    sc_table[0] = 0;
    if (!q)
        return;
    *q++ = 0;
    if (strncmp(q, "table=", 6))
        return;
    q += 6;
    while (k < 8 && (c = *q++) != 0 && c != '&') {
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) {
            sc_table[0] = 0;
            return;
        }
        sc_table[k++] = c;
    }
    sc_table[k] = 0;
}

void appkey_load(void)
{
    uint8_t n, i, ok;
    char c;

    /* the player's name: A-Z and 0-9 only go into the URL unescaped */
    sc_name[0] = 0;
    n = ak_read(AK_NAME);
    if (n) {
        if (n > 8)
            n = 8;
        ok = 1;
        for (i = 0; i < n && akbuf[i]; ++i) {
            c = akbuf[i];
            if (c >= 'a' && c <= 'z')
                c -= 32;
            if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
                ok = 0;
            sc_name[i] = c;
        }
        sc_name[i] = 0;
        if (!ok || i == 0)
            sc_name[0] = 0;
    }

    /* the table: only a URL counts (an unset key can read back as zeros) */
    endpoint_default();
    sc_table[0] = 0;
#ifdef PLAT_NS_ONEWAY
    n = 0;
#else
    n = ak_read(AK_SERVER);
#endif
    if (n > 4 && akbuf[0] == 'h') {
        memcpy(sc_endpt, akbuf, n);
        sc_endpt[n] = 0;
        split_room_url();
    }
    /* no trailing '/', so "<endpoint>/state" never has two */
    n = strlen(sc_endpt);
    if (n && sc_endpt[n - 1] == '/')
        sc_endpt[n - 1] = 0;
}

#ifdef PLAT_NO_KEYBOARD
/* The name typed at boot, kept for next time (the lobby's key: a platform
 * with no lobby to type it in). */
void appkey_save_name(void)
{
    AK_NO_BUS();
    fuji_set_appkey_details(1, 1, DEFAULT);
    fuji_write_appkey(AK_NAME, strlen(sc_name), (uint8_t *)sc_name);
}
#endif

/* The buffer is zeroed first: some buses (the CoCo's) send all 64 bytes of
 * it whatever the count, so the tail must be NULs, not an old URL. */
void appkey_save_room(void)
{
    char *p;
    AK_NO_BUS();
    memset(akbuf, 0, sizeof(akbuf));
    p = add((char *)akbuf, sc_endpt);
    p = add(p, "/?table=");
    p = add(p, sc_table);
    ns_pause();
    fuji_set_appkey_details(1, 1, DEFAULT);
    fuji_write_appkey(AK_SERVER, (uint16_t)(p - (char *)akbuf), akbuf);
    ns_resume();
}

void appkey_clear_room(void)
{
#ifdef PLAT_NS_ONEWAY
    sc_table[0] = 0;
#endif
    AK_NO_BUS();
    memset(akbuf, 0, sizeof(akbuf));    /* zeros read back as no table */
    ns_pause();
    fuji_set_appkey_details(1, 1, DEFAULT);
    fuji_write_appkey(AK_SERVER, 0, akbuf);
    ns_resume();
    sc_table[0] = 0;
}
