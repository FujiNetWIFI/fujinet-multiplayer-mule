/*
 * tiles.h -- the SMS's screen (tiles.c): Mode 4 tiles standing in for the
 * TMS9918A's cells, and its VRAM.
 */
#ifndef TILES_H
#define TILES_H

#include <stdint.h>

#define SV_SPR   0x2000         /* sprite tiles (256 + n) */
#define SV_NAME  0x3800         /* names: 2 bytes a cell, 32 a row */
#define SV_SAT   0x3F00         /* sprite y's; x and tile pairs at +128 */

/* Cell (cy * 32 + cx): its 8 pattern and 8 colour bytes, as Graphics II
 * has them. */
void tile_store(uint16_t cell, const uint8_t *pat, const uint8_t *col);
void tile_load(uint16_t cell, uint8_t *pat, uint8_t *col);

void tiles_init(void);
void tiles_clear(uint8_t bg);   /* every cell the cleared one, colour bg */
void vram_out(uint16_t a, const uint8_t *s, uint8_t n);

extern uint8_t tile_full, tile_live, tile_most;

#endif
