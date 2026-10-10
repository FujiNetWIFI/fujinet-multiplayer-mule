/*
 * map.c -- the planet: 9 x 5 plots (clients/intv/map.bas), the title and
 * message rows around it, and the time bar.
 *
 *   text row 1       title
 *   lines 16..175    the map (plot p at PLOT_X(p), PLOT_Y(p), 16 x 32)
 *   text rows 22-23  message
 */
#include "mule.h"

uint8_t map_dots;               /* 1: production dots */
uint8_t map_blink = NOBODY;     /* plot drawn outlined in white when ... */
uint8_t map_blink_on;           /* ... this is 1 */

void map_draw_plot(uint8_t p)
{
    static PlotLook l;
    uint8_t t = st.map.terrain[p];
    uint8_t o = OWNER(p);
    uint8_t g = MULEOF(p);
    uint8_t k;

    l.store = (t & 3) == TERRAIN_STORE;
    l.river = (t & 3) == TERRAIN_RIVER;
    l.mountains = (t >> 2) & 3;
    l.owned = 0;
    l.icon = ICON_NONE;
    l.dots = 0;
    l.col = C_WHITE;
    if (!l.store && (o < 4 || (p == map_blink && map_blink_on))) {
        l.owned = 1;
        l.icon = ICON_HOUSE;
        if (o < 4 && g < 4)
            l.icon = ICON_FOOD + g;
        if (p == map_blink && map_blink_on) {
            /* up for auction, or a computer working it: outlined in white */
            l.col = C_WHITE;
            if (o > 3)
                l.icon = ICON_NONE;
        } else {
            l.col = PLCOL(o);
            if (map_dots && g < 4) {
                k = st.map.prod[p];
                if (!(k & 128)) {
                    k &= 15;
                    l.dots = k > 8 ? 8 : k;
                }
            }
        }
    }
    gfx_plot(p, &l);
}

void map_draw_all(void)
{
    uint8_t p;
    for (p = 0; p < NUM_PLOTS; ++p) {
        map_draw_plot(p);
        frame_tick();
    }
}

/* Redraw the plots whose owner, M.U.L.E., terrain or production changed. */
void map_update(void)
{
    uint8_t p;
    for (p = 0; p < NUM_PLOTS; ++p)
        if (st.map.terrain[p] != prev.map.terrain[p] || st.map.owner[p] != prev.map.owner[p] ||
            st.map.mule[p] != prev.map.mule[p] || st.map.prod[p] != prev.map.prod[p])
            map_draw_plot(p);
}

void map_enter(void)
{
    anim = AN_NONE;
    fx_stop_all();
    gfx_map_frame();
    scr_now = SCR_MAP;
    map_blink = NOBODY;
    map_blink_on = 0;
    bar_x = MAP_BAR_X;
    bar_top = 16;
    bar_h = 160;
    bar_last = 255;
    map_draw_all();
}

/* ---- the time bar: one line for each PTU of turn time left ------------- */
uint8_t bar_x, bar_top, bar_h, bar_col, bar_last;

void bar_draw(uint8_t px)
{
    if (px > bar_h)
        px = bar_h;
    if (px == bar_last)
        return;
    if (bar_last == 255 || px > bar_last) {
        gfx_fill(bar_x, bar_top, 4, bar_h - px, C_BLACK);
        if (px)
            gfx_fill(bar_x, bar_top + bar_h - px, 4, px, bar_col);
    } else {
        gfx_fill(bar_x, bar_top + bar_h - bar_last, 4, bar_last - px, C_BLACK);
    }
    bar_last = px;
}

/* The original's tick (its VBI, $5E98): with u units of the bar left (PTU
 * on a turn, ticks in an auction) and u under 30, a tick every u frames,
 * high and low by turns, so it hurries as the bar runs out. Called every
 * frame the bar runs; it counts the frames gone by, so a slow loop keeps
 * the original's time. */
static uint8_t bar_tk = 1, bar_tg, bar_fr;

void bar_tick(uint8_t u)
{
    uint8_t d = (uint8_t)frame_count - bar_fr;     /* frames gone by */
    if (!d)
        return;
    bar_fr += d;
    if (!u || u >= 30) {
        bar_tk = 1;                 /* the first tick comes at 29, high */
        bar_tg = 0;
        return;
    }
    if (bar_tk > d) {
        bar_tk -= d;
        return;
    }
    bar_tk = u;
    snd_play(SND_TICK + bar_tg);
    bar_tg ^= 1;
}
