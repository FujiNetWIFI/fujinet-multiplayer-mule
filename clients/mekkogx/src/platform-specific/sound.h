/*
 * sound.h -- the game's sound effects: short blips, no music. SND_ALERT is
 * the one that matters, rung whenever the game wants your input (your turn,
 * the land grant, an auction, the summary, the lobby's countdown): it is
 * the only two-note sound and the loudest. A platform plays them however it
 * can; snd_tick() runs once a frame for effects that last several frames.
 * The one sound that repeats is the time bar's tick as time runs out.
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
#define SND_TICK         10     /* the time bar's tick, high (map.c bar_tick) */
#define SND_TOCK         11     /* and low: they alternate, as the original's */
#define SND_COUNT        12

/* Ids from SND_STEP up are soft: they never cut another sound, only each
 * other. */
#define SND_SOFT(id)     ((id) >= SND_STEP)

void snd_init(void);
void snd_play(uint8_t id);
void snd_tick(void);
void snd_toggle_mute(void);

#endif
