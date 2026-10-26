/*
 * eoscall.h -- the EOS calls the game makes (eoscall.asm): their arguments are
 * these variables.
 */
#ifndef ADAM_EOS_H
#define ADAM_EOS_H

#include <stdint.h>

extern uint16_t eos_blk;
extern void *eos_dst;
extern uint8_t eos_dev;
extern uint8_t joy_ctl[4];      /* controller 1: stick, left fire, right fire, keypad code */

uint8_t eos_read_blk(void);     /* 1K block eos_blk of eos_dev to eos_dst: 0 or an error */
void eos_kbd_start(void);       /* ask the keyboard for a character */
uint8_t eos_kbd_end(void);      /* 0 none yet, 1 failed (ask again), else the character */
void joy_read(void);            /* controller 1 into joy_ctl, read from its port */

#endif
