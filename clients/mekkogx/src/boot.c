/*
 * boot.c -- start-up: FujiNet, who we are, which table (clients/intv/boot.bas).
 *
 * The FujiNet lobby hands a game its details in appkeys (creator 1, app 1):
 *   key 0    the player's name (shared by every FujiNet game)
 *   key 10   the table's URL, "https://host/?table=<id>"
 * Without them we ask for a name and list the default server's tables. The
 * chosen table is written back to key 10, so restarting rejoins it.
 */
#include <string.h>
#include "mule.h"

static void wait_frames(uint8_t n)
{
    while (n--) {
        frame_wait();
        snd_tick();
    }
}

/* Type a name: A-Z and 0-9, up to 8; DELETE or <- rubs out, RETURN ends. */
static void name_entry(void)
{
    uint8_t n = 0, blink;
    char c;
    gfx_clear();
    text_center(6, "YOUR NAME?", C_WHITE);
    text_center(16, "TYPE IT, THEN PRESS " KEY_END_NAME, C_WHITE);
    memset(sc_name, 0, sizeof(sc_name));
    for (;;) {
        frame_wait();
        snd_tick();
        input_poll();
        input_take();
        c = ev_key;
        if (c == KEY_BACK && n) {
            sc_name[--n] = 0;
            snd_play(SND_SELECT);
        } else if (c == KEY_END && n) {
            break;
        } else if (n < 8 && ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
            sc_name[n++] = c;
            snd_play(SND_SELECT);
        }
        blink = (frame_count & 16) ? '_' : ' ';
        gfx_clear_text(16, 10, 10);
        gfx_big(16 - (n & 0xFE), 10, sc_name, C_ORANGE);
        gfx_char(16 - (n & 0xFE) + n * 2, 10, blink, C_WHITE);
    }
    snd_play(SND_SELECT);
}

#define TB_MAX 9
static uint8_t tbuf[1 + TB_MAX * sizeof(MuleTable)];

/* Choose a table from /tables: up/down, then the button (or RETURN). */
void table_select(void)
{
    uint8_t n, i, sel = 0, rep = 0;
    MuleTable *t;
    char name[21];

    for (;;) {
        gfx_clear();
        big_center(0, "CHOOSE A TABLE", C_ORANGE);
        text_center(23, "LOADING...", C_WHITE);
        n = net_tables(tbuf, TB_MAX);
        gfx_clear_text(0, 23, 40);
        if (n == 0) {
            text_center(22, "NO TABLES FOUND AT", C_WHITE);
            text_center(23, sc_endpt, C_WHITE);
            do {
                frame_wait();
                input_poll();
                input_take();
            } while (!ev_btn && ev_key != KEY_END);
            continue;
        }
        t = (MuleTable *)(tbuf + 1);
        for (i = 0; i < n; ++i) {
            memcpy(name, t[i].name, 20);
            name[20] = 0;
            gfx_text(4, 4 + i * 2, name, C_WHITE);
            memcpy(name, t[i].players, 5);
            name[5] = 0;
            gfx_text(30, 4 + i * 2, name, C_WHITE);
        }
        text_center(23, "UP/DOWN, THEN SPACE", C_WHITE);
        sel = 0;
        for (;;) {
            frame_wait();
            snd_tick();
            input_poll();
            input_take();
            for (i = 0; i < n; ++i)
                gfx_char(2, 4 + i * 2, i == sel ? '>' : ' ', C_WHITE);
            if (rep)
                --rep;
            if (!rep) {
                if ((inp_dir & DIR_UP) && sel > 0) {
                    --sel;
                    rep = 12;
                    snd_play(SND_SELECT);
                } else if ((inp_dir & DIR_DOWN) && sel < n - 1) {
                    ++sel;
                    rep = 12;
                    snd_play(SND_SELECT);
                }
            }
            if (ev_btn || ev_key == KEY_END)
                break;
        }
        memcpy(sc_table, t[sel].iD, 8);
        sc_table[8] = 0;
        if (sc_table[0])
            break;
    }
    snd_play(SND_SELECT);
}

void boot(void)
{
    text_center(16, "CONNECTING...", C_WHITE);
    while (!net_detect()) {
        text_center(16, "NO FUJINET FOUND", C_WHITE);
        wait_frames(60);
    }
    appkey_load();
    if (!sc_name[0])
        name_entry();
    if (!sc_table[0])
        table_select();
    appkey_save_room();
}
