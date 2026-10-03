/*
 * fbport.h -- the wire under src/nstrans_fb.c: bytes to and from a FujiNet
 * that speaks FujiBus (SLIP-framed packets), without waiting on it.
 *
 *   fb_putc(c)   a byte toward the FujiNet (a request is written whole)
 *   fb_ready()   non-zero when a reply byte waits
 *   fb_getc()    that byte
 *   fb_sent()    the request is all put: let it go
 */
#ifndef FBPORT_H
#define FBPORT_H

#include <stdint.h>

#ifdef __MSX__
/* The cartridge's registers (page 2 of its slot): the FujiNet keeps what it
 * sends in a ring until it is read, so a reply can wait for the next frame. */
#define FB_GETC   (*(volatile uint8_t *)0xBFFC)
#define FB_STATUS (*(volatile uint8_t *)0xBFFD)
#define FB_PUTC   (*(volatile uint8_t *)0xBFFE)
#define fb_putc(c) (FB_PUTC = (c))
#define fb_ready() (FB_STATUS & 0x80)
#define fb_getc()  (FB_GETC)
#define fb_sent()
#else
/* MS-DOS: the UART, through rings its interrupt fills and empties
 * (src/msdos/fbport.c) */
void fb_putc(uint8_t c);
uint8_t fb_ready(void);
uint8_t fb_getc(void);
void fb_sent(void);
#endif

#endif
