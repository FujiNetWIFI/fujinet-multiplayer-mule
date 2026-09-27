/*
 * sound.c -- the effects on the Apple II speaker.
 *
 * The speaker only clicks while the CPU times it, so an effect is played a
 * slice at a time: snd_tick() spends at most SLICE of each frame on it. A
 * delay d is a tone of 1020000 / (28 * d) = 36428 / d Hz; every client
 * plays this table. No volume here: the alert stands out as the only
 * effect with two notes.
 */
#ifdef __APPLE2__

#include <stdint.h>
#include "sound.h"
#include "plat.h"

void snd_tone(uint8_t delay, uint8_t n);   /* lowlevel.s */

/* ~6 ms of each frame: a slice's clicks = 6000 / (14 * delay). */
#define SLICE_CYCLES 6000u

typedef struct {
    uint8_t d1, f1;         /* delay, frames */
    uint8_t d2, f2;         /* then a second tone (f2 = 0: none) */
} Effect;

static const Effect fx[SND_COUNT] = {
    {  42,  2,   0,  0 },   /* select */
    { 132,  2,   0,  0 },   /* sample */
    { 255,  4,   0,  0 },   /* error (as low as it goes) */
    {  33,  2,   0,  0 },   /* buy */
    {  35,  5,  25,  8 },   /* alert: the only two notes, and the loudest */
    {  70,  3,   0,  0 },   /* auction over */
    {  30,  1,   0,  0 },   /* trade */
    { 165,  4,   0,  0 },   /* event */
    {  47,  5,   0,  0 },   /* founder */
    {   0,  0,   0,  0 },   /* step: none, the CPU would pay every stride */
};

static uint8_t cur_d, cur_f, next_d, next_f, muted;

void snd_init(void)
{
    cur_f = next_f = 0;
}

void snd_play(uint8_t id)
{
    const Effect *e = &fx[id];
    if (muted || !e->f1)
        return;                     /* no footsteps here */
    cur_d = e->d1;
    cur_f = e->f1;
    next_d = e->d2;
    next_f = e->f2;
}

void snd_tick(void)
{
    uint8_t n;
    if (!cur_f)
        return;
    /* two halves, with a look at the clock between (a blank lasts ~4 ms) */
    n = (uint8_t)(SLICE_CYCLES / 2 / (14u * cur_d));
    if (n == 0)
        n = 1;
    snd_tone(cur_d, n);
    frame_tick();
    snd_tone(cur_d, n);
    frame_tick();
    if (--cur_f == 0 && next_f) {
        cur_d = next_d;
        cur_f = next_f;
        next_f = 0;
    }
}

void snd_toggle_mute(void)
{
    muted ^= 1;
    cur_f = 0;
}

#endif
