/*
 * h19.h -- the H19 terminal's screen, drawn in cells and sent by difference.
 *
 * A cell is one byte: the low seven bits are the byte the terminal is sent
 * for it in graphics mode (which is on all the time: 20h-5Dh print as
 * themselves, upper case; `, a-z, {, |, }, ~, ^ and _ are the graphics
 * characters), bit 7 is reverse video. The game draws into bg; sprites lie
 * over it; h19_flush() sends what differs from what the terminal shows
 * (have), a little at a time so the console line never backs up. Rows 0-23
 * are the screen, row 24 the 25th line.
 */
#ifndef H19_H
#define H19_H

#include <stdint.h>

#define SCR_W  80
#define SCR_H  25
#define ROW25  24
#define REV    0x80

/* the graphics characters: the byte sent (graphics mode) */
#define G_VLINE   '`'       /* vertical line */
#define G_HLINE   'a'       /* horizontal line */
#define G_CROSS   'b'
#define G_UR      'c'       /* corners: upper right */
#define G_LR      'd'       /* lower right */
#define G_LL      'e'       /* lower left */
#define G_UL      'f'       /* upper left */
#define G_ARROW_R 'h'
#define G_CHECK   'i'       /* 50% checkerboard */
#define G_ARROW_D 'k'
#define G_Q_LR    'l'       /* quadrants */
#define G_Q_LL    'm'
#define G_Q_UL    'n'
#define G_Q_UR    'o'
#define G_TOP     'p'       /* upper half (reversed: lower half) */
#define G_RIGHT   'q'       /* right half (reversed: left half) */
#define G_TRI_UL  'r'       /* filled upper-left triangle (reversed: lower-right) */
#define G_TEE_D   's'       /* tees: stem down, left, up, right */
#define G_TEE_L   't'
#define G_TEE_U   'u'
#define G_TEE_R   'v'
#define G_X       'w'       /* diagonal cross */
#define G_SLASH   'x'       /* / */
#define G_BSLASH  'y'       /* \ */
#define G_BAR_T   'z'       /* line along the top */
#define G_BAR_B   '{'       /* line along the bottom */
#define G_BAR_L   '|'
#define G_BAR_R   '}'
#define G_BULLET  '^'
#define G_TRI_UR  '_'       /* filled upper-right triangle (reversed: lower-left) */
#define G_BLOCK   (' ' | REV)

extern uint8_t *bg;         /* SCR_W x SCR_H: the screen without sprites */
extern uint8_t *rowp[SCR_H];   /* its rows */
extern uint8_t h89_idle;    /* 1: all sent (the harness waits for it) */
extern uint16_t h89_flushes;

/* Set up the terminal (cells: SCR_W x SCR_H bytes, given by plat_init)
 * and put it back. */
void h19_init(uint8_t *cells);
void h19_exit(void);

/* The screen will change a lot (a new screen): the next flush may clear it
 * first. */
void h19_cleared(void);

/* Drawing into bg. */
void h19_put(uint8_t x, uint8_t y, uint8_t c);
void h19_putn(uint8_t x, uint8_t y, const uint8_t *s, uint8_t n);   /* n cells */
void h19_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t c);
void h19_str(uint8_t x, uint8_t y, const char *s, uint8_t n, uint8_t attr);
void h19_blank(uint8_t y0, uint8_t y1);        /* rows y0..y1 */

/* A sprite: art is { w, h, cells... } (0: see through), its top-left cell
 * at c, r (signed: it may hang off the screen); art 0 takes it away. */
void h19_spr(uint8_t slot, const uint8_t *art, int8_t c, int8_t r);

/* Send some of what changed (as much as the line takes in a few frames). */
void h19_flush(void);

/* Everything again (the terminal was reset, or its screen spoilt). */
void h19_repaint(void);

void h19_bell(void);

#endif
