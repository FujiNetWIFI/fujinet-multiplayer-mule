; auction.asm -- bank 7: the auctions' arena (land, phase 2; goods, 6).
;
; MekkoGX auction.c's arena. The players stand in four columns at their
; price lines; the line a player pushes toward moves a step every 8 frames
; (the Atari's pace), a step being 2 scanlines and st.auction.step dollars
; (4 in a goods auction's wide range). Prices are kept as ENTRIES (0-63, a
; step each) from the base line at entry 45: the store's buying price in a
; goods auction, the opening price in a land auction. Two dashed lines mark
; the best ask and the best bid; the clock is two bars at the edges.
;
; The stick pushes (up raises your line, down lowers it, letting go holds),
; sent when it changes, with the units traded so far; in a goods auction the
; button colludes. The bell rings when the lines are released (LEAD).

        CPU     6502UNDOC
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     1
MHASDEC EQU     0
MHASCMP EQU     1

        INCLUDE "../build/tail.inc"

        INCLUDE "aucram.inc"

EBASE   EQU     45              ; the base line's entry
EUP     EQU     41              ; steps above it
EDOWN   EQU     13              ; and below
ESELL   EQU     10              ; where sellers stand before the lines go
EBUY    EQU     58              ;   and buyers

        ORG     $1000

AENTRY: lda     MENT
        cmp     #ENRUN
        bne     AE1
        jmp     SLOOP
AE1:    cmp     #ENSTATE
        bne     ASETUP
; A state: read on the next overscan, not this one, which the request
; machine's READ has half spent. HFNEW keeps the window whole meanwhile.
        lda     #1
        sta     APEND
        jmp     SLOOP

ASETUP: lda     #BKAUC
        sta     MSCR
        lda     #2
        sta     VBLANK
        jsr     DINIT
        jsr     FNCLS
        lda     #0
        sta     SNOPOLL
        sta     APUSH
        sta     ATRADED
        sta     ABLINK
        lda     #CWHITE
        sta     ATXCOL
        lda     #$FF
        sta     ASTG
        sta     ATKS
        lda     #0
        sta     APEND
        jsr     AREAD
        jmp     SLOOPF

; ---------------------------------------------------------------------------
; AREAD -- a state has arrived.
AREAD:  lda     FNRPLY+SAKIND
        sta     AKIND
        lda     FNRPLY+SAGOOD
        and     #3
        sta     AGOOD
        lda     FNRPLY+SASTEP
        bne     AR1
        lda     #1
AR1:    sta     ASTEPV
; the base: the store's buying price, or the land's opening price
        ldx     #SASBUY-SAUC
        lda     AKIND
        cmp     #1
        bne     AR2
        ldx     #SASTART-SAUC
AR2:    lda     FNRPLY+SAUC,x
        sta     ABASE
        lda     FNRPLY+SAUC+1,x
        sta     ABASEH
; a new stage
        lda     FNRPLY+SASTAGE
        cmp     ASTG
        beq     AR3
        sta     ASTG
        cmp     #STGLEAD
        bne     AR2A
        lda     #SNDBELL        ; the lines are about to go: the bell
        jsr     SNDPLAY
AR2A:   lda     #0
        sta     APUSH
AR3:
; the players: roles, species, lines
        ldx     #3
AR4:    lda     FNRPLY+SAROLE,x
        sta     AROLE,x
        lda     PLOFS,x
        tay
        lda     FNRPLY+SPLAYER+PLSPEC,y
        and     #7
        sta     ASPEC,x
        dex
        bpl     AR4
        ldx     #0
AR5:    stx     MIDX
        jsr     AKEEP           ; my released line: leave it be
        bcs     AR5N
        txa
        asl     a
        tay
        lda     FNRPLY+SATGT,y
        sta     MNUM
        lda     FNRPLY+SATGT+1,y
        sta     MNUMH
        jsr     PENT
        ldx     MIDX
        sta     ATGTE,x
        txa
        asl     a
        tay
        lda     FNRPLY+SAPOS,y
        sta     MNUM
        lda     FNRPLY+SAPOS+1,y
        sta     MNUMH
        jsr     PENT
        ldx     MIDX
        sta     APOSE,x
AR5N:   inx
        cpx     #4
        bne     AR5
; the scale's ends, for the stick
        lda     FNRPLY+SAHI
        sta     MNUM
        lda     FNRPLY+SAHI+1
        sta     MNUMH
        jsr     PENT
        sta     AHIE
        lda     FNRPLY+SALO
        sta     MNUM
        lda     FNRPLY+SALO+1
        sta     MNUMH
        jsr     PENT
        sta     ALOE
        lda     FNRPLY+SASSELL
        sta     MNUM
        lda     FNRPLY+SASSELL+1
        sta     MNUMH
        jsr     PENT
        sta     ASSELE
; the clock: 70 ticks (30 declaring), a bar 64 entries tall
        lda     FNRPLY+SATICKS
        ldx     ASTG
        cpx     #STGACT
        beq     AR6A
        cpx     #STGDECL
        beq     AR6A
        lda     #0
AR6A:   sta     ATKS
        lsr     a
        lsr     a
        lsr     a
        sta     MTMP
        lda     ATKS
        sec
        sbc     MTMP            ; ticks - ticks/8: 70 -> 62
        sta     MTMP
        lda     #ARENT
        sec
        sbc     MTMP
        sta     ABARTOP
; units traded
        lda     FNRPLY+SATRADE
        sta     ATRADED
        jsr     ALINES
        lda     #1
        sta     MCOMP
        rts

; PENT -- the price in MNUM/MNUMH as an entry: EBASE up (dearer) or down,
; a step every ASTEPV dollars (divided by 4 for a step of 4, as the Atari
; does), clamped to the arena.
PENT:   lda     MNUM
        sec
        sbc     ABASE
        tax
        lda     MNUMH
        sbc     ABASEH
        bcc     PE5             ; below the base
        jsr     PDIV
        bne     PE2             ; far above
        cpx     #EUP+1
        bcc     PE3
PE2:    ldx     #EUP
PE3:    stx     MTMP
        lda     #EBASE
        sec
        sbc     MTMP
        rts
PE5:    tay                     ; base - price
        txa
        eor     #$FF
        clc
        adc     #1
        tax
        tya
        eor     #$FF
        adc     #0
        jsr     PDIV
        bne     PE6
        cpx     #EDOWN+1
        bcc     PE7
PE6:    ldx     #EDOWN
PE7:    txa
        clc
        adc     #EBASE
        rts
; AKEEP -- C set if seat X's line is mine, let go in a goods auction, and
; the server's is still on its way to where I let go (or it hasn't my
; release yet): mine stays put rather than jump back, a second at most.
; Keeps X.
AKEEP:  cpx     HSEAT
        bne     AKP0
        lda     AKIND
        cmp     #2
        bne     AKP0
        lda     ASTG
        cmp     #STGACT
        bne     AKP0
        lda     APUSH
        bne     AKP0
        lda     MFRAME
        sec
        sbc     AHELD
        cmp     #60
        bcs     AKP0
        lda     FNRPLY+SAPUSH,x
        bne     AKP1
        txa
        asl     a
        tay
        lda     FNRPLY+SAPOS,y
        cmp     FNRPLY+SATGT,y
        bne     AKP1
        lda     FNRPLY+SAPOS+1,y
        cmp     FNRPLY+SATGT+1,y
        bne     AKP1
AKP0:   clc
        rts
AKP1:   sec
        rts

; APRICE -- the price at seat X's line (its entry): X lo, Y hi.
APRICE: lda     #EBASE
        sec
        sbc     APOSE,x         ; steps over the base, signed
        sta     MNUM
        lda     #0
        sbc     #0
        sta     MNUMH
        lda     ASTEPV
        cmp     #4
        bne     APR1
        asl     MNUM
        rol     MNUMH
        asl     MNUM
        rol     MNUMH
APR1:   lda     MNUM
        clc
        adc     ABASE
        tax
        lda     MNUMH
        adc     ABASEH
        tay
        rts

; PDIV -- A:X (hi:lo) / 4 if the step is 4; returns the high byte in A
; (Z set if it is zero), the low in X.
PDIV:   ldy     ASTEPV
        cpy     #4
        bne     PDV2
        stx     MTMP
        lsr     a
        ror     MTMP
        lsr     a
        ror     MTMP
        ldx     MTMP
PDV2:   cmp     #0
        rts

; ALINES -- the dashed lines: the best ask (the lowest seller, or the
; store's price while it has stock -- the highest entry) and the best bid
; (the highest buyer, or the store's buying price / the land's opening --
; the lowest entry). None before the lines are released.
ALINES: lda     #$FF
        sta     ALN1
        sta     ALN2
        lda     ASTG
        cmp     #STGLEAD
        bcc     AL9
        lda     #EBASE          ; the bid: the base line at worst
        sta     ALN2
        lda     #0
        sta     ALN1
        lda     AKIND
        cmp     #2
        bne     AL1
        ldx     AGOOD
        lda     FNRPLY+SSSTOCK,x
        beq     AL1
        lda     ASSELE          ; the store sells at its price, while it has
        sta     ALN1            ;   stock
AL1:    ldx     #3
AL2:    lda     AROLE,x
        cmp     #ROLESELL
        bne     AL3
        lda     APOSE,x
        cmp     ALN1
        bcc     AL4
        sta     ALN1
        bcs     AL4
AL3:    cmp     #ROLEBUY
        bne     AL4
        lda     APOSE,x
        cmp     ALN2
        bcs     AL4
        sta     ALN2
AL4:    dex
        bpl     AL2
        lda     ALN1
        bne     AL9
        lda     #$FF            ; no seller at all
        sta     ALN1
AL9:    rts

; ---------------------------------------------------------------------------
; APPVBL: the lines' steps, this frame's figures, the stick, the text.
APPVBL: jsr     SNDRUN
; the clock's tick, the original's: under 30 ticks, every `ticks` frames,
; so faster as it runs out (devbar.inc)
        lda     ATKS
        beq     AVT0
        cmp     #30
        bcs     AVT0
        dec     ATKF
        bpl     AVT1
        sta     ATKF
        ldx     SNDCNT          ; never over the bell
        beq     AVT2
        ldx     SNDPTR
        cpx     #SNDBELL
        beq     AVT1
AVT2:   lda     #SNDTCK
        jsr     SNDPLAY
        jmp     AVT1
AVT0:   lda     #0              ; the first tick as soon as it is under 30
        sta     ATKF
AVT1:
        jsr     INSCAN
        jsr     RSTCHK
; a step every 8 frames, toward the targets
        dec     ATICK
        bpl     AV3
        lda     #7
        sta     ATICK
        ldx     #3
AV1:    lda     APOSE,x
        cmp     ATGTE,x
        beq     AV2
        bcc     AV1A
        dec     APOSE,x
        bcs     AV2
AV1A:   inc     APOSE,x
AV2:    dex
        bpl     AV1
        jsr     ALINES
AV3:
; this frame's pair: seats 0,1 or 2,3
        lda     MFRAME
        and     #1
        asl     a
        tax                     ; the first seat
        lda     #0
        jsr     AFIG
        inx
        lda     #1
        jsr     AFIG
; the stick
        ldx     HSEAT
        cpx     #4
        bcs     AV9
        lda     ASTG
        cmp     #STGLEAD
        bcc     AV9
        cmp     #STGDONE
        bcs     AV9
        lda     AROLE,x
        beq     AV9
        ldy     #0
        lda     INCUR
        and     #IN_UP
        beq     AV4
        ldy     #1
AV4:    lda     INCUR
        and     #IN_DOWN
        beq     AV5
        ldy     #2
AV5:    cpy     APUSH
        beq     AV7
        sty     APUSH
; my own line moves at once; the server confirms
        cpy     #1
        bne     AV5A
        lda     AHIE
        sta     ATGTE,x
AV5A:   cpy     #2
        bne     AV5B
        lda     ALOE
        sta     ATGTE,x
AV5B:   cpy     #0
        bne     AV6
        lda     APOSE,x
        sta     ATGTE,x
        lda     AKIND           ; a goods line let go stays where I saw it
        cmp     #2              ; stop: RQHOLD with its price (the server's
        bne     AV6             ; line is behind mine)
        lda     ASTG
        cmp     #STGACT
        bne     AV6
        lda     MFRAME
        sta     AHELD
        jsr     APRICE
        lda     #RQHOLD
        jsr     QACT
        jmp     AV7
AV6:    tya
        tax
        lda     #RQPUSH
        ldy     ATRADED
        jsr     QACT
AV7:    lda     MINP
        and     #IN_FIRE
        beq     AV9
        lda     AKIND
        cmp     #2
        bne     AV9
        lda     ASTG
        cmp     #STGACT
        bne     AV9
        lda     #RQCOLL
        ldx     #0
        ldy     #0
        jmp     QACT
AV9:    rts

; AFIG -- player A (0/1) this frame shows seat X: its column, colour and
; figure, feet on its line; hidden with no role, or blinking.
AFIG:   stx     MTMP2
        tay                     ; Y: which hardware player
        lda     COLX,x
        sta     AFX0,y
        lda     SEATCOL,x
        sta     ACOL0,y
        lda     AROLE,x
        JEQ     AFHIDE
        lda     ASTG
        cmp     #STGLEAD
        bcs     AF2
        lda     #ESELL          ; before the lines go: sellers up, buyers
        ldx     MTMP2           ;   down
        ldy     AROLE,x
        cpy     #ROLESELL
        beq     AF3
        lda     #EBUY
        bne     AF3
AF2:    ldx     MTMP2
        lda     APOSE,x
AF3:    sec
        sbc     #FIGH-1
        bcs     AF4
        lda     #0
AF4:    ldy     MTMP2           ; which hardware player again
        cpy     #2
        bcc     AF5
        dey
        dey
AF5:    sta     AP0TOP,y
; the shape: the species' standing frame (or a step while it moves)
        ldx     MTMP2
        lda     ASPEC,x
        asl     a               ; species * 16
        asl     a
        asl     a
        asl     a
        tay
        lda     APOSE,x
        cmp     ATGTE,x
        tya
        beq     AF6             ; standing still
        bit     MFRAME
        bvc     AF6
        clc
        adc     #8              ; a step, every other 32 frames
AF6:    clc
        adc     #(AFIGS)&$FF
        sta     MTMP
        lda     #(AFIGS)>>8
        adc     #0
        sta     MNUMH
        ldy     MTMP2
        cpy     #2
        bcc     AF7
        dey
        dey
AF7:    lda     AP0TOP,y        ; ptr = shape - top
        eor     #$FF
        sec
        adc     MTMP
        sta     MNUM
        lda     MNUMH
        sbc     #0
        tax
        tya
        asl     a
        tay
        lda     MNUM
        sta     AP0PTR,y
        txa
        sta     AP0PTRH,y
        ldx     MTMP2
        rts
AFHIDE: ldy     MTMP2
        cpy     #2
        bcc     AFH1
        dey
        dey
AFH1:   lda     #200
        sta     AP0TOP,y
        rts

STICK:  lda     APEND
        beq     STK9
        lda     #0
        sta     APEND
        jmp     AREAD
STK9:   rts

PLOFS:  DB      0,PLSIZE,2*PLSIZE,3*PLSIZE
; The columns: the Atari's 39/63/87/111, less POSOBJ's 4.
COLX:   DB      35,59,83,107,35,59
SEATCOL: DB     CPLAY0,CPLAY1,CPLAY2,CPLAY3

MBGT:   DB      0

        INCLUDE "mlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "scrloop.inc"
        INCLUDE "dispauc.inc"

; ---------------- data: tables, not code (tools/dataranges.py) ----------
DSTART:
        ALIGN   8               ; a figure never straddles a page
        INCLUDE "../build/gen/aucspr.inc"

        END
