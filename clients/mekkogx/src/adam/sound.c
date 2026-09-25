/*
 * sound.c -- the effects on the Adam's SN76489 (port FFh).
 *
 * The table is the Apple II's (src/apple2/sound.c) as the PCjr plays it
 * (src/msdos/sound.c), the same chip at the same clock: a delay d there is
 * a tone of 36428 / d Hz, a divisor of 3579545 / 32 / f = 3.07 d, played
 * for so many frames, then maybe a second.
 */
#include "sound.h"

__sfr __at 0xFF SN_PORT;

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

static uint8_t cur_f, next_d, next_f, muted;

static void tone(uint8_t d)
{
    uint16_t n = d * 3u + (d * 18u >> 8);          /* 3.07 d */
    if (n > 1023)
        n = 1023;
    SN_PORT = 0x80 | (n & 15);                      /* tone 0 */
    SN_PORT = (uint8_t)(n >> 4);
    SN_PORT = 0x90 | (d == 200 ? 6 : 2);            /* loud (the step soft) */
}

static void quiet(void)
{
    SN_PORT = 0x9F;
}

void snd_init(void)
{
    uint8_t v;
    for (v = 0; v < 4; ++v)
        SN_PORT = 0x9F | (v << 5);
}

void snd_play(uint8_t id)
{
    const Effect *e = &fx[id];
    if (muted || (id == SND_STEP && cur_f))
        return;                     /* a footstep never cuts another sound */
    tone(e->d1);
    cur_f = e->f1;
    next_d = e->d2;
    next_f = e->f2;
}

void snd_tick(void)
{
    if (!cur_f)
        return;
    if (--cur_f == 0) {
        if (next_f) {
            tone(next_d);
            cur_f = next_f;
            next_f = 0;
        } else {
            quiet();
        }
    }
}

void snd_toggle_mute(void)
{
    muted ^= 1;
    cur_f = 0;
    quiet();
}
