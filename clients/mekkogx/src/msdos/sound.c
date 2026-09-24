/*
 * sound.c -- the effects on the PC speaker, or on the PCjr's SN76496.
 *
 * The table is the Apple II's (src/apple2/sound.c): a delay d there is a
 * tone of 36428 / d Hz, played for so many frames, then maybe a second.
 * Here the hardware makes the square wave, so snd_tick() only moves on:
 *
 *   speaker   PIT channel 2, divisor 1193182 / f = 32.75 d, gated to the
 *             speaker by port 61h bits 0-1
 *   PCjr      the SN76496 at C0h, divisor 3579545 / 32 / f = 3.07 d, with
 *             port 61h bits 5-6 set to 11 to send it to the speaker
 */
#include <conio.h>
#include "sound.h"
#include "plat.h"

extern uint8_t plat_pcjr;       /* plat.c */

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

static uint8_t cur_f, next_d, next_f, muted, sn;

static void sn_write(uint8_t v)
{
    outp(0xC0, v);
}

static void sn_all_off(void)
{
    uint8_t v;
    for (v = 0; v < 4; ++v)
        sn_write(0x9F | (v << 5));
}

static void tone(uint8_t d)
{
    uint16_t n;
    if (sn) {
        n = d * 3u + (d * 18u >> 8);                /* 3.07 d */
        if (n > 1023)
            n = 1023;
        sn_write(0x80 | (n & 15));                  /* tone 0 */
        sn_write((uint8_t)(n >> 4));
        sn_write(0x90 | (d == 200 ? 6 : 2));        /* loud (the step soft) */
    } else {
        n = d * 32u + (d * 193u >> 8);              /* 32.75 d */
        outp(0x43, 0xB6);
        outp(0x42, n & 0xFF);
        outp(0x42, n >> 8);
        outp(0x61, inp(0x61) | 0x03);
    }
}

static void quiet(void)
{
    if (sn)
        sn_write(0x9F);
    else
        outp(0x61, inp(0x61) & ~0x03);
}

void snd_init(void)
{
    sn = plat_pcjr;
    if (sn) {
        sn_all_off();
        outp(0x61, inp(0x61) | 0x60);               /* the SN76496 to the speaker */
    }
    quiet();
}

void snd_off(void)
{
    quiet();
    cur_f = 0;
    if (sn) {
        sn_all_off();
        outp(0x61, inp(0x61) & ~0x60);
    }
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
