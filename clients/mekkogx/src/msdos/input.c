/*
 * input.c -- the keyboard and joystick A on the PC and PCjr.
 *
 * Keys: the arrows walk, SPACE (or the joystick button) is the button,
 * ENTER ends a turn, ESC leaves the table, 0 sits out, BACKSPACE rubs out,
 * F1 turns the sound off and on.
 *
 * Walking needs to know which arrows are held, which the BIOS doesn't say:
 * the keyboard interrupt (INT 9) is hooked to keep a table of the keys
 * that are down, then passed on to the BIOS, which still queues the typed
 * characters (read with INT 16h). On a PC the scan code is at port 60h; on
 * the PCjr the BIOS's NMI handler gathers it from the keyboard's serial
 * line and calls INT 9 with it in AL.
 *
 * The joystick: port 201h's one-shots, timed. It is used if it reads
 * centred at start.
 */
#include <dos.h>
#include <i86.h>
#include <conio.h>
#include <bios.h>
#include "mule.h"

extern uint8_t plat_pcjr;       /* plat.c */

uint8_t inp_dir, inp_btn, ev_btn, ev_key;
uint8_t inp_hook, inp_hook_dir, inp_hook_btn, inp_hook_key;

static uint8_t btn_prev, btn_latch, key_latch;

/* Scan codes (set 1). */
#define SC_ESC   0x01
#define SC_SPACE 0x39
#define SC_UP    0x48
#define SC_LEFT  0x4B
#define SC_RIGHT 0x4D
#define SC_DOWN  0x50
#define SC_KP7   0x47           /* the keypad's diagonals */
#define SC_KP9   0x49
#define SC_KP1   0x4F
#define SC_KP3   0x51

volatile uint8_t kb_down[0x60];         /* 1: the key is held (tools/dossim peeks it) */
static volatile uint8_t space_hit;      /* SPACE went down since the last poll */

static void (__interrupt __far *old_int9)(void);

uint8_t kb_last;                /* the last scan code (tools/dossim) */
static void __interrupt __far kb_isr(union INTPACK r)
{
    uint8_t sc = plat_pcjr ? r.h.al : inp(0x60);
    uint8_t k = sc & 0x7F;
    kb_last = sc;
    if (k < sizeof(kb_down)) {
        if (sc & 0x80) {
            kb_down[k] = 0;
        } else {
            kb_down[k] = 1;
            if (k == SC_SPACE)
                space_hit = 1;
        }
    }
    _chain_intr(old_int9);
}

/* ---- joystick A ----------------------------------------------------------- */
static uint8_t joy_on;
static uint16_t joy_cx, joy_cy;

/* Time both axes' one-shots (loop counts; 0xFFFF: no stick). */
static void joy_read(uint16_t *x, uint16_t *y)
{
    uint16_t n = 0, ax = 0xFFFF, ay = 0xFFFF;
    uint8_t v;
    _disable();
    outp(0x201, 0);
    do {
        v = inp(0x201);
        if (!(v & 1) && ax == 0xFFFF)
            ax = n;
        if (!(v & 2) && ay == 0xFFFF)
            ay = n;
    } while ((v & 3) && ++n < 2000);
    _enable();
    *x = ax;
    *y = ay;
}

void input_init(void)
{
    old_int9 = _dos_getvect(0x09);
    _dos_setvect(0x09, kb_isr);
    joy_read(&joy_cx, &joy_cy);
    joy_on = joy_cx != 0xFFFF && joy_cy != 0xFFFF && joy_cx > 4 && joy_cy > 4;
}

void input_restore(void)
{
    if (old_int9)
        _dos_setvect(0x09, old_int9);
}

/* The typed keys the BIOS has queued, one a frame. */
static void typed(void)
{
    uint16_t k;
    uint8_t c;
    if (key_latch != KEY_NONE || !_bios_keybrd(_KEYBRD_READY))
        return;
    k = _bios_keybrd(_KEYBRD_READ);
    c = (uint8_t)k;
    if (c == 0) {
        if ((k >> 8) == 0x3B)   /* F1 */
            snd_toggle_mute();
        return;
    }
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c == ' ')
        return;                 /* SPACE is the button (INT 9 saw it) */
    if (c == 13 || c == 27 || c == 8 || (c > 32 && c < 96))
        key_latch = c;
}

void input_poll(void)
{
    uint8_t dir = 0, btn;
    uint16_t x, y;

    typed();
    if (kb_down[SC_UP] || kb_down[SC_KP7] || kb_down[SC_KP9]) dir |= DIR_UP;
    if (kb_down[SC_DOWN] || kb_down[SC_KP1] || kb_down[SC_KP3]) dir |= DIR_DOWN;
    if (kb_down[SC_LEFT] || kb_down[SC_KP7] || kb_down[SC_KP1]) dir |= DIR_LEFT;
    if (kb_down[SC_RIGHT] || kb_down[SC_KP9] || kb_down[SC_KP3]) dir |= DIR_RIGHT;
    btn = kb_down[SC_SPACE];
    if (space_hit) {
        space_hit = 0;
        btn_latch = 1;          /* however short the press */
    }
    if (joy_on) {
        joy_read(&x, &y);
        if (x < joy_cx / 2) dir |= DIR_LEFT;
        else if (x != 0xFFFF && x > joy_cx + joy_cx / 2) dir |= DIR_RIGHT;
        if (y < joy_cy / 2) dir |= DIR_UP;
        else if (y != 0xFFFF && y > joy_cy + joy_cy / 2) dir |= DIR_DOWN;
        if ((inp(0x201) & 0x30) != 0x30)
            btn = 1;            /* either of its buttons */
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
}
