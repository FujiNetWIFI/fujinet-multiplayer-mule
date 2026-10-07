/*
 * input.c -- the SMS's control pad 1 (port DCh), its Pause button (the
 * Z80's NMI: z88dk's crt0 flips pause_flag) and the SMS1's Reset (port
 * DDh). Pause can't be held, so the buttons held when it comes say what
 * it means:
 *
 *   1               the button (ready, bid, take a letter)
 *   2               KEY_BACK (rub out), when it is let go
 *   Pause           KEY_END (end the turn, the name)
 *   2 held + Pause  KEY_LEAVE (leave the table, quit)
 *   1 held + Pause  KEY_SITOUT (declare: sit this good out)
 *   Reset           KEY_LEAVE
 *
 * 2 counts only when let go without a Pause meanwhile: pressed, it would
 * be a key of its own, which ends the "leave?" question before the Pause
 * that answers it. (1 counts when pressed: declare ignores it.)
 */
#include <stdint.h>
#include "input.h"

__sfr __at 0xDC PAD_A;          /* 0 = down: up down left right 1 2 */
__sfr __at 0xDD PAD_B;          /* bit 4: Reset (an SMS1's), 0 = down */

#define J_UP    0x01
#define J_DOWN  0x02
#define J_LEFT  0x04
#define J_RIGHT 0x08
#define J_1     0x10
#define J_2     0x20
#define J_RESET 0x10

extern volatile uint8_t pause_flag;     /* crt0 */

uint8_t inp_dir, inp_btn, ev_btn, ev_key;
uint8_t inp_hook, inp_hook_dir, inp_hook_btn, inp_hook_key;
static uint8_t prev_j, prev_r, pause_seen, btn_prev, btn_latch, key_latch;
static uint8_t two_paused;      /* a Pause came while 2 was held */

void input_init(void)
{
    prev_j = ~PAD_A;
    prev_r = ~PAD_B & J_RESET;
    pause_seen = pause_flag;
}

void input_poll(void)
{
    uint8_t j = ~PAD_A, r = ~PAD_B & J_RESET, p = pause_flag, dir = 0, btn, up;

    up = prev_j & ~j;                   /* let go */
    prev_j = j;
    if (j & J_UP)    dir |= DIR_UP;
    if (j & J_DOWN)  dir |= DIR_DOWN;
    if (j & J_LEFT)  dir |= DIR_LEFT;
    if (j & J_RIGHT) dir |= DIR_RIGHT;
    btn = (j & J_1) != 0;
    if (up & J_2) {
        if (!two_paused)
            key_latch = KEY_BACK;
        two_paused = 0;
    }
    if (p != pause_seen) {
        pause_seen = p;
        if (j & J_2) {
            key_latch = KEY_LEAVE;
            two_paused = 1;
        } else {
            key_latch = (j & J_1) ? KEY_SITOUT : KEY_END;
        }
    }
    if (r && !prev_r)
        key_latch = KEY_LEAVE;
    prev_r = r;

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
