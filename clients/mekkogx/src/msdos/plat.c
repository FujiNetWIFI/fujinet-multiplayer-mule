/*
 * plat.c -- the PC and PCjr: start-up, the 60 Hz clock and quitting.
 *
 * The clock is the PIT (channel 0) sped up from 18.2 to 60 Hz; the timer
 * interrupt counts frame_count and passes every 3.3rd tick on to the BIOS,
 * so the time of day and the floppy motor still work. FUJINET.SYS may keep
 * interrupts off while it talks to the FujiNet; the network layer adds what
 * a call took with frame_lost(), and the server's clock keeps us honest
 * (main.c).
 *
 * The PCjr (F000:FFFE = FD) gets its 16-colour mode, which takes 32K of the
 * RAM below 128K for the screen (the last 32K: pages 6 and 7). The BIOS
 * keeps only the last 16K from DOS, so the 16K under it must be free: the
 * game shrinks itself to what it needs and claims the rest up to the top.
 * If DOS or a resident program is in the way, it uses the 4-colour mode.
 *
 * LEAVE goes back to the lobby the way the FujiNet's other DOS games do:
 * mount it and run its AUTOEXEC.BAT (src/msdos/lobby.c).
 */
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include <i86.h>
#include <conio.h>
#include "mule.h"

uint16_t frame_count;
uint8_t plat_jr;                /* the PCjr's 16-colour mode is on */
uint8_t plat_pal;               /* palette registers: 0 no (CGA), 1 EGA/VGA, 2 PCjr */
uint8_t plat_pcjr;              /* this is a PCjr */
static uint16_t seed = 0x1234;
/* for tools/dossim: our PSP, the BIOS's memory size (paragraphs), the
 * largest free block's segment and size (paragraphs) at start */
uint16_t plat_mem[4];
static uint8_t old_mode;

#define PIT_60HZ 19886u         /* 1193182 / 60 */

static void (__interrupt __far *old_int8)(void);
static void (__interrupt __far *old_int1b)(void);
static void (__interrupt __far *old_int23)(void);
static uint16_t bios_acc;

static void __interrupt __far timer_isr(void)
{
    uint16_t a = bios_acc;
    ++frame_count;
    bios_acc += PIT_60HZ;
    if (bios_acc < a)           /* 65536 counts: the BIOS's tick */
        _chain_intr(old_int8);
    outp(0x20, 0x20);
}

static void __interrupt __far no_break(void)
{
}

/* Without FUJINET.SYS nothing answers INT F5h, and its vector may point
 * anywhere (on the PCjr it is left over from BASIC): answer every call
 * with an error instead, so the game says so rather than crashing (and
 * test mode, which needs no network, still runs). FUJINET.SYS's handler is
 * in the segment of its device header: attributes 6002h (a block device
 * with IOCTL and a FAT BPB, over 32M). */
static uint8_t no_fn;
static void (__interrupt __far *old_intf5)(void);

uint8_t plat_net_probe(void)    /* net.c: is FUJINET.SYS there? */
{
    return !no_fn;
}

static uint8_t have_fujinet(void)
{
    void __far *v = _dos_getvect(0xF5);
    uint16_t seg = FP_SEG(v);
    if (!v || seg < 0x60 || seg >= 0xA000)
        return 0;
    return *(uint16_t __far *)MK_FP(seg, 4) == 0x6002;
}
static void __interrupt __far no_fujinet(union INTPACK r)
{
    r.w.ax = 'E';
    r.w.dx = 0;
}

static void pit(uint16_t divisor)
{
    _disable();
    outp(0x43, 0x36);           /* channel 0, lo/hi, mode 3 */
    outp(0x40, divisor & 0xFF);
    outp(0x40, divisor >> 8);
    _enable();
}

/* EGA or VGA: INT 10h AH=12h BL=10h returns BL changed. */
static uint8_t ega_or_vga(void)
{
    union REGS r;
    r.h.ah = 0x12;
    r.h.bl = 0x10;
    int86(0x10, &r, &r);
    return r.h.bl != 0x10;
}

/* Watcom's dynamic overlay manager: more room for the phases' overlays,
 * so that the ones loaded stay (a segment and a size in paragraphs). */
extern void __far _ovl_addarea(unsigned seg, unsigned paras);

#define OVL_MORE 0x400          /* 16K: every overlay can stay */

/* DOS memory: allocate paragraphs (0: couldn't), or say how many there
 * are (as a failure). */
static uint16_t dos_alloc(uint16_t paras, uint16_t *avail)
{
    union REGS r;
    r.h.ah = 0x48;
    r.w.bx = paras;
    intdos(&r, &r);
    if (avail)
        *avail = r.w.bx;
    return r.w.cflag ? 0 : r.w.ax;
}

static void dos_free(uint16_t seg)
{
    union REGS r;
    struct SREGS s;
    segread(&s);
    s.es = seg;
    r.h.ah = 0x49;
    intdosx(&r, &r, &s);
}

/* The 16-colour mode needs linear 18000h-1FFFFh (the PCjr's pages 6-7).
 * The BIOS keeps the top 16K of the 128K from DOS; the 16K under it is
 * DOS's. The game's own block is already no bigger than it needs, so the
 * largest block DOS has left follows it: if that covers 18000h up to the
 * top of the 128K, the part from 18000h is claimed (nothing else can be put
 * there, and the screen can go there) and the part below it goes to the
 * overlays. */
static uint8_t jr_memory(void)
{
    uint16_t top = *(uint16_t __far *)MK_FP(0x40, 0x13) * 64u;   /* KB -> paragraphs */
    uint16_t need = top < 0x2000 ? top : 0x2000;
    uint16_t n, seg, low;

    dos_alloc(0xFFFF, &n);      /* fails, but says how much there is */
    plat_mem[1] = top;
    plat_mem[3] = n;
    seg = n ? dos_alloc(n, 0) : 0;
    plat_mem[2] = seg;
    if (!seg)
        return 0;
    if (seg > 0x1800 || seg + n < need) {
        dos_free(seg);
        return 0;
    }
    /* Split it: below 18000h for the overlays, the rest kept. DOS puts a
     * block's header (its MCB) in the paragraph before it, and the screen
     * would overwrite one at 1800h: the overlays' part ends at 17FEh so
     * the kept block's header is at 17FFh, and it starts at 1800h. */
    dos_free(seg);
    low = 0x17FF - seg;
    if (seg < 0x17FF - 0x40 && dos_alloc(low, 0) == seg) {
        if (dos_alloc(n - low - 1, 0) == 0x1800) {
            _ovl_addarea(seg, low);
            return 1;
        }
        dos_free(seg);          /* (can't be) */
        return 0;
    }
    return dos_alloc(n, 0) != 0;
}

/* The stack's low-water mark: fill what is unused now with a pattern
 * (tools/dossim finds how much stays untouched: plat_stack_free). */
extern unsigned _STACKLOW;

static void stack_fill(void)
{
    uint8_t here;
    uint8_t *p = (uint8_t *)_STACKLOW;
    while (p < &here - 64)
        *p++ = 0xA5;
}

uint16_t plat_stack_free(void)
{
    uint8_t *p = (uint8_t *)_STACKLOW;
    while (*p == 0xA5)
        ++p;
    return (uint16_t)(p - (uint8_t *)_STACKLOW);
}

const char *plat_init(void)
{
    union REGS r;

    stack_fill();

    plat_pcjr = *(uint8_t __far *)MK_FP(0xF000, 0xFFFE) == 0xFD;
    plat_mem[0] = _psp;
    plat_pal = plat_pcjr ? 2 : ega_or_vga();
    r.h.ah = 0x0F;
    int86(0x10, &r, &r);
    old_mode = r.h.al;
    if (plat_pcjr)
        plat_jr = jr_memory();
    if (!plat_jr) {             /* room to spare: keep the overlays in */
        uint16_t n, seg;
        dos_alloc(0xFFFF, &n);  /* (leaving DOS 4K) */
        if (n > 0x140) {
            n = n - 0x100 < OVL_MORE ? n - 0x100 : OVL_MORE;
            if ((seg = dos_alloc(n, 0)) != 0)
                _ovl_addarea(seg, n);
        }
    }

    seed ^= *(uint16_t __far *)MK_FP(0x40, 0x6C);   /* the BIOS tick count */
    old_int8 = _dos_getvect(0x08);
    old_int1b = _dos_getvect(0x1B);
    old_int23 = _dos_getvect(0x23);
    if (!have_fujinet()) {
        no_fn = 1;
        old_intf5 = _dos_getvect(0xF5);
        _dos_setvect(0xF5, no_fujinet);
    }
    _dos_setvect(0x1B, no_break);
    _dos_setvect(0x23, no_break);
    _disable();
    _dos_setvect(0x08, timer_isr);
    pit(PIT_60HZ);
    gfx_init();
    return 0;
}

/* Everything back as DOS had it. */
void plat_restore(void)
{
    union REGS r;
    extern void input_restore(void);    /* input.c */
    extern void snd_off(void);          /* sound.c */
    snd_off();
    input_restore();
    pit(0);
    _dos_setvect(0x08, old_int8);
    _dos_setvect(0x1B, old_int1b);
    _dos_setvect(0x23, old_int23);
    if (no_fn)
        _dos_setvect(0xF5, old_intf5);
    r.h.ah = 0;
    r.h.al = old_mode;
    int86(0x10, &r, &r);
}

void plat_quit(void)
{
    extern void lobby_return(void);     /* lobby.c */
    plat_restore();
    lobby_return();
    exit(0);
}

/* Wait until at least one frame has passed since the last call. */
uint16_t plat_late;             /* frames missed (tools/dossim peeks it) */
void frame_wait(void)
{
    static uint16_t seen;
    uint16_t n;
    while ((n = *(volatile uint16_t *)&frame_count) == seen)
        ;               /* (the timer interrupt counts it) */
    if ((uint16_t)(n - seen) > 1)
        plat_late += n - seen - 1;
    seen = n;
    seed += frame_count;
}

void frame_tick(void)
{
}

void frame_lost(uint8_t jiffies)
{
    _disable();
    frame_count += jiffies;
    _enable();
}

uint8_t plat_rand(void)
{
    /* xorshift, stirred by the frame count */
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return (uint8_t)seed;
}
