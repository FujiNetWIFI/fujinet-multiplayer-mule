/*
 * bank.c -- bank.h on a 64K CoCo 1/2: all-RAM mode, and the overlays read
 * from the disk.
 *
 * BASIC LOADMs mule.bin (the low core, $2600-$7BFF) and runs it. bank_init
 * then:
 *   - copies the Disk ROM ($C000-$DFFF: DSKCON and the DriveWire routines
 *     fujinet-lib calls through $D93F/$D941) and the CPU vectors (which the
 *     SAM fetches from $BFF2-$BFFF) into the RAM underneath, and switches
 *     the SAM to all-RAM (map type 1). Color and Extended BASIC are gone
 *     from then on: their 16K holds the high core.
 *   - reads mulehi.bin (the high core, $8000-$BFEF, and the data at the
 *     top of RAM) with DSKCON.
 * bank_overlay reads mule1..5.bin into the window at $E000 (src/coco/mule.link,
 * tools/coco/lwlink-mule.py). A file is DECB: blocks of 00 len addr data,
 * then FF 0000 exec.
 *
 * The CoCo 3 (COCO3) is already all-RAM, so nothing is copied, and $FFDE is
 * left alone (ROM mode would fetch from a cartridge FujiNet). MULEC3HI.BIN,
 * MULEC31..4.BIN and MULEC3A.BIN load once into blocks 4-7 and the end of
 * block 3 (lwlink-mule.py); bank_overlay maps an overlay's block at $E000.
 * Every CoCo 3 has blocks 0-7. GFXTEST's MULEC35.BIN goes in block 15,
 * BASIC's, dead once the game runs.
 */
#include <string.h>
#include "bank.h"
#include "plat.h"

void plat_irq_init(void);       /* plat.c */

/* DSKCON's parameters, in BASIC's direct page. */
#define DCOPC  (*(uint8_t *)0x00EA)
#define DCDRV  (*(uint8_t *)0x00EB)
#define DCTRK  (*(uint8_t *)0x00EC)
#define DCSEC  (*(uint8_t *)0x00ED)
#define DCBPT  (*(uint8_t **)0x00EE)
#define DCSTA  (*(uint8_t *)0x00F0)
#define DEFDRV (*(uint8_t *)0x095A)     /* the drive BASIC last used */

#ifdef COCO3
/* DECB's disk buffers: nothing reads the disk once the game runs */
#define sec ((uint8_t *)0x0600)
#define fat ((uint8_t *)0x0700)
#else
static uint8_t sec[256];
static uint8_t fat[68];
#endif
static uint8_t drive;
static uint8_t ovl_cur;

#ifndef COCO3
/* Copy [a, a + n) from ROM to the RAM under it, a byte at a time. */
static void rom_to_ram(uint16_t a, uint16_t n)
{
    asm {
        ldx     :a
        ldd     :n
        pshs    u
        tfr     d,u
@loop
        sta     $FFDE           ; ROM
        lda     ,x
        sta     $FFDF           ; RAM
        sta     ,x+
        leau    -1,u
        cmpu    #0
        bne     @loop
        sta     $FFDE
        puls    u
    }
}
#endif

static uint8_t read_sector(uint8_t trk, uint8_t s)
{
    DCOPC = 2;
    DCDRV = drive;
    DCTRK = trk;
    DCSEC = s;
    DCBPT = sec;
    asm {
        pshs    u,y
        jsr     [$C004]
        puls    u,y
    }
    return DCSTA;
}

/* ---- a DECB .bin, streamed from the disk ------------------------------ */
static uint8_t hdr[5], hn;
static uint16_t left;
static uint8_t *at;
static uint8_t done;

static void feed(const uint8_t *p, uint16_t n)
{
    uint16_t k;
    while (n && !done) {
        if (left) {
            k = left < n ? left : n;
            memcpy(at, p, k);
            at += k;
            left -= k;
            p += k;
            n -= k;
            continue;
        }
        hdr[hn++] = *p++;
        --n;
        if (hn == 5) {
            hn = 0;
            if (hdr[0] == 0xFF) {
                done = 1;
                break;
            }
            left = ((uint16_t)hdr[1] << 8) | hdr[2];
            at = (uint8_t *)(((uint16_t)hdr[3] << 8) | hdr[4]);
        }
    }
}

/* Where each file starts, once found (the FAT is kept from the first
 * look: the disk doesn't change). */
static uint8_t f_gran[7], f_known[7];
static uint16_t f_last[7];

/* Load the DECB file NAME (8 chars, space padded) with extension BIN; f is
 * its slot in the cache above. Returns 0 if all went well. */
static uint8_t load(const char *name, uint8_t f)
{
    uint8_t s, i, g, n, trk, last;
    uint8_t *e;
    uint16_t lastbytes;

    if (f_known[f]) {
        g = f_gran[f];
        lastbytes = f_last[f];
        goto found;
    }
    if (read_sector(17, 2))
        return 1;
    memcpy(fat, sec, 68);
    g = 0xFF;
    for (s = 3; s <= 11 && g == 0xFF; ++s) {
        if (read_sector(17, s))
            return 1;
        for (i = 0; i < 8; ++i) {
            e = sec + i * 32;
            if (e[0] == 0xFF)
                break;
            if (memcmp(e, name, 8) == 0 && memcmp(e + 8, "BIN", 3) == 0) {
                g = e[13];
                lastbytes = ((uint16_t)e[14] << 8) | e[15];
                if (lastbytes == 0 || lastbytes > 256)
                    lastbytes = 256;
                break;
            }
        }
        if (i < 8 && e[0] == 0xFF)
            break;
    }
    if (g == 0xFF)
        return 2;
    f_gran[f] = g;
    f_last[f] = lastbytes;
    f_known[f] = 1;
found:
    hn = 0;
    left = 0;
    done = 0;
    while (!done) {
        if (g >= 68)
            return 3;
        n = fat[g];
        last = n >= 0xC0;
        n = last ? (n & 0x3F) : 9;
        trk = g >> 1;
        if (trk >= 17)
            ++trk;
        for (s = 0; s < n && !done; ++s) {
            if (read_sector(trk, (g & 1) * 9 + 1 + s))
                return 4;
            feed(sec, last && s == n - 1 ? lastbytes : 256);
        }
        if (last)
            break;
        g = fat[g];
    }
    return done ? 0 : 5;
}

/* ---- messages before the graphics are up: the text screen ------------- */
static void say(const char *s)
{
    uint8_t *p = (uint8_t *)0x0400 + 32 * 7;
    uint8_t c;
    memset((uint8_t *)0x0400, 0x60, 512);
    while ((c = (uint8_t)*s++) != 0)
        *p++ = c < 0x40 ? c + 0x40 : c;     /* VDG: normal video */
}

#ifdef COCO3
#define MMU7 (*(volatile uint8_t *)0xFFA7)
static const uint8_t ovl_blk[6] = { 0, 4, 5, 5, 6, 7 };
#endif

const char *bank_init(void)
{
#ifdef COCO3
    static char name[] = "MULEC30 ";
    uint8_t num;
#endif
    /* the stack's unused part ($7C00 up to here) gets a mark, so a dump
     * shows how deep it has been (tools/cocosim: dump 7C00 7FFF) */
    asm {
        leax    -16,s
@fill
        lda     #$A5
        sta     ,-x
        cmpx    #$7C00
        bhi     @fill
    }
    say("M.U.L.E. LOADING...");
    drive = DEFDRV;
    plat_irq_init();                /* our IRQ: BASIC's is about to go */
#ifdef COCO3
    if (load("MULEC3HI", 0)) {
        say("CAN'T LOAD MULEC3HI.BIN");
        for (;;)
            ;
    }
#ifdef MULE_GFXTEST
    for (num = 1; num <= 5; ++num) {
        MMU7 = num == 5 ? 15 : 3 + num;
#else
    for (num = 1; num <= 4; ++num) {
        MMU7 = 3 + num;
#endif
        name[6] = '0' + num;
        while (load(name, num))
            ;                       /* a disk error: try again */
    }
    MMU7 = 3;
    while (load("MULEC3A ", 6))
        ;
#else
    asm { orcc #$50 }
    rom_to_ram(0xC000, 0x2000);
    rom_to_ram(0xBFF0, 0x10);
    asm {
        sta     $FFDF               ; all RAM from here on
        andcc   #$EF
    }
    if (load("MULEHI  ", 0)) {
        say("CAN'T LOAD MULEHI.BIN");
        for (;;)
            ;
    }
#endif
    ovl_cur = OV_NONE;
    return 0;
}

void bank_overlay(uint8_t n)
{
#ifndef COCO3
    static char name[] = "MULE0   ";
#endif
    if (n == ovl_cur || n == OV_NONE || n > 5)
        return;
#ifdef COCO3
    ovl_cur = n;
    MMU7 = ovl_blk[n];
#else
    name[4] = '0' + n;
    ovl_cur = n;
    while (load(name, n))
        ;                           /* a disk error: try again */
#endif
}

uint8_t bank_current(void)
{
    return ovl_cur;
}

void bank_get(void *dst, const void *src, uint16_t n)
{
    memcpy(dst, src, n);
}
