/*
 * plat.h -- what every platform provides besides graphics, input and sound:
 * start-up, the 60 Hz jiffy clock, randomness and quitting.
 */
#ifndef PLAT_H
#define PLAT_H

#include <stdint.h>

/* Set up the machine. Returns 0 on success, or a message to show and stop
 * on (for example "NEEDS A 128K APPLE //E, //C OR IIGS"). */
const char *plat_init(void);
void plat_quit(void);

/* Jiffies since boot (60 a second), counted by frame_wait. */
extern uint16_t frame_count;

/* Wait until a frame (vertical blank) has passed since the last call. */
void frame_wait(void);

/* Count a frame if one has begun; long work calls this now and then so the
 * count keeps up (a no-op on a platform whose clock is an interrupt). */
void frame_tick(void);

/* The network layer blocks the CPU; it tells the clock roughly how many
 * jiffies a call took so animations keep time (0 if the platform can
 * measure it itself). */
void frame_lost(uint8_t jiffies);

uint8_t plat_rand(void);

#ifdef PLAT_NET_PROBE
/* Is there a FujiNet? (for a platform whose network_init() can't tell) */
uint8_t plat_net_probe(void);
#endif

#ifdef PLAT_NET_GUARD
/* Around each network call (1 before, 0 after): a platform whose FujiNet
 * driver can hang in a call for long caps it (msdos/plat.c). */
void plat_net_guard(uint8_t on);
#define NET_GUARD(on) plat_net_guard(on)
#else
#define NET_GUARD(on)
#endif

#endif
