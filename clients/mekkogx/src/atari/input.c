/*
 * input.c -- joystick 0 and the keyboard on the Atari.
 *
 * The stick and its button (or the arrow keys and the space bar), RETURN
 * ends a turn, ESC leaves the table, 0 sits out, ctrl-S turns the sound off
 * and on. Keys come from the OS's CH; a direction key counts as held while
 * SKSTAT says a key is down.
 */
#ifdef __ATARI__

#include <stdint.h>
#include "input.h"
#include "sound.h"

#define STICK0 (*(volatile uint8_t *)0x0278)
#define STRIG0 (*(volatile uint8_t *)0x0284)
#define CH     (*(volatile uint8_t *)0x02FC)
#define SKSTAT (*(volatile uint8_t *)0xD20F)

uint8_t inp_dir, inp_btn, ev_btn, ev_key;
uint8_t inp_hook, inp_hook_dir, inp_hook_btn, inp_hook_key;

static uint8_t key_dir, key_btn, btn_prev, btn_latch, key_latch;

/* Keyboard code (bits 0-5, bit 6 shift) -> ASCII (upper case), 0 none. */
static const char kmap[64] = {
    'L', 'J', ';', 0,   0,   'K', '+', '*', 'O', 0,   'P', 'U', 13,  'I', '-', '=',
    'V', 0,   'C', 0,   0,   'B', 'X', 'Z', '4', 0,   '3', '6', 27,  '5', '2', '1',
    ',', ' ', '.', 'N', 0,   'M', '/', 0,   'R', 0,   'E', 'Y', 9,   'T', 'W', 'Q',
    '9', 0,   '0', '7', 8,   '8', '<', '>', 'F', 'H', 'D', 0,   0,   'G', 'S', 'A',
};

void input_init(void)
{
    CH = 0xFF;
}

void input_poll(void)
{
    uint8_t k, c, btn, dir = 0, s;

    k = CH;
    if (k != 0xFF) {
        CH = 0xFF;
        c = kmap[k & 0x3F];
        key_dir = 0;
        key_btn = 0;
        if (k & 0x80) {                 /* control: the arrows, ctrl-S */
            switch (k & 0x3F) {
            case 0x06: key_dir = DIR_LEFT; break;       /* ctrl + */
            case 0x07: key_dir = DIR_RIGHT; break;      /* ctrl * */
            case 0x0E: key_dir = DIR_UP; break;         /* ctrl - */
            case 0x0F: key_dir = DIR_DOWN; break;       /* ctrl = */
            case 0x3E: if (!inp_hook) snd_toggle_mute(); break;
            }
            c = 0;
        } else if (c == ' ') {
            key_btn = 1;
        }
        if (c)
            key_latch = c == 8 ? KEY_BACK : c;
    } else if (SKSTAT & 0x04) {
        key_dir = 0;                    /* let go */
        key_btn = 0;
    }
    dir = key_dir;
    btn = key_btn || !STRIG0;
    s = ~STICK0 & 15;                   /* bits: 1 up, 2 down, 4 left, 8 right */
    if (s & 1) dir |= DIR_UP;
    if (s & 2) dir |= DIR_DOWN;
    if (s & 4) dir |= DIR_LEFT;
    if (s & 8) dir |= DIR_RIGHT;

    if (inp_hook) {
        dir = inp_hook_dir;
        btn = inp_hook_btn;
        if (inp_hook_key) {
            key_latch = inp_hook_key;
            inp_hook_key = 0;
        }
    }
    inp_dir = dir;
    inp_btn = btn;
    if (btn && !btn_prev)
        btn_latch = 1;
    btn_prev = btn;
}

void input_take(void)
{
    ev_btn = btn_latch;
    btn_latch = 0;
    ev_key = key_latch;
    key_latch = KEY_NONE;
    if (ev_key == ' ')
        ev_key = KEY_NONE;
}

#endif
