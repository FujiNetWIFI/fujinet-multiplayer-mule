/*
 * plat.c -- the Heathkit H89 (Heath/Zenith CP/M 2.2.04, the H19 terminal,
 * the H89-ESP32 FujiNet card): start-up, the frame clock and quitting.
 *
 * The clock is the BIOS's 2 ms interrupt, counted in 60ths of a second by
 * h89io.asm. The screen's two cell buffers take the memory past the
 * program's variables, up to the stack under the BDOS (the CCP's memory
 * too: we leave by a warm boot, which loads it again).
 */
#include <string.h>
#include <arch/z80.h>
#include "mule.h"
#include "h19.h"
#include "h89.h"

#define STACK_ROOM 384             /* (192 measured, live and walking) */

static uint16_t seed = 0x1234;
extern uint8_t _BSS_END_tail[];

const char *plat_init(void)
{
    uint16_t top = *(uint16_t *)6, end = (uint16_t)_BSS_END_tail;
    if (top < end + STACK_ROOM + SCR_W * SCR_H) {
        cpm_print("M.U.L.E. NEEDS MORE MEMORY: A 62K OR 64K CP/M\r\n$");
        cpm_exit();
    }
    if (!con_install()) {
        cpm_print("M.U.L.E.: THIS BIOS IS NOT ONE IT KNOWS\r\n$");
        cpm_exit();
    }
    h19_init(_BSS_END_tail);
#if H89_PROF
    /* (a profiling build: the room under the stack marked, so the harness
     * can see how deep the stack went) */
    {
        uint8_t *p = _BSS_END_tail + SCR_W * SCR_H, *sp = (uint8_t *)&top;
        while (p < sp - 64)
            *p++ = 0x5A;
    }
#endif
    gfx_init();
    return 0;
}

void plat_quit(void)
{
    h19_exit();
    con_remove();
    cpm_exit();
}

/* The card's 82C55: port C's EN (bit 2) low is an 82C55 whose ESP32 isn't
 * started yet (fujinet-lib starts it); high, the direction bit (PC0) must
 * follow a bit set/reset, or there is no 82C55 (an empty port reads FFh,
 * and fujinet-lib would wait on it for ever). ESC on the "no FujiNet"
 * screen goes back to CP/M. */
uint8_t plat_net_probe(void)
{
    uint8_t c = z80_inp(PPI_C), ok;
    int k;
    while ((k = kbd_get()) >= 0)
        if (k == 27)
            plat_quit();
    if (!(c & 0x04))
        return 1;
    z80_outp(PPI_CTRL, 0x01);       /* PC0 set */
    ok = z80_inp(PPI_C) & 0x01;
    z80_outp(PPI_CTRL, 0x00);       /* PC0 reset (ESP32 to H89) */
    return ok && !(z80_inp(PPI_C) & 0x01);
}

/* Wait until at least one frame has passed since the last call, sending
 * to the terminal meanwhile. */
uint8_t h89_stall;              /* most frames gone by between two waits
                                 * (the harness reads and clears it) */
#if H89_PROF
extern uint16_t h89_prof[256];
extern uint8_t h89_prof_at;
uint16_t h89_stall_pcs[256];
uint8_t h89_stall_at;
#endif

void frame_wait(void)
{
    static uint16_t seen;
    uint16_t d;
    while (*(volatile uint16_t *)&frame_count == seen)
        con_pump();
    d = frame_count - seen;
    if (d > h89_stall && d < 256) {
        h89_stall = (uint8_t)d;
#if H89_PROF
        /* (a profiling build: what ran in the worst stall, for the harness) */
        memcpy(h89_stall_pcs, h89_prof, sizeof(h89_stall_pcs));
        h89_stall_at = h89_prof_at;
#endif
    }
    seen = frame_count;
    seed += frame_count;
    gfx_sync();
}

void frame_tick(void)
{
    con_pump();
}

void frame_lost(uint8_t jiffies)
{
    (void)jiffies;              /* the interrupt never misses one */
}

uint8_t plat_rand(void)
{
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return (uint8_t)seed;
}
