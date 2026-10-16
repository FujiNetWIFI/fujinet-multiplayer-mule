/*
 * grant.c -- the land grant (clients/intv/grant.bas, isr.bas isr_grant).
 *
 * The cursor sweeps the map a plot every CTU (12 jiffies), skipping the
 * store and owned plots without losing time; the button claims the plot
 * under it. The server judges the claim as of the press: it accepts a plot
 * the cursor left up to 36 jiffies ago, and takes the plot from a computer
 * that claimed it on arrival within that time.
 *
 * Between states the cursor steps on its own, by the frame each step is
 * due, never past the plot the state says a computer will stop it on
 * (h.grantStop; an older server sends 0: then at most one step past the
 * server's cursor).
 *
 * Round 1 opens with the transport ship leaving ("the ship will be back in
 * 12 months") before the first step; the server holds the cursor for it.
 */
#include <string.h>
#include "mule.h"

extern const uint8_t fx_depart[];

#define SLOT_CURSOR 2

static uint8_t g_cur, g_step, g_show;
static uint16_t g_due;          /* the frame the cursor's next step is due */
static uint8_t g_lim;           /* the furthest plot it may step to */
static uint8_t g_stepf;         /* the frame (low byte) of its last step */
static uint8_t gr_depart, gr_claimed;
static uint8_t gr_press;        /* the cursor's plot when the button went
                                 * down (255 none): the loop may be busy with
                                 * a poll for a while before it acts on it */
static uint8_t gr_was;          /* the plot before the cursor's last step
                                 * (255 none): a press seen the frame after a
                                 * step was made with that plot on screen */
static uint8_t gr_want;         /* the plot we claimed and wait on (255 none) */
static uint8_t gr_park;         /* 1: turned down as the server's cursor wasn't
                                 * there yet: sent again when it is */

void grant_enter(void)
{
    map_dots = 0;
    map_enter();
    anim = AN_GRANT;
    g_show = 0;
    g_cur = g_lim = 255;
    gr_claimed = gr_depart = gr_park = 0;
    gr_press = gr_want = gr_was = 255;
    msg_row = 22;
    msg_col = C_WHITE;
    if (rnd == 1 && pe_now() < 700) {
        /* the ship leaves the store first */
        gr_depart = 1;
        title_show(T_TRANSPORT, 0);
        msg_show(M_DEPART, 0, 0);
        fx_flag = 0;            /* (the summary's landing left it 1) */
        fx_start(0, fx_depart);
    } else {
        title_show(T_LANDGRANT, 0);
        msg_show(M_SELECT, 0, 0);
        if (my_seat < 4)
            snd_play(SND_ALERT);
    }
}

/* The ship has gone: the grant proper begins. */
static void depart_done(void)
{
    gr_depart = 0;
    fx_stop_all();
    title_show(T_LANDGRANT, 0);
    msg_show(M_SELECT, 0, 0);
    if (my_seat < 4)
        snd_play(SND_ALERT);
    g_show = 1;
}

/* The free plot after p (NUM_PLOTS past the last). */
static uint8_t next_free(uint8_t p)
{
    for (;;) {
        if (++p >= NUM_PLOTS)
            return p;
        if (p != STORE_PLOT && OWNER(p) == 255)
            return p;
    }
}

void grant_state(void)
{
    uint8_t k, j, mine, again;
    uint16_t due;

    map_update();
    /* the server's cursor: -1 (255) before it starts, 45 when it's done */
    k = (uint8_t)st.grant.cursor;
    g_step = st.grant.step;
    if (g_step == 0)
        g_step = 12;
    due = rx_frame + st.grant.next;
    if (my_seat < 4) {
        mine = (uint8_t)st.grant.claimed[my_seat];
        if (mine < NUM_PLOTS) {
            gr_claimed = 1;
            gr_want = 255;
        }
        j = gr_want;
        if (j < NUM_PLOTS && mine >= NUM_PLOTS) {
            /* only the state answering the claim judges it: those before
             * were made before the server saw it (its cursor may have gone
             * on, and a computer's plot may still be ours) */
            again = 0;
            if (act_sent == ACT_CLAIM) {
                if (st.h.err && (int8_t)k < (int8_t)j)
                    gr_park = 1;    /* the server's cursor wasn't there yet */
                else
                    again = 1;      /* turned down */
            } else if (gr_park && act == ACT_NONE && (int8_t)k >= (int8_t)j) {
                gr_park = 0;
                if (k < NUM_PLOTS)
                    act_send(ACT_CLAIM, j);
                else
                    again = 1;
            }
            if (again) {
                /* press again */
                gr_want = 255;
                gr_claimed = gr_park = 0;
                snd_play(SND_ERROR);
            }
        }
    }
    /* (while our claim is out the cursor waits on its plot) */
    if (gr_want >= NUM_PLOTS) {
        if (k == g_cur)
            g_due = due;
        else if (k < NUM_PLOTS && g_cur == next_free(k)
                 && !memcmp(st.grant.claimed, prev.grant.claimed, 4)) {
            /* we made the step it waited for a little early: no jump back */
            g_due = due + g_step;
        } else {
            /* a claim (ours, or another's the cursor went back for) */
            g_cur = k;
            g_due = due;
            gr_was = 255;
        }
    }
    j = st.h.grantStop;
    g_lim = j ? j - 1 : next_free(k);
    /* the server's cursor has started: our ship (which set out late, after
     * the map) has gone too, or the first plots would pass unseen */
    if (gr_depart && k < NUM_PLOTS)
        depart_done();
    if (!gr_depart)
        g_show = 1;
}

/* Every frame (more than once to catch up): the cursor steps to the next
 * free plot when its step is due, blanking for that frame, like the Atari. */
void grant_isr(void)
{
    uint8_t n;
    if (ev_btn && gr_press == 255 && g_show && g_cur < NUM_PLOTS) {
        gr_press = g_cur;
        if ((uint8_t)((uint8_t)frame_count - g_stepf) <= 1 && gr_was < NUM_PLOTS)
            gr_press = gr_was;
    }
    if (g_cur != NUM_PLOTS && gr_want >= NUM_PLOTS && (int16_t)(frame_count - g_due) >= 0) {
        n = next_free(g_cur);
        if (n <= g_lim) {
            gr_was = g_cur;
            g_cur = n;
            g_due += g_step;
            g_stepf = (uint8_t)frame_count;
            spr_hide(SLOT_CURSOR);
            return;
        }
    }
    if (g_cur >= NUM_PLOTS || !g_show) {
        spr_hide(SLOT_CURSOR);
        return;
    }
    spr_set(SLOT_CURSOR, SH_CURSOR, 0, PLOT_X(g_cur), PLOT_Y(g_cur), C_WHITE, 0);
}

void grant_tick(void)
{
    uint8_t k;
    msg_tick();
    if (gr_depart) {
        if (fx_flag == 1 || !fx[0].st)
            depart_done();
        return;
    }
    if (ev_btn) {
        /* claim the plot the cursor was on when the button went down, and
         * hold the cursor there until the server answers */
        k = gr_press;
        if (k >= NUM_PLOTS)
            k = g_cur;
        gr_press = 255;
        if (!gr_claimed && my_seat < 4 && k < NUM_PLOTS) {
            act_send(ACT_CLAIM, k);
            gr_claimed = 1;
            gr_want = k;
            g_cur = k;
            gr_was = 255;
            snd_play(SND_BUY);
        }
    }
}
