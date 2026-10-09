/*
 * neshw.h -- the NES as the M.U.L.E. client drives it (src/nes/).
 *
 * The cart is the FujiNet NES cartridge (fujinet-firmware pico/nes) running
 * the image as an MMC3 board (mapper 4): 128K PRG, 16K CHR-RAM, 8K WRAM at
 * $6000 (src/nes/mule-nes.cfg).
 *
 *   PRG  $8000-$9FFF  R6: a phase's overlay, the art or the strings
 *        $A000-$BFFF  R7: more of the program, never switched
 *        $C000-$FFFF  the program (fixed)
 *   CHR  three 4K pattern tables, one for each third of the screen (a
 *        bitmap: every cell its own tile, as the TMS9918A's Graphics II),
 *        and the sprites' 4K. MMC3's scanline interrupt switches the
 *        background's banks a half table at a time (crt0.s).
 *
 * The screen is the TMS9918A clients' 256 x 192, on name table rows 2..25
 * (lines 16..207); above and below, the margins show the sprites' first
 * tile (pair 0's top), which stays blank.
 */
#ifndef NESHW_H
#define NESHW_H

#include <stdint.h>

#define PPU_CTRL    (*(volatile uint8_t *)0x2000)
#define PPU_MASK    (*(volatile uint8_t *)0x2001)
#define PPU_STATUS  (*(volatile uint8_t *)0x2002)
#define PPU_SCROLL  (*(volatile uint8_t *)0x2005)
#define PPU_ADDR    (*(volatile uint8_t *)0x2006)
#define PPU_DATA    (*(volatile uint8_t *)0x2007)
#define JOY1        (*(volatile uint8_t *)0x4016)

/* MMC3 */
#define MMC3_SEL    (*(volatile uint8_t *)0x8000)
#define MMC3_BANK   (*(volatile uint8_t *)0x8001)

/* PRG banks (8K): see mule-nes.cfg */
#define BANK_OVL1   0           /* .. BANK_OVL1 + 4: OVERLAY1..5 */
#define BANK_ART    5
#define BANK_STR    6
#define BANK_R7     7

/* Select an R6 bank from C: the shadow first, so the interrupt (which
 * writes MMC3's select itself) puts it back (crt0.s). */
extern uint8_t mmc3_sel;
#pragma zpsym ("mmc3_sel")
void __fastcall__ prg_r6(uint8_t bank);

/* ---- the PPU queue (crt0.s drains it in the blank) -------------------
 * A command: op, third, PPU address (high, low), then
 *   Q_COPY  n, then n bytes            (n 1..64)
 *   Q_FILL  n, value                   (n 1..255)
 *   Q_PAINT eight (mask, plane 0 bits, plane 1 bits): a cell's 16 bytes
 *           read, the masked pixels replaced
 * third is 0..2 for an address in the background's pattern table (the
 * third it is in), Q_NOBANK otherwise. */
#define Q_COPY   1
#define Q_FILL   2
#define Q_PAINT  3
#define Q_ROW    4              /* n, then n x (mask, plane 0, plane 1): one
                                 * line of n cells (n <= 16), the address
                                 * plane 0's line in the first */
#define Q_NOBANK 0xFF

extern uint8_t ppuq[256];
extern uint8_t qhead, qtail;
#pragma zpsym ("qhead")
#pragma zpsym ("qtail")
extern volatile uint8_t ppu_direct;     /* 1: rendering off, main drains */
#pragma zpsym ("ppu_direct")
extern volatile uint8_t ppu_off;        /* set by the NMI that turned it off */
#pragma zpsym ("ppu_off")
extern volatile uint8_t spr_ovf;        /* bit 5: a ninth sprite on a line */
#pragma zpsym ("spr_ovf")
void __fastcall__ ppu_drain(uint8_t budget);    /* crt0.s: 255 = all */

/* The sprites, sent each frame: two pages, the game making the one the
 * NMI doesn't send (oam_show) and naming it in oam_next when it is whole.
 * (A page is named by its address's high byte.) */
extern uint8_t oam[256], oam2[256];
extern volatile uint8_t oam_show, oam_next;
#pragma zpsym ("oam_show")
#pragma zpsym ("oam_next")

#endif
