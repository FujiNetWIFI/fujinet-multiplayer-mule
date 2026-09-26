; sound.inc -- two cues on TIA channel 0, and nothing else.
;
; The bell rings when the game wants you: "press the button to start" on
; your turn, the land grant's cursor setting off, an auction opening, the
; summary waiting for your button. The tick is the timer running out: the
; MekkoGX client's SND_TICK (pokey.s SNX_TIMER), a short blip whose pitch
; alternates, fired more and more often as the time bar empties.
;
; The left difficulty switch at A mutes both.

; SNDPLAY -- start cue A.
SNDPLAY: sta    SNDPTR
        cmp     #SNDBELL
        bne     SNP1
        lda     #32             ; frames the bell rings
        sta     SNDCNT
        rts
SNP1:   lda     SNDALT          ; the tick: alternate 7 (~1960 Hz) and 4
        eor     #3              ; 7 <-> 4
        cmp     #7
        beq     SNP2
        lda     #4
SNP2:   sta     SNDALT
        lda     #3
        sta     SNDCNT
        rts

        IFNDEF  MHASSRUN
MHASSRUN EQU    1
        ENDIF
        IF      MHASSRUN
; SNDRUN -- once a frame.
SNDRUN: lda     SNDCNT
        beq     SNDOFF
        dec     SNDCNT
        bit     SWCHB           ; bit 6: the left difficulty switch, A = mute
        bvs     SNDOFF
        lda     #4              ; a pure tone
        sta     AUDC0
        lda     SNDPTR
        cmp     #SNDBELL
        bne     SNR1
        lda     #11             ; ~1300 Hz, decaying from 15
        sta     AUDF0
        lda     SNDCNT
        lsr     a
        sta     AUDV0
        rts
SNR1:   lda     SNDALT
        sta     AUDF0
        lda     SNDCNT
        asl     a
        asl     a
        sta     AUDV0
        rts
SNDOFF: lda     #0
        sta     AUDV0
        rts
        ENDIF
