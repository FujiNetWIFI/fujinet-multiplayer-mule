/*
 * auction.c -- the per-good status screen, declaring, and both auctions
 * (clients/intv/auction.bas, auneed.bas, isr.bas isr_auction).
 *
 * The screen, from the Atari's display list (lines):
 *   big text rows 0-1   title and the good (land: the price line on row 2)
 *   36..144             the arena: the store's sell line at 48, its buy line
 *                       (and the land start price) at 118, 2 lines a step
 *   row 18              BUY / SELL, or each player's price
 *   row 20              money      row 21  units      rows 22-23  message
 * The players stand in columns at colour clocks 39/63/87/111, feet on their
 * price line.
 */
#include <string.h>
#include "mule.h"

#define AU_Y_BASE 118
#define AU_Y_SELL 48
#define AU_Y_TOP  36
#define AU_Y_BOT  144
#define STEPS_UP   ((AU_Y_BASE - AU_Y_TOP) / 2)     /* 41 */
#define STEPS_DOWN ((AU_Y_BOT - AU_Y_BASE) / 2)     /* 13 */
#define LINE_X    16            /* the dotted lines, clear of the store's icons */
#define LINE_W    120
#define ROW_LBL   18
#define ROW_MONEY 20
#define ROW_UNITS 21
/* Each player's field under them: ten 40-column cells, or the cells under
 * the same stretch of the picture (TX). */
#if TCOLS == 40
#define AU_FX(i)  ((i) * 10)
#define AU_FW(i)  10
#else
static const uint8_t au_fx[5] = { TX(0), TX(10), TX(20), TX(30), TX(40) };
#define AU_FX(i)  au_fx[i]
#define AU_FW(i)  (au_fx[(i) + 1] - au_fx[i])
#endif

static const uint8_t col_x[4] = { 39, 63, 87, 111 };

static uint8_t au_kind, au_good, au_stage, au_new, au_stepv;
static uint16_t au_lim;         /* land: 30 steps, how far a bid runs over the base */
static uint8_t au_role[4];
static uint16_t au_pos[4], au_tgt[4], au_base;
static uint8_t au_on, au_tick;
static uint8_t au_ph;           /* steps taken: the walk frames and blinking */
static uint8_t au_money_all;    /* the money fields were cleared: all again */
static uint8_t au_push;         /* our stick as sent: 0 released, 1 up, 2 down */
static uint8_t au_held;         /* frame (low byte) our line was let go */
static uint8_t au_ln1, au_ln2;  /* lines of the sellers' / buyers' price lines (255 none) */
static uint16_t au_lbl[4];      /* what row 18 shows per player: 0 label, 1 none, else a price */
static uint16_t au_hb, au_sb;   /* land: price line / "bids start at" shown */
static uint8_t au_msg;          /* message on the message rows */
static uint8_t au_blink;        /* seats whose sprite blinks (bit per seat) */
static uint8_t au_show_t;       /* per-good status: step shown */
static uint8_t au_bar[4], au_bar_t[4];

/* ---------------------------------------------------------------------------
 * Prices and lines.
 * ------------------------------------------------------------------------- */
#if defined(__ATARI__) || defined(__LYNX__)
/* What runs every frame stays out of the overlay here: its window is full
 * and main RAM isn't. */
#pragma code-name (push, "CODE")
#endif
/* A player, and their colour: calls, not the 29-byte stride inline at
 * every use. */
static MulePlayer *au_pl(uint8_t i)
{
    return &st.players[i];
}

static uint8_t au_col(uint8_t i)
{
    return pl_col[au_pl(i)->color & 3];
}

static uint8_t price_y(uint16_t w)
{
    uint16_t d;
    if (w >= au_base) {
        d = w - au_base;
        if (au_stepv == 4)
            d >>= 2;
        if (d > STEPS_UP)
            d = STEPS_UP;
        return AU_Y_BASE - (uint8_t)d * 2;
    }
    d = au_base - w;
    if (au_stepv == 4)
        d >>= 2;
    if (d > STEPS_DOWN)
        d = STEPS_DOWN;
    return AU_Y_BASE + (uint8_t)d * 2;
}

/* Place the players (at each step of the lines, and after a state). The
 * walk frames and the blinking go by the steps too, so a player is drawn
 * once a step, with the price lines (a slow machine drew them twice). */
static void au_draw(void)
{
    uint8_t i, y, f;
    for (i = 0; i < 4; ++i) {
        frame_tick();           /* (the Apple's clock: this takes a while) */
        if (au_role[i] == ROLE_NONE) {
            spr_hide(i);
            continue;
        }
        if (au_stage < STAGE_LEAD)
            y = au_role[i] == ROLE_SELLER ? AU_Y_SELL : AU_Y_BOT;
        else
            y = price_y(au_pos[i]);
        /* dealing: two steps on, two off */
        if ((au_blink & (1 << i)) && (au_ph & 2)) {
            spr_hide(i);
            continue;
        }
        f = (au_pl(i)->species & 7) * 11 + 5;
        if (au_pos[i] != au_tgt[i] && (au_ph & 1))
            ++f;
        spr_set(i, SH_SPECIES, f, col_x[i], (int16_t)y - 22, au_col(i), 0);
    }
}

/* One price step of a line towards its target. */
static void au_step(uint8_t i)
{
    uint16_t p = au_pos[i], t = au_tgt[i];
    if (p < t) {
        p += au_stepv;
        if (p > t)
            p = t;
    } else if (p > t) {
        p = p > t + au_stepv ? p - au_stepv : t;
    }
    au_pos[i] = p;
    /* land: a bid past 30 steps over the base scrolls the scale up, the
     * high bidder staying at the top and the rest sliding down */
    if (au_kind == 1 && au_role[i] == ROLE_BUYER && p > au_base + au_lim)
        au_base = p - au_lim;
}

void auction_isr(void)
{
    uint8_t i;
    if (!au_on)
        return;
    if (++au_tick < 8)
        return;
    au_tick = 0;
    ++au_ph;
    for (i = 0; i < 4; ++i)
        au_step(i);
    au_draw();
}

#if defined(__ATARI__) || defined(__LYNX__)
#pragma code-name (pop)
#endif

/* The dotted price lines: the sellers' (lowest ask, or the store's price
 * while it has stock) and the buyers' (highest bid, or the store's buying
 * price / the bottom of the land scale). */
static void au_lines(void)
{
    uint8_t i, j = 255, y, k;
    uint16_t w;
    if (au_stage == STAGE_SHOW || (au_kind == 1 && au_stage < STAGE_LEAD))
        return;
    w = 0xFFFF;
    if (au_kind == 2 && st.store.stock[au_good] > 0)
        w = st.auction.storeSell;
    for (i = 0; i < 4; ++i)
        if (au_role[i] == ROLE_SELLER && au_stage >= STAGE_LEAD && au_pos[i] < w)
            w = au_pos[i];
    if (w != 0xFFFF)
        j = price_y(w);
    w = au_kind == 2 ? st.auction.storeBuy : au_base;
    for (i = 0; i < 4; ++i)
        if (au_role[i] == ROLE_BUYER && au_stage >= STAGE_LEAD && au_pos[i] > w)
            w = au_pos[i];
    y = price_y(w);
    /* never over the labels */
    if (j != 255 && j > 126)
        j = 255;
    if (y > 126)
        y = 255;
    if (j == au_ln1 && y == au_ln2)
        return;
    /* the new lines first (a line that stays is not drawn again), then
     * the old ones off: where a slow machine takes frames over the four,
     * a line lingers rather than going missing */
    i = au_ln1;
    k = au_ln2;
    au_ln1 = j;
    au_ln2 = y;
    if (j != 255 && j != i && j != k)
        gfx_hdots(LINE_X, j, LINE_W, C_WHITE);
    if (y != 255 && y != i && y != k)
        gfx_hdots(LINE_X, y, LINE_W, C_WHITE);
    if (i != 255 && i != j && i != y)
        gfx_hdots(LINE_X, i, LINE_W, C_BLACK);
    if (k != 255 && k != j && k != y)
        gfx_hdots(LINE_X, k, LINE_W, C_BLACK);
}

/* Seat i's field on row 18 cleared; its first cell. */
static uint8_t au_field(uint8_t i)
{
    uint8_t x = AU_FX(i);
    gfx_clear_text(x, ROW_LBL, AU_FW(i));
    return x;
}

/* Row 18, over each player's money: BUY / SELL while the player stands at
 * its starting line, else the price it has moved to (as on the Atari).
 * One a frame: four at once, as the lines step together, held a slow
 * machine's loop for several frames. They take turns (au_lbl_i), or on a
 * machine that goes round fewer times than there are labels between two
 * steps the last player's would never come. */
static uint8_t au_lbl_i;

static void au_lbl_tick(void)
{
    uint8_t i, n, x;
    uint16_t w;
    for (n = 4; n; --n) {
        i = au_lbl_i = (au_lbl_i + 1) & 3;
        if (au_role[i] == ROLE_NONE) {
            w = 1;
        } else if (au_stage < STAGE_LEAD) {
            w = 0;
        } else {
            w = au_pos[i];
            if (au_role[i] == ROLE_BUYER && w <= st.auction.lo)
                w = 0;
            if (au_role[i] == ROLE_SELLER && w >= st.auction.hi)
                w = 0;
            if (w == 1)
                w = 2;
        }
        if (w == au_lbl[i])
            continue;
        au_lbl[i] = w;
        x = au_field(i);
        if (w == 0)
            gfx_big(x + BIGW, ROW_LBL, str_word(au_role[i] == ROLE_SELLER ? W_SELL : W_BUY), au_col(i));
        else if (w > 1)
            big_num(x, ROW_LBL, w, 5, au_col(i));
        break;
    }
}

static void au_labels(void)
{
    uint8_t i;
    for (i = 0; i < 4; ++i)
        au_lbl[i] = 0xFFFF;
    au_lbl_tick();
    if (au_kind == 2) {
        /* the store's icons: selling above the cyan one, buying under the brown */
        gfx_pic(136, 32, PIC_STOREICON, C_CYAN);
        gfx_pic(4, 112, PIC_STOREICON, C_BROWN);
        put_num(TX(32), 3, st.auction.storeSell, 4, C_WHITE);
        put_num(0, 16, st.auction.storeBuy, 4, C_WHITE);
    }
}

/* Money and (goods) units, in ten-cell fields under the players: again
 * only when a player's changed (a trade), not for every state that comes
 * (up to twenty a second; the big digits are slow to draw). */
static void au_money(void)
{
    uint8_t i;
    if (!au_money_all && !memcmp(st.players, prev.players, sizeof(st.players)))
        return;
    au_money_all = 0;
    for (i = 0; i < 4; ++i) {
        big_num(AU_FX(i), ROW_MONEY, au_pl(i)->money, 5, au_col(i));
        if (au_kind == 2 && au_stage >= STAGE_LEAD)
            put_num(AU_FX(i) + 4, ROW_UNITS, au_pl(i)->goods[au_good], 4, C_WHITE);
    }
}

/* The auction clock: a bar, a line for every tick left. Between states it
 * runs on by itself: the tick under way has tickLeft jiffies left, and
 * each one after lasts 25, or 6 when idle (and while declaring). */
static uint8_t au_ticks_now(void)
{
    uint16_t el = since_rx();
    uint8_t per = 25;
    if (au_stage == STAGE_DECLARE || (st.auction.flags & AF_IDLE))
        per = 6;
    /* as if the tick under way were whole: el + per - tickLeft */
    el += per;
    el = el > st.auction.tickLeft ? (el - st.auction.tickLeft) / per : 0;
    return el < st.auction.ticks ? st.auction.ticks - (uint8_t)el : 0;
}

static void au_timebar(void)
{
    uint8_t u = 0;
    bar_col = C_WHITE;
    if (au_stage == STAGE_ACTIVE || au_stage == STAGE_DECLARE)
        u = au_ticks_now();
    bar_tick(u);
    bar_draw(u);
}

static void au_message(void)
{
    uint8_t k = 255;
    msg_row = 22;
    msg_col = C_WHITE;
    if (au_stage == STAGE_DECLARE && my_seat < 4) k = M_DECLARE;
    if (au_stage == STAGE_DONE) k = M_AUCOVER;
    if (au_kind == 1 && au_stage == STAGE_DONE)
        k = (uint8_t)st.auction.winner < 4 ? M_SOLD : M_NOBIDS;
    if (au_kind == 2 && au_stage == STAGE_ACTIVE && (st.auction.flags & AF_COLLUDING))
        k = M_COLLUDE;
    if (au_kind == 2 && au_stage == STAGE_ACTIVE && (st.auction.flags & AF_CRITICAL))
        k = M_CRITICAL;
    /* land: the bottom of the (scrolling) scale, as on the Atari */
    if (au_kind == 1 && au_stage == STAGE_ACTIVE) {
        if (k != au_msg || au_base != au_sb) {
            au_msg = k;
            au_sb = au_base;
            msg_show(M_BIDSTART, au_base, 0);
        }
        return;
    }
    if (k == au_msg)
        return;
    au_msg = k;
    if (k == 255)
        msg_clear();
    else
        msg_show(k, 0, 0);
}

/* The land auction's price line (row 2): the opening price, then the high
 * bid, then what it sold for. */
static void au_land_line(void)
{
    uint8_t i, id = M_BIDSTART;
    uint16_t v = st.auction.start;
    char buf[24];
    if (au_kind != 1)
        return;
    for (i = 0; i < 4; ++i)
        if (au_role[i] == ROLE_BUYER && au_pos[i] >= v) {
            v = au_pos[i];
            id = M_HIGHBID;
        }
    if (au_stage == STAGE_DONE && st.auction.price) {
        v = st.auction.price;
        id = M_SALEPRICE;
    }
    if (v == au_hb)
        return;
    au_hb = v;
    text_expand(buf, str_msg(id), v, 0);
    text_center(2, buf, C_WHITE);
}

/* Who is dealing blinks: in a land auction the high bidder (the winner once
 * sold); in a goods auction the buyer and seller whose lines meet, or
 * whoever meets the store's line. */
static void au_blinkers(void)
{
    uint8_t i, j = 255, k = 255;
    uint16_t w, lo;
    au_blink = 0;
    if (au_stage < STAGE_ACTIVE)
        return;
    if (au_kind == 1) {
        if (au_stage == STAGE_DONE) {
            j = (uint8_t)st.auction.winner;
            if (j < 4)
                au_blink = 1 << j;
            return;
        }
        w = st.auction.start;
        for (i = 0; i < 4; ++i)
            if (au_role[i] == ROLE_BUYER && au_pos[i] >= w) {
                w = au_pos[i];
                j = i;
            }
        if (j < 4)
            au_blink = 1 << j;
        return;
    }
    if (au_stage != STAGE_ACTIVE)
        return;
    w = 0;
    lo = 0xFFFF;
    for (i = 0; i < 4; ++i) {
        if (au_role[i] == ROLE_BUYER && au_pos[i] >= w) {
            w = au_pos[i];
            j = i;
        }
        if (au_role[i] == ROLE_SELLER && au_pos[i] <= lo) {
            lo = au_pos[i];
            k = i;
        }
    }
    if (j < 4 && k < 4 && w >= lo) {
        au_blink = (1 << j) | (1 << k);
        return;
    }
    if (j < 4 && w >= st.auction.storeSell)
        au_blink = 1 << j;
    if (k < 4 && lo <= st.auction.storeBuy)
        au_blink |= 1 << k;
}

/* ---------------------------------------------------------------------------
 * The per-good status (stage 0, 561 jiffies): previous amount, usage,
 * spoilage, production, then what the store holds; each player's bar
 * stands over them.
 * ------------------------------------------------------------------------- */

/* The units of au_good seat i needs in round r: food by round (none after
 * 12), energy one for each of its other M.U.L.E.s. */
static uint8_t au_need(uint8_t i, uint8_t r)
{
    uint8_t n = 0, mp, g;
    if (au_good == GOOD_FOOD) {
        if (r <= 12) n = 3;
        if (r >= 4 && r <= 12) n = 4;
        if (r >= 8 && r <= 12) n = 5;
    } else if (au_good == GOOD_ENERGY) {
        for (mp = 0; mp < NUM_PLOTS; ++mp)
            if (OWNER(mp) == i) {
                g = MULEOF(mp);
                if (g < 4 && g != GOOD_ENERGY)
                    ++n;
            }
    }
    return n;
}

#define BAR_BOT 112             /* bars stand clear of the players' heads */
#define BAR_MAX 80              /* 4 lines a unit, at most 20 units */

/* Bar i, which was `was` lines high, now au_bar[i]: only the lines between
 * change (the stage began with the area clear). */
static void au_bar_draw(uint8_t i, uint8_t was)
{
    uint8_t x = col_x[i] + 1, n = au_bar[i];
    if (n > was)
        gfx_fill(x, BAR_BOT - n, 8, n - was, au_col(i));
    else if (n < was)
        gfx_fill(x, BAR_BOT - was, 8, was - n, C_BLACK);
}

static void au_status_targets(void)
{
    uint8_t i, c, j, n;
    uint16_t w, a;
    MulePlayer *p;
    for (i = 0; i < 4; ++i) {
        p = au_pl(i);
        w = p->goods[au_good];
        c = p->prod[au_good];
        j = p->spoil[au_good];
        n = au_need(i, rnd);
        /* previous = now - production + spoilage + usage */
        a = w + j + n;
        a = a >= c ? a - c : 0;
        if (au_show_t == 1)
            a = a >= n ? a - n : 0;
        if (au_show_t == 2)
            a = w >= c ? w - c : 0;
        if (au_show_t >= 3)
            a = w;
        a *= 4;
        if (a > BAR_MAX)
            a = BAR_MAX;
        au_bar_t[i] = (uint8_t)a;
    }
}

/* Once the store's stock is shown: what each player needs next round, a
 * dotted line at that height by the bar and, under the player, how far
 * short (-) or over (+). */
static void au_need_draw(void)
{
    uint8_t i, n, y;
    uint16_t w, v;
    char buf[6];
    for (i = 0; i < 4; ++i) {
        n = au_need(i, rnd + 1);
        if (!n)
            continue;
        w = au_pl(i)->goods[au_good];
        y = n > 20 ? BAR_BOT - BAR_MAX : BAR_BOT - n * 4;
        gfx_hdots(col_x[i] - 8, y, 8, C_WHITE);
        gfx_hdots(col_x[i] + 12, y, 8, C_WHITE);
        if (w >= n) {
            v = w - n;
            buf[0] = '+';
        } else {
            v = n - w;
            buf[0] = '-';
        }
        fmt_num(buf + 1, v);
        gfx_big(au_field(i) + BIGW, ROW_LBL, buf, w >= n ? C_WHITE : C_ORANGE);
    }
}

static void au_status_setup(void)
{
    uint8_t i;
    /* the players standing at the bottom */
    for (i = 0; i < 4; ++i)
        au_role[i] = ROLE_BUYER;
    au_stage = STAGE_SHOW;
    au_blink = 0;
    au_draw();
    for (i = 0; i < 4; ++i) {
        au_role[i] = st.auction.role[i];
        au_bar[i] = au_bar_t[i] = 0;
    }
    au_show_t = 255;
}

static void au_status_tick(void)
{
    uint16_t w = pe_now();
    uint8_t k, i;
    if (w < 127) k = 0;
    else if (w < 250) k = 1;
    else if (w < 373) k = 2;
    else if (w < 496) k = 3;
    else k = 4;
    if (k != au_show_t) {
        au_show_t = k;
        au_status_targets();
        msg_row = 22;
        msg_col = C_WHITE;
        if (k < 4) {
            msg_show(M_PREVIOUS + k, 0, 0);
        } else {
            w = st.store.stock[au_good];
            msg_show(w ? M_STOREHAS : M_STORENO, w, 0);
            au_need_draw();
        }
    }
    /* bars move a line every frame (the Atari's pixel every other) */
    for (i = 0; i < 4; ++i) {
        k = au_bar[i];
        if (k < au_bar_t[i]) {
            au_bar[i] += 2;
            if (au_bar[i] > au_bar_t[i])
                au_bar[i] = au_bar_t[i];
            au_bar_draw(i, k);
        } else if (k > au_bar_t[i]) {
            au_bar[i] = k >= au_bar_t[i] + 2 ? k - 2 : au_bar_t[i];
            au_bar_draw(i, k);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Screen setup.
 * ------------------------------------------------------------------------- */
static void auction_enter(void)
{
    anim = AN_NONE;
    au_on = 0;
    fx_stop_all();
    gfx_clear();
    scr_now = SCR_AUCTION;
    au_ln1 = au_ln2 = 255;
    au_show_t = 255;
    au_msg = 255;
    au_hb = 0xFFFF;
    bar_x = 152;
    bar_top = 32;
    bar_h = 112;
    bar_last = 255;
}

/* "AUCTION #n" / "STATUS #n" over the good, or the land title. */
static void auction_title(void)
{
    char buf[24];
    uint8_t n;
    gfx_clear_text(0, 0, TCOLS);
    gfx_clear_text(0, 1, TCOLS);
    if (au_kind == 1) {
        text_expand(buf, str_title(T_LANDAUCTION), rnd, 0);
        big_center(0, buf, C_ORANGE);
        return;
    }
    strcpy(buf, str_word(au_stage <= STAGE_DECLARE ? W_STATUS : W_AUCTION));
    n = strlen(buf);
    fmt_num(buf + n, rnd);
    big_center(0, buf, C_ORANGE);
    big_center(1, str_good(au_good), C_BLUE);
}

static void au_stage_begin(void)
{
    uint8_t i;
    for (i = 2; i < 24; ++i)
        gfx_clear_text(0, i, TCOLS);
    au_ln1 = au_ln2 = 255;
    au_msg = 255;
    bar_last = 255;
    au_money_all = 1;
    au_sb = 0xFFFF;
    au_hb = 0xFFFF;
    /* a new plot or good: the server has no push from us yet */
    if (au_stage == STAGE_SHOW)
        au_push = 0;
    if (au_kind == 2 && au_stage == STAGE_SHOW) {
        au_on = 0;
        anim = AN_NONE;
        au_status_setup();
        return;
    }
    /* declaring, and the lines about to go: your hand to the stick */
    if ((au_stage == STAGE_DECLARE || au_stage == STAGE_LEAD) && my_seat < 4)
        snd_play(SND_ALERT);
    if (au_stage == STAGE_DONE)
        snd_play(SND_AUCTION_OVER);
    au_labels();
}

void landauc_enter(void)
{
    au_stage = 255;
    au_push = 0;
    au_traded = 0;
    map_dots = 0;
}

void goods_enter(void)
{
    /* declare and trading share the screen; a new good starts it afresh */
    if (last_phase != PHASE_DECLARE && last_phase != PHASE_TRADING)
        scr_now = SCR_NONE;
    if (phase == PHASE_DECLARE)
        scr_now = SCR_NONE;
    au_stage = 255;
    au_push = 0;
    au_traded = 0;
}

/* ---------------------------------------------------------------------------
 * A new state in phases 2, 5 or 6.
 * ------------------------------------------------------------------------- */
void auction_state(void)
{
    uint8_t i, k, j, c, n;
    au_kind = st.auction.kind;
    au_new = st.auction.stage;
    au_good = (uint8_t)st.auction.good & 3;
    au_stepv = st.auction.step;
    if (au_stepv == 0)
        au_stepv = 1;
    au_lim = au_stepv * 30;
    for (i = 0; i < 4; ++i)
        au_role[i] = st.auction.role[i];

    /* the land auction starts with the plot blinking on the map */
    if (au_kind == 1 && au_new == STAGE_SHOW) {
        if (scr_now != SCR_MAP || au_stage != STAGE_SHOW) {
            if (scr_now != SCR_MAP)
                map_enter();
            title_show(T_LANDAUCTION, rnd);
            msg_row = 22;
            msg_col = C_WHITE;
            msg_show(M_BIDSTART, st.auction.start, 0);
        }
        au_stage = au_new;
        map_blink = (uint8_t)st.auction.plot;
        return;
    }
    if (map_blink < NUM_PLOTS && scr_now == SCR_MAP) {
        i = map_blink;
        map_blink = NOBODY;
        map_draw_plot(i);
    }
    map_blink = NOBODY;

    if (scr_now != SCR_AUCTION || au_new != au_stage) {
        if (scr_now != SCR_AUCTION)
            auction_enter();
        au_stage = au_new;
        auction_title();
        au_stage_begin();
    }

    /* where the lines are drawn from: in a land auction the scale scrolls up
     * with the bidding (auction_isr), so the base only resets for a new plot */
    if (au_kind == 1) {
        if (au_stage < STAGE_ACTIVE || au_base < st.auction.start)
            au_base = st.auction.start;
    } else {
        au_base = st.auction.storeBuy;
    }
    /* our released line: the server's is behind what we showed and
     * carries on to where we let go (ACT_HOLD), so ours stays put rather
     * than jumping back until the server's comes to rest (or a second) */
    k = 255;
    if (au_stage == STAGE_ACTIVE && au_push == 0 &&
        (uint8_t)((uint8_t)frame_count - au_held) < 60)
        k = my_seat;
    /* The server steps the lines on every 8th jiffy of its clock: ours
     * step in time with it, and the state (rx_age jiffies old) is moved on
     * by the steps it missed, so a line doesn't jump back and walk on. */
    j = ((uint8_t)st.h.t & 7) + rx_age;
    au_tick = j & 7;
    j = au_stage == STAGE_ACTIVE ? j >> 3 : 0;
    c = 255;
    if (au_kind == 2 && (st.auction.flags & AF_CRITICAL))
        c = st.auction.seller;             /* its line is held */
    for (i = 0; i < 4; ++i) {
        if (i == k && (st.auction.push[i] || st.auction.pos[i] != st.auction.target[i]))
            continue;
        au_pos[i] = st.auction.pos[i];     /* snap; the frame handler walks it on */
        au_tgt[i] = i == c ? au_pos[i] : st.auction.target[i];
        for (n = j; n; --n)
            au_step(i);
    }
    if (au_stage >= STAGE_LEAD) {
        au_on = 1;
        anim = AN_AUCTION;
    }
    if (au_kind == 2 && au_stage == STAGE_SHOW)
        return;
    au_money();
    au_lines();
    au_timebar();
    au_message();
    au_land_line();
    au_blinkers();
    au_draw();
    if (st.auction.traded != au_traded) {
        if (st.auction.traded > au_traded)
            snd_play(SND_TRADE);
        au_traded = st.auction.traded;
    }
}

/* Tell the server when our stick changes (up, down, released). It is sent
 * before the lines are released too; the server holds it, so we start
 * moving with the computers. The push carries au_traded, so the server can
 * give back units that traded after what we have shown when we back off. */
static void au_stick(void)
{
    uint8_t k = 0;
    uint16_t w;
    if (inp_dir & DIR_UP) k = 1;
    if (inp_dir & DIR_DOWN) k = 2;
    if (k == au_push || act != ACT_NONE)
        return;
    au_push = k;
    if (k == 0 && au_stage == STAGE_ACTIVE) {
        act_send(ACT_HOLD, au_pos[my_seat]);
        au_held = (uint8_t)frame_count;
    } else
        act_send(ACT_PUSH, k);
    if (au_stage != STAGE_ACTIVE)
        return;
    /* move our own line at once; the server confirms */
    if (k == 1) {
        if (au_kind == 1) {
            /* land: as high as our money goes (the scale scrolls up) */
            w = au_pl(my_seat)->money;
            au_tgt[my_seat] = w - (w & 3);
        } else {
            au_tgt[my_seat] = st.auction.hi;
        }
    }
    /* down; let go, a line (a land bid too) stays where it is */
    if (k == 2)
        au_tgt[my_seat] = st.auction.lo;
    else if (k == 0)
        au_tgt[my_seat] = au_pos[my_seat];
}

void auction_tick(void)
{
    uint8_t i;
    if (au_kind == 1 && au_stage == STAGE_SHOW) {
        /* the plot for sale blinks on the map (outlined, 16 frames on and 16
         * off); the button skips the wait */
        if (scr_now == SCR_MAP && map_blink < NUM_PLOTS &&
            ((frame_count >> 4) & 1) != map_blink_on) {
            map_blink_on = (frame_count >> 4) & 1;
            map_draw_plot(map_blink);
        }
        msg_tick();
        if (my_seat < 4) {
            if (ev_btn)
                act_send(ACT_READY, 0);
            au_stick();
        }
        return;
    }
    if (scr_now != SCR_AUCTION)
        return;
    msg_tick();
    if (au_kind == 2 && au_stage == STAGE_SHOW) {
        au_status_tick();
        return;
    }
    au_lines();
    au_lbl_tick();
    au_timebar();
    if (au_stage == STAGE_DECLARE && my_seat < 4) {
        /* push the stick to declare: up = seller, down = buyer */
        i = au_role[my_seat];
        if ((inp_dir & DIR_UP) && i != ROLE_SELLER) {
            au_role[my_seat] = ROLE_SELLER;
            act_send(ACT_DECLARE, 2);
            au_labels();
            snd_play(SND_SELECT);
        } else if ((inp_dir & DIR_DOWN) && i != ROLE_BUYER) {
            au_role[my_seat] = ROLE_BUYER;
            act_send(ACT_DECLARE, 1);
            au_labels();
            snd_play(SND_SELECT);
        }
        if (ev_key == KEY_SITOUT &&
            (i != ROLE_NONE || !(au_pl(my_seat)->flags & PF_DECLARED))) {
            au_role[my_seat] = ROLE_NONE;
            act_send(ACT_DECLARE, 0);
            au_labels();
        }
        au_draw();
        return;
    }
    if ((au_stage == STAGE_ACTIVE || au_stage == STAGE_LEAD) && my_seat < 4) {
        au_stick();
        if (ev_btn && au_kind == 2 && au_stage == STAGE_ACTIVE)
            act_send(ACT_COLLUDE, 0);
    }
}
