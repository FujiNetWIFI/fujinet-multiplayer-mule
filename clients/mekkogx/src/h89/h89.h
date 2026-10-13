/*
 * h89.h -- the Heathkit H89's console and clock (h89io.asm), and what the
 * H89 files share among themselves.
 */
#ifndef H89_H
#define H89_H

#include <stdint.h>

#define TX_RING 64              /* h89io.asm's TXN */

uint8_t con_install(void);
void con_remove(void);
void con_put(uint8_t c) __z88dk_fastcall;
uint8_t con_free(void);
uint8_t con_flushed(void);
void con_pump(void);
int kbd_get(void);
uint16_t ticks_now(void);
void cpm_print(const char *s) __z88dk_fastcall;
void cpm_exit(void);

extern uint16_t tx_count;
extern uint8_t tx_held;

/* the 82C55 on the FujiNet card (fbport.c, plat.c) */
#define PPI_A    0xD0
#define PPI_C    0xD2
#define PPI_CTRL 0xD3

/* gfx.c: what changed out to the terminal (a held clear done first, the
 * 25th line's legend kept up) */
void gfx_sync(void);

#endif
