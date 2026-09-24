/*
 * bank.h -- memory the game can't address directly.
 *
 * Some platforms keep parts of the game out of main memory (the Apple II:
 * in auxiliary RAM) and bring them in when needed:
 *   - overlays: a phase's code, swapped into one window;
 *   - far data: the strings and pictures, copied in a piece at a time.
 * On a platform with room for everything these are no-ops / memcpy.
 */
#ifndef BANK_H
#define BANK_H

#include <stdint.h>

#define OV_NONE    0
#define OV_BOOT    1        /* boot.c: name, table list */
#define OV_STATUS  2        /* status.c: lobby, summary, game over */
#define OV_GRANT   3        /* grant.c, events.c: land grant, production */
#define OV_AUCTION 4        /* auction.c */
#define OV_DEVELOP 5        /* develop.c */

/* Load what lives out of reach; returns 0 or an error message. */
const char *bank_init(void);

/* Make overlay n the one in the window (no-op if it is already). */
void bank_overlay(uint8_t n);
uint8_t bank_current(void);

/* Copy n bytes of far data at src to dst. */
void bank_get(void *dst, const void *src, uint16_t n);

#endif
