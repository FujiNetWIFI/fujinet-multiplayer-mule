/*
 * main.c -- M.U.L.E. (Tournament) for FujiNet 8-bit computers.
 *
 * The frame loop (clients/intv/mule.bas main_loop + isr.bas frame_isr):
 * wait for the frame, put the sprites up, sample the input, move what
 * moves on its own (walking, the grant cursor, auction lines, scripts),
 * take one step of the network request, then the phase's per-frame work.
 */
#include <string.h>
#include "mule.h"
#include "bank.h"

MuleState st, prev;
uint8_t phase, rnd, my_seat, last_phase, last_round, st_ok;
uint16_t rx_pe, rx_frame, ph_t0;
uint8_t scr_now, anim;
const uint8_t pl_col[4] = { C_BLUE, C_PURPLE, C_GREEN, C_P3 };

/* tools/a2sim: once the title shows 3C C3 here, poke $5A into
 * test_hook[0] to start the offline test mode (see test.c); [1] phase,
 * [2] stage/event/seat, [3] player event. The game answers $A5. */
uint8_t test_hook[4] = { 1, 0, 0, 0 };  /* initialised: in DATA, loaded early and
                                         * clear of the LC image in BSS */
uint8_t dbg_at;                 /* where the loop is (tools/a2sim peeks it) */

/* The overlay each phase's code is in (bank.h). */
static const uint8_t ovl_of[9] = {
    OV_STATUS, OV_GRANT, OV_AUCTION, OV_DEVELOP, OV_GRANT,
    OV_AUCTION, OV_AUCTION, OV_STATUS, OV_STATUS,
};

typedef void (*fn)(void);
static const fn enters[9] = {
    lobby_enter, grant_enter, landauc_enter, develop_enter, production_enter,
    goods_enter, goods_enter, summary_enter, gameover_enter,
};
static void no_state(void) {}
static const fn states[9] = {
    lobby_state, grant_state, auction_state, develop_state, no_state,
    auction_state, auction_state, summary_state, no_state,
};
static const fn ticks[9] = {
    lobby_tick, grant_tick, auction_tick, develop_tick, production_tick,
    auction_tick, auction_tick, summary_tick, gameover_tick,
};

/* How often to ask: often while things move, rarely while we walk (the
 * server has nothing to tell us then) or wait. */
void poll_rate(void)
{
    uint8_t w = 30;
    if (!net_fail) {
        switch (phase) {
        case PHASE_GRANT:
        case PHASE_LANDAUCTION:
        case PHASE_DECLARE:     w = 12; break;
        case PHASE_TRADING:     w = 8; break;
        case PHASE_PRODUCTION:
        case PHASE_GAMEOVER:    w = 60; break;
        case PHASE_DEVELOP:     if (dv_state == DV_WALK) w = 60; break;
        }
    }
    poll_wait = w;
}

/* The server's clock keeps ours honest. While a network call blocks, a
 * platform may miss frames; the server's jiffies (st.h.t) don't. The gap
 * between the two only grows when we lost frames (a slow reply makes it
 * shrink), so whenever it is larger than the largest seen before, the
 * difference is added to our clock.
 *
 * A reply slower than the fastest is that much older than it looks: rx_age
 * says by how much, and whatever runs on from a state (pe_now, since_rx,
 * the auction lines, the grant cursor) starts that many jiffies on.
 *
 * Where the clock is an interrupt that never misses a frame (frame_lost
 * does nothing: the NES, the Lynx), a gap larger than before is only a
 * faster reply than any yet, and it is the one to measure from. The lobby
 * has no clock (t is 0): the first state of a game starts afresh. */
uint8_t rx_age;

static void clock_sync(void)
{
    static uint16_t base;
    static uint8_t have;
    uint16_t lag = st.h.t - frame_count, d, f;
    rx_age = 0;
    if (test_mode || st.h.phase == PHASE_LOBBY) {
        have = 0;
        return;
    }
    if (!have) {
        have = 1;
        base = lag;
        return;
    }
    d = lag - base;
    if (d && d < 60) {
        f = frame_count;
        frame_lost((uint8_t)d);
        if (frame_count == f)
            base = lag;
    } else if (d >= 60 && d < 0x8000)
        base = lag;             /* a long stall (or a new game): start again */
    else if (d) {
        d = base - lag;         /* older than the fastest reply */
        if (d > 120)
            base = lag;         /* the server's clock went back: start again */
        else
            rx_age = d > 30 ? 30 : (uint8_t)d;
    }
}

/* A full state is in st. */
void state_arrived(void)
{
    uint8_t k = st.h.phase;
    if (k > PHASE_GAMEOVER)
        return;
    clock_sync();
    phase = k;
    rnd = st.h.round;
    my_seat = (uint8_t)st.h.mySeat;
    rx_pe = st.h.elapsed;
    rx_frame = frame_count - rx_age;
    st_ok = 1;
    bank_overlay(ovl_of[phase]);
    if (phase != last_phase || (phase == PHASE_GRANT && rnd != last_round)) {
        msg_scroll = 0;
        fx_stop_all();
        anim = AN_NONE;
        enters[phase]();
    }
    states[phase]();
    memcpy(&prev, &st, sizeof(MuleState));
    last_phase = phase;
    last_round = rnd;
}

static void reply(void)
{
    if (net_have == 1) {
        net_fail = 0;
        state_arrived();
    } else if (net_have == 2) {
        /* the server refused us outright (unknown table, table full...) */
        if (st.h.err == ERR_TABLEFULL || st.h.err == ERR_SEAT) {
            choose_table();
            last_phase = 255;
            st_ok = 0;
        }
    } else {
        ++net_fail;
    }
    poll_rate();
}

/* Another table (the appkey cleared, the list, the choice kept): that is
 * all in the boot overlay, so swap it in, then back. */
void choose_table(void)
{
    uint8_t cur = bank_current();
    bank_overlay(OV_BOOT);
    table_rechoose();
    bank_overlay(cur);
}

void game_start(void)
{
    last_phase = last_round = 255;
    scr_now = SCR_NONE;
    st_ok = 0;
    poll_wait = 0;
    act = ACT_NONE;
    gfx_clear();
    text_center(11, "JOINING...", C_WHITE);
}

/* Leave the table (ESC in the lobby, or confirmed in a game: the server
 * gives the computer our seat) and pick another. */
void leave_table(void)
{
    anim = AN_NONE;
    fx_stop_all();
    spr_hide_all();
    act = ACT_LEAVE;
    net_sync();
    choose_table();
    game_start();
}

/* ESC in a game: ask first. The game stands still while we ask; the leave
 * key again leaves, any other key or the button carries on. */
static void leave_ask(void)
{
    msg_row = 22;
    msg_col = C_WHITE;
    msg_show(M_LEAVEASK, 0, 0);
    do {
        frame_wait();
        spr_flush();
        snd_tick();
        msg_tick();
        input_poll();
        input_take();
    } while (!ev_btn && ev_key == KEY_NONE);
    if (ev_key == KEY_LEAVE && !test_mode) {
        leave_table();
        return;
    }
    msg_clear();
    ev_btn = 0;
    ev_key = KEY_NONE;
}

void main(void)
{
    const char *err = bank_init();
    uint8_t i, spec_at = 0;
    uint16_t done, n;

    if (!err)
        err = plat_init();
    input_init();
    snd_init();
    if (err) {
        gfx_clear();
        text_center(11, err, C_WHITE);
        for (;;)
            ;
    }
    gfx_clear();
    big_center(8, str_word(W_MULE), C_ORANGE);
    big_center(10, str_word(W_TOURNAMENT), C_BLUE);
    test_hook[0] = 0x3C;            /* the harness may poke now (not while */
    test_hook[1] = 0xC3;            /* loading: that would hit the image) */
    for (i = 0; i < 45; ++i) {
        frame_wait();
        snd_tick();
        if (test_hook[0] == 0x5A)
            break;
    }
    if (test_hook[0] == 0x5A) {
        test_hook[0] = 0xA5;        /* tell the harness */
        bank_overlay(OV_BOOT);
        test_start(test_hook[1], test_hook[2], test_hook[3]);
    } else {
        bank_overlay(OV_BOOT);
        boot();
    }
    game_start();

    done = frame_count;
    for (;;) {
        dbg_at = 1;
        frame_wait();
        dbg_at = 2;
        spr_flush();                /* what moved last time round */
        dbg_at = 3;
        snd_tick();
        input_poll();
        input_take();
        /* watching (no seat): only the leave key counts */
        if (st_ok && my_seat > 3) {
            inp_dir = inp_btn = ev_btn = 0;
            if (ev_key != KEY_LEAVE)
                ev_key = KEY_NONE;
        }
#ifdef MULE_AUTOPLAY
        autoplay();
#endif
        dbg_at = 4;
        /* what moves by itself, once for every frame that has passed (a
         * slow frame, or a network call, is caught up with): more than two
         * behind, half of it now and the rest after, so a stall plays out
         * as a moment's hurry rather than one jump (a loop that always
         * takes three frames still keeps up); more than 8, the rest is
         * let go */
        n = frame_count - done;
        if (n > 8) {
            done = frame_count - 8;
            n = 8;
        }
        if (n > 2)
            n = (n + 1) >> 1;
        done += n;
        while (n--) {
            if (anim == AN_WALK)
                walk_isr();
            else if (anim == AN_GRANT)
                grant_isr();
            else if (anim == AN_AUCTION)
                auction_isr();
            fx_run();
        }
        dbg_at = 6;
        /* the network: one step a frame */
        if (net_busy()) {
            if (net_step())
                reply();
        } else {
            if (poll_wait)
                --poll_wait;
            if (poll_wait == 0 || act != ACT_NONE) {
                if (test_mode) {
                    test_poll();
                    poll_rate();
                } else {
                    net_begin();
                    if (!net_busy())
                        reply();
                }
            }
        }
        dbg_at = 7;
        if (st_ok && phase != PHASE_LOBBY && ev_key == KEY_LEAVE) {
            /* a spectator has no seat to lose: no question */
            if (my_seat > 3 && !test_mode)
                leave_table();
            else
                leave_ask();
            done = frame_count;
            continue;
        }
        if (st_ok)
            ticks[phase]();
        /* the spectator's tag, every 32 frames (a repaint takes it off) */
        if (st_ok && my_seat > 3 && (uint8_t)(frame_count >> 5) != spec_at) {
            spec_at = (uint8_t)(frame_count >> 5);
            spec_tag();
        }
        /* (what moved is drawn by the flush after the next frame_wait,
         * in the blank: background drawing puts back any sprites it took
         * off itself) */
    }
}
