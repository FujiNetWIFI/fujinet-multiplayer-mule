/*
 * gfx.c -- src/platform-specific/gfx.h on the H19 terminal: the Atari's
 * picture in the terminal's own characters (src/h89/h19.c sends them).
 *
 * No colour: the players are their seat numbers, 1-4, wherever they are
 * (the map's plots, the figures, the 25th line's legend: "1 NAME 2 NAME
 * ..."); your own plots have their borders in reverse video; orange text
 * (the titles) is reversed.
 *
 * Three screens:
 *   MODE_FULL  text rows as they are; x / 2 a column, y / 8 a row (the
 *              auctions, the summaries, the lobby, the boot screens)
 *   MODE_MAP   the map: plots of 8 x 3 cells at rows 1-15, columns 4-75
 *              (a plot is 32 lines: 3 rows), the title on row 0, the
 *              message on rows 22-23
 *   MODE_DEV   the development turn: the map, and the town under it at
 *              rows 16-22 (the H19 takes seconds over a screen: walking
 *              between the two draws neither again); the message on row
 *              23 and the 25th line, the money at the top right
 * The time bar stands at the right edge (column 77, or 76 in an auction).
 *
 *   plot (owned by seat 2, an energy M.U.L.E., two mountains, 3 dots):
 *       +2-E---+        the corners and lines are the H19's graphics
 *       |/\/\  |        characters; ours in reverse; the dots on the
 *       +***---+        bottom edge are its bullets
 */
#include <string.h>
#include "mule.h"
#include "h19.h"
#include "h89.h"

#define MODE_FULL 0
#define MODE_MAP  1
#define MODE_DEV  2

#define MAP_C0    4             /* the map's first column, row */
#define MAP_R0    1
#define MONEY_X   72            /* MAP/DEV: row 0 from here is the money's */
#define TOWN_C0   16
#define TOWN_R0   16

static uint8_t mode;
static uint8_t clear_later;     /* a clear on the development screen, held */
static void clear_now(void);
static uint8_t lines_on[SCR_H]; /* auction price lines on a row */
static uint8_t row25_msg;       /* the 25th line has a message, not the legend */
static uint8_t legend[SCR_W], legend_at;
static uint16_t tm_money = 0xFFFF, tm_price;
static uint8_t tm_mules;

/* ---- rows and columns --------------------------------------------------- */
/* The screen row of text row ty (255: none). */
static uint8_t trow(uint8_t ty)
{
    if (ty > 23)
        return 255;
    if (mode == MODE_FULL)
        return ty;
    if (ty < 2)
        return 0;
    if (mode == MODE_DEV) {
        if (ty == 22)
            return 23;
        if (ty == 23)
            return ROW25;
        return 255;
    }
    return ty >= 16 ? ty : 255;
}

/* The map's row for line y. */
static int8_t map_row(int16_t y)
{
    if (y < 16)
        return MAP_R0 - 1;
    return MAP_R0 + (int8_t)(((uint16_t)(y - 16) * 3) >> 5);
}

/* The town strip's row for a figure whose top is at line y. */
static int8_t town_row(int16_t y)
{
    if (y <= 34) return 16;     /* in an upper building */
    if (y < 58) return 17;      /* its doorway */
    if (y < 62) return 18;      /* the street */
    if (y < 66) return 19;
    if (y < 72) return 20;
    if (y < 90) return 21;      /* a lower building's doorway */
    return 22;
}

/* ---- text ----------------------------------------------------------------- */
static uint8_t attr_of(uint8_t col)
{
    return col == C_ORANGE ? REV : 0;
}

/* n cells from x on screen row r: cut at the money's corner. */
static uint8_t clip(uint8_t x, uint8_t r, uint8_t n)
{
    uint8_t lim = (r == 0 && mode != MODE_FULL) ? MONEY_X : SCR_W;
    if (x >= lim)
        return 0;
    return n > lim - x ? lim - x : n;
}

void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    uint8_t r;
    clear_now();
    r = trow(ty);
    if (r == 255 || !(n = clip(tx, r, n)))
        return;
    h19_str(tx, r, s, n, attr_of(col));
    if (r == ROW25)
        row25_msg = 1;
}

void gfx_text(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_textn(tx, ty, s, TCOLS, col);
}

void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col)
{
    char s[2];
    s[0] = c;
    s[1] = 0;
    gfx_textn(tx, ty, s, 1, col);
}

void gfx_clear_text(uint8_t tx, uint8_t ty, uint8_t w)
{
    uint8_t r;
    clear_now();
    r = trow(ty);
    if (r == 255 || !(w = clip(tx, r, w)))
        return;
    lines_on[r] = 0;
    if (r == ROW25) {
        /* the 25th line: what was under the message */
        if (tx == 0 && w == SCR_W)
            row25_msg = 0;
        h19_putn(tx, ROW25, legend + tx, w);
        return;
    }
    h19_fill(tx, r, w, ' ');
}

void gfx_bign(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
{
    gfx_textn(tx, ty, s, n, col);
}

void gfx_big(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
{
    gfx_textn(tx, ty, s, TCOLS, col);
}

/* ---- the 25th line: who's who ----------------------------------------- */
/* "1 NAME  2 NAME  3 NAME  4 NAME", centred, ours reversed: rebuilt every
 * 16 frames if the names or our seat changed (spr_flush). */
static void legend_build(uint8_t *d)
{
    uint8_t k, n, a, x, c;
    const char *s;
    memset(d, ' ', SCR_W);
    if (!st_ok)
        return;
    n = 0;
    for (k = 0; k < 4; ++k)
        n += 4 + (uint8_t)strlen(PL(k).name);
    x = n < SCR_W ? (SCR_W - n + 2) / 2 : 0;
    for (k = 0; k < 4; ++k) {
        a = k == my_seat ? REV : 0;
        d[x++] = ('1' + k) | a;
        d[x++] = ' ' | a;
        for (s = PL(k).name; *s && x < SCR_W - 2; ++s) {
            c = (uint8_t)*s;
            if (c >= 'a' && c <= 'z')
                c -= 32;
            if (c < 0x20 || c > 0x5D)
                c = '?';
            d[x++] = c | a;
        }
        x += 2;
    }
}

static void gfx_legend(void)
{
    static uint8_t want[SCR_W];
    legend_build(want);
    if (!memcmp(want, legend, SCR_W))
        return;
    memcpy(legend, want, SCR_W);
    if (!row25_msg)
        h19_putn(0, ROW25, legend, SCR_W);
}

static void legend_show(void)
{
    row25_msg = 0;
    h19_putn(0, ROW25, legend, SCR_W);
}

/* ---- screens ------------------------------------------------------------ */
void gfx_init(void)
{
    memset(legend, ' ', SCR_W);
}

static uint8_t strip_on;        /* the town is on the screen */
static uint8_t plots_skip;      /* map_enter's redraw of plots already up */

static void wipe(void)
{
    strip_on = 0;
    spr_hide_all();
    h19_blank(0, 23);
    memset(lines_on, 0, sizeof(lines_on));
    legend_show();
    h19_cleared();
}

/* The development screen's clear is held: walking into town clears the
 * screen and draws the town, and walking out draws the map, both of which
 * are on the screen already (gfx_town, gfx_map_frame call it off). Any
 * other drawing does it first. */
static void clear_now(void)
{
    if (clear_later) {
        clear_later = 0;
        wipe();
        mode = MODE_FULL;
    }
}

void gfx_clear(void)
{
    if (mode == MODE_DEV && phase == PHASE_DEVELOP) {
        spr_hide_all();
        clear_later = 1;
        return;
    }
    clear_later = 0;
    wipe();
    mode = MODE_FULL;
}

static void dev_strip(void);

void gfx_map_frame(void)
{
    if (phase == PHASE_DEVELOP && mode == MODE_DEV) {
        clear_later = 0;            /* the town and map are up already: */
        spr_hide_all();
        dev_strip();
        plots_skip = 1;             /* (map_enter draws them all next) */
        return;
    }
    clear_later = 0;
    wipe();
    mode = MODE_MAP;
    if (phase == PHASE_DEVELOP) {
        mode = MODE_DEV;
        dev_strip();
    }
}

/* ---- the map ------------------------------------------------------------ */
static const char good_letter[4] = { 'F', 'E', 'S', 'C' };

/* The seat whose colour col is (4: none). */
static uint8_t seat_of(uint8_t col)
{
    uint8_t s;
    for (s = 0; s < 4; ++s)
        if (PLCOL(s) == col)
            return s;
    return 4;
}

/* A plot's 24 cells (3 rows of 8), from a template. */
static const char plot_store[] = "faaaaaac`STORE `eac  fad";
static const char plot_owned[] = "faaaaaac`      `eaaaaaad";
static const char plot_white[] = "iiiiiiiii      iiiiiiiii";
static const uint8_t dot_at[8] = { 17, 18, 19, 20, 21, 22, 23, 16 };
static uint8_t cells[24];

void gfx_plot(uint8_t p, const PlotLook *l)
{
    uint8_t c = p, r = MAP_R0, i, m, o, *q;

    if (plots_skip) {
        if (p == NUM_PLOTS - 1)
            plots_skip = 0;
        return;
    }
    clear_now();
    while (c >= 9) {                    /* (a division is a library call) */
        c -= 9;
        r += 3;
    }
    c = MAP_C0 + 8 * c;
    o = OWNER(p);
    memset(cells, ' ', 24);
    if (l->store) {
        memcpy(cells, plot_store, 24);
    } else {
        /* outlined in white: a plot for sale is boxed (blinking), one
         * owned gets a checkered border (a computer working it, a quake) */
        if (l->owned)
            memcpy(cells, l->col == C_WHITE && OWNER(p) < 4 ? plot_white : plot_owned, 24);
        q = cells + 9;
        for (m = l->mountains; m; --m) {
            *q++ = G_SLASH;
            *q++ = G_BSLASH;
        }
        if (l->river) {                 /* (inside the box when owned) */
            for (q = cells + 3, i = 3; i; --i, q += 8)
                q[0] = q[1] = G_CHECK;
            if (l->owned)
                cells[3] = cells[4] = cells[19] = cells[20] = l->col == C_WHITE && o < 4 ? G_CHECK : G_HLINE;
        }
        if (l->owned) {
            if (o < 4)
                cells[1] = '1' + o;
            if (l->icon >= ICON_FOOD && l->icon <= ICON_CRYSTITE)
                cells[3] = good_letter[l->icon - ICON_FOOD];
            for (i = 0; i < l->dots && i < 8; ++i)
                cells[dot_at[i]] = G_BULLET;
            if (o == my_seat && l->col != C_WHITE) {
                /* ours: the border reversed */
                for (q = cells, i = 24; i; --i, ++q)
                    if (i > 16 || i <= 8 || i == 16 || i == 9)
                        *q |= REV;
            }
        }
    }
    for (q = cells, i = 0; i < 3; ++i, ++r, q += 8)
        h19_putn(c, r, q, 8);
}

/* ---- the town ------------------------------------------------------------- */
/* Rows 16-22, columns 16-63: four outfitters along the top, their doors
 * (the gaps) where the Atari's are; the street; the assay office, the
 * land office, the pub and the corral along the bottom. */
/* Run-length: a byte with bit 7 set repeats the next one (count & 127);
 * a row ends at 0. */
static const char town_art[] =
    "fCRYSTITE\202acfSMITHORE\202acfaENERGY\203acf\203aFOOD\203ac\0"
    "e\204ad e\203ade\204ad e\203ade\204ad e\203ade\204ad e\203ad\0"
    "\260 \0"
    "\260 \0"
    "\260 \0"
    "f\204ac f\202acf\203ac f\203acf\202ac f\203acf\202ac\211 fac\0"
    "eASSAY\204adeLAND\205adePUB\205adeCORRAL\210ad\0";

static void dev_art(void)
{
    static uint8_t row[48];
    const char *s = town_art;
    uint8_t *q = row, y = TOWN_R0, n, c;
    while (y < TOWN_R0 + 7) {
        c = (uint8_t)*s++;
        if (!c) {
            h19_putn(TOWN_C0, y++, row, 48);
            q = row;
            continue;
        }
        n = 1;
        if (c & 0x80) {
            n = c & 0x7F;
            c = (uint8_t)*s++;
        }
        while (n--)
            *q++ = c;
    }
}

static void town_prices(void)
{
    char buf[12];
    uint8_t n;
    if (tm_money == 0xFFFF || mode != MODE_DEV)
        return;
    buf[0] = '$';
    n = 1 + fmt_num(buf + 1, tm_money);
    h19_fill(MONEY_X, 0, SCR_W - MONEY_X, ' ');
    h19_str(SCR_W - 1 - n, 0, buf, n, 0);
    /* the corral's price and stock, in its name */
    buf[0] = '$';
    n = 1 + fmt_num(buf + 1, tm_price);
    buf[n++] = ' ';
    n += fmt_num(buf + n, tm_mules);
    h19_fill(TOWN_C0 + 39, TOWN_R0 + 6, 8, G_HLINE);
    h19_str(TOWN_C0 + 40, TOWN_R0 + 6, buf, n > 7 ? 7 : n, 0);
}

static void dev_strip(void)
{
    if (!strip_on)
        dev_art();
    strip_on = 1;
    h19_put(TOWN_C0 - 1, TOWN_R0 + 3, '<');
    h19_put(TOWN_C0 + 49, TOWN_R0 + 3, '>');
    town_prices();
}

void gfx_town(void)
{
    uint8_t up = clear_later;      /* (a held clear called off: all there) */
    clear_later = 0;
    mode = MODE_DEV;
    dev_strip();
    if (!up)
        map_draw_all();
}

void gfx_town_money(uint16_t money, uint16_t price, uint8_t mules)
{
    tm_money = money;
    tm_price = price;
    tm_mules = mules;
    town_prices();
}

/* ---- rectangles and lines --------------------------------------------- */
/* The time bar (src/map.c bar_draw): its fills say where the bar's top is;
 * it is drawn in one column, two steps a row (right half, lower quarter),
 * scaled to its rows. */
static void time_bar(uint8_t level)
{
    uint8_t col, r0, r1, rows, k, i, r, g;
    if (mode == MODE_FULL) {
        col = 76;
        r0 = bar_top >> 3;
        r1 = (bar_top + bar_h - 1) >> 3;
    } else {
        col = 77;
        r0 = MAP_R0;
        r1 = mode == MODE_DEV ? 22 : MAP_R0 + 14;
    }
    rows = r1 - r0 + 1;
    k = (uint8_t)(((uint16_t)level * (rows * 2) + bar_h / 2) / bar_h);
    for (i = 0; i < rows; ++i) {
        r = r1 - i;
        g = ' ';
        if (k >= 2 * i + 2)
            g = G_RIGHT;
        else if (k == 2 * i + 1)
            g = G_Q_LR;
        h19_put(col, r, g);
    }
}

/* Cells' halves: a cell holds a top and a bottom half (8 lines a row);
 * the glyph for each pair. */
static uint8_t halves_of(uint8_t g)
{
    if (g == G_BLOCK) return 3;
    if (g == G_TOP) return 1;
    if (g == (G_TOP | REV)) return 2;
    return 0;
}

static const uint8_t glyph_of_halves[4] = { ' ', G_TOP, G_TOP | REV, G_BLOCK };

void gfx_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
{
    uint8_t c0, c1, c, r, r0, r1, m, hv, y1;
    clear_now();
    if (!w || !h)
        return;
    if (x == bar_x && w == 4) {
        time_bar(col == C_BLACK ? bar_top + bar_h - (y + h) : bar_top + bar_h - y);
        return;
    }
    if (mode != MODE_FULL)
        return;
    c0 = x >> 1;
    c1 = (x + w - 1) >> 1;
    y1 = y + h;                 /* lines y..y1-1 */
    r0 = y >> 3;
    r1 = (y1 - 1) >> 3;
    for (r = r0; r <= r1 && r < 24; ++r) {
        /* the halves it covers (their middles); one assignment: zsdcc
         * -SO3 lost the second of two `m |= n` (tools/z80/scan-lost-store.py) */
        m = (y <= r * 8 + 2 && y1 > r * 8 + 2) + 2 * (y <= r * 8 + 6 && y1 > r * 8 + 6);
        if (!m)
            continue;
        for (c = c0; c <= c1; ++c) {
            hv = halves_of(rowp[r][c]);
            hv = col == C_BLACK ? hv & ~m : hv | m;
            h19_put(c, r, glyph_of_halves[hv]);
        }
    }
}

/* Dotted lines: the auctions' price lines and the "needs" marks, a row of
 * ─ on the row of their line (a row, not the row's top, middle or bottom:
 * a line moved within its row costs the line again, and the H19's line
 * can't keep up with that). Two lines may share a row: a row is counted,
 * and cleared when its last line goes. */
void gfx_hdots(uint8_t x, uint8_t y, uint8_t w, uint8_t col)
{
    static uint8_t tmp[SCR_W];
    uint8_t r = y >> 3, c0 = x >> 1, n = w >> 1, i, *p;
    clear_now();
    if (mode != MODE_FULL || r > 23 || c0 >= SCR_W)
        return;
    if (n > SCR_W - c0)
        n = SCR_W - c0;
    if (col == C_BLACK) {
        if (lines_on[r])
            --lines_on[r];
        if (lines_on[r])
            return;
        p = rowp[r] + c0;
        for (i = 0; i < n; ++i)
            tmp[i] = p[i] == G_HLINE ? ' ' : p[i];
        h19_putn(c0, r, tmp, n);
        return;
    }
    if (w >= 32)
        ++lines_on[r];
    h19_fill(c0, r, n, G_HLINE);
}

/* Small pictures: the store's icon (the auctions), fire on the store. */
void gfx_pic(uint8_t x, uint8_t y, uint8_t pic, uint8_t col)
{
    uint8_t c = x >> 1, r;
    clear_now();
    if (mode == MODE_FULL)
        r = (y + 4) >> 3;
    else
        r = (uint8_t)map_row(y + 4);
    switch (pic) {
    case PIC_STOREICON:
        h19_put(c, r, G_TRI_UL | REV);      /* a roof */
        h19_put(c + 1, r, G_TRI_UR | REV);
        break;
    case PIC_FIRE0:
        h19_put(c, r, col == C_BLACK ? ' ' : G_TRI_UL | REV);
        h19_put(c + 1, r, col == C_BLACK ? ' ' : G_TRI_UR | REV);
        break;
    case PIC_FIRE1:
        h19_put(c, r, col == C_BLACK ? ' ' : G_Q_LR);
        h19_put(c + 1, r, col == C_BLACK ? ' ' : G_Q_LL);
        break;
    }
}

/* ---- sprites ---------------------------------------------------------- */
/* Art: { width, height, cells } (0: see through). */
static uint8_t art_seat[8][3];          /* 1x1: the seat number, [4..7] plain */
static const uint8_t art_mule[] = { 1, 1, 'M' };
static const uint8_t art_wampus[] = { 1, 1, '*' };
static const uint8_t art_cursor[] = { 8, 3,
    G_CHECK, G_CHECK, G_CHECK, G_CHECK, G_CHECK, G_CHECK, G_CHECK, G_CHECK,
    G_CHECK, 0, 0, 0, 0, 0, 0, G_CHECK,
    G_CHECK, G_CHECK, G_CHECK, G_CHECK, G_CHECK, G_CHECK, G_CHECK, G_CHECK };
static const uint8_t art_ship[] = { 7, 2,
    0, G_Q_LR, G_TOP | REV, G_TOP | REV, G_TOP | REV, G_Q_LL, 0,
    G_TRI_UL | REV, G_BLOCK, G_BLOCK, G_BLOCK, G_BLOCK, G_BLOCK, G_TRI_UR | REV };
static const uint8_t art_pirate[] = { 7, 2,
    0, G_Q_LR, G_TOP | REV, G_TOP | REV, G_TOP | REV, G_Q_LL, 0,
    G_TRI_UL | REV, G_BLOCK, G_X | REV, G_X | REV, G_X | REV, G_BLOCK, G_TRI_UR | REV };
static const uint8_t art_dock0[] = { 4, 1, G_TOP | REV, G_BLOCK, G_BLOCK, G_TOP | REV };
static const uint8_t art_dock1[] = { 3, 1, G_Q_LR, G_BLOCK, G_Q_LL };
static const uint8_t art_dock2[] = { 2, 1, G_BLOCK, G_BLOCK };
static const uint8_t art_dock3[] = { 1, 1, G_TOP | REV };
static const uint8_t *const art_dock[4] = { art_dock0, art_dock1, art_dock2, art_dock3 };
static const uint8_t art_pest0[] = { 1, 1, G_X };
static const uint8_t art_pest1[] = { 1, 1, '+' };
static const uint8_t art_cloud[] = { 3, 1, G_TOP | REV, G_BLOCK, G_TOP | REV };
static const uint8_t art_bolt[] = { 1, 1, G_SLASH };
static const uint8_t art_spark[] = { 1, 1, '+' };
static const uint8_t art_meteor[] = { 1, 1, G_BULLET };

static uint8_t slot_seat[SPR_SLOTS];

/* Per shape: its art (0: chosen by frame), and the spot of the Atari's
 * sprite (from its top left, colour clocks and lines) that the art's
 * middle goes over. */
static const uint8_t *const shape_art[SH_COUNT] = {
    0, 0, art_mule, art_wampus, art_cursor, 0, 0, 0, 0, 0,
    art_cloud, art_bolt, art_spark, art_meteor,
};
static const int8_t shape_ax[SH_COUNT] = { 4, 4, 4, 4, 0, 0, 0, 0, 8, 4, 8, 4, 1, 2 };
static const int8_t shape_ay[SH_COUNT] = { 11, 4, 4, 4, 0, 0, 8, 0, 8, 4, 4, 8, 1, 2 };

void spr_set(uint8_t slot, uint8_t shape, uint8_t frame, int16_t x, int16_t y,
             uint8_t col, uint8_t flags)
{
    const uint8_t *a;
    int16_t c, r;
    uint8_t s;
    (void)flags;
    if (slot >= SPR_SLOTS || shape >= SH_COUNT)
        return;
    clear_now();
    a = shape_art[shape];
    switch (shape) {
    case SH_SPECIES:
    case SH_FIGURE:
        s = seat_of(col);
        if (s < 4)
            slot_seat[slot] = s;
        else
            s = 4 + (slot_seat[slot] & 3);  /* (white: the winner's pulse) */
        a = art_seat[s];
        break;
    case SH_HULL_TOP: a = frame ? art_pirate : art_ship; break;
    case SH_DOCK:     a = art_dock[frame & 3]; break;
    case SH_PEST:     a = (frame & 1) ? art_pest1 : art_pest0; break;
    }
    if (!a) {                       /* the ship's nose, its lower hull */
        h19_spr(slot, 0, 0, 0);
        return;
    }

    x += shape_ax[shape];
    c = (x >> 1) - (a[0] >> 1);
    if (shape == SH_CURSOR) {
        c = x >> 1;                         /* (a plot's own corner) */
        r = MAP_R0 + 3 * ((y - 16) >> 5);
    } else if (mode == MODE_DEV && (shape == SH_SPECIES || (shape == SH_MULE && frame < 2))) {
        /* in town (its mule walks 6 lines lower) */
        r = town_row(shape == SH_MULE ? y - 6 : y);
    } else if (mode != MODE_FULL) {
        r = map_row(y + shape_ay[shape]) - (a[1] >> 1);
        if (r < MAP_R0 || r > MAP_R0 + 14)
            a = 0;
    } else {
        if (scr_now == SCR_AUCTION)
            r = (y + 14) >> 3;              /* above its price line */
        else if (scr_now == SCR_FOUNDER && slot == 0)
            r = (y + 12) >> 3;              /* between FIRST and FOUNDER */
        else
            r = (y + 6) >> 3;
        if (r < 0 || r > 23)
            a = 0;
    }
    if (c < -8 || c >= SCR_W)
        a = 0;
    h19_spr(slot, a, (int8_t)c, (int8_t)r);
}

void spr_hide(uint8_t slot)
{
    if (slot < SPR_SLOTS)
        h19_spr(slot, 0, 0, 0);
}

void spr_hide_all(void)
{
    uint8_t i;
    for (i = 0; i < SPR_SLOTS; ++i)
        h19_spr(i, 0, 0, 0);
}

/* What changed, out to the terminal (frame_wait, spr_flush): the legend
 * now and then. */
void gfx_sync(void)
{
    uint8_t s;
    if (!art_seat[0][0])
        for (s = 0; s < 4; ++s) {
            art_seat[s][0] = art_seat[s][1] = art_seat[4 + s][0] = art_seat[4 + s][1] = 1;
            art_seat[s][2] = ('1' + s) | REV;
            art_seat[4 + s][2] = '1' + s;
        }
    if ((uint8_t)(frame_count >> 4) != legend_at) {
        legend_at = (uint8_t)(frame_count >> 4);
        gfx_legend();
    }
    clear_now();
    h19_flush();
}

/* (frame_wait has just sent what changed: the game calls this right after
 * it) */
void spr_flush(void)
{
}
