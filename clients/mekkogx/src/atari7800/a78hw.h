/*
 * a78hw.h -- the 7800 port's hardware and memory map (asm: a78.inc), and
 * what the display's modules share with C.
 *
 *   $0040-$00FF  zero page
 *   $1800-$1FFF  the console's RAM: zones 0-15's display lists, two slots each
 *   $2200-$27FF  zones 16-23's, the DLL, the DLIs' tables (maria.s, nmi.s)
 *   $4000-$5FFF  the cart's RAM: character blocks, 8 pages each (CHARBASE
 *                is the first; glyph line k of code c at + (7 - k) * 256 + c)
 *                  DYNB  the drawing cells (1-127, again at 128-255)
 *                  TXTB  the font (0-63); terrain and sprite pieces (64-255)
 *                  BIGB  big text (2-byte codes: value 1, then value 2)
 *                  MTB   the map's glyphs, or the town's characters
 *   $6000-$63BF  SCREEN: 24 rows x 40 bytes (gfx.c)
 *   $63C0-       the data, the variables; the C stack at the top
 *   $8000-$BFFF  the SuperGame window: REPL, then a bank's own (mule-a78.cfg)
 *   $C000-$FFFF  the fixed bank
 */
#ifndef A78HW_H
#define A78HW_H

#include <stdint.h>

#define ROWS     24
#define DYNB     0x4000
#define TXTB     0x4800
#define BIGB     0x5000
#define MTB      0x5800
#define SCREEN   ((uint8_t *)0x6000)
#define CB_DYN   0x40
#define CB_TXT   0x48
#define CB_BIG   0x50
#define CB_MT    0x58
#define COL_MTN  64
#define COL_RIV  72
#define COL_STORE 80
#define COL_BAR  96
#define CT_160   0x48
#define CT_320   0x4B
#define CT_CW2   0x10
#define RM_TEXT  0
#define RM_BIG   1
#define RM_DRAW  2
#define RM_MAP   3
#define RM_TOWN  4

#define MSTAT    (*(volatile uint8_t *)0x28)
#define RANDOM   (*(volatile uint8_t *)0x045A)
#define SWCHA    (*(volatile uint8_t *)0x0280)
#define CTLSWA   (*(volatile uint8_t *)0x0281)
#define SWCHB    (*(volatile uint8_t *)0x0282)
#define CTLSWB   (*(volatile uint8_t *)0x0283)
#define INPT0    (*(volatile uint8_t *)0x08)
#define INPT1    (*(volatile uint8_t *)0x09)
#define INPT4    (*(volatile uint8_t *)0x0C)

/* maria.s */
void mt_init(void);
void mt_compose(void);
void mt_blank(void);
extern uint8_t zdirty[ROWS], zflip[ROWS];
extern volatile uint8_t dsp_ready;
extern uint8_t tr_lo[ROWS * 4], tr_pw[ROWS * 4], tr_x[ROWS * 4];
extern uint8_t mt_dma[ROWS], mt_shed, mt_ntsc, mapidx[64];
/* nmi.s */
#define NDLI 26
void mt_start(void);
extern volatile uint8_t tbl_pending;
extern uint8_t nx_cb[NDLI], nx_ca[NDLI], nx_cb2[NDLI], nx_f[NDLI], nx_p[NDLI * 6];
extern uint8_t nx_dli[ROWS], nx_n, dsp_colbk, spr_pal[4], ter_pal[4];
/* bank.c */
void bank_art_on(void);
void bank_art_off(void);

#endif
