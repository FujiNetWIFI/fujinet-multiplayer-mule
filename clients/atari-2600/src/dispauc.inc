; dispauc.inc -- the auction arena: two text rows, 64 entries of two lines
; (the Atari's arena, 2 lines a price step), three text rows.
;
; The four players stand in their columns at their price lines. Two
; hardware players draw four figures by taking turns: seats 0 and 1 on even
; frames, 2 and 3 on odd (MFRAME bit 0) -- 30 Hz each, the 2600's usual
; trade. The playfield is reflected (one write a line serves both halves):
; the dashed lines at the best ask and the best bid, and the auction clock
; as two bars at the screen's edges, full at the top.
;
; Visible band (192, timed pad at the bottom):
;    1  unblank
;   12  text rows 0-1
;    3  position this frame's two figures, HMOVE
;  128  the arena
;    1  lead-out
;    2  the players back for the text
;   18  text rows ARB0..ARB0+2
;   27  pad

RNOP    EQU     16
R3      EQU     1
HM0     EQU     15
HM1     EQU     0
TP0     EQU     FNTEXT+$000
TP1     EQU     FNTEXT+$080
TP2     EQU     FNTEXT+$100
TP3     EQU     FNTEXT+$180
TP4     EQU     FNTEXT+$200
TP5     EQU     FNTEXT+$280
TINK    EQU     FNTCLH-1
VBTIM   EQU     43
OSTIM   EQU     35
VITIM   EQU     227

ARENT   EQU     64              ; entries in the arena
ARB0    EQU     17              ; the first text row under it
FIGH    EQU     8               ; a figure's height, entries

; The kernel's cells (PV).
AFX0    EQU     PV+0            ; P0's and P1's x this frame
AFX1    EQU     PV+1
AP0PTR  EQU     PV+2            ; shape - top, for (ptr),y
AP0PTRH EQU     PV+3
AP1PTR  EQU     PV+4
AP1PTRH EQU     PV+5
AP0TOP  EQU     PV+6            ; top entries (>= 64 hides)
AP1TOP  EQU     PV+7
AP0CNT  EQU     PV+8
AP1CNT  EQU     PV+9
ACOL0   EQU     PV+10
ACOL1   EQU     PV+11
ALN1    EQU     PV+12           ; the best ask's entry, $FF none
ALN2    EQU     PV+13           ; the best bid's entry, $FF none
ABARTOP EQU     PV+14           ; the clock's top entry (64 = empty)
ATXCOL  EQU     PV+15
AKEND   EQU     PV+16

DASH    EQU     $55             ; a dashed line

; ---------------- DINIT ----------------
DINIT:  lda     #0
        sta     PF0
        sta     PF1
        sta     PF2
        sta     ENAM0
        sta     ENAM1
        sta     ENABL
        sta     RESMP0
        sta     RESMP1
        sta     REFP0
        sta     REFP1
        sta     HMCLR
        sta     COLUBK
        sta     GRP0
        sta     GRP1
        sta     GRP0
        lda     #1              ; reflected: one write a line, both halves
        sta     CTRLPF
        rts

DFRESH: lda     #OSTIM
        sta     TIM64T
DLOOP:  jsr     DFRAME
        jmp     DLOOP

DFRAME: bit     TIMINT
        bpl     DFRAME
        sta     WSYNC
        lda     #2
        sta     VSYNC
        sta     WSYNC
        sta     WSYNC
        sta     WSYNC
        lda     #0
        sta     VSYNC
        lda     #2
        sta     VBLANK
        lda     #VBTIM
        sta     TIM64T
        inc     MFRAME
        bne     DFR1
        inc     MFRAMEH
DFR1:   jsr     APPVBL
        jsr     TXTPOS
        lda     AP0TOP
        clc
        adc     #FIGH
        sta     AP0CNT
        lda     AP1TOP
        clc
        adc     #FIGH
        sta     AP1CNT
        lda     #0
        sta     COLUBK
        sta     PF0
        sta     PF1
        sta     PF2
        lda     #CWHITE
        sta     COLUPF
        lda     ATXCOL
        sta     COLUP0
        sta     COLUP1
DVBLP:  bit     TIMINT
        bpl     DVBLP
        sta     WSYNC
        lda     #0
        sta     VBLANK
        lda     #VITIM
        sta     TIM64T

        ldy     #0
        jsr     TINK5
        jsr     TINK5

        lda     AFX0
        ldx     #0
        jsr     POSOBJ
        lda     AFX1
        ldx     #1
        jsr     POSOBJ
        sta     WSYNC
        sta     HMOVE
        lda     #0
        sta     NUSIZ0
        sta     NUSIZ1
        sta     VDELP0
        sta     VDELP1
        lda     ACOL0
        sta     COLUP0
        lda     ACOL1
        sta     COLUP1

; ---- the arena: line A the playfield and P0, line B P1 ----
        ldy     #0
AKA:    sta     WSYNC           ; 0
        ldx     #0              ; 2
        cpy     ALN1            ; 5
        bne     AK1             ; 7/8
        ldx     #DASH           ; 9
AK1:    cpy     ALN2            ; 12
        bne     AK2             ; 14/15
        ldx     #DASH           ; 16
AK2:    stx     PF1             ; 19
        stx     PF2             ; 22
        lda     #0              ; 24
        cpy     ABARTOP         ; 27
        bcc     AK3             ; 29/30
        lda     #$30            ; 31   clocks 8-15 (and 144-151)
AK3:    sta     PF0             ; 34
        lda     #FIGH-1         ; 36
        dcp     AP0CNT          ; 41
        bcs     AK4             ; 43/44
        lda     #0              ; 45
        DB      $2C             ; 49
AK4:    lda     (AP0PTR),y      ; 49/50
        sta     GRP0            ; 52/53
        sta     WSYNC
        lda     #FIGH-1         ; 2
        dcp     AP1CNT          ; 7
        bcs     AK5             ; 9/10
        lda     #0              ; 11
        DB      $2C             ; 15
AK5:    lda     (AP1PTR),y      ; 15/16
        sta     GRP1            ; 18/19
        iny                     ; 20
        cpy     #ARENT          ; 22
        bne     AKA             ; 25

        sta     WSYNC           ; the lead-out
        lda     #0
        sta     GRP0
        sta     GRP1
        sta     PF0
        sta     PF1
        sta     PF2

        jsr     TXTPOS
        lda     ATXCOL
        sta     COLUP0
        sta     COLUP1
        ldy     #ARB0*FNTCLH
        jsr     TINK5
        jsr     TINK5
        jsr     TINK5

DFPAD:  bit     TIMINT
        bpl     DFPAD
        sta     WSYNC
        lda     #2
        sta     VBLANK
        lda     #OSTIM
        sta     TIM64T
        rts

; TXTPOS -- the players where the 48-pixel text wants them (disptext's).
TXTPOS: sta     WSYNC
        lda     #0
        sta     GRP0
        sta     GRP1
        sta     GRP0
        sta     WSYNC
        REPT    RNOP
        nop
        ENDM
        IF      R3
        sta     PAD3
        ENDIF
        sta     RESP0
        sta     RESP1
        lda     #HM0*16
        sta     HMP0
        lda     #(HM1*16)&$FF
        sta     HMP1
        sta     WSYNC
        sta     HMOVE
        lda     #$03
        sta     NUSIZ0
        sta     NUSIZ1
        lda     #1
        sta     VDELP0
        sta     VDELP1
        rts

POSOBJ: sta     WSYNC
        sec
APOS1:  sbc     #15
        bcs     APOS1
        eor     #7
        asl     a
        asl     a
        asl     a
        asl     a
        sta     HMP0,x
        sta     RESP0,x
        rts

TINK5:  tsx
        stx     SAVSP
        tya
        clc
        adc     #TINK
        sta     MLINE
TI1:    sta     WSYNC
        lda     TP0,y
        sta     GRP0
        lda     TP1,y
        sta     GRP1
        lda     TP2,y
        sta     GRP0
        lda     TP4,y
        tax
        txs
        lda     TP3,y
        tax
        lda     TP5,y
        stx     GRP1
        tsx
        stx     GRP0
        sta     GRP1
        sty     GRP0
        iny
        cpy     MLINE
        bne     TI1
        ldx     SAVSP
        txs
        sta     WSYNC
        lda     #0
        sta     GRP0
        sta     GRP1
        sta     GRP0
        iny
        rts
