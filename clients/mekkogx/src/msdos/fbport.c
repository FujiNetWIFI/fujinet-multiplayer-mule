/*
 * fbport.c -- MS-DOS: the serial port FUJINET.SYS talks to, used directly
 * for stream mode's packets (src/nstrans_fb.c, src/fbport.h).
 *
 * FUJINET.SYS polls the UART with its interrupt off, inside each INT F5h
 * call: the game waits out every round trip (25 ms a status at 9600 bps,
 * more with the network's delay). Here the UART's interrupt
 * (uartisr.asm) sends a request and gathers the reply, and the game looks
 * at it on the next frame.
 *
 * The driver's settings aren't visible from here (they are its CONFIG.SYS
 * line), so the port is found by what it left: 8 bits, no parity (LCR 03h),
 * DTR, RTS and OUT2 on (MCR 0Bh), no interrupts (IER 0). Each port that
 * looks so is tried with a STATUS (fb_try); the first that answers is
 * kept. Its IRQ is the usual one: 4 for 3F8h/3E8h, 3 for 2F8h/2E8h.
 *
 * FUJINET.SYS still owns the port for everything else (opening the
 * socket, appkeys, MULE.EXE's overlays): the UART's interrupt is off
 * between our calls (fb_release), and INT 21h waits for the one going to
 * finish first, so a DOS read never finds our bytes on the wire.
 */
#if defined(PLAT_NETSTREAM) && defined(PLAT_FUJIBUS)

#include <dos.h>
#include <conio.h>
#include <i86.h>
#include "mule.h"
#include "nstrans.h"
#include "fbport.h"

/* uartisr.asm */
extern uint16_t fb_base;
extern volatile uint8_t fb_rhead, fb_rtail, fb_thead, fb_ttail, fb_lsr;
extern uint8_t fb_rring[256], fb_tring[64];
extern void __far fb_isr(void);
#pragma aux fb_base "_*"
#pragma aux fb_rhead "_*"
#pragma aux fb_rtail "_*"
#pragma aux fb_thead "_*"
#pragma aux fb_ttail "_*"
#pragma aux fb_lsr "_*"
#pragma aux fb_rring "_*"
#pragma aux fb_tring "_*"
#pragma aux fb_isr "_*"

#define IER 1
#define IIR 2
#define FCR 2
#define LCR 3
#define MCR 4
#define LSR 5

static uint8_t vec, mask_bit, mask_old, fifo_on;
static void (__interrupt __far *old_irq)(void);
static void (__interrupt __far *old_int21)(void);
static uint8_t in_dos;

uint16_t getss(void);
#pragma aux getss = "mov ax, ss" value [ax];

/* (A request longer than the ring starts going, and waits for room.) */
void fb_putc(uint8_t c)
{
    uint8_t h = (fb_thead + 1) & 63;
    if (h == fb_ttail) {
        fb_sent();
        while (h == fb_ttail)
            ;
    }
    fb_tring[fb_thead] = c;
    fb_thead = h;
}

uint8_t fb_ready(void)
{
    return fb_rhead != fb_rtail;
}

uint8_t fb_getc(void)
{
    return fb_rring[fb_rtail++];
}

/* The request is in the ring: the first byte goes now, and the
 * transmitter's interrupt sends the rest. */
void fb_sent(void)
{
    uint16_t b = fb_base;
    _disable();
    if ((inp(b + LSR) & 0x20) && fb_ttail != fb_thead) {
        outp(b, fb_tring[fb_ttail]);
        fb_ttail = (fb_ttail + 1) & 63;
    }
    outp(b + IER, 3);
    _enable();
}

/* Between our calls the port is FUJINET.SYS's: no interrupt. */
void fb_release(void)
{
    if (fb_base)
        outp(fb_base + IER, 0);
}

/* DOS (an overlay being read over FUJINET.SYS): our call ends first. Only
 * for the game's own calls (its stack): nstrans_fb.c's locals are near. */
static void __interrupt __far dos_hook(void)
{
    if (!in_dos && getss() == FP_SEG((void __far *)&in_dos)) {
        in_dos = 1;
        _enable();
        nt_quiesce();
        in_dos = 0;
    }
    _chain_intr(old_int21);
}

static void detach(void)
{
    uint16_t b = fb_base;
    if (!b)
        return;
    _disable();
    outp(b + IER, 0);
    outp(0x21, (inp(0x21) & ~mask_bit) | mask_old);
    _dos_setvect(vec, old_irq);
    if (fifo_on)
        outp(b + FCR, 0);
    fb_base = 0;
    _enable();
}

/* The i-th port that looks like FUJINET.SYS's, attached: 0 when there are
 * no more. (Candidates: the BIOS's COM1-COM4, then the usual four.) */
static const uint16_t usual[] = { 0x3F8, 0x2F8, 0x3E8, 0x2E8 };

uint8_t fb_try(uint8_t i)
{
    uint8_t k, irq, n = 0;
    uint16_t b;

    detach();
    for (k = 0; k < 8; ++k) {   /* (one listed twice is tried twice) */
        b = k < 4 ? *(uint16_t __far *)MK_FP(0x40, k * 2) : usual[k - 4];
        if (!b || inp(b + LCR) != 0x03 || (inp(b + MCR) & 0x0F) != 0x0B || inp(b + IER) != 0)
            continue;
        if (n++ != i)
            continue;
        irq = (b & 0x100) ? 4 : 3;
        vec = 8 + irq;
        mask_bit = 1 << irq;
        fifo_on = 0;
        if (!(inp(b + IIR) & 0xC0)) {
            outp(b + FCR, 0x07);    /* a 16550: its FIFO (a byte's trigger) */
            if ((inp(b + IIR) & 0xC0) == 0xC0)
                fifo_on = 1;
            else
                outp(b + FCR, 0);
        }
        fb_rhead = fb_rtail = fb_thead = fb_ttail = fb_lsr = 0;
        _disable();
        old_irq = _dos_getvect(vec);
        _dos_setvect(vec, (void (__interrupt __far *)(void))fb_isr);
        mask_old = inp(0x21) & mask_bit;
        outp(0x21, inp(0x21) & ~mask_bit);
        fb_base = b;
        _enable();
        (void)inp(b + LSR);
        (void)inp(b);
        return 1;
    }
    return 0;
}

/* This one answered: it stays (and DOS waits for our calls). */
void fb_keep(void)
{
    if (!old_int21) {
        old_int21 = _dos_getvect(0x21);
        _dos_setvect(0x21, dos_hook);
    }
}

/* Leaving: everything back as FUJINET.SYS had it (plat_restore). */
void fb_off(void)
{
    nt_quiesce();
    detach();
    if (old_int21) {
        _dos_setvect(0x21, old_int21);
        old_int21 = 0;
    }
}

#else
void fb_off(void)
{
}
#endif
