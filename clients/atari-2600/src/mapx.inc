; mapx.inc -- bank 12's other job: the map's land grant and production. Bank 4
; hops here from its STICK once a frame and comes back with ENPOST (XBACK).
; A state is read on the first visit after it came (GFRESH): the reply window
; is whole until the next request, and none starts before this visit.
;
; THE LAND GRANT. The server steps a cursor across the free plots, one every
; grant.step jiffies; FIRE claims the plot under it (the server takes the plot
; the cursor left up to 36 jiffies ago, too). Between polls the cursor steps
; on its own to the free plot after the server's, unless a computer will stop
; it there first (header.grantStop; an older server sends 0). Round 1 opens
; with the transport ship rising out of the store and flying off.
;
; PRODUCTION. The colony event's plot is boxed in red: blinking, or the store's
; for a fire; acid rain's box sweeps its row.

CE_ACID EQU     3
CE_FIRE EQU     8

MAPX:   lda     HROUND          ; a new phase (or round): afresh
        asl
        asl
        asl
        asl
        ora     HPHASE
        cmp     GPH
        beq     MX1
        sta     GPH
        lda     #0
        sta     GSAID
        sta     GSHIPT
        sta     GEVT
        sta     GCLAIM
        lda     #$FF
        sta     GNF
        sta     GCUR
        lda     #200
        sta     P1TOP
MX1:    lda     GFRESH
        beq     MX3
        lda     #0
        sta     GFRESH
        lda     HPHASE
        cmp     #PHGRANT
        bne     MX2
        jsr     GRSTATE
        jmp     MX3
MX2:    jsr     PRSTATE
MX3:    lda     HPHASE
        cmp     #PHGRANT
        bne     MX4
        jsr     GRFRAME
        jmp     XBACK
MX4:    jsr     PRFRAME
        jmp     XBACK

; GRSTATE -- a state in the grant: our claim's answer, the server's cursor and
; countdown, and where ours may step next.
GRSTATE: ldx    HSEAT
        cpx     #4
        bcs     GS2
        lda     FNRPLY+SGCLAIM,x
        bmi     GS1
        lda     #1              ; ours
        sta     GCLAIM
        bne     GS2
GS1:    lda     NREQ            ; turned down (only the answer says: a poll
        cmp     #RQCLAIM        ;   in flight lands first): press again
        bne     GS2
        lda     FNRPLY+SHERR
        beq     GS2
        lda     #0
        sta     GCLAIM
GS2:    lda     FNRPLY+SGSTEP
        sta     GSTEP
        ldy     FNRPLY+SGCUR    ; Y = the server's cursor ($FF before, 45 after)
        cpy     #NPLOTS
        bcs     GS2B
        lda     GSAID           ; it has set off: the bell, and what to do
        lsr
        bcs     GS2C
        lda     GSAID
        ora     #1
        sta     GSAID
        lda     #SNDBELL
        jsr     SNDPLAY
        lda     #MSSELECT
        sta     MSGREQ
        bne     GS2C
GS2B:   cpy     #$FF            ; round 1, before it starts: the ship leaves
        bne     GS2C            ;   (once, and not for one who joins late)
        lda     HROUND
        cmp     #1
        bne     GS2C
        lda     GSAID
        and     #2
        bne     GS2C
        lda     FNRPLY+SHELAP+1
        cmp     #2
        bcs     GS2C
        lda     GSAID
        ora     #2
        sta     GSAID
        lda     #1
        sta     GSHIPT
        lda     #MSSHIP
        sta     MSGREQ
GS2C:   tya                     ; X = the free plot after it
        tax
GS3:    inx
        cpx     #NPLOTS
        bcs     GS4
        cpx     #STOREPL
        beq     GS3
        lda     FNRPLY+SMOWNER,x
        bpl     GS3
GS4:    lda     FNRPLY+SGNEXT+1 ; A = its countdown (far off, the ship
        beq     GS5             ;   leaving: 0, wait for a nearer state)
        lda     #0
        beq     GS6
GS5:    lda     FNRPLY+SGNEXT
        bne     GS6
        lda     #1
GS6:    cpy     GCUR            ; on its plot: its countdown
        bne     GS7
        sta     GNEXT
        jmp     GS9
GS7:    cpx     GCUR            ; on the plot after it, a little early (the
        bne     GS8             ;   step due within one; after a claim it
        cmp     #1              ;   waits longer, and the cursor goes there):
        bcc     GS8             ;   wait on
        cmp     GSTEP
        beq     GS7A
        bcs     GS8
GS7A:   clc
        adc     GSTEP
        sta     GNEXT
        ldx     #$FF
        bne     GS9B
GS8:    sty     GCUR
        sta     GNEXT
GS9:    lda     FNRPLY+SHSTOP   ; no further than a computer's stop
        beq     GS9B
        cpx     FNRPLY+SHSTOP
        bcc     GS9B
        ldx     #$FF
GS9B:   stx     GNF
        rts

; GRFRAME -- the button claims the plot on the screen (read this frame), and
; the cursor steps when its countdown runs out.
GRFRAME: lda    MINP
        and     #IN_FIRE
        beq     GF1
        ldx     HSEAT           ; a spectator claims nothing
        cpx     #4
        bcs     GF1
        lda     GCLAIM
        bne     GF1
        ldx     GCUR
        cpx     #NPLOTS
        bcs     GF1
        inc     GCLAIM
        lda     #$FF            ; it waits there for the answer
        sta     GNF
        lda     #SNDBELL
        jsr     SNDPLAY
        ldx     GCUR
        lda     #RQCLAIM
        ldy     #0
        jsr     QACT
GF1:    lda     GNEXT
        beq     GF3
        dec     GNEXT
        bne     GF3
        ldx     GNF
        bmi     GF2             ; $FF: the next state says
        stx     GCUR
        lda     #$FF
        sta     GNF
        lda     GSTEP
        sta     GNEXT
        bne     GF3
GF2:    inc     GNEXT           ; (and asks again next frame)
GF3:    lda     #CWHITE
        sta     FIGCOL
        jsr     XPLACE

; SHIPRUN -- round 1: the transport ship rises out of the store and flies off
; to the left (player 1), a tick every four frames.
SHIPRUN: lda    GSHIPT
        beq     SR9
        lda     MFRAME
        and     #3
        bne     SR1
        inc     GSHIPT
SR1:    lda     GSHIPT
        cmp     #64
        bcs     SR2
        lsr                     ; rising: an entry every 4 ticks, from the
        lsr                     ;   store's middle (36)
        eor     #$FF
        clc
        adc     #37
        sta     P1TOP
        lda     #80
        sta     MULX
        bne     SR3
SR2:    cmp     #144            ; then left, a clock a tick, off the map
        bcs     SR8
        eor     #$FF
        clc
        adc     #145
        sta     MULX
        lda     #20
        sta     P1TOP
SR3:    lda     #(MSHIP)&$FF
        sec
        sbc     P1TOP
        sta     P1PTR
        lda     #(MSHIP)>>8
        sbc     #0
        sta     P1PTRH
        lda     #CWHITE
        sta     MULCOL
        lda     #0
        sta     MULREF
        rts
SR8:    lda     #0
        sta     GSHIPT
SR9:    lda     #200
        sta     P1TOP
        rts

; PRSTATE -- production's colony event and its plot (the reply's slice 1).
PRSTATE: lda    #1
        jsr     XSLICE
        lda     FNRPLY+SCEVENT-512
        sta     GEVT
        lda     FNRPLY+SCPLOT-512
        sta     GEPL
        lda     #0
; XSLICE -- show reply slice A, and wait (bounded) for the cart to say so.
XSLICE: pha
        ldx     #FR_RXSL
        jsr     FNRW
        pla
        ldy     #0
XSL1:   cmp     FNSECH
        beq     XSL2
        dey
        bne     XSL1
XSL2:   rts

; PRFRAME -- the colony event's plot, boxed in red.
PRFRAME: lda    GEVT
        beq     PRF8
        cmp     #CE_ACID
        bne     PRF1
        lda     MFRAME          ; acid rain: along its row, a plot every
        lsr                     ;   16 frames
        lsr
        lsr
        lsr
PRF0:   cmp     #9
        bcc     PRF0A
        sbc     #9
        bcs     PRF0
PRF0A:  clc
        adc     GEPL
        tax
        bcc     PRF9
PRF1:   ldx     #STOREPL        ; a fire: the store
        cmp     #CE_FIRE
        beq     PRF2
        ldx     GEPL            ; the rest: its plot ($FF: none)
PRF2:   lda     MFRAME          ; blinking: 16 frames on, 16 off
        and     #16
        beq     PRF9
PRF8:   ldx     #$FF
PRF9:   stx     GCUR
        lda     #CRED
        sta     FIGCOL

; XPLACE -- the box (player 0, double width) over plot GCUR in FIGCOL, or off
; the map.
XPLACE: lda     GCUR
        cmp     #NPLOTS
        bcc     XP1
        lda     #200
        sta     P0TOP
        rts
XP1:    ldx     #0
XP2:    cmp     #9
        bcc     XP3
        sbc     #9
        inx
        bne     XP2
XP3:    tay
        lda     XPLOTX,y
        sta     FIGX
        lda     XROWENT,x
        sta     P0TOP
        lda     #5
        sta     FIGNUS
        lda     #(MCURS)&$FF
        sec
        sbc     P0TOP
        sta     P0PTR
        lda     #(MCURS)>>8
        sbc     #0
        sta     P0PTRH
        rts
