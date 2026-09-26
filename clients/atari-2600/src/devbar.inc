; devbar.inc -- the development time bar and the timer's tick, from the
; vblank (banks 5 and 6). The turn's time left now is what the last state
; said less the frames since; a playfield pixel is 6 steps of pTU jiffies
; (16 pixels, 96 steps: most of a turn). Under 30 steps the timer ticks,
; every `steps` frames, so faster as it empties (MekkoGX map.c).

DBSTEPS EQU     DVBTN           ; the steps left, as last worked out

DBAR:   lda     DVST
        cmp     #DVWALK
        beq     DBR0
        lda     #0
        sta     BARPF1
        sta     BARPF2
        rts
DBR0:   lda     MFRAME          ; the bar: every 16 frames
        and     #15
        bne     DBR3
        lda     MFRAME          ; jiffies since the arrival
        sec
        sbc     HRXF
        sta     MTMP
        lda     MFRAMEH
        sbc     HRXFH
        sta     MTMP2
        lda     DVLEFT          ; left now
        sec
        sbc     MTMP
        sta     MNUM
        lda     DVLEFTH
        sbc     MTMP2
        bcs     DBR1
        lda     #0
        sta     MNUM
DBR1:   sta     MNUMH
; steps = left / pTU: 16 by 8 bits, shift and subtract (the quotient builds
; in MNUM/MNUMH, the remainder in A)
        lda     #0
        ldx     #16
DBR2:   asl     MNUM
        rol     MNUMH
        rol     a
        bcs     DBR2A
        cmp     DVPTU
        bcc     DBR2B
DBR2A:  sbc     DVPTU
        inc     MNUM
DBR2B:  dex
        bne     DBR2
        lda     MNUMH           ; over 255 steps: 255
        beq     DBR2C
        lda     #255
        bne     DBR2D
DBR2C:  lda     MNUM
DBR2D:  sta     DBSTEPS
        ldx     #0              ; pixels = steps / 6
DBR5:   cmp     #6
        bcc     DBR6
        sbc     #6
        inx
        bne     DBR5
DBR6:   cpx     #17
        bcc     DBR7
        ldx     #16
DBR7:   lda     BAR1,x
        sta     BARPF1
        lda     BAR2,x
        sta     BARPF2
; the tick: under 30 steps, every `steps` frames
DBR3:   lda     DBSTEPS
        cmp     #30
        bcs     DBR9
        dec     DVTK
        bpl     DBR9
        sta     DVTK
        lda     #SNDTCK
        jmp     SNDPLAY
DBR9:   rts

; 0-16 pixels across PF1 (MSB first) and then PF2 (LSB first)
BAR1:   DB      $00,$80,$C0,$E0,$F0,$F8,$FC,$FE,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
BAR2:   DB      $00,$00,$00,$00,$00,$00,$00,$00,$00,$01,$03,$07,$0F,$1F,$3F,$7F,$FF
