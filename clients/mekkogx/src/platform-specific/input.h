/*
 * input.h -- the joystick/keyboard, reduced to what M.U.L.E. needs.
 *
 * input_poll() runs once a frame. Directions are levels (held); the button
 * and keys are latched until the game takes them with input_take(), so a
 * press made while the game waits on the network is not lost.
 */
#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

/* inp_dir bits (the Intellivision disc's, which the game logic was written
 * against). */
#define DIR_DOWN  1
#define DIR_RIGHT 2
#define DIR_UP    4
#define DIR_LEFT  8

/* Keys (ev_key). Printable characters come through as themselves (upper
 * case) for name entry. */
#define KEY_NONE   0
#define KEY_END    13       /* end the turn (RETURN / ENTER) */
#define KEY_LEAVE  27       /* leave the table (ESC) */
#define KEY_BACK   8        /* rub out (name entry) */
#define KEY_SITOUT '0'      /* declare: sit this good out */

/* What the keys are called, for the prompts (a platform's build may define
 * its own). */
#ifndef KEY_END_NAME
#define KEY_END_NAME "RETURN"
#endif
#ifndef LEAVE_KEY_NAME
#define LEAVE_KEY_NAME "ESC"        /* KEY_LEAVE */
#endif
#ifndef BUTTON_NAME
#define BUTTON_NAME "SPACE"         /* the button, at the table list */
#endif
#ifndef BACK_KEY_NAME
#define BACK_KEY_NAME "B"           /* KEY_BACK, picking letters */
#endif
#ifndef QUIT_HINT
#define QUIT_HINT "ESC: QUIT"       /* at the table list (plat_quit) */
#endif
#ifndef LOBBY_HINT
#define LOBBY_HINT "<- -> SPECIES   SPACE READY   ESC LEAVE"
#endif
/* Keys that work anywhere, under the table list's hint (none: no line) */
#if defined(COCO3)
#define KEYS_HINT "CLEAR: RGB/CMP   SHIFT-CLEAR: SOUND"
#elif defined(_CMOC_VERSION_)
#define KEYS_HINT "SHIFT-CLEAR: SOUND"
#endif

extern uint8_t inp_dir;     /* level */
extern uint8_t inp_btn;     /* level: button held */
extern uint8_t ev_btn;      /* taken: the button was pressed */
extern uint8_t ev_key;      /* taken: a key, KEY_NONE if none */

void input_init(void);
void input_poll(void);      /* every frame */
void input_take(void);      /* move the latches into ev_btn / ev_key */

/* Test hook: while inp_hook is non-zero, inp_hook_dir/inp_hook_btn/
 * inp_hook_key replace the hardware (tools/a2sim pokes them). */
extern uint8_t inp_hook, inp_hook_dir, inp_hook_btn, inp_hook_key;

#ifdef __H89__
/* A terminal's arrows keep going until stopped (src/h89/input.c): the
 * game stops them where the walk starts again (src/develop.c). */
#define INPUT_STOP
void input_stop(void);
#endif

#endif
