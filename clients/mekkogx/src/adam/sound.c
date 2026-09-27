/*
 * sound.c -- the effects on the Adam's SN76489 (port FFh).
 *
 * The table is the Apple II's (src/apple2/sound.c) as the PCjr plays it
 * (src/msdos/sound.c), the same chip at the same clock: a delay d there is
 * a tone of 36428 / d Hz, a divisor of 3579545 / 32 / f = 3.07 d, played
 * for so many frames, then maybe a second. The frames are counted in the
 * NMI (nmi.asm), so a note ends on time however long the game is busy.
 */
#include "sound.h"

__sfr __at 0xFF SN_PORT;

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
};

/* What the frame clock plays (src/adam/nmi.asm snd_frame): snd_f frames
 * of the tone sounding, then snd_nf frames of the one in snd_n (its three
 * bytes for the chip), then quiet. The NMI never touches the chip while
 * snd_f is 0, so snd_play clears it first and sets it last. */
volatile uint8_t snd_f, snd_nf;
uint8_t snd_n[3];
static uint8_t muted, vol;

/* A tone's three bytes for tone 0: the divisor, then the volume. */
static void tone_bytes(uint8_t *b, uint8_t d)
{
    uint16_t n = d * 3u + (d * 18u >> 8);          /* 3.07 d */
    if (n > 1023)
        n = 1023;
    b[0] = 0x80 | (n & 15);
    b[1] = (uint8_t)(n >> 4);
    b[2] = 0x90 | vol;                              /* attenuation */
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
    uint8_t b[3];
    if (muted || (id == SND_STEP && snd_f))
        return;                     /* a footstep never cuts another sound */
    snd_f = 0;
    vol = id == SND_ALERT ? 0 : id == SND_STEP ? 8 : 4;    /* the alert loudest */
    tone_bytes(b, e->d1);
    SN_PORT = b[0];
    SN_PORT = b[1];
    SN_PORT = b[2];
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
    quiet();
}
