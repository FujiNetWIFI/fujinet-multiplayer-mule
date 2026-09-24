/*
 * input.c -- keyboard and joystick on the Apple II.
 *
 * Keys: arrows or I/J/K/M walk, space (or either joystick button / apple
 * key) is the button, RETURN ends a turn, ESC leaves the table, 0 sits out,
 * ctrl-S turns the sound off and on.
 *
 * The keyboard gives a key and, on the //e and later, "a key is down" (bit 7
 * of $C010), so a direction key counts as held until it is let go. The
 * joystick (if one is centred at start-up) is read one axis a frame, since
 * each paddle read takes up to 3 ms.
 */
#ifdef __APPLE2__

#include <stdint.h>
#include "input.h"
#include "sound.h"

#define KBD     (*(volatile uint8_t *)0xC000)
#define KBDSTRB (*(volatile uint8_t *)0xC010)
#define BUTN0   (*(volatile uint8_t *)0xC061)
#define BUTN1   (*(volatile uint8_t *)0xC062)
/* Clear the keyboard strobe. (A bare "KBDSTRB;" is dropped by cc65.) */
#define CLEAR_STROBE() __asm__("bit $C010")

uint8_t inp_dir, inp_btn, ev_btn, ev_key;
uint8_t inp_hook, inp_hook_dir, inp_hook_btn, inp_hook_key;

static uint8_t key_dir;         /* direction of the key being held */
static uint8_t key_btn;         /* the space bar is being held */
static uint8_t btn_prev;
static uint8_t btn_latch, key_latch;
static uint8_t joy_on, joy_axis, joy_x = 128, joy_y = 128;

uint8_t paddle(uint8_t n);      /* pread.s */

void input_init(void)
{
    uint8_t x = paddle(0), y = paddle(1);
    joy_on = x > 40 && x < 216 && y > 40 && y < 216;
    CLEAR_STROBE();
}

void input_poll(void)
{
    uint8_t k, btn, dir = 0;

    if (KBD & 0x80) {
        k = KBD & 0x7F;
        CLEAR_STROBE();
        if (k >= 'a' && k <= 'z')
            k -= 32;
        key_dir = 0;
        key_btn = 0;
        switch (k) {
        case 8:  case 'J': key_dir = DIR_LEFT; break;
        case 21: case 'K': key_dir = DIR_RIGHT; break;
        case 11: case 'I': key_dir = DIR_UP; break;
        case 10: case 'M': key_dir = DIR_DOWN; break;
        case ' ': key_btn = 1; break;
        }
        if (k == 0x13 && !inp_hook)   /* ctrl-S */
            snd_toggle_mute();
        /* every key is also a key (name entry, END, ESC...) */
        key_latch = k == 127 ? KEY_BACK : k;
    } else if (!(KBDSTRB & 0x80)) {
        key_dir = 0;            /* let go */
        key_btn = 0;
    }
    dir = key_dir;
    btn = key_btn || (BUTN0 & 0x80) || (BUTN1 & 0x80);

    if (joy_on) {
        if (joy_axis ^= 1)
            joy_x = paddle(0);
        else
            joy_y = paddle(1);
        if (joy_x < 64) dir |= DIR_LEFT;
        if (joy_x > 192) dir |= DIR_RIGHT;
        if (joy_y < 64) dir |= DIR_UP;
        if (joy_y > 192) dir |= DIR_DOWN;
    }

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
    /* the space bar is the button, not a key */
    if (ev_key == ' ')
        ev_key = KEY_NONE;
}

#endif
