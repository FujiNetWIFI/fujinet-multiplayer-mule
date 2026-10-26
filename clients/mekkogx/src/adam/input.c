/*
 * input.c -- the Adam's keyboard and controller 1.
 *
 * Keys: the arrows walk, SPACE (or a fire button) is the button, RETURN
 * ends a turn, ESCAPE leaves the table, 0 sits out, BACKSPACE rubs out,
 * SmartKey VI turns the sound off and on. On the controller's keypad * ends
 * a turn and 0 sits out.
 *
 * The keyboard is an AdamNet device that sends characters, not keys held:
 * an arrow counts as held for a while after each one it sends (its repeat
 * keeps it held).
 */
#include "mule.h"
#include "eoscall.h"

uint8_t inp_dir, inp_btn, ev_btn, ev_key;
uint8_t inp_hook, inp_hook_dir, inp_hook_btn, inp_hook_key;

static uint8_t btn_prev, btn_latch, key_latch, pad_prev;
static uint8_t held_dir, held_t, space_t;

#define K_UP     0xA0
#define K_RIGHT  0xA1
#define K_DOWN   0xA2
#define K_LEFT   0xA3
#define K_SMART6 0x86
#define HOLD     20             /* frames an arrow stays held: its first
                                 * repeat comes in about a third of a second */

void input_init(void)
{
    eos_kbd_start();
}

/* A character from the keyboard, if one came. */
static void typed(void)
{
    uint8_t c = eos_kbd_end();
    if (c == 0)
        return;                 /* still waiting */
    eos_kbd_start();
    if (c == 1)
        return;                 /* it failed: asked again */
    switch (c) {
    case K_UP:    held_dir = DIR_UP;    held_t = HOLD; return;
    case K_RIGHT: held_dir = DIR_RIGHT; held_t = HOLD; return;
    case K_DOWN:  held_dir = DIR_DOWN;  held_t = HOLD; return;
    case K_LEFT:  held_dir = DIR_LEFT;  held_t = HOLD; return;
    case K_SMART6: snd_toggle_mute(); return;
    case ' ':
        btn_latch = 1;
        space_t = 6;
        return;
    }
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (key_latch == KEY_NONE &&
        (c == 13 || c == 27 || c == 8 || (c > 32 && c < 96)))
        key_latch = c;
}

void input_poll(void)
{
    uint8_t dir = 0, btn = 0, j, pad;

    typed();
    if (held_t) {
        --held_t;
        dir = held_dir;
    }
    if (space_t) {
        --space_t;
        btn = 1;
    }
    joy_read();
    j = joy_ctl[0];             /* 1 north, 2 east, 4 south, 8 west */
    if (j & 1) dir |= DIR_UP;
    if (j & 2) dir |= DIR_RIGHT;
    if (j & 4) dir |= DIR_DOWN;
    if (j & 8) dir |= DIR_LEFT;
    if (joy_ctl[1] || joy_ctl[2])   /* left, right fire */
        btn = 1;
    pad = joy_ctl[3];
    if (pad != pad_prev && key_latch == KEY_NONE) {
        if (pad == 0x09)        /* * */
            key_latch = KEY_END;
        else if (pad == 0x0A)   /* 0 */
            key_latch = KEY_SITOUT;
    }
    pad_prev = pad;

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
