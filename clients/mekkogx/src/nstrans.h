/*
 * nstrans.h -- the bytes under stream mode (src/nsnet.c): each platform
 * that uses it has an nstrans.c of its own.
 */
#ifndef NSTRANS_H
#define NSTRANS_H

#include <stdint.h>

#define NT_LOST 255

uint8_t nt_detect(void);                        /* is there a FujiNet? */
uint8_t nt_open(const char *host, uint16_t port);   /* 1: the link is up */
uint8_t nt_close(void);                         /* 1: closed (it can open again) */
uint8_t nt_get(char *c);                        /* 1: a byte; 0: none yet; NT_LOST */
uint8_t nt_put(uint8_t c);                      /* 0: it can't go (the frame is lost) */
void nt_flush(void);                            /* a frame is complete: send it */

#endif
