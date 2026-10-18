;
; lowlevel.s -- the paddles, the speaker, ProDOS.
;

        .export         _paddle, _snd_tone, _mli
        .import         popa

PTRIG   := $C070
PADDL0  := $C064
SPKR    := $C030

; uint8_t paddle(uint8_t n): the paddle's timer, 0..255 (about 11 cycles a
; count, like the monitor's PREAD).
.proc   _paddle
        tax
        lda     PTRIG
        ldy     #0
        nop
        nop
:       lda     PADDL0,x
        bpl     done
        iny
        bne     :-
        dey
done:   tya
        ldx     #0
        rts
.endproc

; void snd_tone(uint8_t delay, uint8_t n): n clicks of the speaker, delay x
; 14 cycles apart (a square wave of 1020000 / (28 * delay) Hz).
.proc   _snd_tone
        sta     count
        jsr     popa
        sta     delay
        lda     count
        beq     done
loop:   bit     SPKR
        ldx     delay
:       pha
        pla
        nop
        dex
        bne     :-
        dec     count
        bne     loop
done:   rts
        .bss
count:  .res 1
delay:  .res 1
        .code
.endproc

; ---------------------------------------------------------------------------
; void aux_get(void *dst, const void *src, uint16_t n): auxiliary RAM (src)
; to main RAM (dst). void aux_put(void *dst, const void *src, uint16_t n):
; main (src) to auxiliary (dst). Both use the firmware's AUXMOVE.

        .export         _aux_get, _aux_put
        .import         popax

A1L     := $3C
A2L     := $3E
A4L     := $42
AUXMOVE := $C311

.proc   _aux_get
        clc
        bcc     auxmove
.endproc

.proc   _aux_put
        sec
        ; fall through
.endproc

.proc   auxmove
        php                     ; the direction
        sta     n
        stx     n+1
        jsr     popax           ; src
        sta     A1L
        stx     A1L+1
        jsr     popax           ; dst
        sta     A4L
        stx     A4L+1
        lda     n
        ora     n+1
        beq     none
        lda     A1L             ; A2 = src + n - 1
        clc
        adc     n
        sta     A2L
        lda     A1L+1
        adc     n+1
        sta     A2L+1
        lda     A2L
        bne     :+
        dec     A2L+1
:       dec     A2L
        plp
        jmp     AUXMOVE
none:   plp
        rts
        .bss
n:      .res 2
        .code
.endproc

; ---------------------------------------------------------------------------
; The 60 Hz clock. frame_tick() counts a frame (frame_count) each time it
; sees a vertical blank begin, so it must be called at least every 4 ms or
; so (the blank lasts about 4.4 ms): frame_wait spins on it, and the long
; drawing loops call it as they go (hgr_tick in hires.s, bank.c, map.c).
;   //e    RDVBLBAR ($C019) bit 7 is 0 during the blank
;   IIgs   the same, inverted
;   //c    the VBL interrupt flag (enabled, with the CPU's IRQs masked),
;          reset by reading PTRIG ($C070)

        .export         _vbl_init, _frame_tick
        .import         _frame_count

RDVBLBAR := $C019
ENVBL    := $C05B
IOUDISON := $C07E
IOUDISOFF := $C07F

; void vbl_init(uint8_t mode): 0 //e, 1 IIgs, 2 //c
.proc   _vbl_init
        sta     mode
        cmp     #2
        bne     :+
        sei                     ; //c: the flag without the interrupt
        sta     IOUDISOFF
        bit     ENVBL
        sta     IOUDISON
        bit     PTRIG
:       lda     #0
        sta     last
        rts
.endproc

.proc   _frame_tick
        lda     mode
        cmp     #2
        beq     iic
        lda     RDVBLBAR
        ldx     mode
        beq     :+
        eor     #$80            ; IIgs: inverted
:       and     #$80            ; $00 in the blank, $80 drawing
        cmp     last
        beq     done
        sta     last
        tax                     ; (sta sets no flags)
        bne     done            ; drawing began: nothing to count
count:  inc     _frame_count
        bne     done
        inc     _frame_count+1
done:   rts
iic:    bit     RDVBLBAR
        bpl     done
        bit     PTRIG
        jmp     count
.endproc

        .bss
mode:   .res 1
last:   .res 1
        .code

; uint8_t __fastcall__ mli(uint8_t cmd, void *parms): a ProDOS MLI call
; (written into the code, which is RAM); its error, 0 for none.
.proc   _mli
        sta     parms
        stx     parms+1
        jsr     popa
        sta     cmd
        jsr     $BF00
cmd:    .byte   0
parms:  .addr   0
        ldx     #0
        rts
.endproc
