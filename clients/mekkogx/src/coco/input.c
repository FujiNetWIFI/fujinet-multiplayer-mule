/*
 * input.c -- keyboard and the right joystick on the CoCo 1/2.
 *
 * Keys: the arrows walk, SPACE (or the joystick button) is the button,
 * ENTER ends a turn, BREAK leaves the table, 0 sits out, CLEAR swaps the
 * artifact colours, SHIFT-CLEAR turns the sound off and on. LEFT is also
 * the rub-out key for typing a name.
 *
 * The keyboard is a matrix: PIA0 B ($FF02) drives a column low, A ($FF00)
 * reads the rows (0 = pressed). The IRQ scans it every frame (plat.c) and
 * queues the keys that went down, so a quick tap is never lost while the
 * game is busy. Bit 7 of $FF00 is the joystick comparator: the stick is
 * compared with thresholds on the DAC rather than measured.
 */
#include "mule.h"

void gfx_swap_colours(void);    /* gfx.c */

#define PIA0A  (*(volatile uint8_t *)0xFF00)
#define PIA0CA (*(volatile uint8_t *)0xFF01)
#define PIA0B  (*(volatile uint8_t *)0xFF02)
#define PIA0CB (*(volatile uint8_t *)0xFF03)
#define DAC    (*(volatile uint8_t *)0xFF20)

uint8_t inp_dir, inp_btn, ev_btn, ev_key;
uint8_t inp_hook, inp_hook_dir, inp_hook_btn, inp_hook_key;

extern uint8_t kb_rows[8];      /* plat.c: the matrix, [column], 1 = down */
extern uint8_t kb_q[8], kb_head, kb_tail;
static uint8_t btn_prev, btn_latch, key_latch;
static uint8_t joy_on, joy_axis, joy_dir;

/* What each key is (column, row): letters and digits as themselves. */
static const char keys[7][8] = {
    { '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G' },
    { 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O' },
    { 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W' },
    { 'X', 'Y', 'Z', 0, 0, 0, 0, ' ' },          /* up down left right */
    { '0', '1', '2', '3', '4', '5', '6', '7' },
    { '8', '9', ':', ';', ',', '-', '.', '/' },
    { 13, 0, 27, 0, 0, 0, 0, 0 },                /* ENTER CLEAR BREAK .. SHIFT */
};

#define DOWN(r, c) (kb_rows[c] & (1 << (r)))

/* Compare the right joystick's axis (0 x, 1 y) with DAC level v (0..63):
 * 1 if the stick is above it. */
static uint8_t above(uint8_t axis, uint8_t v)
{
    uint8_t i;
    if (axis)
        PIA0CA |= 0x08;         /* mux: right y */
    else
        PIA0CA &= ~0x08;        /* right x */
    PIA0CB &= ~0x08;
    DAC = (v << 2) | 0x02;      /* keep the RS-232 line (bit 1) marking */
    i = PIA0A;                  /* (the comparator settles meanwhile) */
    return PIA0A >> 7;
}

void input_init(void)
{
    /* the stick is used if it is centred now */
    joy_on = above(0, 20) && !above(0, 44) && above(1, 20) && !above(1, 44);
}

void input_poll(void)
{
    uint8_t c, r, k, btn, dir = 0, shift;

    PIA0B = 0xFF;
    btn = (~PIA0A & 0x03) != 0;         /* either joystick's button */
    shift = DOWN(6, 7) != 0;
    /* the keys that went down (name entry, END, ESC...): one a frame, so
     * each gets to the game */
    while (kb_tail != kb_head && key_latch == KEY_NONE) {
        k = kb_q[kb_tail];
        kb_tail = (kb_tail + 1) & 7;
        c = k >> 3;
        r = k & 7;
        if (r == 6 && c == 1) {             /* CLEAR */
            if (shift)
                snd_toggle_mute();
            else
                gfx_swap_colours();
        } else if (r == 3 && c == 5) {
            key_latch = KEY_BACK;           /* LEFT rubs out too */
        } else if (r == 3 && c == 7) {
            btn_latch = 1;                  /* SPACE: the button, however short */
        } else if (r < 7 && keys[r][c]) {
            key_latch = keys[r][c];
        }
    }
    if (DOWN(3, 3)) dir |= DIR_UP;
    if (DOWN(3, 4)) dir |= DIR_DOWN;
    if (DOWN(3, 5)) dir |= DIR_LEFT;
    if (DOWN(3, 6)) dir |= DIR_RIGHT;
    if (DOWN(3, 7)) btn = 1;

    if (joy_on) {               /* one axis a frame */
        if (joy_axis ^= 1) {
            joy_dir &= DIR_UP | DIR_DOWN;
            if (!above(0, 16)) joy_dir |= DIR_LEFT;
            else if (above(0, 48)) joy_dir |= DIR_RIGHT;
        } else {
            joy_dir &= DIR_LEFT | DIR_RIGHT;
            if (!above(1, 16)) joy_dir |= DIR_UP;
            else if (above(1, 48)) joy_dir |= DIR_DOWN;
        }
        dir |= joy_dir;
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
