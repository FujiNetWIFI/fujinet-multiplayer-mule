/*
 * lxhw.h -- the Lynx port's memory map, hardware and shared declarations.
 *
 *   $A000  SCREEN  the frame buffer Mikey shows (160 x 102, 4 bits a pixel,
 *                  80 bytes a line, the left pixel in the high nibble)
 *   $C000  BACK    the background: gfx_* draw here, spr_flush copies what
 *                  changed to SCREEN and draws the sprites over it
 *   $E000          the overlay window, then the C stack (mule-lynx.cfg)
 */
#ifndef LXHW_H
#define LXHW_H

#include <stdint.h>

#define SCREEN ((uint8_t *)0xA000)
#define BACK   ((uint8_t *)0xC000)
#define LX_W   160
#define LX_H   102
#define LX_BPL 80

/* Suzy */
#define SUZY_VIDBAS   (*(volatile uint16_t *)0xFC08)
#define SUZY_SCBNEXT  (*(volatile uint16_t *)0xFC10)
#define SUZY_SPRGO    (*(volatile uint8_t *)0xFC91)
#define SUZY_SPRSYS   (*(volatile uint8_t *)0xFC92)
#define SUZY_JOYSTICK (*(volatile uint8_t *)0xFCB0)
#define SUZY_SWITCHES (*(volatile uint8_t *)0xFCB1)

/* Mikey */
#define MIKEY_TIM(n, r) (*(volatile uint8_t *)(0xFD00 + (n) * 4 + (r)))
#define MIKEY_INTRST  (*(volatile uint8_t *)0xFD80)
#define MIKEY_INTSET  (*(volatile uint8_t *)0xFD81)
#define MIKEY_DISPADR (*(volatile uint16_t *)0xFD94)
#define MIKEY_GREEN   ((volatile uint8_t *)0xFDA0)
#define MIKEY_BLUERED ((volatile uint8_t *)0xFDB0)

/* The display's coordinates (see gfx.c): an Atari line -> a Lynx line. */
extern const uint8_t row_y[24];         /* text row -> its top line */
uint8_t __fastcall__ ly(int16_t y);

/* irq.s: the VBL counts frame_count (plat.h) and runs the sound */
void vbl_init(void);

/* suzy.s: draw the sprite chain at scb into the buffer at dst */
void __fastcall__ suzy_draw(const void *scb);
extern uint16_t suzy_dst;

#endif
