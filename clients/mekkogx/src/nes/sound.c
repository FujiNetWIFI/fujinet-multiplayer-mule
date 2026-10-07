/*
 * sound.c -- the effects on the NES's first pulse channel.
 *
 * The table is the Adam's (src/adam/sound.c), from the Apple II's: a delay
 * d there is a tone of 36428 / d Hz, and the pulse channel's timer for f Hz
 * is 1789773 / 16 / f - 1, so 3.07 d - 1. A tone sounds so many frames,
 * then maybe a second; the frames are counted in the NMI (crt0.s
 * snd_frame_nmi), so a note ends on time however long the game is busy.
 */
#include "sound.h"

#define APU_PULSE1 ((volatile uint8_t *)0x4000)
#define APU_STATUS (*(volatile uint8_t *)0x4015)

typedef struct {
    uint8_t d1, f1;         /* delay, frames */
    uint8_t d2, f2;         /* then a second tone (f2 = 0: none) */
} Effect;

static const Effect fx[SND_COUNT] = {
    {  42,  3,   0,  0 },   /* select */
    { 132,  3,   0,  0 },   /* sample */
    { 255,  8,   0,  0 },   /* error (as low as it goes) */
    {  33,  4,   0,  0 },   /* buy */
    {  35,  8,  25, 14 },   /* alert: the only two notes, and the loudest */
    {  70,  6,   0,  0 },   /* auction over */
    {  30,  2,   0,  0 },   /* trade */
    { 165,  8,   0,  0 },   /* event */
    {  47, 10,   0,  0 },   /* founder */
    { 200,  1,   0,  0 },   /* step: a soft click */
    {  13,  2,   0,  0 },   /* tick: the time bar running out (map.c) */
    {  18,  2,   0,  0 },   /* tock */
};

/* What the NMI plays: snd_f frames of the tone sounding, then snd_nf
 * frames of the one in snd_n (timer low, timer high, volume), then quiet.
 * The NMI never touches the channel while snd_f is 0, so snd_play clears
 * it first and sets it last. */
volatile uint8_t snd_f, snd_nf;
uint8_t snd_n[3];
static uint8_t muted, vol, soft;

/* A tone's three bytes: the timer, then duty 50% and the volume. */
static void tone_bytes(uint8_t *b, uint8_t d)
{
    uint16_t t = d * 3u + (d * 18u >> 8) - 1;      /* 3.07 d - 1 */
    b[0] = (uint8_t)t;
    b[1] = (uint8_t)(t >> 8) | 0x08;                /* (a length, so it plays) */
    b[2] = 0xB0 | vol;                              /* 50%, constant volume */
}

void snd_init(void)
{
    APU_STATUS = 0x01;          /* pulse 1 on */
    APU_PULSE1[0] = 0x30;
    APU_PULSE1[1] = 0x08;       /* no sweep */
}

void snd_play(uint8_t id)
{
    const Effect *e = &fx[id];
    uint8_t b[3];
    if (muted || (SND_SOFT(id) && (snd_f || snd_nf) && !soft))
        return;                     /* a footstep or tick cuts only its own */
    soft = SND_SOFT(id);
    snd_f = 0;
    vol = id == SND_ALERT ? 15 : soft ? 5 : 10;     /* the alert loudest */
    tone_bytes(b, e->d1);
    APU_PULSE1[0] = b[2];
    APU_PULSE1[2] = b[0];
    APU_PULSE1[3] = b[1];
    snd_nf = 0;
    if (e->f2) {
        tone_bytes(snd_n, e->d2);
        snd_nf = e->f2;
    }
    snd_f = e->f1;
}

/* The NMI keeps time (a blocked main loop mustn't hold a note). */
void snd_tick(void)
{
}

void snd_toggle_mute(void)
{
    snd_f = 0;
    muted ^= 1;
    APU_PULSE1[0] = 0x30;
}
