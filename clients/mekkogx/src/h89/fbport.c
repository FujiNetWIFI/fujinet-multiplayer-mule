/*
 * fbport.c -- src/fbport.h's wire on the H89: the FujiNet card's 82C55 at
 * D0h, port A in mode 2 (both ways), handshaken by port C:
 *
 *   PC7  /OBF  high: the byte we wrote has been taken (or none waits)
 *   PC5  IBF   high: a byte from the ESP32 waits in port A
 *   PC1  /OE   the 74LVC245 between port A and the ESP32 (low: on)
 *   PC0  DIR   the 245's direction: 1 toward the ESP32, 0 toward us
 *
 * A request is written with the 245 turned toward the ESP32, then turned
 * back (fb_sent) so the reply can come in; fujinet-lib keeps the direction
 * it last set (ESP32 to H89) in a variable, which this leaves true. The
 * ESP32 hands over a reply byte at a time and waits up to a second for
 * each to be taken, so a reply can be read over several frames.
 *
 * A write waits for the byte before it to be taken, a little: if the card
 * never takes it (stuck, or gone), the rest of the request goes unwaited
 * and nstrans_fb.c's reply timeout gives up on it.
 *
 * fuji_bus_call, the call every fujinet-lib function makes (the open, the
 * close, the appkeys, the table list), is here too, on the same wire:
 * fujinet-lib's own (bus/h89) is then never linked in. It is 1.6K that
 * repeats what this does, and its writes wait for ever on a card that
 * doesn't answer.
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "fbport.h"
#include "h89.h"

__sfr __at 0xD0 PPI_PA;
__sfr __at 0xD2 PPI_PC;
__sfr __at 0xD3 PPI_CR;

#define OBF_HIGH 0x80
#define IBF      0x20
#define TAKE_WAIT 30            /* ticks (60 ms) for a byte to be taken */

static uint8_t to_esp, stuck;

static void taken(void)
{
    uint16_t t;
    if (stuck || (PPI_PC & OBF_HIGH))
        return;
    t = ticks_now();
    while (!(PPI_PC & OBF_HIGH))
        if ((uint16_t)(ticks_now() - t) > TAKE_WAIT) {
            stuck = 1;
            return;
        }
}

void fb_putc(uint8_t c)
{
    if (!to_esp) {
        to_esp = 1;
        stuck = 0;
        PPI_CR = 0x02;          /* PC1 reset: the 245 on */
        PPI_CR = 0x01;          /* PC0 set: toward the ESP32 */
    }
    taken();
    PPI_PA = c;
}

void fb_sent(void)
{
    taken();
    PPI_CR = 0x00;              /* PC0 reset: toward us, for the reply */
    to_esp = 0;
}

uint8_t fb_ready(void)
{
    return PPI_PC & IBF;
}

uint8_t fb_getc(void)
{
    return PPI_PA;
}

/* ---- fuji_bus_call: a whole call, waited on --------------------------- */
#define SL_END    0xC0
#define SL_ESC    0xDB
#define SL_ESCEND 0xDC
#define SL_ESCESC 0xDD
#define F_REPLY   0x10          /* FUJI_FIELD_REPLY: data comes back */
#define FIRST_WAIT 7500         /* ticks (15 s) for a reply to begin */
#define NEXT_WAIT  500          /* and 1 s between its bytes */

extern const uint8_t fuji_field_numbytes_table[];  /* fujinet-lib's */
static uint8_t card_up;

/* The card's 82C55 set up, as fujinet-lib's port_init does: EN low is an
 * 82C55 not set up yet (a mode set clears port C and would restart a
 * running ESP32, so only then), and the ESP32 is started. */
static void card_init(void)
{
    uint16_t t;
    if (!(PPI_PC & 0x04)) {
        PPI_CR = 0xC0;          /* port A mode 2, port C's low half out */
        PPI_PC = 0x06;          /* the 245 off, EN high */
        PPI_PC = 0x02;          /* EN low: the ESP32 in reset */
        PPI_PC = 0x06;          /* and out of it */
        t = ticks_now();
        while ((uint16_t)(ticks_now() - t) < 100)
            con_pump();
    }
    (void)PPI_PA;
    PPI_CR = 0x02;              /* the 245 on */
    PPI_CR = 0x00;              /* toward us */
}

static uint16_t fold(uint16_t chk, const uint8_t *p, uint16_t n)
{
    while (n--) {
        chk += *p++;
        chk = (chk >> 8) + (chk & 0xFF);
    }
    return chk;
}

static void out(uint8_t c)
{
    if (c == SL_END || c == SL_ESC) {
        fb_putc(SL_ESC);
        c = c == SL_END ? SL_ESCEND : SL_ESCESC;
    }
    fb_putc(c);
}

/* A byte of the reply, or -1 if none comes in wait ticks. */
static int get(uint16_t wait)
{
    uint16_t t = ticks_now();
    while (!(PPI_PC & IBF)) {
        if ((uint16_t)(ticks_now() - t) > wait)
            return -1;
        con_pump();
    }
    return PPI_PA;
}

bool fuji_bus_call(uint8_t device, uint8_t fuji_cmd, uint8_t fields,
                   uint8_t aux1, uint8_t aux2, uint8_t aux3, uint8_t aux4,
                   const void *buf, size_t buf_length)
{
    static uint8_t h[10];
    uint8_t n, rd = fields & F_REPLY, *dst = (uint8_t *)buf;
    uint16_t len, got, sum, i;
    int k;

    if (!card_up) {
        card_init();
        card_up = 1;
    }
    n = 6 + fuji_field_numbytes_table[fields & 7];
    len = n + (rd ? 0 : buf_length);
    h[0] = device;
    h[1] = fuji_cmd;
    h[2] = (uint8_t)len;
    h[3] = len >> 8;
    h[4] = 0;
    h[5] = fields;
    h[6] = aux1;
    h[7] = aux2;
    h[8] = aux3;
    h[9] = aux4;
    sum = fold(0, h, n);
    if (!rd)
        sum = fold(sum, dst, buf_length);
    h[4] = (uint8_t)sum;
    while (fb_ready())                  /* what's left of a reply gone by */
        (void)fb_getc();
    fb_putc(SL_END);
    for (i = 0; i < n; ++i)
        out(h[i]);
    if (!rd)
        for (i = 0; i < buf_length; ++i)
            out(dst[i]);
    fb_putc(SL_END);
    fb_sent();
    if (stuck)
        return false;

    /* the reply, from its first byte that isn't an END (its opening END
     * may be lost, as src/nstrans_fb.c knows), judged once its header's
     * length is in (its last END isn't waited for) */
    do {
        if ((k = get(FIRST_WAIT)) < 0)
            return false;
    } while (k == SL_END);
    got = sum = 0;
    for (;;) {
        if (k == SL_END)
            break;
        if (k == SL_ESC) {
            if ((k = get(NEXT_WAIT)) < 0)
                return false;
            k = k == SL_ESCEND ? SL_END : k == SL_ESCESC ? SL_ESC : k;
        }
        if (got < 6)
            h[got] = (uint8_t)k;
        else if (rd && got - 6 < buf_length)
            dst[got - 6] = (uint8_t)k;
        if (got != 4) {                 /* (the checksum is taken as 0) */
            sum += (uint8_t)k;
            sum = (sum >> 8) + (sum & 0xFF);
        }
        ++got;
        if (got >= 6 && got == (h[2] | (uint16_t)h[3] << 8))
            break;
        if ((k = get(NEXT_WAIT)) < 0)
            return false;
    }
    return got >= 6 && got == (h[2] | (uint16_t)h[3] << 8) && (uint8_t)sum == h[4] &&
           h[0] == device && h[1] == 0x06;
}
