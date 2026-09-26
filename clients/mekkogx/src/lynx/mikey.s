;
; mikey.s -- POKEY's four voices on Mikey's four audio channels.
;
; pokey.s leaves what it would write to POKEY in _pk_out (AUDF1 AUDC1 ...
; AUDF4 AUDC4 AUDCTL); each channel gets the same pitch, noise and volume:
;
;   pitch   POKEY's output flips every AUDF+1 ticks of its clock: 64 kHz
;           (15.6 us), 15 kHz with AUDCTL bit 0 (64 us), 1.79 MHz for
;           voice 1 or 3 with AUDCTL bit 6 or 5 (+4 ticks; +7 when joined
;           to the next, AUDCTL bits 4 and 3, a 16-bit count). That time in
;           microseconds becomes a Mikey clock (1..64 us) and a count.
;   noise   AUDC bits 7-5: pure tones are a square wave (feedback tap 0);
;           the 4-bit polys a 15-step LFSR, the rest a 4095-step one.
;   volume  AUDC bits 3-0, times 8; bit 4 (volume only) holds the level.
;
; Registers are written only when they change: rewriting a timer restarts
; its wave. Runs in the VBL (irq.s), after pokey.s.
;
        .pc02
        .export _mikey_out, _snd_rand, _mikey_init
        .import _pk_out

        .include "lynx.inc"

        .zeropage
us:     .res 2                  ; the half period, in microseconds

        .bss
lastf:  .res 4                  ; per channel: the backup, control and
lastc:  .res 4                  ; feedback last written
lastk:  .res 4
lastv:  .res 4                  ; and the volume
rnd:    .res 2
aud:    .res 1                  ; this channel's AUDC
ch:     .res 1
ctl:    .res 1                  ; AUDCTL

        .rodata
; channel register offsets from AUD0VOL
choff:  .byte   $00, $08, $10, $18

        .code

; void mikey_init(void): the channels silent, the shift registers seeded
_mikey_init:
        ldx     #3
@c:     ldy     choff,x
        lda     #0
        sta     AUD0VOL,y       ; volume
        sta     AUD0VOL+5,y     ; control: stopped
        lda     #1
        sta     AUD0VOL+3,y     ; shift register
        lda     #$FF
        sta     lastf,x
        sta     lastc,x
        sta     lastk,x
        sta     lastv,x
        dex
        bpl     @c
        stz     MSTEREO         ; every channel to both sides
        lda     #$A5
        sta     rnd
        sta     rnd+1
        rts

; uint8_t snd_rand(void): a byte of noise (for pokey.s's RANDOM). Keeps X, Y.
_snd_rand:
        lda     rnd
        asl     a
        rol     rnd+1
        bcc     @n
        eor     #$2D
@n:     sta     rnd
        eor     rnd+1
        rts

_mikey_out:
        lda     _pk_out+8
        sta     ctl
        ldx     #3
@ch:    stx     ch
        txa
        asl     a
        tay
        lda     _pk_out+1,y
        sta     aud
        ; ---- the half period in microseconds -> us
        lda     _pk_out,y       ; AUDF (low byte of a joined pair)
        sta     us
        stz     us+1
        cpx     #0
        beq     @fast1
        cpx     #2
        beq     @fast3
        cpx     #1
        beq     @join12
        ; voice 4: joined to 3?
        lda     ctl
        and     #$08
        beq     @slow
        lda     _pk_out+4       ; AUDF3 low, AUDF4 high
        sta     us
        lda     _pk_out+6
        sta     us+1
        lda     ctl
        and     #$20            ; 3 at 1.79 MHz
        bne     @fast16
        bra     @slow16
@join12:
        lda     ctl
        and     #$10
        beq     @slow
        lda     _pk_out+0
        sta     us
        lda     _pk_out+2
        sta     us+1
        lda     ctl
        and     #$40
        bne     @fast16
        bra     @slow16
@fast1: lda     ctl
        and     #$40
        bne     @fast8
        bra     @slow
@fast3: lda     ctl
        and     #$20
        bne     @fast8
        ; fall through
@slow:  ; (AUDF + 1) ticks of 16 us (64 us at 15 kHz)
@slow16:
        inc     us
        bne     @s1
        inc     us+1
@s1:    ldy     #4              ; x16
        lda     ctl
        lsr     a
        bcc     @s2
        ldy     #6              ; x64
@s2:    asl     us
        rol     us+1
        bcs     @sat
        dey
        bne     @s2
        bra     @have
@sat:   lda     #$FF
        sta     us
        sta     us+1
        bra     @have
@fast8: ; (AUDF + 4) x 0.56 us
        lda     #4
        bra     @fadd
@fast16:
        lda     #7
@fadd:  clc
        adc     us
        sta     us
        bcc     @f1
        inc     us+1
@f1:    ; us = us/2 + us/16 (x 0.5625)
        lda     us+1
        lsr     a
        sta     rnd_t+1
        lda     us
        ror     a
        sta     rnd_t           ; /2
        lsr     rnd_t+1
        ror     rnd_t
        lsr     rnd_t+1
        ror     rnd_t
        lsr     rnd_t+1
        ror     rnd_t           ; /16
        lda     us+1
        lsr     a
        sta     us+1
        lda     us
        ror     a
        clc
        adc     rnd_t
        sta     us
        lda     us+1
        adc     rnd_t+1
        sta     us+1
@have:  ; ---- us -> a clock (1 << k us) and a count up to 256
        ldy     #0
@k:     lda     us+1
        beq     @k2             ; < 256 already
        cmp     #1
        bne     @kn
        lda     us
        beq     @k2             ; exactly 256
@kn:    cpy     #6
        beq     @k2             ; as slow as Mikey goes
        lsr     us+1
        ror     us
        iny
        bra     @k
@k2:    lda     us
        sec
        sbc     #1              ; the count: backup + 1 clocks
        bcs     @k3
        lda     #0
@k3:    ldx     ch
        pha
        ; the control byte: reload, count, clock y
        tya
        ora     #$18
        sta     ctl_t
        ; the feedback taps from the distortion
        lda     aud
        and     #$E0
        cmp     #$A0
        beq     @pure
        cmp     #$E0
        beq     @pure
        cmp     #$C0
        beq     @poly4
        cmp     #$40
        beq     @poly4
        lda     #$A9            ; taps 11, 5, 3, 0: 4095 steps
        bra     @tap
@poly4: lda     #$0C            ; taps 3, 2: 15 steps
        bra     @tap
@pure:  lda     #$01
@tap:   ldy     choff,x
        cmp     lastk,x
        beq     @t1
        sta     lastk,x
        sta     AUD0VOL+1,y     ; feedback
@t1:    pla
        cmp     lastf,x
        beq     @t2
        sta     lastf,x
        sta     AUD0VOL+4,y     ; backup
@t2:    lda     ctl_t
        cmp     lastc,x
        beq     @t3
        sta     lastc,x
        sta     AUD0VOL+5,y     ; control
@t3:    ; ---- the volume
        lda     aud
        and     #$0F
        asl     a
        asl     a
        asl     a
        cmp     lastv,x
        beq     @t4
        sta     lastv,x
        sta     AUD0VOL,y
@t4:    lda     aud
        and     #$10
        beq     @next
        lda     lastv,x         ; volume only: a level, no wave
        sta     AUD0VOL+2,y
        lda     #0
        sta     AUD0VOL+5,y
        lda     #$FF
        sta     lastc,x
@next:  dex
        bmi     @r
        jmp     @ch
@r:     rts

        .bss
rnd_t:  .res 2
ctl_t:  .res 1
