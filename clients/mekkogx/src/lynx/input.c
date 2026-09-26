/*
 * input.c -- the Lynx's pad and buttons.
 *
 *   pad        walk / choose
 *   A          the button
 *   B          rub out (name entry)
 *   OPTION 1   RETURN: end the turn, finish a name
 *   OPTION 2   ESC: leave the table
 *   OPT1 + B   0: sit this good out (declare)
 *   PAUSE      the sound off and on
 *
 * The joystick register holds the pad (bits 4-7: up down left right, as
 * the Lynx is held normally) and the buttons (bit 0 A, bit 1 B, bit 2
 * option 2, bit 3 option 1); the switches register bit 0 is PAUSE.
 */
#ifdef __LYNX__

#include <stdint.h>
#include "input.h"
#include "sound.h"
#include "lxhw.h"

uint8_t inp_dir, inp_btn, ev_btn, ev_key;
uint8_t inp_hook, inp_hook_dir, inp_hook_btn, inp_hook_key;

static uint8_t prev_j, prev_p, btn_prev, btn_latch, key_latch;

void input_init(void)
{
    prev_j = SUZY_JOYSTICK;
    prev_p = SUZY_SWITCHES & 1;
}

void input_poll(void)
{
    uint8_t j = SUZY_JOYSTICK, p = SUZY_SWITCHES & 1, dir = 0, btn, down;

    down = j & ~prev_j;                 /* newly pressed */
    prev_j = j;
    if (j & 0x80) dir |= DIR_UP;
    if (j & 0x40) dir |= DIR_DOWN;
    if (j & 0x20) dir |= DIR_LEFT;
    if (j & 0x10) dir |= DIR_RIGHT;
    btn = j & 0x01;
    if (down & 0x02)
        key_latch = (j & 0x08) ? KEY_SITOUT : KEY_BACK;
    else if (down & 0x08)
        key_latch = KEY_END;
    else if (down & 0x04)
        key_latch = KEY_LEAVE;
    if (p && !prev_p && !inp_hook)
        snd_toggle_mute();
    prev_p = p;

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
}

#endif
