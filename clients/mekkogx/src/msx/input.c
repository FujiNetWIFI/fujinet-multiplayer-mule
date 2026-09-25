/*
 * input.c -- the MSX's keyboard and joystick 1, through the BIOS.
 *
 * Keys: the arrows walk, SPACE (or the stick's first button) is the
 * button, RETURN ends a turn, ESC leaves the table, 0 sits out, BS rubs
 * out, F1 turns the sound off and on. The arrows and SPACE are read from
 * the keyboard matrix (held); the rest are the BIOS's typed characters.
 */
#include "mule.h"

uint8_t inp_dir, inp_btn, ev_btn, ev_key;
uint8_t inp_hook, inp_hook_dir, inp_hook_btn, inp_hook_key;

static uint8_t btn_prev, btn_latch, key_latch, f1_prev;

uint8_t msx_snsmat(uint8_t row) __z88dk_fastcall;
uint8_t msx_gtstck(uint8_t stick) __z88dk_fastcall;
uint8_t msx_gttrig(uint8_t trig) __z88dk_fastcall;
uint8_t msx_chsns(void);
uint8_t msx_chget(void);

/* GTSTCK's 1 (up) .. 8 (up-left), clockwise, as DIR_ bits */
static const uint8_t stick_dir[9] = {
    0, DIR_UP, DIR_UP | DIR_RIGHT, DIR_RIGHT, DIR_DOWN | DIR_RIGHT,
    DIR_DOWN, DIR_DOWN | DIR_LEFT, DIR_LEFT, DIR_UP | DIR_LEFT,
};

void input_init(void)
{
}

static void typed(void)
{
    uint8_t c;
    if (key_latch != KEY_NONE || !msx_chsns())
        return;
    c = msx_chget();
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c == 13 || c == 27 || c == 8 || (c > 32 && c < 96))
        key_latch = c;          /* (not SPACE nor the arrows: 1Ch-1Fh) */
}

void input_poll(void)
{
    uint8_t dir = 0, btn, k, f1;

    typed();
    k = ~msx_snsmat(8);         /* 1 = down */
    if (k & 0x20) dir |= DIR_UP;
    if (k & 0x40) dir |= DIR_DOWN;
    if (k & 0x10) dir |= DIR_LEFT;
    if (k & 0x80) dir |= DIR_RIGHT;
    btn = k & 0x01;             /* SPACE */
    k = msx_gtstck(1);
    if (k < 9)
        dir |= stick_dir[k];
    if (msx_gttrig(1))
        btn = 1;
    f1 = !(msx_snsmat(6) & 0x20);
    if (f1 && !f1_prev)
        snd_toggle_mute();
    f1_prev = f1;

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
