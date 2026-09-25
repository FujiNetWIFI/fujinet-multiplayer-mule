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

/* More of the original's sounds, for a platform that has them (the Atari:
 * its build defines SND_EXTRA and provides snd_extra()). Elsewhere SND_X()
 * and SND_XA() are nothing. snd_arg is the id's argument. */
#define SNX_TITLE   0       /* the title theme (the lobby) */
#define SNX_TUNE    1       /* the round's tune (arg: round) */
#define SNX_SHIP    2       /* the colony ship landing */
#define SNX_EVENT   3       /* a colony event (arg: CE_*) */
#define SNX_NEWS    4       /* a player event's jingle (arg: event 1-22) */
#define SNX_CHIRPS  5       /* production */
#define SNX_OUTFIT  6       /* a M.U.L.E. being outfitted */
#define SNX_TIMER   7       /* the time bar ticks (arg: what is left; 0 stops) */
#ifdef SND_EXTRA
extern uint8_t snd_arg;
void snd_extra(uint8_t id);
#define SND_X(id)     snd_extra(id)
#define SND_XA(id, a) (snd_arg = (a), snd_extra(id))
#else
#define SND_X(id)
#define SND_XA(id, a)
#endif

void snd_init(void);
void snd_play(uint8_t id);
void snd_tick(void);
void snd_toggle_mute(void);

#endif
