/*
 * sound.c -- the effects on the CoCo's 1-bit sound output (PIA1 B bit 1).
 *
 * As on the Apple II (src/apple2/sound.c, whose table this is), the CPU
 * times the square wave, so an effect is played a slice at a time:
 * snd_tick() spends about SLICE_CYCLES of each frame on it. The table's
 * delays d give 36428 / d Hz; snd_tone's half period is 8 cycles a count
 * at 0.895 MHz, so the count is 1.5 d.
 */
#include "sound.h"
#include "plat.h"

#define SLICE_CYCLES 2400u

/* n half-period pairs of the square wave, each half `count` loops. */
static void snd_tone(uint16_t count, uint8_t n)
{
    asm {
        pshs    u
        ldu     :count
        ldb     :n
@wave
        lda     $FF22
        ora     #$02
        sta     $FF22
        tfr     u,x
@hi     leax    -1,x
        bne     @hi
        lda     $FF22
        anda    #$FD
        sta     $FF22
        tfr     u,x
@lo     leax    -1,x
        bne     @lo
        decb
        bne     @wave
        puls    u
    }
}

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
    /* PB1 (the 1-bit sound) an output; the analog sound (the DAC, which the
     * joystick reads use) off, or every stick read would click */
    asm {
        lda     $FF23
        anda    #$FB            ; the data direction register
        sta     $FF23
        lda     $FF22
        ora     #$02
        sta     $FF22
        lda     $FF23
        ora     #$04            ; back to the data register
        anda    #$F7            ; sound mux off
        sta     $FF23
    }
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
    n = (uint8_t)(SLICE_CYCLES / (24u * cur_d));   /* a wave is ~24 d cycles */
    if (n == 0)
        n = 1;
    snd_tone(cur_d + (cur_d >> 1), n);
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

