/*
 * vdp.h -- the TMS9918A, as the Adam and the MSX both have it.
 *
 *   Adam  ports BEh (data) and BFh (control). The VDP's interrupt is the
 *         Z80's NMI, which counts the frames (src/adam/nmi.asm). Reading
 *         the status in it would reset the address latch under a burst,
 *         so VDP_LOCK/VDP_UNLOCK tell it to wait (vdp_busy).
 *   MSX   ports 98h and 99h. The BIOS's interrupt reads the status register
 *         (which resets the address latch) sixty times a second and
 *         fujinet-lib times out on its JIFFY count, so the interrupt stays
 *         on: VDP_LOCK/VDP_UNLOCK keep it out of an address + data burst.
 *   SMS   ports BEh and BFh, as the Adam's, but its own mode (Mode 4: see
 *         src/sms/vram_sms.h). The frame interrupt (z88dk's crt0) reads
 *         the status as the MSX's does: the same VDP_LOCK/VDP_UNLOCK.
 *
 * Accesses are 8 us apart at the least (in Graphics II, while the screen is
 * drawn): the C loops that do them are slower than that.
 */
#ifndef VDP_H
#define VDP_H

#include <stdint.h>

#ifdef __ADAM__
__sfr __at 0xBE VDP_DATA;
__sfr __at 0xBF VDP_CTRL;
/* The NMI can't be masked: while vdp_busy is set it leaves the status
 * alone and says so in vdp_pend, and VDP_UNLOCK reads it then. */
extern volatile uint8_t vdp_busy, vdp_pend;
void vdp_catch_up(void);
#define VDP_LOCK()   (vdp_busy = 1)
#define VDP_UNLOCK() do { vdp_busy = 0; if (vdp_pend) vdp_catch_up(); } while (0)
#define VDP_R1_IE 0x20          /* the interrupt: on (the NMI, src/adam/nmi.asm) */
#elif defined(__SMS__)
__sfr __at 0xBE VDP_DATA;
__sfr __at 0xBF VDP_CTRL;
#define VDP_LOCK()   __asm__("di")
#define VDP_UNLOCK() __asm__("ei")
#define VDP_R1_IE 0x20          /* the frame interrupt: on (src/sms/smsint.asm) */
#else
__sfr __at 0x98 VDP_DATA;
__sfr __at 0x99 VDP_CTRL;
#define VDP_LOCK()   __asm__("di")
#define VDP_UNLOCK() __asm__("ei")
#define VDP_R1_IE 0x20          /* the interrupt: on (the BIOS's) */
#endif

/* VRAM (Graphics II, a sequential name table: a 256 x 192 bitmap). */
#define VR_PAT   0x0000         /* patterns: cell row * 256 + cell * 8 + line */
#define VR_NAME  0x1800
#define VR_SAT   0x1B00         /* sprite attributes */
#define VR_COL   0x2000         /* colours, laid out as the patterns */
#define VR_SPAT  0x3800         /* sprite patterns (16 x 16: 32 bytes each) */

#define VDP_WADDR(a) (VDP_CTRL = (uint8_t)(a), VDP_CTRL = (uint8_t)((a) >> 8) | 0x40)
#define VDP_RADDR(a) (VDP_CTRL = (uint8_t)(a), VDP_CTRL = (uint8_t)((a) >> 8) & 0x3F)
#define VDP_REG(r, v) (VDP_CTRL = (uint8_t)(v), VDP_CTRL = 0x80 | (r))

/* The status bits seen by the platform's frame clock since spr_flush last
 * looked (bit 6: a fifth sprite on a line). */
extern uint8_t vdp_seen;

#endif
