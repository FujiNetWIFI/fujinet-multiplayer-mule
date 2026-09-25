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
 */
#ifndef ATARI_H
#define ATARI_H

#include <stdint.h>

#define DYNCH   ((uint8_t *)0xA800)
#define FONT    ((uint8_t *)0xAC00)
#define MAPCH   ((uint8_t *)0xAE00)
#define TOWNCH  ((uint8_t *)0xB000)
#define SCREEN  ((uint8_t *)0xB400)
#define PMBASE_PAGE 0xB8
#define DLIST   ((uint8_t *)0xB800)
#define MISSILES ((uint8_t *)0xBB00)
#define PLAYER(n) ((uint8_t *)(0xBC00 + ((n) << 8)))

/* Hardware and OS locations. */
#define RTCLOK  (*(volatile uint8_t *)0x14)
#define CRITIC  (*(volatile uint8_t *)0x42)
#define PAL     (*(volatile uint8_t *)0xD014)
#define RANDOM  (*(volatile uint8_t *)0xD20A)
#define CONSOL  (*(volatile uint8_t *)0xD01F)

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
