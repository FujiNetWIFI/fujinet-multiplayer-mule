/*
 * text.c -- numbers, expanding strings (strings.c) and the message line.
 *
 * A '%' in a string is replaced by a number. Messages go on two text rows
 * (msg_row, msg_row + 1) as on the Atari; one longer than both scrolls
 * along the second row.
 */
#include <string.h>
#include "mule.h"

/* The amount multiplier of each event (the amount is x * 25 * (round/4+1));
 * 0 none, 100 per food M.U.L.E. (x2), 101 per mining M.U.L.E. (x2), 102 per
 * energy M.U.L.E. (x1). */
const uint8_t pevent_x[22] = {
    0, 0, 2, 4, 100, 4, 8, 2, 3, 6, 4, 2, 0, 0, 3, 101, 102, 6, 4, 4, 4, 0,
};

/* ---- numbers ---------------------------------------------------------- */
/* v in decimal at dst (NUL-terminated); returns its length. Subtracts
 * powers of ten: cc65's division is slow. */
static const uint16_t pow10[4] = { 10000, 1000, 100, 10 };

uint8_t fmt_num(char *dst, uint16_t v)
{
    uint8_t i, n = 0, d;
    uint16_t p;
    for (i = 0; i < 4; ++i) {
        p = pow10[i];
        d = '0';
        while (v >= p) {
            v -= p;
            ++d;
        }
        if (d != '0' || n)
            dst[n++] = d;
    }
    dst[n++] = '0' + (uint8_t)v;
    dst[n] = 0;
    return n;
}

static void num_right(char *buf, uint16_t v, uint8_t w)
{
    char d[6];
    uint8_t n = fmt_num(d, v);
    memset(buf, ' ', w);
    buf[w] = 0;
    if (n > w)
        n = w;
    memcpy(buf + w - n, d + (strlen(d) - n), n);
}

void put_num(uint8_t tx, uint8_t ty, uint16_t v, uint8_t w, uint8_t col)
{
    char buf[8];
    num_right(buf, v, w);
    gfx_text(tx, ty, buf, col);
}

void big_num(uint8_t tx, uint8_t ty, uint16_t v, uint8_t w, uint8_t col)
{
    char buf[8];
    num_right(buf, v, w);
    gfx_big(tx, ty, buf, col);
}

void big_num32(uint8_t tx, uint8_t ty, uint32_t v, uint8_t w, uint8_t col)
{
    char buf[10];
    uint8_t i = w;
    buf[w] = 0;
    do {
        buf[--i] = '0' + (uint8_t)(v % 10);
        v /= 10;
    } while (v && i);
    while (i)
        buf[--i] = ' ';
    gfx_big(tx, ty, buf, col);
}

/* ---- expanding and showing -------------------------------------------- */
char txt[128];
uint8_t msg_row = 22, msg_col = C_WHITE, msg_scroll;
static uint8_t msg_len, msg_pos;
static uint16_t msg_last;

uint8_t text_expand(char *dst, const char *src, uint16_t v1, uint16_t v2)
{
    uint8_t n = 0;
    char c;
    while ((c = *src++) != 0 && n < 120) {
        if (c == '%') {
            n += fmt_num(dst + n, v1);
            v1 = v2;
        } else {
            dst[n++] = c;
        }
    }
    dst[n] = 0;
    return n;
}

void text_center(uint8_t ty, const char *s, uint8_t col)
{
    uint8_t n = strlen(s);
    gfx_clear_text(0, ty, 40);
    gfx_text(n < 40 ? (40 - n) / 2 : 0, ty, s, col);
}

void big_center(uint8_t ty, const char *s, uint8_t col)
{
    uint8_t n = strlen(s);
    gfx_clear_text(0, ty, 40);
    gfx_big(n < 20 ? (20 - n) & 0xFE : 0, ty, s, col);
}

void title_show(uint8_t id, uint16_t v)
{
    char buf[32];
    text_expand(buf, str_title(id), v, 0);
    text_center(1, buf, C_WHITE);
}

void msg_clear(void)
{
    msg_scroll = 0;
    gfx_clear_text(0, msg_row, 40);
    gfx_clear_text(0, msg_row + 1, 40);
}

/* The message in txt: centred on one row, word-wrapped onto two, or (longer
 * still) scrolling in from the right along the second. */
void msg_start(uint8_t len)
{
    uint8_t cut;
    msg_len = len;
    msg_clear();
    if (len <= 40) {
        gfx_text((40 - len) / 2, msg_row, txt, msg_col);
    } else if (len <= 80) {
        cut = 40;
        while (cut > 0 && txt[cut] != ' ')
            --cut;
        if (cut == 0 || len - cut - 1 > 40)
            cut = 40;
        gfx_textn((40 - cut) / 2, msg_row, txt, cut, msg_col);
        if (txt[cut] == ' ')
            ++cut;
        gfx_text((40 - (len - cut)) / 2, msg_row + 1, txt + cut, msg_col);
    } else {
        msg_scroll = 1;
        msg_pos = 0;
        msg_last = frame_count;
    }
}

void msg_show(uint8_t id, uint16_t v1, uint16_t v2)
{
    msg_start(text_expand(txt, str_msg(id), v1, v2));
}

/* Every frame: advance a scrolling message a character every 4 frames. */
void msg_tick(void)
{
    uint8_t i, k;
    static char win[41];
    if (!msg_scroll || (uint16_t)(frame_count - msg_last) < 4)
        return;
    msg_last = frame_count;
    /* columns 0..39 show characters msg_pos-40 .. msg_pos-1 */
    for (i = 0; i < 40; ++i) {
        k = msg_pos + i;
        win[i] = (k >= 40 && k < 40 + msg_len) ? txt[k - 40] : ' ';
    }
    win[40] = 0;
    gfx_text(0, msg_row + 1, win, msg_col);
    if (++msg_pos >= msg_len + 44)
        msg_pos = 0;
}
