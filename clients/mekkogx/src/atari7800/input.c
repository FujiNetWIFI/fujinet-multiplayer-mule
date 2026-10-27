/*
 * input.c -- the 7800's joystick (left port) and console switches.
 *
 *   stick            walk / choose
 *   button 1         the button
 *   button 2         rub out (name entry); RESET does the same, for a
 *                    one-button 2600 stick
 *   PAUSE            RETURN: end the turn, finish a name
 *   SELECT           ESC: leave the table
 *   PAUSE + button 2 0: sit this good out (declare)
 *
 * Two-button mode (CTLSWB bit 2 out, SWCHB 0): button 1 is INPT1 bit 7,
 * button 2 INPT0 bit 7, both set while pressed; a 2600 stick's one button
 * shows on INPT4 (bit 7 clear). SWCHA's high nibble is the stick (right,
 * left, down, up; 0 pressed), SWCHB the switches (bit 0 RESET, 1 SELECT,
 * 3 PAUSE; 0 pressed). Only RIOT stores: never the TIA (INPTCTRL).
 */
#include <stdint.h>
#include "input.h"
#include "a78hw.h"

uint8_t inp_dir, inp_btn, ev_btn, ev_key;
uint8_t inp_hook, inp_hook_dir, inp_hook_btn, inp_hook_key;

#define J_B1     0x01
#define J_B2     0x02
#define J_PAUSE  0x04
#define J_SELECT 0x08

/* SWCHA >> 4 (pressed = 1 after inverting) -> DIR_*: up, down, left, right */
static const uint8_t dir_of[16] = {
    0, DIR_UP, DIR_DOWN, DIR_UP | DIR_DOWN, DIR_LEFT, DIR_LEFT | DIR_UP, DIR_LEFT | DIR_DOWN, DIR_LEFT | DIR_UP | DIR_DOWN,
    DIR_RIGHT, DIR_RIGHT | DIR_UP, DIR_RIGHT | DIR_DOWN, DIR_RIGHT | DIR_UP | DIR_DOWN,
    DIR_RIGHT | DIR_LEFT, DIR_RIGHT | DIR_LEFT | DIR_UP, DIR_RIGHT | DIR_LEFT | DIR_DOWN, 15,
};

static uint8_t prev_j, btn_prev, btn_latch, key_latch;

static uint8_t read_pad(uint8_t *dir)
{
    uint8_t j = 0, s = ~SWCHB;
    *dir = dir_of[(uint8_t)~SWCHA >> 4];
    if ((INPT1 & 0x80) || !(INPT4 & 0x80))
        j |= J_B1;
    if ((INPT0 & 0x80) || (s & 0x01))
        j |= J_B2;
    if (s & 0x08)
        j |= J_PAUSE;
    if (s & 0x02)
        j |= J_SELECT;
    return j;
}

void input_init(void)
{
    uint8_t d;
    CTLSWA = 0;                 /* the sticks: inputs */
    CTLSWB = 0x04;              /* two-button mode */
    SWCHB = 0;
    prev_j = read_pad(&d);
}

void input_poll(void)
{
    uint8_t dir, j = read_pad(&dir), btn, down;

    down = j & ~prev_j;                 /* newly pressed */
    prev_j = j;
    btn = (j & J_B1) != 0;
    if (down & J_B2)
        key_latch = (j & J_PAUSE) ? KEY_SITOUT : KEY_BACK;
    else if (down & J_PAUSE)
        key_latch = KEY_END;
    else if (down & J_SELECT)
        key_latch = KEY_LEAVE;

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
