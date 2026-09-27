/*
 * develop.c -- the development phase: turns, walking, the town
 * (clients/intv/develop.bas, isr.bas, terrain.bas).
 *
 * The server keeps no player position. We walk locally and send actions:
 * buy/return a M.U.L.E. at the corral, outfit it at a shop, install it on a
 * plot, assay, list land, catch the wampus, gamble at the pub. A turn
 * starts in the middle of town, as on the Atari.
 *
 * Walking positions are kept in the Intellivision client's MOB units, so
 * every constant below is the one measured there (clients/intv/docs/
 * ANIMATION.md): x = colour clock + 8, y = scanline / 2 + 8. The screen
 * gets (x - 8, (y - 8) * 2).
 */
#include <string.h>
#include "mule.h"

#define SX(x) ((int16_t)(x) - 8)
#define SY(y) (((int16_t)(y) - 8) * 2)

/* sprite slots */
#define SLOT_PL     0
#define SLOT_MULE   1
#define SLOT_WAMPUS 2

/* walk events for the main loop */
#define WE_NONE        0
#define WE_TOWN_LEFT   1
#define WE_TOWN_RIGHT  2
#define WE_STORE_LEFT  3
#define WE_STORE_RIGHT 4
#define WE_BUILDING    5

/* buildings: the outfitters are GOOD_*, so the shop is the good */
#define B_ASSAY  4
#define B_LAND   5
#define B_PUB    6
#define B_CORRAL 7

/* town geometry (Atari HPOS - 40) */
#define TOWN_Y_TOP   37         /* street: player top 37..41 */
#define TOWN_Y_BOT   41
#define TOWN_Y_IN_DN 57         /* walked into a lower building */
#define TOWN_Y_IN_UP 21         /* walked into an outfitter */
#define TOWN_X_LEFT  36         /* one more step left leaves town */
#define TOWN_X_RIGHT 132
#define TOWN_X_START 84

/* Doors: the four outfitters (by good: food, energy, smithore, crystite;
 * on the Atari they stand crystite, smithore, energy, food from the left),
 * then assay, land, pub, corral. */
static const uint8_t door_lo[8] = { 120, 96, 72, 48, 48, 68, 88, 108 };
static const uint8_t door_hi[8] = { 120, 96, 72, 48, 48, 68, 88, 124 };

uint8_t dv_state;
static uint8_t dv_seat, dv_flags, dv_mine, dv_ptu, dv_evshown;
static uint8_t dv_sample;       /* plot we took an assay sample from (255 none) */
static uint8_t dv_office;       /* 1: the land office waits for a plot */
static uint8_t dv_msg_seat, dv_msg_kind;
static uint16_t dv_left, dv_el; /* turn time left / elapsed at the last poll */
static uint8_t dv_wp;           /* wampus showing (255 none) */
static uint8_t wp_x, wp_y;
static uint8_t dv_bwait;        /* in a doorway waiting for the server */
static uint16_t dv_bt0;
static uint8_t dv_ai, dv_ait, dv_aip;   /* a computer's plot marks */

/* walking */
static uint8_t w_x, w_y, w_town, w_hold, w_event, w_bldg;
static uint8_t w_cnt, w_step, w_frame, w_face, w_sp, w_col, w_in, w_mule;
static uint8_t w_tx[4], w_ty[4], w_th;

void develop_enter(void)
{
    dv_state = 255;
    dv_msg_seat = 255;
    w_mule = 0;
    map_dots = 0;
    dv_ai = 0;
    dv_bwait = 0;
    scr_now = SCR_NONE;
}

/* ---------------------------------------------------------------------------
 * Walking, every frame (isr_walk). One step every 6 frames (map: 14 on the
 * river, 21 in the mountains); a step is 4 x / 2 y in town, 2 / 1 on the
 * map. On the map the player is the Atari's small figure, in town the
 * species.
 * ------------------------------------------------------------------------- */
static void walk_draw(void)
{
    uint8_t i, fl;
    if (w_town)
        spr_set(SLOT_PL, SH_SPECIES, w_sp * 11 + w_face, SX(w_x), SY(w_y), w_col, 0);
    else
        spr_set(SLOT_PL, SH_FIGURE, w_sp * 10 + w_face, SX(w_x), SY(w_y), w_col, 0);
    if (w_mule) {
        i = w_th;                       /* the oldest remembered step */
        if (w_tx[i] == 0)
            i = 0;
        if (w_tx[i]) {
            fl = (w_face == 2 || w_face == 3) ? SPR_FLIPX : 0;
            if (w_town)
                spr_set(SLOT_MULE, SH_MULE, w_frame, SX(w_tx[i]), SY(w_ty[i] + 3), C_WHITE, fl);
            else
                spr_set(SLOT_MULE, SH_MULE, 2 + w_frame, SX(w_tx[i]), SY(w_ty[i]), C_WHITE, fl);
        }
    } else {
        spr_hide(SLOT_MULE);
    }
}

/* How long a map step takes: the Atari times a step by what its figure
 * touches where it stands ($5837): a mountain and no playfield makes it 21
 * frames, the river 14. Here: the figure's middle against the mountains'
 * corner of a plot, and against the river's band. */
static void walk_terrain(void)
{
    uint8_t cx = w_x - 8 + 2, cy = (w_y - 8) * 2 + 2;     /* figure body, cc / lines */
    uint8_t px, py, p, t;
    if (cx < 8 || cy < 16)
        return;
    px = (cx - 8) / 16;
    py = (cy - 16) / 32;
    if (px > 8 || py > 4)
        return;
    p = py * 9 + px;
    t = st.map.terrain[p];
    if ((t & 3) == TERRAIN_RIVER) {
        if (cx + 4 >= 76 && cx <= 83)
            w_cnt += 8;
    } else if ((t >> 2) & 3) {
        /* the peaks stand in the plot's upper left (x 1..8, lines 2..20) */
        if (cx + 4 > PLOT_X(p) + 1 && cx < PLOT_X(p) + 9 &&
            cy + 6 > PLOT_Y(p) + 2 && cy < PLOT_Y(p) + 20)
            w_cnt += 15;
    }
}

static void walk_town(int8_t ix, int8_t iy)
{
    uint8_t i;
    w_cnt = 5;
    /* along the street: 4 a step; out of either end: back to the map */
    if (ix < 0) {
        if (w_x <= TOWN_X_LEFT) {
            w_event = WE_TOWN_LEFT;
            w_hold = 1;
            return;
        }
        w_x -= 4;
    } else if (ix > 0) {
        if (w_x >= TOWN_X_RIGHT) {
            w_event = WE_TOWN_RIGHT;
            w_hold = 1;
            return;
        }
        w_x += 4;
    }
    /* the street is two steps deep; pushing on at a door walks you in; the
     * outfitters let you in only with a M.U.L.E. */
    if (iy > 0) {
        if (w_y < TOWN_Y_BOT) {
            w_y += 2;
        } else {
            for (i = 4; i < 8; ++i)
                if (w_x >= door_lo[i] && w_x <= door_hi[i]) {
                    w_in = 1;
                    w_bldg = i;
                }
        }
    } else if (iy < 0) {
        if (w_y > TOWN_Y_TOP) {
            w_y -= 2;
        } else if (w_mule) {
            for (i = 0; i < 4; ++i)
                if (w_x == door_lo[i]) {
                    w_in = 2;
                    w_bldg = i;
                }
        }
    }
}

/* Through a door: 2 a step until inside, then develop_tick runs the
 * building's action and puts us back on the street. */
static void walk_building(void)
{
    w_cnt = 5;
    w_frame ^= 1;
    if (w_in == 1) {
        w_y += 2;
        w_face = 6 + w_frame;
        if (w_y >= TOWN_Y_IN_DN) {
            w_in = 0;
            w_event = WE_BUILDING;
            w_hold = 1;
        }
    } else {
        w_y -= 2;
        w_face = 8 + w_frame;
        if (w_y <= TOWN_Y_IN_UP) {
            w_in = 0;
            w_event = WE_BUILDING;
            w_hold = 1;
        }
    }
    walk_draw();
}

/* The map: w_x, w_y is the small figure's top left; the Atari keeps it to
 * HPOS 55..197, lines 45..197 (x 15..157, y 16..92). */
static void walk_map(int8_t ix, int8_t iy)
{
    w_cnt = 5;
    if (ix < 0 && w_x > 15) w_x -= 2;
    if (ix > 0 && w_x < 157) w_x += 2;
    if (iy < 0 && w_y > 16) --w_y;
    if (iy > 0 && w_y < 92) ++w_y;
    walk_terrain();
    /* into the store (plot 22 is x 80..95, y 48..63) */
    if (w_x + 5 >= 80 && w_x + 2 <= 95 && w_y + 3 >= 48 && w_y <= 63) {
        w_event = w_x < 88 ? WE_STORE_LEFT : WE_STORE_RIGHT;
        w_hold = 1;
    }
}

void walk_isr(void)
{
    int8_t ix = 0, iy = 0;
    if (w_hold)
        return;
    if (w_cnt) {
        --w_cnt;
        return;
    }
    if (w_in) {
        walk_building();
        return;
    }
    if (inp_dir & DIR_LEFT) ix = -1;
    if (inp_dir & DIR_RIGHT) ix = 1;
    if (inp_dir & DIR_UP) iy = -1;
    if (inp_dir & DIR_DOWN) iy = 1;
    if (!ix && !iy) {
        if (w_step) {
            w_step = 0;
            w_face = 5;
            walk_draw();
        }
        return;
    }
    if (w_town)
        walk_town(ix, iy);
    else
        walk_map(ix, iy);
    if (w_hold)
        return;
    w_step = 1;
    w_frame ^= 1;
    if (ix > 0) w_face = w_frame;
    if (ix < 0) w_face = 2 + w_frame;
    if (!ix && iy > 0) w_face = 6 + w_frame;
    if (!ix && iy < 0) w_face = 8 + w_frame;
    snd_play(SND_STEP);
    /* remember where we were, for the M.U.L.E. to follow */
    w_tx[w_th] = w_x;
    w_ty[w_th] = w_y;
    w_th = (w_th + 1) & 3;
    walk_draw();
}

/* ---------------------------------------------------------------------------
 * The town screen.
 * ------------------------------------------------------------------------- */
static void town_money(void)
{
    if (scr_now != SCR_TOWN || my_seat > 3)
        return;
    msg_row = 20;
    msg_show(M_MONEY, PL(my_seat).money, 0);
    msg_row = 22;
    /* the corral: price and how many are left */
    put_num(TX(26), 19, st.store.mulePrice, 3, C_WHITE);
    put_num(TX(27), 13, st.store.mules, 2, C_WHITE);
}

static void dv_resume(void)
{
    w_hold = 0;
    w_face = 5;
    walk_draw();
    anim = AN_WALK;
}

static void town_enter(void)
{
    anim = AN_NONE;
    fx_stop_all();
    gfx_clear();
    scr_now = SCR_TOWN;
    gfx_town();
    title_show(T_DEVELOPMENT, rnd);
    town_money();
    bar_x = 136;
    bar_top = 16;
    bar_h = 144;
    bar_last = 255;
    w_town = 1;
    dv_resume();
}

static void dev_map_enter(void)
{
    map_enter();
    title_show(T_DEVELOPMENT, rnd);
}

/* The screen blanks for about 22 frames between the town and the map. */
static void dv_blank(void)
{
    uint8_t k;
    anim = AN_NONE;
    spr_hide_all();
    gfx_clear();
    for (k = 0; k < 22; ++k) {
        frame_wait();
        snd_tick();
    }
    for (k = 0; k < 4; ++k)
        w_tx[k] = 0;
}

static void dv_start_walk(void)
{
    uint8_t k;
    dv_state = DV_WALK;
    dv_sample = 255;
    dv_office = 0;
    dv_evshown = 0;
    dv_bwait = 0;
    w_sp = PL(my_seat).species & 7;
    w_col = PLCOL(my_seat);
    bar_col = w_col;
    for (k = 0; k < 4; ++k)
        w_tx[k] = w_ty[k] = 0;
    w_x = TOWN_X_START;
    w_y = TOWN_Y_TOP + 2;
    w_in = 0;
    w_event = 0;
    w_cnt = 0;
    town_enter();
    snd_play(SND_SELECT);
}

static void dv_stop_walk(void)
{
    anim = AN_NONE;
    w_hold = 1;
    dv_state = DV_OVER;
    spr_hide(SLOT_PL);
    spr_hide(SLOT_MULE);
    spr_hide(SLOT_WAMPUS);
    if (scr_now != SCR_MAP)
        dev_map_enter();
}

/* ---------------------------------------------------------------------------
 * Buildings. The action goes to the server; its answer puts us back out.
 * ------------------------------------------------------------------------- */
static void back_to_street(void)
{
    dv_bwait = 0;
    if (dv_state == DV_WALK) {
        w_y = TOWN_Y_TOP + 2;
        snd_play(SND_SELECT);
        dv_resume();
    }
}

static void dv_building(void)
{
    uint8_t a = ACT_NONE;
    uint16_t arg = 0;
    if (w_bldg == B_CORRAL) {
        a = w_mule ? ACT_RETURN : ACT_BUYMULE;
    } else if (w_bldg <= GOOD_CRYSTITE) {
        a = ACT_OUTFIT;
        arg = w_bldg;
    } else if (w_bldg == B_ASSAY) {
        if (dv_sample < NUM_PLOTS) {
            a = ACT_ASSAY;
            arg = dv_sample;
        } else {
            msg_show(M_NOSAMPLE, 0, 0);
        }
    } else if (w_bldg == B_LAND) {
        dv_office = 1;
        msg_show(M_LANDOFFICE, 0, 0);
    } else if (w_bldg == B_PUB) {
        a = ACT_PUB;
    }
    if (a != ACT_NONE) {
        /* stand in the door until the server answers */
        act_send(a, arg);
        dv_bwait = 1;
        dv_bt0 = frame_count;
        return;
    }
    back_to_street();
}

static void dv_building_result(void)
{
    uint8_t c = st.h.err;
    if (c == ERR_MONEY) {
        msg_show(M_NOMONEY, 0, 0);
    } else if (c == ERR_NOSTOCK) {
        msg_show(M_NOMULES, 0, 0);
    } else if (c) {
        snd_play(SND_ERROR);
    } else {
        if (act_sent == ACT_ASSAY) {
            msg_show(M_ASSAY + (st.h.result & 7), 0, 0);
            dv_sample = 255;
        }
        if (act_sent == ACT_BUYMULE || act_sent == ACT_OUTFIT)
            snd_play(SND_BUY);
    }
    town_money();
}

/* What the server said about a map action we sent. */
static void dv_map_result(void)
{
    uint8_t id = 0;
    if (act_sent < ACT_INSTALL || act_sent > ACT_WAMPUS)
        return;
    msg_col = C_WHITE;
    if (st.h.err) {
        snd_play(SND_ERROR);
        act_sent = 0;
        return;
    }
    if (act_sent == ACT_INSTALL) {
        if (st.h.result == 1) {
            id = M_RANAWAY;
            snd_play(SND_ERROR);
        } else {
            snd_play(SND_BUY);
        }
    } else if (act_sent == ACT_WAMPUS) {
        id = M_CAUGHT;
        snd_play(SND_FOUNDER);
    } else if (act_sent == ACT_SELL) {
        id = M_FORSALE;
    } else if (act_sent == ACT_UNSELL) {
        id = M_NOTFORSALE;
    }
    if (id)
        msg_show(id, st.h.result, 0);
    act_sent = 0;
}

/* The button on the map: catch the wampus, install the M.U.L.E. we lead,
 * list land after the land office, or take an assay sample. */
static void dv_button(void)
{
    uint8_t mp = 255, x, y;
    /* the plot under the middle of the small figure (w_x + 4, w_y + 2) */
    if (w_x >= 12 && w_y >= 14) {
        x = (w_x - 12) / 16;
        y = (w_y + 2 - 16) / 16;
        if (x < 9 && y < 5)
            mp = y * 9 + x;
    }
    if (dv_wp < 12 && !w_mule) {
        /* within 8 of a showing wampus catches it */
        if (w_x + 8 >= wp_x && w_x <= wp_x + 8 && w_y + 4 >= wp_y && w_y <= wp_y + 8) {
            act_send(ACT_WAMPUS, 0);
            return;
        }
    }
    if (mp >= NUM_PLOTS)
        return;
    if (w_mule) {
        act_send(ACT_INSTALL, mp);
        snd_play(SND_SELECT);
        return;
    }
    if (dv_office) {
        if (OWNER(mp) == my_seat) {
            act_send((st.map.prod[mp] & 64) ? ACT_UNSELL : ACT_SELL, mp);
            dv_office = 0;
        }
        return;
    }
    dv_sample = mp;
    snd_play(SND_SAMPLE);
}

/* The wampus: turn.wampus lists when (turn jiffies) and where it shows. */
static void dv_wampus(void)
{
    uint8_t n = st.turn.nWampus, k, mp, c;
    uint16_t w = dv_el + since_rx();
    MuleWampus *wp;
    dv_wp = 255;
    for (k = 0; k < n && k < MAX_WAMPUS; ++k)
        if (w >= st.turn.wampus[k].from && w < st.turn.wampus[k].to)
            dv_wp = k;
    if (dv_wp > 11) {
        spr_hide(SLOT_WAMPUS);
        return;
    }
    wp = &st.turn.wampus[dv_wp];
    mp = wp->plot;
    c = wp->mount;
    /* on the plot's mountains */
    wp_x = (mp % 9) * 16 + 16 + 1 + c * 2;
    wp_y = (mp / 9) * 16 + 16 + 1 + c * 2;
    /* it flashes: 8 frames on, 8 off */
    if (frame_count & 8)
        spr_set(SLOT_WAMPUS, SH_WAMPUS, 0, SX(wp_x), SY(wp_y), C_WHITE, 0);
    else
        spr_hide(SLOT_WAMPUS);
}

/* The player event: shown at the start of the turn, as on the Atari. */
static void dv_event_msg(void)
{
    uint8_t k, j, n = 0, mp, mg;
    uint16_t v, v2 = 0;
    if (dv_evshown)
        return;
    dv_evshown = 1;
    k = st.turn.event;
    if (k == 0 || k > 22)
        return;
    /* amount: x * 25 * (round / 4 + 1), or per M.U.L.E. for three events */
    v = 25 * (rnd / 4 + 1);
    j = pevent_x[k - 1];
    if (j >= 100) {
        for (mp = 0; mp < NUM_PLOTS; ++mp)
            if (OWNER(mp) == my_seat) {
                mg = MULEOF(mp);
                if (j == 100 && mg == GOOD_FOOD) ++n;
                if (j == 101 && (mg == GOOD_SMITHORE || mg == GOOD_CRYSTITE)) ++n;
                if (j == 102 && mg == GOOD_ENERGY) ++n;
            }
        if (j == 102) {
            v *= n;
        } else {
            v2 = v * 2 * n;
            v *= 2;
        }
        if (j == 101)
            v = v2;
    } else {
        v *= j;
    }
    strcpy(txt, "*** ");
    msg_col = C_WHITE;
    msg_start(4 + text_expand(txt + 4, str_pevent(k - 1), v, v2));
}

/* ---------------------------------------------------------------------------
 * A computer's turn: the server plays it at once, so the map already shows
 * what it did. On the Atari the computer's figure stands in the store for
 * its whole turn; here it also walks out to the plot that gained a
 * M.U.L.E. (or one of its plots) with the M.U.L.E. behind, which stays
 * there, then walks back into the store and waits. (dv_ai_walk)
 * ------------------------------------------------------------------------- */
static uint8_t ai_script[2][40];

static uint8_t *ai_move(uint8_t *s, int8_t dx, int8_t dy, uint8_t n)
{
    *s++ = FX_MOVE;
    *s++ = (uint8_t)dx;
    *s++ = (uint8_t)dy;
    *s++ = 6;
    *s++ = n;
    return s;
}

static void dv_ai_land(uint8_t on)
{
    uint8_t mp;
    for (mp = 0; mp < NUM_PLOTS; ++mp)
        if (OWNER(mp) == dv_seat) {
            if (on) {
                map_blink = mp;
                map_blink_on = 1;
            }
            map_draw_plot(mp);
            map_blink = NOBODY;
        }
}

static void dv_ai_walk(void)
{
    uint8_t j = 255, n = 0, mp, x, y, a, hc, hi, sp, col;
    uint8_t *s;
    Actor *f;
    const uint8_t c = 83;           /* in the store (MOB x 83, y 52) */

    fx_stop_all();
    for (mp = 0; mp < NUM_PLOTS; ++mp)
        if (OWNER(mp) == dv_seat && mp != STORE_PLOT) {
            if (j == 255)
                j = mp;
            if (MULEOF(mp) < 4 && st.map.mule[mp] != prev.map.mule[mp]) {
                j = mp;
                n = 1;
            }
        }
    if (j > 44)
        j = 13;                     /* no land: out and back */
    x = (j % 9) * 16 + 16 + 4;
    y = (j / 9) * 16 + 16 + 6;
    sp = PL(dv_seat).species & 7;
    col = PLCOL(dv_seat);
    if (dv_ai == 2)
        map_draw_all();             /* the last one's land */
    dv_aip = j;
    dv_ai = 1;
    hc = x >= c ? (x - c) / 2 : (c - x) / 2;
    hi = y >= 52 ? y - 52 : 52 - y;
    for (a = 0; a < 2; ++a) {
        if (a == 1 && !n)
            break;
        f = &fx[a];
        f->kind = FK_SINGLE;
        f->slot = a;                /* 0 the figure, 1 its M.U.L.E. */
        f->shape = a ? SH_MULE : SH_FIGURE;
        f->fbase = a ? 0 : sp * 10;
        f->frame = a ? 2 : 0;
        f->d2 = 1;
        f->col = a ? C_WHITE : col;
        f->flags = 0;
        f->x = SX(c);
        f->y = SY(52);
        s = ai_script[a];
        if (a) {
            *s++ = FX_WAIT;
            *s++ = 24;
        }
        /* across 2 every 6 frames, then down/up 1 (2 lines) every 6 */
        *s++ = FX_FLAGS;
        *s++ = x >= c ? 0 : SPR_FLIPX;
        s = ai_move(s, x >= c ? 2 : -2, 0, hc + 1);
        s = ai_move(s, 0, y >= 52 ? 2 : -2, hi + 1);
        if (a) {
            /* the M.U.L.E. is installed: it goes no further */
            *s++ = FX_WAIT;
            *s++ = 20;
            *s++ = FX_HIDE;
            *s++ = FX_END;
        } else {
            /* 90 frames on the plot (outlined in white: FX_SIG 3 / 4), then
             * the same way back, facing home, and a long wait in the store */
            *s++ = FX_SIG;
            *s++ = 3;
            *s++ = FX_WAIT;
            *s++ = 90;
            *s++ = FX_SIG;
            *s++ = 4;
            s = ai_move(s, 0, y >= 52 ? -2 : 2, hi + 1);
            *s++ = FX_FLAGS;
            *s++ = x >= c ? SPR_FLIPX : 0;
            s = ai_move(s, x >= c ? -2 : 2, 0, hc + 1);
            *s++ = FX_WAIT;
            *s++ = 255;
            *s++ = FX_LOOP;
            *s++ = 200;
            *s++ = 2;
        }
        f->st = 0;
        fx_start(a, ai_script[a]);
        f->hidden = 0;
        spr_set(a, f->shape, f->fbase + f->frame, f->x, f->y, f->col, 0);
    }
}

/* Per frame in a computer's turn: its land outlined in white for 40 frames
 * as the turn starts, and the plot it works while it is there. */
static void dv_ai_marks(void)
{
    if (dv_ai == 1) {
        dv_ai_land(1);
        dv_ai = 2;
        dv_ait = (uint8_t)frame_count;
    } else if (dv_ai == 2) {
        if ((uint8_t)((uint8_t)frame_count - dv_ait) >= 40) {
            dv_ai_land(0);
            dv_ai = 0;
        }
    }
    if (fx_flag == 3 || fx_flag == 4) {
        if (fx_flag == 3) {
            map_blink = dv_aip;
            map_blink_on = 1;
        }
        if (dv_aip < NUM_PLOTS)
            map_draw_plot(dv_aip);
        map_blink = NOBODY;
        fx_flag = 0;
    }
}

/* ---------------------------------------------------------------------------
 * A new state.
 * ------------------------------------------------------------------------- */
static void say_turn(void)
{
    uint8_t k, n = 0;
    const char *nm = PL(dv_seat).name;
    for (k = 0; k < 8 && nm[k]; ++k)
        txt[n++] = nm[k];
    strcpy(txt + n, str_msg(M_TURN));
    msg_start(n + 7);
}

static void dv_bar(void);

void develop_state(void)
{
    uint8_t k;
    dv_seat = (uint8_t)st.turn.seat;
    dv_flags = st.turn.flags;
    dv_left = st.turn.left;
    dv_el = st.turn.elapsed;
    dv_ptu = st.turn.pTU;
    if (dv_ptu == 0)
        dv_ptu = 28;
    dv_mine = dv_seat == my_seat;

    if (dv_mine && (dv_flags & TF_STARTED) && !(dv_flags & TF_ENDED)) {
        if (dv_state != DV_WALK)
            dv_start_walk();
        /* the server's view of what we lead wins over ours */
        w_mule = (dv_flags & TF_HASMULE) ? 1 : 0;
        if (dv_bwait && act_sent >= ACT_BUYMULE && act_sent <= ACT_PUB &&
            act_sent != ACT_INSTALL && act_sent != ACT_SELL && act_sent != ACT_UNSELL &&
            act_sent != ACT_WAMPUS) {
            dv_building_result();
            back_to_street();
        }
        if (scr_now == SCR_MAP) {
            map_update();
            dv_map_result();
        }
        return;
    }
    /* anything else happens on the map */
    if (dv_state == DV_WALK)
        dv_stop_walk();
    if (scr_now != SCR_MAP)
        dev_map_enter();
    msg_row = 22;
    if (dv_mine && !(dv_flags & TF_STARTED)) {
        if (dv_state != DV_PROMPT) {
            dv_state = DV_PROMPT;
            msg_col = C_WHITE;
            msg_show(M_START, 0, 0);
            snd_play(SND_ALERT);
        }
    } else if (dv_seat < 4) {
        /* someone's turn: say so once per turn, then once when it ends. A
         * computer's turn is over on the server at once but plays out on
         * the screen, so it stays "taking his turn" until the next seat */
        k = 0;
        if (PL(dv_seat).flags & PF_AI)
            k = 1;
        else if (dv_flags & TF_ENDED)
            k = 2;
        if (dv_seat != dv_msg_seat || k != dv_msg_kind) {
            dv_msg_seat = dv_seat;
            dv_msg_kind = k;
            msg_col = C_WHITE;
            if (k == 1) {
                dv_state = DV_WAIT;
                msg_show(M_COMPUTER, 0, 0);
                dv_ai_walk();
            } else if (k == 2) {
                dv_state = DV_OVER;
                if (act_sent == ACT_PUB && dv_mine)
                    msg_show(M_GAMBLE, st.h.result, 0);
                else if (dv_left == 0)
                    msg_show(M_TIMEOUT, 0, 0);
                else
                    msg_clear();
            } else {
                dv_state = DV_WAIT;
                say_turn();
            }
        }
    }
    /* the time bar for whoever is playing, in their colour */
    if (dv_seat < 4)
        bar_col = PLCOL(dv_seat);
    if ((dv_flags & TF_STARTED) && !(dv_flags & TF_ENDED))
        dv_bar();
    else
        bar_draw(0);
    map_update();
}

/* The turn's time bar, run down from the last state (which is since_rx
 * old already when it comes: rx_age). A line more than it shows now is
 * only a later reply's jitter, not time given back: it stays down. */
static void dv_bar(void)
{
    uint16_t w = since_rx();
    uint8_t px = w < dv_left ? (dv_left - w) / dv_ptu : 0;
    if (bar_last != 255 && px == bar_last + 1)
        return;
    bar_draw(px);
}

void develop_tick(void)
{
    msg_tick();
    if (dv_state == DV_PROMPT) {
        if (ev_btn) {
            act_send(ACT_BEGIN, 0);
            snd_play(SND_SELECT);
        }
        return;
    }
    if (dv_state != DV_WALK) {
        if (dv_state == DV_WAIT)
            dv_ai_marks();
        /* watching: run the bar down between polls */
        if ((dv_flags & TF_STARTED) && !(dv_flags & TF_ENDED))
            dv_bar();
        return;
    }

    /* our own turn */
    dv_bar();
    dv_event_msg();
    if (dv_bwait) {
        if ((uint16_t)(frame_count - dv_bt0) > 600)
            back_to_street();       /* no answer: give up waiting */
        return;
    }
    if (w_event) {
        switch (w_event) {
        case WE_TOWN_LEFT:
        case WE_TOWN_RIGHT:
            w_x = w_event == WE_TOWN_LEFT ? 68 : 99;
            w_y = 52;
            w_event = 0;
            w_town = 0;
            dv_blank();
            dev_map_enter();
            bar_last = 255;
            dv_resume();
            w_cnt = 20;             /* the first map step waits 20 frames */
            break;
        case WE_STORE_LEFT:
        case WE_STORE_RIGHT:
            w_x = w_event == WE_STORE_LEFT ? TOWN_X_LEFT : TOWN_X_RIGHT;
            w_y = TOWN_Y_TOP + 2;
            w_event = 0;
            dv_blank();
            town_enter();
            break;
        case WE_BUILDING:
            w_event = 0;
            dv_building();
            break;
        }
        return;
    }
    if (!w_town) {
        dv_wampus();
        if (ev_btn)
            dv_button();
    }
    if (ev_key == KEY_END)
        act_send(ACT_END, 0);
}
