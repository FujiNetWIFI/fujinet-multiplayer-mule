/*
 * atari.h -- the Atari port's memory map and shared declarations.
 *
 * Graphics live at the top of RAM (mule-atari.cfg, __GFX_START__):
 *   $A800  DYNCH   the drawing charset (mode 4 cells made on the fly)
 *   $AC00  FONT    the original's font, its 64 characters (modes 2 and 6)
 *   $AE00  MAPCH   the original's map characters, 64 (mode 6)
 *   $B000  TOWNCH  the original's town characters (mode 4)
 *   $B400  SCREEN  24 rows x 40 bytes (a mode 6 row uses the first 20)
 *   $B800  PMG     PMBASE: $B800-$BAFF display list and DLI tables (PMGFREE),
 *                  $BB00 missiles, $BC00-$BFFF players 0-3
 *
 * The 5200 (src/atari5200) has the same at $2800-$3FFF, the top of its
 * 16K, and GTIA and POKEY elsewhere (hw.inc is the assembly's copy).
 */
#ifndef ATARI_H
#define ATARI_H

#include <stdint.h>

#ifdef __ATARI5200__
#define GFX_BASE 0x2800
#define GTIA_BASE 0xC000
#define POKEY_BASE 0xE800
#else
#define GFX_BASE 0xA800
#define GTIA_BASE 0xD000
#define POKEY_BASE 0xD200
#endif

#define DYNCH   ((uint8_t *)GFX_BASE)
#define FONT    ((uint8_t *)(GFX_BASE + 0x0400))
#define MAPCH   ((uint8_t *)(GFX_BASE + 0x0600))
#define TOWNCH  ((uint8_t *)(GFX_BASE + 0x0800))
#define SCREEN  ((uint8_t *)(GFX_BASE + 0x0C00))
#define PMBASE_PAGE ((GFX_BASE + 0x1000) >> 8)
#define DLIST   ((uint8_t *)(GFX_BASE + 0x1000))
#define MISSILES ((uint8_t *)(GFX_BASE + 0x1300))
#define PLAYER(n) ((uint8_t *)(GFX_BASE + 0x1400 + ((n) << 8)))

/* Hardware and OS locations (the 5200's BIOS keeps its clock and shadows
 * in zero page: its crt0's VBI copies them). */
#ifdef __ATARI5200__
#define RTCLOK  (*(volatile uint8_t *)0x02)
#define SDMCTL  (*(volatile uint8_t *)0x07)
#define SDLSTL  (*(volatile uint16_t *)0x05)
#else
#define RTCLOK  (*(volatile uint8_t *)0x14)
#define CRITIC  (*(volatile uint8_t *)0x42)
#define SDMCTL  (*(volatile uint8_t *)0x022F)
#define SDLSTL  (*(volatile uint16_t *)0x0230)
#endif
#define PAL     (*(volatile uint8_t *)(GTIA_BASE + 0x14))
#define CONSOL  (*(volatile uint8_t *)(GTIA_BASE + 0x1F))
#define GRACTL  (*(volatile uint8_t *)(GTIA_BASE + 0x1D))
#define PMBASE  (*(volatile uint8_t *)0xD407)
#define RANDOM  (*(volatile uint8_t *)(POKEY_BASE + 0x0A))

/* The display (gfxa.s): 24 rows of 8 scanlines. Each row's DLI (the
 * previous line's) loads what differs from the row before: */
#define ROWS 24
#define RF_PF   1       /* CHBASE, COLPF0-3 */
#define RF_HPOS 2       /* HPOSP0-3 */
#define RF_COL  4       /* COLPM0-3 */
#define RF_SIZE 8       /* SIZEP0-3 */
extern uint8_t row_flags[ROWS];
extern uint8_t row_chb[ROWS];
extern uint8_t row_pf[4 * ROWS];     /* [i * ROWS + row] */
extern uint8_t row_hp[4 * ROWS];
extern uint8_t row_cm[4 * ROWS];
extern uint8_t row_sz[4 * ROWS];
extern uint8_t dsp_colbk, dsp_prior, dsp_sizem, dsp_hposm[4];
extern volatile uint8_t dsp_ready;      /* 0 while the tables are rewritten */
void dsp_init(void);                    /* gfxa.s: DLI + immediate VBI */

/* sound.c: run by the VBI */
void snd_vbi(void);

#endif
