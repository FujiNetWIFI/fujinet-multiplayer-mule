/*
 * sound.c -- the H89's one sound: the H19's bell (BEL), a fixed beep. It
 * is rung for SND_ALERT, the call to your hand at every moment the game
 * wants your input; the other effects are silent (src/platform-specific/
 * sound.h). f1 turns it off and on.
 */
#include "sound.h"
#include "h19.h"

static uint8_t muted;

void snd_init(void)
{
}

void snd_play(uint8_t id)
{
    if (id == SND_ALERT && !muted)
        h19_bell();
}

void snd_tick(void)
{
}

void snd_toggle_mute(void)
{
    muted ^= 1;
}
