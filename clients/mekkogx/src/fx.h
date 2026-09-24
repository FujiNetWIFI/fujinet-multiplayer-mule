/*
 * fx.h -- scripted sprite animations (clients/intv/fx.bas).
 *
 * The Atari plays set pieces between phases: the transport ship, the
 * pirates, the pest, the acid rain cloud, the players walking into the
 * status summary... Each is a little script run by one of four actors,
 * stepped once a frame, so the pieces keep the Atari's speed.
 *
 * Script bytes (positions are colour clocks / scanlines, see gfx.h):
 *   FX_POS x+64, y+64          jump there
 *   FX_MOVE dx, dy, per, n     n steps of (dx, dy) (signed), one every per frames
 *   FX_WAIT n                  n frames
 *   FX_LOOK kind, slot, shape, frame, d2, col, col2
 *                              what it looks like: kind FK_*; sprite slot
 *                              (a ship uses slot..slot+2); d2 = frames to
 *                              the second walk frame (each move step
 *                              toggles it); col2: a ship's lower hull
 *   FX_FRAME f                 change the frame
 *   FX_FLAGS f                 change the sprite flags (SPR_FLIPX)
 *   FX_HIDE                    take its sprites off the screen (until the
 *                              next FX_POS, FX_FRAME or FX_LOOK)
 *   FX_COL c                   change the colour
 *   FX_SIG v                   fx_flag = v, for the game to see
 *   FX_LOOP n, back            repeat the `back` bytes before this n more times
 *   FX_END                     stop (the sprites stay as they are)
 */
#ifndef FX_H
#define FX_H

#include <stdint.h>

#define FX_END   0
#define FX_POS   1
#define FX_MOVE  2
#define FX_WAIT  3
#define FX_LOOK  4
#define FX_FRAME 5
#define FX_FLAGS 6
#define FX_HIDE  7
#define FX_SIG   8
#define FX_LOOP  9
#define FX_COL   10

#define FK_SINGLE 1
#define FK_SHIP   3

#define XB(x) ((uint8_t)((x) + 64))     /* a position byte */
#define SB(v) ((uint8_t)(int8_t)(v))    /* a signed byte */

typedef struct {
    uint8_t st;             /* running */
    int16_t x, y;
    uint8_t wait, n, per;
    int8_t dx, dy;
    uint8_t kind, slot, shape, frame, d2, col, col2, flags;
    uint8_t fbase;          /* added to frame (a species' first frame) */
    uint8_t alt, rep, hidden;
    const uint8_t *pc;
} Actor;

extern Actor fx[4];
extern uint8_t fx_on, fx_flag;

void fx_start(uint8_t a, const uint8_t *script);
void fx_stop_all(void);
void fx_run(void);          /* every frame */

#endif
