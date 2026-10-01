/*
 * nstrans.h -- the bytes under stream mode (src/nsnet.c): src/nstrans_fn.c
 * (an N:TCP socket through fujinet-lib) or a platform's own nstrans.c.
 */
#ifndef NSTRANS_H
#define NSTRANS_H

#include <stdint.h>

#define NT_LOST 255
#define NT_DROP 254             /* nt_get: a read failed and nt_dropped
                                 * bytes the FujiNet had are gone */

extern uint16_t nt_dropped;

uint8_t nt_detect(void);                        /* is there a FujiNet? */
uint8_t nt_open(const char *host, uint16_t port);   /* 1: the link is up */
uint8_t nt_close(void);                         /* 1: closed (it can open again) */
uint8_t nt_get(char *c);                        /* 1: a byte; 0: none yet; NT_LOST, NT_DROP */
uint8_t nt_put(uint8_t c);                      /* 0: it can't go (the frame is lost) */
uint8_t nt_flush(void);                         /* a frame is complete: send it; 0: it didn't go */

#endif
