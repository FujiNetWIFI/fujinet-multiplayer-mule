/*
 * sound.c -- the effects on the Apple II speaker.
 *
 * The speaker only clicks while the CPU times it, so an effect is played a
 * slice at a time: snd_tick() spends at most SLICE of each frame on it. The
 * tones are the Intellivision client's (clients/intv/sound.bas: PSG period
 * v -> 111861 / v Hz), turned into snd_tone delays (1020000 / (28 * d) Hz,
 * so d = v * 0.33).
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
    {  42,  4,   0,  0 },   /* select: v 127 */
    { 132,  6,   0,  0 },   /* sample: v 400 */
    { 255, 12,   0,  0 },   /* error: v 900 (as low as it goes) */
    {  33,  5,  25,  8 },   /* buy: 100 then 75 */
    {  56, 10,  42, 14 },   /* my turn: 170, 127 */
    {  70,  8,  53,  8 },   /* declare: 213, 160 */
    {  53, 10,  70, 16 },   /* auction over: 160, 213 */
    {  30,  3,   0,  0 },   /* trade: 90 */
    {  83,  2,   0,  0 },   /* tick: 250 */
    { 165, 20, 231, 20 },   /* event: 500, 700 */
    {  70, 20,  35, 40 },   /* founder: 213, 106 */
    { 106, 10,  70, 20 },   /* boot: 320, 213 */
    { 200,  1,   0,  0 },   /* step: a soft click */
};

static uint8_t cur_d, cur_f, next_d, next_f, muted;

void snd_init(void)
{
    cur_f = next_f = 0;
}

void snd_play(uint8_t id)
{
    const Effect *e = &fx[id];
    if (muted || (id == SND_STEP && cur_f))
        return;                     /* a footstep never cuts another sound */
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
