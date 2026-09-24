/*
 * sound.h -- the game's sound effects. A platform plays them however it can;
 * snd_tick() runs once a frame for effects that last several frames.
 */
#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

#define SND_SELECT       0
#define SND_SAMPLE       1
#define SND_ERROR        2
#define SND_BUY          3
#define SND_MYTURN       4
#define SND_DECLARE      5
#define SND_AUCTION_OVER 6
#define SND_TRADE        7
#define SND_TICK         8
#define SND_EVENT        9
#define SND_FOUNDER      10
#define SND_BOOT         11
#define SND_STEP         12     /* a footstep: only when nothing else sounds */
#define SND_COUNT        13

void snd_init(void);
void snd_play(uint8_t id);
void snd_tick(void);
void snd_toggle_mute(void);

#endif
