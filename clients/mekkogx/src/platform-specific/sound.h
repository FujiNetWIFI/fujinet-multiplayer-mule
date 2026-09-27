/*
 * sound.h -- the game's sound effects: short blips, no music. SND_ALERT is
 * the one that matters, rung whenever the game wants your input (your turn,
 * the land grant, an auction, the summary, the lobby's countdown): it is
 * the only two-note sound and the loudest. A platform plays them however it
 * can; snd_tick() runs once a frame for effects that last several frames.
 */
#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

#define SND_SELECT       0
#define SND_SAMPLE       1
#define SND_ERROR        2
#define SND_BUY          3
#define SND_ALERT        4      /* your hand to the stick: the loud one */
#define SND_AUCTION_OVER 5
#define SND_TRADE        6
#define SND_EVENT        7
#define SND_FOUNDER      8
#define SND_STEP         9      /* a footstep: only when nothing else sounds */
#define SND_COUNT        10

void snd_init(void);
void snd_play(uint8_t id);
void snd_tick(void);
void snd_toggle_mute(void);

#endif
