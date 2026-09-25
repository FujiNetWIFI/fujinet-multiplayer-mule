/*
 * status.c -- the lobby, the status summary and the game-over screen
 * (clients/intv/status.bas).
 *
 * Summary (410 jiffies of drawing, then everyone presses their button): the
 * players walk in from the right along the bottom, 4 every 6 frames, then
 * climb to their rows in rank order; then the table prints.
 */
#include <string.h>
#include "mule.h"

extern const uint8_t fx_land[];

static uint8_t su_order[4];     /* seats by rank */
static uint8_t su_drawn, su_pressed, su_land;

/* the seats in rank order (rank 1 first); anyone without a rank last */
static void su_rank(void)
{
    uint8_t n = 0, k, i, j, found;
    for (k = 1; k <= 4; ++k)
        for (i = 0; i < 4; ++i)
            if (PL(i).rank == k && n < 4)
                su_order[n++] = i;
    for (i = 0; i < 4; ++i) {
        found = 0;
        for (j = 0; j < n; ++j)
            if (su_order[j] == i)
                found = 1;
        if (!found && n < 4)
            su_order[n++] = i;
    }
}

/* Walk-in scripts: each player enters 30 frames after the one before,
 * walks left to x 11 (33 steps of 4), then climbs to its row, 4 lines every
 * 6 frames. The walking frames are 2/3 (left), the climbing 8/9 (back). */
#define SU_SCRIPT(w, n) FX_WAIT, w, FX_MOVE, SB(-4), 0, 6, 33, FX_FRAME, 8, FX_MOVE, 0, SB(-4), 6, n, FX_END
static const uint8_t su_walk0[] = { SU_SCRIPT(1, 30) };
static const uint8_t su_walk1[] = { SU_SCRIPT(30, 22) };
static const uint8_t su_walk2[] = { SU_SCRIPT(60, 14) };
static const uint8_t su_walk3[] = { SU_SCRIPT(90, 6) };
static const uint8_t *const su_walks[4] = { su_walk0, su_walk1, su_walk2, su_walk3 };

#define SU_ROW(i) (4 + (i) * 4)

/* Land value of seat k: 500 a plot, plus the outfit cost + 35 for each
 * M.U.L.E. on one (the server's ScoreOf; the binary sends only money and
 * the total). */
static uint16_t su_land_of(uint8_t k)
{
    uint16_t v = 0;
    uint8_t mp, g;
    for (mp = 0; mp < NUM_PLOTS; ++mp)
        if (OWNER(mp) == k) {
            v += 500;
            g = MULEOF(mp);
            if (g < 4)
                v += (g + 1) * 25 + 35;
        }
    return v;
}

/* The numbers (they change as players press, so redrawn per state). */
static void su_values(void)
{
    uint8_t i, k, g;
    uint16_t v;
    for (i = 0; i < 4; ++i) {
        k = su_order[i];
        big_num(6, SU_ROW(i), PL(k).money, 6, PLCOL(k));
        big_num(18, SU_ROW(i), su_land_of(k), 5, PLCOL(k));
        v = 0;
        for (g = 0; g < 4; ++g)
            v += PL(k).goods[g] * st.store.price[g];
        big_num(28, SU_ROW(i), v, 6, PLCOL(k));
        gfx_text(8, SU_ROW(i) + 2, str_word(W_TOTAL), C_WHITE);
        big_num(26, SU_ROW(i) + 2, PL(k).score, 7, PLCOL(k));
    }
    big_num32(26, 21, st.colony.total, 7, C_ORANGE);
}

static void su_table(void)
{
    uint8_t i, k;
    char buf[24];
    su_drawn = 1;
    fx_stop_all();
    /* the players stand at the left of their rows */
    for (i = 0; i < 4; ++i) {
        k = su_order[i];
        spr_set(i, SH_SPECIES, (PL(k).species & 7) * 11 + 8, 11, 32 + i * 32, PLCOL(k), 0);
    }
    strcpy(buf, str_word(W_SUMMARY));
    fmt_num(buf + strlen(buf), rnd);
    big_center(0, buf, C_ORANGE);
    /* headings right-aligned over their columns (money ends at cell 17,
     * land at 27, goods and the totals at 39) */
    gfx_text(13, 2, str_word(W_MONEY), C_WHITE);
    gfx_text(24, 2, str_word(W_LAND), C_WHITE);
    gfx_text(35, 2, str_word(W_GOODS), C_WHITE);
    gfx_big(6, 21, str_word(W_COLONY), C_ORANGE);
    su_values();
    if (phase == PHASE_SUMMARY) {
        msg_row = 22;
        msg_col = C_WHITE;
        msg_show(M_PRESSALL, 0, 0);
    }
}

static void su_begin(void)
{
    uint8_t i, k;
    Actor *a;
    anim = AN_NONE;
    fx_stop_all();
    gfx_clear();
    ph_t0 = frame_count;
    scr_now = SCR_SUMMARY;
    su_drawn = su_pressed = 0;
    su_rank();
    /* arriving late: straight to the table */
    if (pe_now() > 400 && rnd > 0) {
        su_table();
        return;
    }
    for (i = 0; i < 4; ++i) {
        k = su_order[i];
        a = &fx[i];
        a->kind = FK_SINGLE;
        a->slot = i;
        a->shape = SH_SPECIES;
        a->fbase = (PL(k).species & 7) * 11;
        a->frame = 2;
        a->d2 = 1;
        a->col = PLCOL(k);
        a->flags = 0;
        a->x = 143;
        a->y = 152;
        fx_start(i, su_walks[i]);
    }
}

/* Round 0's summary follows the colonists' arrival: the Atari lands the
 * transport ship first ("you're landing on the planet Irata."). */
void summary_enter(void)
{
    anim = AN_NONE;
    fx_stop_all();
    SND_XA(SNX_TUNE, rnd);
    su_land = 0;
    if (rnd == 0 && pe_now() < 300) {
        su_land = 1;
        map_enter();
        title_show(T_TRANSPORT, 0);
        msg_row = 22;
        msg_col = C_WHITE;
        msg_show(M_LANDING, 0, 0);
        fx_flag = 0;
        fx_start(0, fx_land);
        SND_X(SNX_SHIP);
        return;
    }
    su_begin();
}

void summary_state(void)
{
    if (su_land)
        return;
    if (!su_drawn && (uint16_t)(frame_count - ph_t0) >= 410)
        su_table();
    if (su_drawn)
        su_values();
}

void summary_tick(void)
{
    if (su_land) {
        msg_tick();
        if (fx_flag == 1 || !fx[0].st) {
            su_land = 0;
            su_begin();
        }
        return;
    }
    if (!su_drawn && (uint16_t)(frame_count - ph_t0) >= 410)
        su_table();
    if (su_drawn) {
        msg_tick();
        if (ev_btn && !su_pressed && my_seat < 4) {
            su_pressed = 1;
            act_send(ACT_READY, 0);
            snd_play(SND_SELECT);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Game over: FIRST FOUNDER, then the final summary and the colony's rating.
 * ------------------------------------------------------------------------- */
static uint8_t go_stage;

void gameover_enter(void)
{
    uint8_t i, k;
    anim = AN_NONE;
    SND_XA(SNX_TUNE, 12);
    fx_stop_all();
    gfx_clear();
    scr_now = SCR_FOUNDER;
    go_stage = 0;
    su_rank();
    /* "FIRST <winner> FOUNDER" */
    gfx_big(4, 3, "FIRST", C_GREEN);
    gfx_big(24, 3, "FOUNDER", C_GREEN);
    /* the others stand down the left */
    for (i = 1; i < 4; ++i) {
        k = su_order[i];
        spr_set(i, SH_SPECIES, (PL(k).species & 7) * 11 + 5, 4, 48 + i * 36, PLCOL(k), 0);
    }
    snd_play(SND_FOUNDER);
}

void gameover_tick(void)
{
    uint8_t k = su_order[0], c, i;
    if (go_stage == 0) {
        /* the winner pulses, ~32 frames a step */
        c = (frame_count & 32) ? C_WHITE : PLCOL(k);
        spr_set(0, SH_SPECIES, (PL(k).species & 7) * 11 + 5, 68, 12, c, 0);
        if (pe_now() > 540) {
            go_stage = 1;
            spr_hide_all();
            for (i = 0; i < 24; ++i)
                gfx_clear_text(0, i, 40);
            su_table();
            /* "OVERALL, " + the colony's rating */
            if (st.colony.rating < 7) {
                strcpy(txt, str_word(W_OVERALL));
                msg_row = 22;
                msg_col = C_WHITE;
                msg_start(9 + text_expand(txt + 9, str_rating(st.colony.rating), 0, 0));
            }
        }
    } else {
        msg_tick();
    }
}

/* ---------------------------------------------------------------------------
 * The lobby: seats, species, ready (the Atari's "you have selected a
 * tournament level game" screen). Left/right change our species, the
 * button toggles ready, ESC leaves the table.
 * ------------------------------------------------------------------------- */
static uint8_t lb_species, lb_sent, lb_last[4];
static uint16_t lb_lastcd;

#define LB_ROW(k) (6 + (k) * 4)

void lobby_enter(void)
{
    uint8_t k;
    anim = AN_NONE;
    fx_stop_all();
    gfx_clear();
    scr_now = SCR_TEXT;
    for (k = 0; k < 4; ++k)
        lb_last[k] = 255;
    big_center(1, str_word(W_MULE), C_ORANGE);
    big_center(3, str_word(W_TOURNAMENT), C_BLUE);
    text_center(21, str_msg(M_LOBBYHELP), C_WHITE);
    SND_X(SNX_TITLE);
    lb_sent = 0;
    lb_lastcd = 1;
    if (my_seat < 4)
        lb_species = PL(my_seat).species & 7;
}

void lobby_state(void)
{
    uint8_t k, c, j, r;
    const char *s;
    for (k = 0; k < 4; ++k) {
        r = LB_ROW(k);
        c = PL(k).flags;
        j = PL(k).species & 7;
        if (lb_last[k] == (uint8_t)(j + c * 8) && PL(k).name[0])
            continue;
        lb_last[k] = j + c * 8;
        gfx_clear_text(0, r, 40);
        gfx_clear_text(0, r + 1, 40);
        if (PL(k).name[0]) {
            gfx_bign(8, r, PL(k).name, 8, PLCOL(k));
            gfx_text(28, r, str_species(j), C_WHITE);
            s = 0;
            if (c & PF_AI)
                s = str_word(W_COMPUTER);
            else if (c & PF_READY)
                s = str_msg(M_READY);
            if (s)
                gfx_text(28, r + 1, s, C_WHITE);
            /* the species, standing */
            spr_set(k, SH_SPECIES, j * 11 + 5, 12, r * 8 - 6, PLCOL(k), 0);
        } else {
            gfx_text(8, r, str_word(W_OPEN), C_WHITE);
            spr_hide(k);
        }
    }
    if (my_seat > 3) {
        msg_row = 22;
        msg_col = C_WHITE;
        msg_show(M_SPECTATE, 0, 0);
    }
}

void lobby_tick(void)
{
    uint16_t w, v;
    msg_tick();
    /* ESC: leave this table and pick another */
    if (ev_key == KEY_LEAVE) {
        act = ACT_LEAVE;
        net_sync();
        appkey_clear_room();
        choose_table();
        appkey_save_room();
        game_start();
        return;
    }
    /* the countdown to the start, in seconds */
    w = (uint16_t)st.h.countdown;
    if (!(w & 0x8000)) {
        v = since_rx();
        v = v < w ? (w - v) / 60 : 0;
        if (v != lb_lastcd) {
            lb_lastcd = v;
            msg_row = 22;
            msg_col = C_WHITE;
            msg_show(M_LEAVEHELP, v, 0);
        }
    } else if (lb_lastcd != 0xFFFF) {
        lb_lastcd = 0xFFFF;
        msg_row = 22;
        msg_col = C_WHITE;
        msg_show(M_WAITING, 0, 0);
    }
    if (my_seat > 3)
        return;
    /* species: left/right, one change a push */
    if (inp_dir & (DIR_LEFT | DIR_RIGHT)) {
        if (!lb_sent) {
            lb_sent = 1;
            lb_species = (inp_dir & DIR_RIGHT) ? (lb_species + 1) & 7 : (lb_species + 7) & 7;
            act_send(ACT_SPECIES, lb_species);
            snd_play(SND_SELECT);
        }
    } else {
        lb_sent = 0;
    }
    if (ev_btn) {
        act_send((PL(my_seat).flags & PF_READY) ? ACT_UNREADY : ACT_READY, 0);
        snd_play(SND_SELECT);
    }
}
