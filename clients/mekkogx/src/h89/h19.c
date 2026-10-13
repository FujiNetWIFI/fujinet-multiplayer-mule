/*
 * h19.c -- the H19's screen: what the game draws (bg), the sprites over
 * it, and a dirty bit for each cell the terminal may not show yet.
 * h19_flush() sends the dirty cells, row by row, with as few escape
 * sequences as it can:
 *
 *   - graphics mode stays on (ESC F once): text is upper case, which
 *     graphics mode prints as text, so the only attribute ever switched is
 *     reverse video (ESC p / ESC q);
 *   - the cursor moves the cheapest way: across up to three cells by
 *     sending them again, back by BS, down a row by LF, else ESC Y r c;
 *   - the rest of a row gone blank is ESC K;
 *   - a new screen with much to wipe is ESC E first.
 *
 * A cell is dirty when the game changes it (to something else), or a
 * sprite comes onto it or leaves it. At 9600 baud the line takes a byte a
 * millisecond, about 16 a frame. A flush puts no more than FILL bytes in
 * the console's ring (h89io.asm), so whatever is queued is on the screen
 * within a few frames, and rows the sprites cross go first: the player's
 * figure answers the keys at once even while a whole screen is still being
 * drawn behind it. A row not finished stays dirty for the next flush.
 *
 * Dirty bits rather than a copy of the terminal's screen: 250 bytes, not
 * 2000, which a 62K CP/M (the one FNDISK needs) doesn't have. The price is
 * a cell changed and changed back before it is sent goes again.
 *
 * Adapted from fujinet-config's src/h89/h19.c (same author).
 */
#include <string.h>
#include "h19.h"
#include "h89.h"
#include "gfx.h"

#define ESC     0x1B
#define UNKNOWN 0xFF
#define FILL    56              /* bytes in the ring after a flush, at most */
#define OPMAX   7               /* one cell's worst: ESC Y r c, ESC p, c */
#define STALE_FOR_ERASE 4
#define ROWB    (SCR_W / 8)     /* dirty bytes a row */
#define WIPE_FOR_CLEAR 64       /* cells to blank that make ESC E worth it */

uint8_t *bg;
uint8_t *rowp[SCR_H];
#ifdef H89_COUNT
uint16_t h89_n[6];              /* calls: put, fill, fill cells, sweep, str, putn */
#define COUNT(i, n) (h89_n[i] += (n))
#else
#define COUNT(i, n)
#endif
uint8_t h89_idle;
uint16_t h89_flushes;

uint8_t dirty[SCR_H][ROWB];     /* (h19cell.asm writes these too) */
uint8_t rowdirty[SCR_H], any_dirty;     /* (a flush with nothing to do is quick) */
static uint8_t hot[SCR_H], any_hot;
static uint8_t line[SCR_W];
static uint8_t cur_x, cur_y, cur_rev;
static uint8_t new_screen, active, rr;
static int8_t budget;
static const uint8_t bit_of[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

/* sprites: wanted, and as they were last laid over the rows */
static const uint8_t *s_art[SPR_SLOTS], *s_sent[SPR_SLOTS];
static int8_t s_c[SPR_SLOTS], s_r[SPR_SLOTS], s_sc[SPR_SLOTS], s_sr[SPR_SLOTS];
static uint8_t s_dirty;

#define DIRTY(x, y)  (dirty[y][(x) >> 3] & bit_of[(x) & 7])
#define SETD(x, y)   (dirty[y][(x) >> 3] |= bit_of[(x) & 7])
#define CLRD(x, y)   (dirty[y][(x) >> 3] &= ~bit_of[(x) & 7])

static void out(uint8_t c)
{
    con_put(c);
    --budget;
}

static void esc(uint8_t c)
{
    con_put(ESC);
    con_put(c);
    budget -= 2;
}

static void setrev(uint8_t r)
{
    if (r != cur_rev) {
        cur_rev = r;
        esc(r ? 'p' : 'q');
    }
}

/* Sent as they are. */
static void send_all(const char *s)
{
    while (*s)
        con_put((uint8_t)*s++);
}

/* The modes: the power-up ones (ESC z: no hold screen...), no reverse,
 * cursor off, no key click, no LF after CR nor CR after LF, discard at the
 * end of a line (never wrap), keypad shifted (its arrows are ESC A-D),
 * the 25th line on; lines 1-24 cleared, and the 25th; graphics mode, for
 * good. */
static const char init_seq[] =
    "\033z\033q\033x5\033x2\033y8\033y9\033w\033t\033x1\033E\033Y8 \033l\033F";
/* Graphics off, the 25th line off (which erases it), the DIP switches'
 * modes and a clear screen. */
static const char exit_seq[] = "\033G\033q\033y1\033z";

void h19_init(uint8_t *cells)
{
    uint8_t y;
    bg = cells;
    for (y = 0; y < SCR_H; ++y)
        rowp[y] = cells + (uint16_t)y * SCR_W;
    memset(bg, ' ', SCR_W * SCR_H);
    memset(dirty, 0, sizeof(dirty));
    memset(rowdirty, 0, sizeof(rowdirty));
    send_all(init_seq);
    cur_x = cur_y = UNKNOWN;
    cur_rev = 0;
    active = 1;
}

void h19_exit(void)
{
    uint16_t t;
    if (!active)
        return;
    active = 0;
    send_all(exit_seq);
    while (!con_flushed())
        con_pump();
    t = ticks_now();            /* (the terminal takes a moment) */
    while ((uint16_t)(ticks_now() - t) < 100)
        ;
}

void h19_cleared(void)
{
    new_screen = 1;
}

/* ---- drawing into bg -------------------------------------------------- */
/* (h19_put, h19_putn, h19_fill: h19cell.asm) */

void h19_str(uint8_t x, uint8_t y, const char *s, uint8_t n, uint8_t attr)
{
    uint8_t c, *p, *d, ch = 0;
    if (y >= SCR_H)
        return;
    p = rowp[y] + x;
    d = dirty[y];
    for (; n && x < SCR_W && (c = (uint8_t)*s++) != 0; --n, ++x, ++p) {
        if (c >= 'a' && c <= 'z')
            c -= 32;
        else if (c == '_')
            c = G_BAR_B;                    /* (graphics mode: ▁ looks the part) */
        else if (c == '|')
            c = G_VLINE;
        else if (c < 0x20 || c > 0x5D)
            c = '?';
        c |= attr;
        if (*p != c) {
            *p = c;
            d[x >> 3] |= bit_of[x & 7];
            ch = 1;
        }
    }
    if (ch)
        rowdirty[y] = any_dirty = 1;
}

void h19_blank(uint8_t y0, uint8_t y1)
{
    for (; y0 <= y1 && y0 < SCR_H; ++y0)
        h19_fill(0, y0, SCR_W, ' ');
}

/* ---- sprites ---------------------------------------------------------- */
void h19_spr(uint8_t slot, const uint8_t *art, int8_t c, int8_t r)
{
    if (s_art[slot] == art && (!art || (s_c[slot] == c && s_r[slot] == r)))
        return;
    s_art[slot] = art;
    s_c[slot] = c;
    s_r[slot] = r;
    s_dirty = 1;
}

/* The cells an art covers at c, r: dirty, their rows first. */
static void mark(const uint8_t *art, int8_t c, int8_t r)
{
    int8_t y, y1, x, x1;
    if (!art)
        return;
    y1 = r + (int8_t)art[1];
    x1 = c + (int8_t)art[0];
    for (y = r; y < y1; ++y) {
        if (y < 0 || y >= SCR_H)
            continue;
        hot[y] = rowdirty[y] = any_hot = any_dirty = 1;
        for (x = c; x < x1; ++x)
            if (x >= 0 && x < SCR_W)
                SETD(x, y);
    }
}

/* The sprites' moves since the last flush. */
static void spr_update(void)
{
    uint8_t i;
    if (!s_dirty)
        return;
    s_dirty = 0;
    for (i = 0; i < SPR_SLOTS; ++i) {
        if (s_sent[i] == s_art[i] &&
            (!s_art[i] || (s_sc[i] == s_c[i] && s_sr[i] == s_r[i])))
            continue;
        mark(s_sent[i], s_sc[i], s_sr[i]);
        mark(s_art[i], s_c[i], s_r[i]);
        s_sent[i] = s_art[i];
        s_sc[i] = s_c[i];
        s_sr[i] = s_r[i];
    }
}

/* Row y as it should look: bg with the sprites over it (slot 0 on top); a
 * reversed sprite cell over a reversed one shows plain (the player's figure
 * on its own plot's border). */
static void compose(uint8_t y)
{
    uint8_t i, k, w;
    int8_t c, x;
    const uint8_t *a;
    memcpy(line, rowp[y], SCR_W);
    for (i = SPR_SLOTS; i-- > 0;) {
        a = s_sent[i];
        if (!a || (int8_t)y < s_sr[i] || (int8_t)y >= s_sr[i] + (int8_t)a[1])
            continue;
        w = a[0];
        a += 2 + (uint8_t)((int8_t)y - s_sr[i]) * w;
        c = s_sc[i];
        for (k = 0; k < w; ++k, ++a) {
            x = c + (int8_t)k;
            if (*a && x >= 0 && x < SCR_W)     /* (reverse on reverse: plain) */
                line[(uint8_t)x] = *a ^ (*a & line[(uint8_t)x] & REV);
        }
    }
}

/* ---- sending ---------------------------------------------------------- */
/* One cell, where the cursor is. */
static void send(uint8_t e)
{
    setrev(e & REV);
    out(e & 0x7F);
    /* (discard at the end of a line: the last column keeps the cursor) */
    cur_x = cur_x >= SCR_W - 1 ? UNKNOWN : cur_x + 1;
}

/* Cells x0..x1-1 of the row in line (all already sent: the terminal shows
 * them) sent again to move right, if no reverse switch is needed. */
static uint8_t resend(uint8_t x0, uint8_t x1)
{
    uint8_t i;
    for (i = x0; i < x1; ++i)
        if ((line[i] & REV) != cur_rev)
            return 0;
    for (i = x0; i < x1; ++i)
        send(line[i]);
    return 1;
}

/* Along the row: right by sending again, left by BS. */
static uint8_t along(uint8_t x)
{
    if (cur_x == x)
        return 1;
    if (cur_x < x)
        return x - cur_x <= 3 && resend(cur_x, x);
    if (cur_x - x <= 2) {
        while (cur_x != x) {
            out(8);
            --cur_x;
        }
        return 1;
    }
    return 0;
}

/* The cursor to x, y (line: row y, sent up to x). */
static void go(uint8_t x, uint8_t y)
{
    if (cur_x != UNKNOWN && cur_y != UNKNOWN) {
        if (cur_y == y && along(x))
            return;
        /* down a row: LF (never from the last row: it would scroll) */
        if (cur_y + 1 == y && y < ROW25 &&
            (cur_x >= x ? cur_x - x <= 2 : x - cur_x <= 2)) {
            out(10);
            cur_y = y;
            if (along(x))
                return;
        }
    }
    esc('Y');
    out(32 + y);
    out(32 + x);
    cur_x = x;
    cur_y = y;
}

/* Row y's dirty cells, while the budget lasts: 0 if it ran out. A run of
 * dirty cells goes out after one cursor move. (Static variables: zsdcc's
 * frame variables cost 19 T-states a touch, and this is the hot path.) */
static uint8_t sweep(uint8_t y)
{
    static uint8_t x, e, last, i, *d, b;

    COUNT(3, 1);
    compose(y);
    rowdirty[y] = 0;
    d = dirty[y];
    last = UNKNOWN - 1;         /* (not looked for yet) */
    for (x = 0; x < SCR_W; ++x) {
        if (!d[x >> 3]) {
            x |= 7;             /* (eight clean cells) */
            continue;
        }
        if (!(d[x >> 3] & bit_of[x & 7]))
            continue;
        if (budget < OPMAX) {
            rowdirty[y] = any_dirty = 1;
            return 0;
        }
        e = line[x];
        if (e == ' ' && x < SCR_W - 1) {
            if (last == UNKNOWN - 1)    /* the last cell that isn't blank */
                for (last = SCR_W - 2; last != UNKNOWN && line[last] == ' '; --last)
                    ;
            if (last == UNKNOWN || x > last) {
                /* the rest of the row is blank: ESC K, if enough goes */
                for (b = 0, i = x; i < SCR_W; ++i)
                    if (DIRTY(i, y))
                        ++b;
                if (b >= STALE_FOR_ERASE) {
                    go(x, y);
                    setrev(0);
                    esc('K');
                    for (i = x; i < SCR_W; ++i)
                        CLRD(i, y);
                    return 1;
                }
            }
        }
        go(x, y);
        /* the run */
        for (;;) {
            e = line[x];
            if ((e & REV) != cur_rev) {
                cur_rev = e & REV;
                esc(cur_rev ? 'p' : 'q');
            }
            out(e & 0x7F);
            CLRD(x, y);
            if (++x >= SCR_W || !DIRTY(x, y) || budget < 3)
                break;
        }
        /* (discard at the end of a line: the last column keeps the cursor) */
        cur_x = x >= SCR_W ? UNKNOWN : x;
        --x;
    }
    return 1;
}

/* Every cell the terminal must show, now that it shows nothing: dirty. */
static void all_dirty(uint8_t y0, uint8_t y1)
{
    uint8_t y, i, k, b, *q, *d;
    for (y = y0; y <= y1; ++y) {
        compose(y);
        rowdirty[y] = 0;
        for (q = line, d = dirty[y], i = ROWB; i; --i, ++d) {
            for (b = 0, k = 1; k; k <<= 1, ++q)
                if (*q != ' ')
                    b += k;
            *d = b;
            if (b)
                rowdirty[y] = any_dirty = 1;
        }
    }
}

/* A new screen: if much of the old one must be wiped, ESC E does it. */
static void maybe_clear(void)
{
    uint16_t wipe = 0;
    uint8_t y, i, k, b, *p, *d;
    new_screen = 0;
    for (y = 0; y < ROW25; ++y) {
        if (!rowdirty[y])
            continue;
        for (p = rowp[y], d = dirty[y], i = ROWB; i; --i, ++d) {
            b = *d;
            if (!b) {
                p += 8;
                continue;
            }
            for (k = 1; k; k <<= 1, ++p)
                if ((b & k) && *p == ' ')
                    ++wipe;
        }
    }
    if (wipe >= WIPE_FOR_CLEAR) {
        setrev(0);
        esc('E');
        cur_x = cur_y = 0;
        all_dirty(0, ROW25 - 1);
    }
}

void h19_flush(void)
{
    static uint8_t y, k, more;

    if (!active)
        return;
    ++h89_flushes;
    con_pump();
    budget = (int8_t)(FILL - (TX_RING - 1 - con_free()));
    if (s_dirty)
        spr_update();
    if (new_screen)
        maybe_clear();
    more = 0;
    if (any_hot) {
        for (y = 0; y < SCR_H; ++y)
            if (hot[y]) {
                if (budget < OPMAX || !sweep(y)) {
                    more = 1;
                    goto done;
                }
                hot[y] = 0;
            }
        any_hot = 0;
    }
    if (any_dirty) {
        for (k = 0; k < SCR_H; ++k) {
            y = rr + k;
            if (y >= SCR_H)
                y -= SCR_H;
            if (!rowdirty[y])
                continue;
            if (budget < OPMAX || !sweep(y)) {
                rr = y;
                more = 1;
                goto done;
            }
        }
        any_dirty = 0;
    }
done:
    h89_idle = !more && con_flushed();
}

void h19_repaint(void)
{
    if (!active)
        return;
    setrev(0);
    send_all("\033E\033Y8 \033l");
    cur_x = cur_y = UNKNOWN;
    all_dirty(0, ROW25);
}

void h19_bell(void)
{
    con_put(7);
}
