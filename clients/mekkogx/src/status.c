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

/* The summary's columns: each number right-aligned under its heading,
 * clear of the players standing at the left. */
#if TCOLS == 40
#define SU_MONEY_X   6          /* big, 6 digits: ends at cell 17 */
#define SU_MONEY_H   13
#define SU_LAND_X    18         /* 5 digits: ends at 27 */
#define SU_LAND_H    24
#define SU_GOODS_X   28         /* 6 digits: ends at 39 */
#define SU_GOODS_H   35
#define SU_TOTAL_X   26         /* 7 digits */
#define SU_TOTAL_L   8
#define SU_COLONY_X  6
#define GO_FIRST_X   4          /* "FIRST <winner> FOUNDER" */
#define GO_FOUNDER_X 24
#define LB_NAME_X    8          /* the lobby */
#define LB_SPEC_X    28
#define LB_HELP_ROW  21
#elif TCOLS == 80
#define SU_MONEY_X   14         /* (the H19: big is small) 6 digits: ends at 19 */
#define SU_MONEY_H   15
#define SU_LAND_X    28         /* 5 digits: ends at 32 */
#define SU_LAND_H    29
#define SU_GOODS_X   42         /* 6 digits: ends at 47 */
#define SU_GOODS_H   43
#define SU_TOTAL_X   41         /* 7 digits */
#define SU_TOTAL_L   34
#define SU_COLONY_X  14
#define GO_FIRST_X   29
#define GO_FOUNDER_X 39
#define LB_NAME_X    12
#define LB_SPEC_X    40
#define LB_HELP_ROW  20
#else
#define SU_MONEY_X   10         /* 6 digits: ends at cell 15 */
#define SU_MONEY_H   11
#define SU_LAND_X    19         /* 5 digits: ends at 23 */
#define SU_LAND_H    20
#define SU_GOODS_X   26         /* 6 digits: ends at 31 */
#define SU_GOODS_H   27
#define SU_TOTAL_X   25         /* 7 digits */
#define SU_TOTAL_L   10
#define SU_COLONY_X  6
#define GO_FIRST_X   7
#define GO_FOUNDER_X 17
#define LB_NAME_X    8
#define LB_SPEC_X    (TCOLS - 12)
#define LB_HELP_ROW  20         /* (it may take two rows) */
#endif

/* big_num for the colony's total, which can pass 65535: in 16 bits, as
 * q * 10000 + r (a 32-bit division is a library routine of its own on most
 * of these machines, and slow on all of them). Only this screen needs it. */
static void big_num32(uint8_t tx, uint8_t ty, uint32_t v, uint8_t w, uint8_t col)
{
    char buf[10];
    uint16_t h = (uint16_t)(v >> 16), q = (uint16_t)v / 10000, r = (uint16_t)v % 10000;
    uint8_t i = w, k;
    while (h--) {               /* 65536 = 6 * 10000 + 5536 */
        q += 6;
        if ((r += 5536) >= 10000) {
            r -= 10000;
            ++q;
        }
    }
    buf[w] = 0;
    for (k = 0; k < 4 && i; ++k) {      /* the low four digits */
        buf[--i] = '0' + (uint8_t)(r % 10);
        r /= 10;
        if (!r && !q)
            break;
    }
    while (q && i) {
        buf[--i] = '0' + (uint8_t)(q % 10);
        q /= 10;
    }
    while (i)
        buf[--i] = ' ';
    gfx_big(tx, ty, buf, col);
}

static uint8_t su_order[4];     /* seats by rank */
static uint8_t su_drawn, su_land;

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
        big_num(SU_MONEY_X, SU_ROW(i), PL(k).money, 6, PLCOL(k));
        big_num(SU_LAND_X, SU_ROW(i), su_land_of(k), 5, PLCOL(k));
        v = 0;
        for (g = 0; g < 4; ++g)
            v += PL(k).goods[g] * st.store.price[g];
        big_num(SU_GOODS_X, SU_ROW(i), v, 6, PLCOL(k));
        gfx_text(SU_TOTAL_L, SU_ROW(i) + 2, str_word(W_TOTAL), C_WHITE);
        big_num(SU_TOTAL_X, SU_ROW(i) + 2, PL(k).score, 7, PLCOL(k));
    }
    big_num32(SU_TOTAL_X, 21, st.colony.total, 7, C_ORANGE);
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
    /* headings right-aligned over their columns */
    gfx_text(SU_MONEY_H, 2, str_word(W_MONEY), C_WHITE);
    gfx_text(SU_LAND_H, 2, str_word(W_LAND), C_WHITE);
    gfx_text(SU_GOODS_H, 2, str_word(W_GOODS), C_WHITE);
    gfx_big(SU_COLONY_X, 21, str_word(W_COLONY), C_ORANGE);
    su_values();
    if (phase == PHASE_SUMMARY) {
        msg_row = 22;
        msg_col = C_WHITE;
        msg_show(M_PRESSALL, 0, 0);
        if (my_seat < 4)
            snd_play(SND_ALERT);
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
    su_drawn = 0;
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
        return;
    }
    su_begin();
}

void summary_state(void)
{
    uint8_t all = 0;
    if (su_land)
        return;
    if (!su_drawn && (uint16_t)(frame_count - ph_t0) >= 410) {
        su_table();
        all = 1;
    }
    /* the big digits again only when a number under them changed: the
     * players (money, goods, score), the plots and their M.U.L.E.s (owner
     * and mule, one after the other), the prices, the colony's total */
    if (su_drawn && (all ||
        memcmp(st.players, prev.players, sizeof(st.players)) ||
        memcmp(st.map.owner, prev.map.owner, sizeof(st.map.owner) + sizeof(st.map.mule)) ||
        memcmp(st.store.price, prev.store.price, sizeof(st.store.price)) ||
        st.colony.total != prev.colony.total))
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
        /* again while the server doesn't show us ready: a press it never
         * got mustn't hold the whole table */
        if (ev_btn && my_seat < 4 && !(PL(my_seat).flags & PF_READY)) {
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
    fx_stop_all();
    gfx_clear();
    scr_now = SCR_FOUNDER;
    go_stage = 0;
    su_rank();
    /* "FIRST <winner> FOUNDER" */
    gfx_big(GO_FIRST_X, 3, "FIRST", C_GREEN);
    gfx_big(GO_FOUNDER_X, 3, "FOUNDER", C_GREEN);
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
                gfx_clear_text(0, i, TCOLS);
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
static uint8_t lb_spec;         /* watching (no seat): the waiting line says so */

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
    text_center(LB_HELP_ROW, str_msg(M_LOBBYHELP), C_WHITE);
    lb_sent = 0;
    lb_lastcd = 0xFFFE;             /* neither a count nor waiting */
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
        gfx_clear_text(0, r, TCOLS);
        gfx_clear_text(0, r + 1, TCOLS);
        if (PL(k).name[0]) {
            gfx_bign(LB_NAME_X, r, PL(k).name, 8, PLCOL(k));
            gfx_text(LB_SPEC_X, r, str_species(j), C_WHITE);
            s = 0;
            if (c & PF_AI)
                s = str_word(W_COMPUTER);
            else if (c & PF_READY)
                s = str_msg(M_READY);
            if (s)
                gfx_text(LB_SPEC_X, r + 1, s, C_WHITE);
            /* the species, standing */
            spr_set(k, SH_SPECIES, j * 11 + 5, 12, r * 8 - 6, PLCOL(k), 0);
        } else {
            gfx_text(LB_NAME_X, r, str_word(W_OPEN), C_WHITE);
            spr_hide(k);
        }
    }
}

void lobby_tick(void)
{
    uint16_t w, v;
    msg_tick();
    /* ESC: leave this table and pick another */
    if (ev_key == KEY_LEAVE) {
        leave_table();
        return;
    }
    /* the countdown to the start, in seconds (redrawn only when it
     * changes: a scrolling line would start again) */
    if ((my_seat > 3) != lb_spec) {
        lb_spec = my_seat > 3;
        lb_lastcd = 0xFFFE;
        gfx_clear_text(0, 0, 9);        /* the SPECTATOR tag (text.c) */
    }
    w = (uint16_t)st.h.countdown;
    if (!(w & 0x8000)) {
        v = since_rx();
        v = v < w ? (w - v) / 60 : 0;
        if (v != lb_lastcd) {
            if (lb_lastcd >= 0xFFFE && my_seat < 4)
                snd_play(SND_ALERT);    /* the countdown has begun */
            lb_lastcd = v;
            msg_row = 22;
            msg_col = C_WHITE;
            msg_show(M_LEAVEHELP, v, 0);
        }
    } else if (lb_lastcd != 0xFFFF) {
        lb_lastcd = 0xFFFF;
        msg_row = 22;
        msg_col = C_WHITE;
        msg_show(lb_spec ? M_SPECTATE : M_WAITING, 0, 0);
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

#ifdef MULE_AUTOPLAY
/* Autoplay: ready once all four seats are players (a slower machine still
 * booting isn't shut out by the all-ready start), and on with the summary
 * a moment after its table is up. A press every 45 frames until the
 * server shows us ready. */
void ap_status(void)
{
    uint8_t k, n = 0;
    if (PL(my_seat).flags & PF_READY)
        return;
    if (phase == PHASE_LOBBY) {
        for (k = 0; k < 4; ++k)
            if (PL(k).name[0] && !(PL(k).flags & PF_AI))
                ++n;
        if (n == 4 && ap_every(45))
            ev_btn = 1;
    } else if (su_drawn && (uint16_t)(frame_count - ph_t0) > 560 && ap_every(45)) {
        ev_btn = 1;
    }
}
#endif
