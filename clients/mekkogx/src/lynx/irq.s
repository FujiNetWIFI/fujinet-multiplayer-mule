;
; irq.s -- the Lynx's 60 Hz clock: the vertical blank (timer 2) counts
; frame_count and runs the sound (pokey.s).
;
; ComLynx must not wait while the sound runs (one byte of buffering in the
; UART at 62500 baud): the VBL is acknowledged and interrupts go back on
; for the sound, so the serial driver's come through nested. cc65's IRQ
; dispatch (callirq) isn't re-entrant: a nested one leaves the interrupted
; one's loop index at its end, so that dispatch just stops early (clock()'s
; VBL counter may miss a tick). Carry clear: the others still run.
;
        .pc02
        .export _vbl_init, _frame_count
        .import _snd_vbi
        .interruptor vbl_irq, 1         ; after the others (clock() is 2):
                                        ; they must see the VBL before we
                                        ; acknowledge it

        .include "lynx.inc"

        .bss
_frame_count: .res 2
busy:   .res 1

        .code

; void vbl_init(void): the VBL interrupt on
_vbl_init:
        lda     #$80
        tsb     VTIMCTLA
        rts

vbl_irq:
        lda     INTSET
        and     #VBL_INTERRUPT
        bne     @vbl
        clc
        rts
@vbl:   sta     INTRST
        inc     _frame_count
        bne     @c
        inc     _frame_count+1
@c:     lda     busy
        bne     @done           ; last frame's sound is still running
        inc     busy
        cli
        jsr     _snd_vbi
        sei
        stz     busy
@done:  clc
        rts
