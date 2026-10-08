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
/* k cells from cell, all the same (on one row). */
void tile_run(uint16_t cell, uint8_t k, const uint8_t *pat, const uint8_t *col);
/* A line across a row of cells (tileline.asm; its parameters: tiles.c). */
void tile_hline(void);
extern uint16_t hl_cell;
extern const uint8_t *hl_pic;
extern uint8_t hl_n, hl_r, hl_lm, hl_rm, hl_c;

void tiles_init(void);
void tiles_clear(uint8_t bg);   /* every cell the cleared one, colour bg */
void vram_out(uint16_t a, const uint8_t *s, uint8_t n);

/* A sprite's pattern sent in its colour (sprout.asm; its parameters). */
void spr_out(void);
extern uint16_t so_addr;
extern const uint8_t *so_pat;
extern uint8_t so_col;

/* The patterns of shape's frame f, part (32 bytes), made before: 1 if so
 * (tiles.c, in the cartridge's RAM). */
uint8_t spr_cache_get(uint8_t shape, uint8_t f, uint8_t part, uint8_t *pat);
void spr_cache_put(uint8_t shape, uint8_t f, uint8_t part, const uint8_t *pat);

/* gfx.c's PLOT_MEMO: a plot (its PlotLook's 7 bytes, the background)
 * drawn before is drawn again by its tiles' names (1: done); one drawn the
 * long way is remembered. */
uint8_t plot_memo_show(uint8_t p, const uint8_t *look, uint8_t bgc);
void plot_memo_keep(uint8_t p, const uint8_t *look, uint8_t bgc);
/* A character (glyph g, colours col) written before: its tile on cell (1:
 * done); else, once the cell is stored, remembered. */
uint8_t tile_glyph(uint16_t cell, uint8_t g, uint8_t col);
void tile_glyph_keep(uint16_t cell, uint8_t g, uint8_t col);
/* gfx.c's TOWN_MEMO: the town the same way. */
uint8_t town_memo_show(uint8_t bgc);
void town_memo_keep(uint8_t bgc);

extern uint8_t tile_full, tile_live, tile_most;
extern uint8_t tiles_ok;        /* 0: the cartridge's RAM didn't answer */

#endif
