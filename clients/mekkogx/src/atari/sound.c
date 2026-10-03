/*
 * sound.c -- the effects on POKEY's voice 1 (the Lynx: src/lynx/sound.c).
 *
 * The table is the Apple II's (src/apple2/sound.c): a delay d there is a
 * tone of 36428 / d Hz, which at POKEY's 64 kHz clock (63921 / 2 (N + 1)
 * Hz) is an AUDF of 0.877 d - 1. snd.s plays it in the VBI, so a tone ends
 * on time however long the game is busy.
 */
#if defined(__ATARI__) || defined(__LYNX__)

#include "sound.h"

extern volatile uint8_t snd_f;          /* snd.s */
extern uint8_t snd_fq, snd_c, snd_nf, snd_nfq;

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
    {  13,  2,   0,  0 },   /* tick: AUDF $0A, as the original's */
    {  19,  2,   0,  0 },   /* tock: AUDF $0F */
};

static uint8_t muted, soft;

static uint8_t audf(uint8_t d)
{
    return (uint8_t)((d * 225u >> 8) - 1);          /* 0.877 d - 1 */
}

void snd_init(void)
{
    snd_nf = 0;
    snd_f = 0;
}

void snd_play(uint8_t id)
{
    const Effect *e = &fx[id];
    if (muted || (SND_SOFT(id) && (snd_f || snd_nf) && !soft))
        return;                     /* a footstep or tick cuts only its own */
    soft = SND_SOFT(id);
    /* the VBI plays nothing new while snd_f is 0: it goes last */
    snd_nf = 0;
    snd_f = 0;
    snd_fq = audf(e->d1);
    snd_c = 0xA0 | (id == SND_ALERT ? 15 : soft ? 5 : 10);
    snd_nfq = audf(e->d2);
    snd_nf = e->f2;
    snd_f = e->f1;
}

/* The VBI keeps time. */
void snd_tick(void)
{
}

void snd_toggle_mute(void)
{
    muted ^= 1;
    snd_nf = 0;
    snd_f = 0;
}

#endif
