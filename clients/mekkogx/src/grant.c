/*
 * grant.c -- the land grant (clients/intv/grant.bas, isr.bas isr_grant).
 *
 * The cursor sweeps the map a plot every CTU (12 jiffies), skipping the
 * store and owned plots without losing time; the button claims the plot
 * under it. The server judges the claim as of the press: it accepts a plot
 * the cursor left up to 36 jiffies ago (our cursor runs late, and the claim
 * waits for the poll in progress), and takes the plot from a computer that
 * claimed it on arrival within that time.
 *
 * Round 1 opens with the transport ship leaving ("the ship will be back in
 * 12 months") before the first step; the server holds the cursor for it.
 */
#include "mule.h"

extern const uint8_t fx_depart[];

#define SLOT_CURSOR 2

static uint8_t g_cur, g_next, g_step, g_show;
static uint8_t gr_depart, gr_claimed;
static uint8_t gr_srv;          /* the cursor in the last state */
static uint8_t gr_press;        /* the cursor's plot when the button went
                                 * down (255 none): the loop may be busy with
                                 * a poll for a while before it acts on it */
static uint8_t gr_was;          /* the plot before the cursor's last step
                                 * (255 none): a press seen the frame after a
                                 * step was made with that plot on screen */
static uint8_t gr_want;         /* the plot we claimed and wait on (255 none) */

void grant_enter(void)
{
    map_dots = 0;
    map_enter();
    anim = AN_GRANT;
    g_show = 0;
    g_cur = 255;
    g_next = 0;
    gr_claimed = gr_depart = 0;
    gr_press = gr_want = gr_was = 255;
    msg_row = 22;
    msg_col = C_WHITE;
    if (rnd == 1 && pe_now() < 700) {
        /* the ship leaves the store first */
        gr_depart = 1;
        title_show(T_TRANSPORT, 0);
        msg_show(M_DEPART, 0, 0);
        fx_start(0, fx_depart);
    } else {
        title_show(T_LANDGRANT, 0);
        msg_show(M_SELECT, 0, 0);
    }
}

void grant_state(void)
{
    uint8_t k, j, mine;
    uint16_t w;

    map_update();
    /* sync the cursor: -1 (255) before it starts, 45 when it's done */
    k = (uint8_t)st.grant.cursor;
    gr_srv = k;
    g_step = st.grant.step;
    if (g_step == 0)
        g_step = 12;
    w = st.grant.next;
    if (w > 255)
        w = 255;
    if (k != g_cur || w < g_next) {
        if (k != g_cur)
            gr_was = 255;
        g_cur = k;
        g_next = (uint8_t)w;
        if (g_next == 0)
            g_next = 1;
    }
    if (!gr_depart)
        g_show = 1;
    if (my_seat < 4) {
        mine = (uint8_t)st.grant.claimed[my_seat];
        if (mine < NUM_PLOTS) {
            gr_claimed = 1;
            gr_want = 255;
        }
        j = gr_want;
        if (j < NUM_PLOTS && mine >= NUM_PLOTS) {
            /* every claim is answered by the next state, so a plot a
             * computer owns is not lost yet: the server may still give it */
            if (act_sent == ACT_CLAIM && st.h.err && k < j) {
                /* turned down because the server's cursor wasn't there yet
                 * (it holds after a computer's claim, which our cursor can't
                 * know): keep the claim, and send it again when it arrives */
                act_sent = 0;
            } else if (k >= NUM_PLOTS || k > j || (act_sent == ACT_CLAIM && st.h.err)) {
                /* turned down, or the cursor is past it: press again */
                gr_want = 255;
                gr_claimed = 0;
                snd_play(SND_ERROR);
            } else if (k == j && act == ACT_NONE && act_sent != ACT_CLAIM) {
                act_send(ACT_CLAIM, j);
            }
        }
    }
}

/* Every frame: the cursor moves to the next free plot every g_step frames,
 * blanking for one frame at each step, like the Atari. */
void grant_isr(void)
{
    if (g_cur >= NUM_PLOTS || !g_show) {
        spr_hide(SLOT_CURSOR);
        return;
    }
    if (ev_btn && gr_press == 255) {
        gr_press = g_cur;
        if (g_next == g_step && gr_was < NUM_PLOTS)
            gr_press = gr_was;
    }
    if (g_next > 0) {
        --g_next;
        /* at most one step past where the server last had it: the server
         * holds the cursor after a computer's claim, which we only learn
         * from the next state */
        if (g_next == 0 && gr_srv < NUM_PLOTS && g_cur > gr_srv)
            g_next = 1;
        if (g_next == 0) {
            gr_was = g_cur;
            for (;;) {
                if (++g_cur >= NUM_PLOTS)
                    break;
                if (g_cur != STORE_PLOT && OWNER(g_cur) == 255)
                    break;
            }
            g_next = g_step;
            spr_hide(SLOT_CURSOR);
            return;
        }
    }
    spr_set(SLOT_CURSOR, SH_CURSOR, 0, PLOT_X(g_cur), PLOT_Y(g_cur), C_WHITE, 0);
}

void grant_tick(void)
{
    uint8_t k;
    msg_tick();
    if (gr_depart) {
        if (fx_flag == 1 || !fx[0].st) {
            /* the ship has gone: the grant proper begins */
            gr_depart = 0;
            fx_stop_all();
            title_show(T_LANDGRANT, 0);
            msg_show(M_SELECT, 0, 0);
            g_show = 1;
        }
        return;
    }
    if (ev_btn) {
        /* claim the plot the cursor was on when the button went down, and
         * hold the cursor there for the server's pause after a claim (4
         * steps); the next state corrects it if we lost */
        k = gr_press;
        if (k >= NUM_PLOTS)
            k = g_cur;
        gr_press = 255;
        if (!gr_claimed && my_seat < 4 && k < NUM_PLOTS) {
            act_send(ACT_CLAIM, k);
            gr_claimed = 1;
            gr_want = k;
            g_cur = k;
            g_next = g_step * 4;
            gr_was = 255;
            snd_play(SND_BUY);
        }
    }
}
