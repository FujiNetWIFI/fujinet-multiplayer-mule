/*
 * events.c -- the set pieces: ships, colony events, production
 * (clients/intv/events.bas). Speeds and lengths are the Atari's
 * (clients/intv/docs/ANIMATION.md); positions are converted from the
 * Intellivision's MOB units (x - 8 colour clocks, (y - 8) * 2 lines).
 *
 * Sprite slots: the ship is 5-7 (nose, upper and lower hull), the docking
 * ship 8, event sprites 3 and 4. The transport ship's scripts are in fx.c
 * (the grant and the summary use them too).
 */
#include "mule.h"

extern const uint8_t fx_land[];

#define PIRATE_Y 66             /* the pirate's hull sits 8 lines lower in its frame */

/* Pirate ship: fast pass, back to the store, raid, away. */
static const uint8_t fx_pirate[] = {
    FX_LOOK, FK_SHIP, 5, SH_NOSE, 1, 0, C_WHITE, C_RED,
    FX_POS, XB(182), XB(PIRATE_Y),
    FX_WAIT, 36,
    FX_MOVE, SB(-1), 0, 1, 230,
    FX_HIDE, FX_WAIT, 18,
    FX_POS, XB(-40), XB(PIRATE_Y),
    FX_MOVE, 1, 0, 4, 78,
    FX_MOVE, 5, 0, 6, 9,
    FX_SIG, 2,                          /* landed: "pirates take all crystite!" */
    FX_WAIT, 150,
    FX_MOVE, SB(-5), 0, 8, 9,
    FX_MOVE, SB(-1), 0, 4, 90,
    FX_HIDE,
    FX_SIG, 1,
    FX_END
};

/* Pest: crawls around its plot (set x/y first), 2 cc every 5 frames,
 * pausing 11 frames at each turn. */
static const uint8_t fx_pest[] = {
    FX_LOOK, FK_SINGLE, 3, SH_PEST, 0, 1, C_WHITE, 0,
    FX_FRAME, 0,
    FX_MOVE, 2, 0, 5, 3, FX_WAIT, 11,
    FX_MOVE, 0, 2, 5, 3, FX_WAIT, 11,
    FX_MOVE, SB(-2), 0, 5, 3, FX_WAIT, 11,
    FX_MOVE, 0, SB(-2), 5, 3, FX_WAIT, 11,
    FX_LOOP, 6, 28,
    FX_HIDE,
    FX_SIG, 1,
    FX_END
};

/* Acid rain: the cloud drifts along its row, its colour cycling, with its
 * lightning 24 lines under it, flickering. */
static const uint8_t fx_cloud[] = {
    FX_LOOK, FK_SINGLE, 3, SH_CLOUD, 0, 0, C_WHITE, 0,
    FX_FRAME, 0, FX_COL, C_WHITE, FX_MOVE, 1, 0, 4, 5,
    FX_FRAME, 1, FX_COL, C_BLUE, FX_MOVE, 1, 0, 4, 5,
    FX_FRAME, 2, FX_COL, C_WHITE, FX_MOVE, 1, 0, 4, 5,
    FX_FRAME, 3, FX_COL, C_LTBLUE, FX_MOVE, 1, 0, 4, 5,
    FX_LOOP, 7, 36,
    FX_HIDE,
    FX_SIG, 1,
    FX_END
};
static const uint8_t fx_bolt[] = {
    FX_LOOK, FK_SINGLE, 4, SH_BOLT, 0, 0, C_YELLOW, 0,
    FX_FRAME, 0, FX_MOVE, 1, 0, 4, 5,
    FX_HIDE, FX_MOVE, 1, 0, 4, 5,
    FX_FRAME, 0, FX_MOVE, 1, 0, 4, 5,
    FX_HIDE, FX_MOVE, 1, 0, 4, 5,
    FX_LOOP, 7, 26,
    FX_HIDE,
    FX_END
};

/* Sunspot sparks: single pixels flickering around the map. */
static const uint8_t fx_sparks[] = {
    FX_LOOK, FK_SINGLE, 3, SH_SPARK, 0, 0, C_WHITE, 0,
    FX_POS, XB(32), XB(44), FX_WAIT, 6, FX_POS, XB(112), XB(124), FX_WAIT, 6,
    FX_POS, XB(64), XB(28), FX_WAIT, 6, FX_POS, XB(142), XB(84), FX_WAIT, 6,
    FX_POS, XB(12), XB(144), FX_WAIT, 6, FX_POS, XB(92), XB(50), FX_WAIT, 6,
    FX_POS, XB(52), XB(112), FX_WAIT, 6, FX_POS, XB(128), XB(36), FX_WAIT, 6,
    FX_LOOP, 7, 40,
    FX_HIDE,
    FX_SIG, 1,
    FX_END
};
static const uint8_t fx_sparks2[] = {
    FX_LOOK, FK_SINGLE, 4, SH_SPARK, 0, 0, C_YELLOW, 0,
    FX_WAIT, 3,
    FX_POS, XB(80), XB(136), FX_WAIT, 6, FX_POS, XB(22), XB(74), FX_WAIT, 6,
    FX_POS, XB(132), XB(152), FX_WAIT, 6, FX_POS, XB(102), XB(24), FX_WAIT, 6,
    FX_POS, XB(44), XB(164), FX_WAIT, 6, FX_POS, XB(152), XB(60), FX_WAIT, 6,
    FX_POS, XB(8), XB(104), FX_WAIT, 6, FX_POS, XB(88), XB(94), FX_WAIT, 6,
    FX_LOOP, 7, 40,
    FX_HIDE,
    FX_END
};

/* Radiation: the plot's M.U.L.E. flashes, then bolts off to the right. */
static const uint8_t fx_radiation[] = {
    FX_LOOK, FK_SINGLE, 3, SH_MULE, 2, 0, C_WHITE, 0,
    FX_FRAME, 2, FX_WAIT, 6, FX_HIDE, FX_WAIT, 6,
    FX_LOOP, 5, 7,
    FX_FRAME, 2,
    FX_MOVE, 2, 0, 1, 90,
    FX_HIDE,
    FX_SIG, 1,
    FX_END
};

/* Meteorite: falls from the top right onto its plot (set the start). */
static const uint8_t fx_meteor[] = {
    FX_LOOK, FK_SINGLE, 3, SH_METEOR, 0, 0, C_ORANGE, 0,
    FX_FRAME, 0,
    FX_MOVE, SB(-2), 2, 2, 60,
    FX_HIDE,
    FX_SIG, 1,
    FX_END
};

/* ---------------------------------------------------------------------------
 * The production phase: ProductionDisplay (1421 jiffies) on the server,
 * which is what the Atari takes.
 * ------------------------------------------------------------------------- */
static uint8_t pr_event, pr_plot, pr_started, pr_done;
static uint16_t pr_last;        /* the fire / quake's last flicker */

static void pr_event_start(void)
{
    Actor *a = &fx[0];
    switch (pr_event) {
    case CE_PIRATES:
        fx_start(0, fx_pirate);
        break;
    case CE_SHIPRETURNS:
        fx_start(0, fx_land);
        break;
    case CE_PEST:
        a->x = PLOT_X(pr_plot);
        a->y = PLOT_Y(pr_plot);
        fx_start(0, fx_pest);
        break;
    case CE_ACIDRAIN:
        a->x = 0;
        a->y = PLOT_Y(pr_plot);
        fx_start(0, fx_cloud);
        fx[1].x = 0;
        fx[1].y = PLOT_Y(pr_plot) + 24;
        fx_start(1, fx_bolt);
        break;
    case CE_SUNSPOT:
        fx_start(0, fx_sparks);
        fx_start(1, fx_sparks2);
        break;
    case CE_RADIATION:
        a->x = PLOT_X(pr_plot) + 8;
        a->y = PLOT_Y(pr_plot) + 8;
        fx_start(0, fx_radiation);
        break;
    case CE_METEORITE:
        /* from 120 cc right and 120 lines up of the plot */
        a->x = PLOT_X(pr_plot) + 120;
        if (a->x > 182)
            a->x = 182;
        a->y = PLOT_Y(pr_plot) - 120;
        fx_start(0, fx_meteor);
        break;
    case CE_FIRE:
    case CE_QUAKE:
        ph_t0 = frame_count;
        break;
    }
    pr_started = 1;
    snd_play(SND_EVENT);
    SND_XA(SNX_EVENT, pr_event);
}

void production_enter(void)
{
    map_dots = 0;
    map_enter();
    pr_event = st.colony.event;
    pr_plot = (uint8_t)st.colony.plot;
    if (pr_plot >= NUM_PLOTS)
        pr_plot = 0;
    pr_started = pr_done = 0;
    fx_flag = 0;
    msg_row = 22;
    msg_col = C_WHITE;
    if (pr_event >= 1 && pr_event <= 9) {
        title_show(T_EVENT + pr_event - 1, 0);
        msg_show(M_EVENT + pr_event - 1, 0, 0);
        pr_event_start();
    } else {
        title_show(T_PRODUCTION, rnd);
        pr_done = 1;
    }
}

void production_tick(void)
{
    uint8_t k;
    msg_tick();
    if (pr_done) {
        /* the production dots (the Atari shows them plot by plot) */
        if (!map_dots) {
            map_dots = 1;
            map_draw_all();
            SND_X(SNX_CHIRPS);
        }
        return;
    }
    if (pr_event == CE_FIRE) {
        /* the store burns for 640 frames: fire flickers over it */
        if ((uint16_t)(frame_count - ph_t0) < 640) {
            if ((uint16_t)(frame_count - pr_last) >= 4) {
                pr_last = frame_count;
                k = plat_rand();
                gfx_pic(PLOT_X(STORE_PLOT) + (k & 1) * 8, PLOT_Y(STORE_PLOT) + (k & 2) * 8,
                        PIC_FIRE0 + ((k >> 2) & 1), (k & 8) ? C_ORANGE : C_YELLOW);
                if ((k & 0x30) == 0)
                    map_draw_plot(STORE_PLOT);
            }
            return;
        }
        pr_done = 1;
    } else if (pr_event == CE_QUAKE) {
        /* the ground shakes: plots jolt (flash outlined) for 300 frames */
        if ((uint16_t)(frame_count - ph_t0) < 300) {
            if ((uint16_t)(frame_count - pr_last) >= 4) {
                pr_last = frame_count;
                if (map_blink < NUM_PLOTS) {
                    k = map_blink;
                    map_blink = NOBODY;
                    map_draw_plot(k);
                }
                map_blink = plat_rand() % NUM_PLOTS;
                map_blink_on = 1;
                map_draw_plot(map_blink);
            }
            return;
        }
        if (map_blink < NUM_PLOTS) {
            k = map_blink;
            map_blink = NOBODY;
            map_draw_plot(k);
        }
        pr_done = 1;
    } else if (fx_flag == 1 || (pr_started && !fx[0].st && !fx[1].st)) {
        pr_done = 1;
    }
    if (pr_done) {
        fx_stop_all();
        title_show(T_PRODUCTION, rnd);
        map_dots = 1;
        map_draw_all();
        SND_X(SNX_CHIRPS);
    }
}
