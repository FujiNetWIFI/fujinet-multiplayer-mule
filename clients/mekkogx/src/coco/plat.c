/*
 * plat.c -- CoCo 1/2 start-up, the 60 Hz clock and quitting.
 *
 * The clock is the VDG's field sync interrupt (PIA0 CB1), counted by
 * irq_isr. The DriveWire ROM masks interrupts while it talks to the
 * FujiNet, so the network layer adds the jiffies a call took with
 * frame_lost(), and the server's clock keeps us honest (main.c).
 *
 * By the time plat_init runs, bank_init has put the machine in all-RAM
 * mode (see bank.c): the BASIC ROMs are gone, so the IRQ handler must not
 * chain to them.
 */
#include "mule.h"

uint16_t frame_count;
static uint16_t seed = 0x1234;

/* The keyboard, scanned here 60 times a second so that no key press is
 * missed however long the game's own frame takes (input.c reads these):
 * kb_rows[c] is column c's rows (1 = down, joystick buttons masked out),
 * and each key that went down is queued as column * 8 + row. */
uint8_t kb_rows[8], kb_prev[8];
uint8_t kb_q[8], kb_head, kb_tail;

/* The IRQ: on the field sync count a frame and scan the keyboard; clear
 * whichever PIA0 flag brought us here. */
interrupt asm void irq_isr(void)
{
    asm {
        lda     $FF03
        bpl     @hsync
        lda     $FF02           ; clears the field sync flag
        ldd     _frame_count
        addd    #1
        std     _frame_count
        ; the keyboard: rows low with no column driven are joystick buttons
        lda     #$FF
        sta     $FF02
        lda     $FF00
        coma
        anda    #$7F
        pshs    a               ; 0,s: the buttons
        leax    _kb_rows,pcr
        leay    _kb_prev,pcr
        ldb     #$FE            ; column 0 low
        clr     ,-s             ; 0,s: column; 1,s: buttons
@col
        stb     $FF02
        lda     $FF00
        coma
        anda    #$7F
        pshs    b
        ldb     2,s
        comb
        pshs    b
        anda    ,s+             ; not the buttons
        puls    b
        sta     ,x
        eora    ,y              ; changed ...
        anda    ,x              ; ... and down now: pressed
        beq     @same
        pshs    b,x,y
        ldb     5,s             ; the column
        aslb
        aslb
        aslb
@bit
        lsra
        bcc     @nobit
        pshs    a,b
        lda     _kb_head
        leax    _kb_q,pcr
        stb     a,x             ; queue column * 8 + row
        inca
        anda    #7
        cmpa    _kb_tail
        beq     @full
        sta     _kb_head
@full
        puls    a,b
@nobit
        incb
        tsta
        bne     @bit
        puls    b,x,y
@same
        lda     ,x+
        sta     ,y+
        inc     ,s
        orcc    #$01            ; next column low
        rolb
        bcs     @col
        leas    2,s
        lda     #$FF
        sta     $FF02
        bra     @out
@hsync
        lda     $FF00           ; (not enabled; clear it anyway)
@out
    }
}

void plat_irq_init(void)
{
    asm {
        orcc    #$50
        lda     #$7E            ; JMP irq_isr at the RAM vector ($010C)
        sta     $010C
        leax    _irq_isr,pcr
        stx     $010D
        lda     $FF01           ; PIA0 CA1 (hsync) interrupt off
        anda    #$FE
        sta     $FF01
        lda     $FF03           ; CB1 (field sync) on, falling edge
        ora     #$01
        anda    #$FD
        sta     $FF03
        lda     $FF02
        andcc   #$EF            ; IRQ on (FIRQ stays off)
    }
}

/* Which artifact colour is which: show a bar and let the player say. */
static void colour_check(void)
{
    uint8_t i;
    gfx_clear();
    gfx_fill(40, 72, 80, 24, C_BLUE);
    text_center(14, "IS THE BAR BLUE?", C_WHITE);
    text_center(16, "ENTER: YES   CLEAR: NO", C_WHITE);
    for (i = 0; i < 240; ++i) {         /* 4 s, then on with it */
        frame_wait();
        input_poll();
        input_take();
        if (ev_btn || ev_key == KEY_END)
            break;
    }
}

const char *plat_init(void)
{
    gfx_init();
    colour_check();
    return 0;
}

void plat_quit(void)
{
    asm {
        orcc    #$50
        clr     $0071           ; cold start
        sta     $FFDE           ; ROM back in
        jmp     [$FFFE]
    }
}

/* Wait until at least one frame has passed since the last call. */
void frame_wait(void)
{
    static uint16_t seen;
    while (frame_count == seen)
        ;
    seen = frame_count;
    seed += frame_count;
}

void frame_tick(void)
{
}

void frame_lost(uint8_t jiffies)
{
    asm { orcc #$10 }           /* the IRQ changes it too */
    frame_count += jiffies;
    asm { andcc #$EF }
}

uint8_t plat_rand(void)
{
    /* xorshift, stirred by the frame count */
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return (uint8_t)seed;
}
