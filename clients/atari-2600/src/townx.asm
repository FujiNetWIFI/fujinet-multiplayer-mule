; townx.asm -- bank 12: the walker's logic in the town (bank 6); bank 11 is
; the map's. MekkoGX develop.c.
;
; Entered from the screen's overscan and sent back to it (MSCR) with
; ENPOST, or on to the other screen:
;   ENRUN    a frame: the walk, the button, where the figures are
;   ENSTATE  a state has arrived (the reply window is whole)
;   ENENTER  the screen was just entered
;   ENTOWN   the map again, out of one end of the town's street
;
; The server keeps no position; the figure walks here and the actions go to
; the server. The turn waits for your button (the bell), then starts in the
; middle of the town. Walking off either end of the street comes out on the
; map beside the store, walking into the store goes back in.
;
; On the map: one step every 6 frames (14 over the river, 21 over a plot's
; mountains), 2 x units or 1 y unit (one entry). The M.U.L.E. follows a step
; behind. The button installs it on the plot under you; without one it
; catches a showing wampus, lists a plot after a visit to the land office,
; or takes an assay sample. SELECT ends the turn.
;
; In the town: 4 x units a step along the street, which is two steps deep;
; pushing on at a door walks in (the outfitters only with a M.U.L.E.), and
; the building's action goes to the server; its answer puts you back out.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     0
MHASDEC EQU     0
MHASTXT EQU     0               ; no text here: bank 10 draws it
MHASSRUN EQU    0               ; the sound runs in the screen banks

        INCLUDE "../build/tail.inc"
        INCLUDE "devram.inc"
        INCLUDE "devaddr.inc"
        INCLUDE "mapram.inc"

; the map kernel's cells (dispmap.inc), which the town's kernel shares
FIGX    EQU     PV+0
MULX    EQU     PV+1
P0PTR   EQU     PV+2
P0PTRH  EQU     PV+3
P1PTR   EQU     PV+4
P1PTRH  EQU     PV+5
P0TOP   EQU     PV+6
P1TOP   EQU     PV+7
FIGCOL  EQU     PV+10
MULCOL  EQU     PV+11
FIGNUS  EQU     PV+12
BARCOL  EQU     PV+17
P0HV    EQU     PV+19
MULREF  EQU     PV+20

MAPYTOP EQU     16              ; y units: the first entry, map and town
WIN     EQU     DVBW            ; bit 7: walking in through a door (bits 0-2
                                ;   the building, bit 6 down); DVBW's low
                                ;   bit: waiting for the server
; the doors (develop.c door_lo/hi): the outfitters by good, then assay,
; land, pub, corral; x units

        ORG     $1000

YENTRY: lda     MSCR            ; the map's grant and production (mapx.inc)
        cmp     #BKMAP
        bne     YE0
        jmp     MAPX
YE0:    lda     MENT
        cmp     #ENRUN
        bne     YE1
        jsr     TFRAMEX
        jmp     XBACK
YE1:    cmp     #ENSTATE
        bne     YE2
        jsr     TREADX
        jmp     XBACK
; ENENTER: walked in (or the turn began here)
YE2:    lda     #11             ; the town's figures are 11 entries tall
        sta     P0HV
        jsr     TSPRITE
        lda     #1
        sta     MCOMP
        jmp     XBACK

; ===========================================================================
; The town.
; ===========================================================================

; TREADX -- a state has arrived, in the town.
TREADX: jsr     TURNX
        lda     FNRPLY+STFLAGS  ; the turn is over (the pub, the time): the
        and     #TF_ENDED       ;   map
        beq     TR1
        jsr     ACTRES
        lda     #DVOVER
        sta     DVST
        lda     #ENENTER
        sta     MENT
        lda     #BKDEV
        sta     MSCR
        jmp     MGOTO
TR1:    lda     DVBW            ; waiting in a doorway (not walking in):
        cmp     #1              ;   the answer puts us out
        bne     TR2
        lda     DVACT
        cmp     NREQ
        bne     TR2             ; not the answer yet
        jsr     ACTRES
        jsr     TSTREET
TR2:    lda     #1
        sta     MCOMP
        jmp     TSPRITE

; TSTREET -- back out on the street.
TSTREET: lda    #0
        sta     DVBW
        lda     #TWYTOP+2
        sta     WY
        lda     #4
        sta     WFACE
        rts

; TFRAMEX -- a frame in the town.
TFRAMEX: lda    MINP            ; SELECT ends the turn
        and     #IN_SEL
        beq     TF1
        lda     #RQEND
        ldx     #0
        ldy     #0
        jsr     QACT
TF1:    lda     DVBW            ; waiting in a doorway for the server
        bmi     TF2             ; (walking in)
        bne     TF9
TF2:    lda     WCNT
        beq     TF3
        dec     WCNT
        jmp     TSPRITE
TF3:    lda     DVBW
        bpl     TF4
        jsr     TWALKIN
        jmp     TSPRITE
TF4:    lda     INCUR
        and     #INDIRS
        bne     TF5
        lda     #4
        sta     WFACE
TF9:    jmp     TSPRITE
TF5:    jsr     STEPX
        lda     INCUR
        and     #IN_LEFT
        beq     TF6
        lda     #1
        jsr     SETFACE
        lda     WX
        cmp     #TWXLEFT+1
        bcs     TF5A
        jmp     TLEAVE
TF5A:   sec
        sbc     #4
        sta     WX
TF6:    lda     INCUR
        and     #IN_RIGHT
        beq     TF7
        lda     #0
        jsr     SETFACE
        lda     WX
        cmp     #TWXRGHT
        bcc     TF6A
        jmp     TLEAVE
TF6A:   clc
        adc     #4
        sta     WX
TF7:    lda     INCUR
        and     #IN_DOWN
        beq     TF8
        lda     #2
        jsr     SETFACE
        lda     WY
        cmp     #TWYBOT
        bcs     TF7A
        inc     WY
        inc     WY
        jmp     TSPRITE
TF7A:   ldx     #BASSAY         ; at the bottom of the street: a door below?
TF7B:   lda     WX
        cmp     DOORLO,x
        bcc     TF7C
        cmp     DOORHI,x
        bcc     TF7D
        beq     TF7D
TF7C:   inx
        cpx     #BCORRAL+1
        bne     TF7B
        jmp     TSPRITE
TF7D:   lda     DOORC,x         ; in through the middle of the door
        sta     WX
        txa                     ; walking in, down
        ora     #$C0
        sta     DVBW
        jmp     TSPRITE
TF8:    lda     INCUR
        and     #IN_UP
        beq     TF8X
        lda     #3
        jsr     SETFACE
        lda     WY
        cmp     #TWYTOP+1
        bcc     TF8A
        dec     WY
        dec     WY
        jmp     TSPRITE
TF8A:   lda     WMULE           ; the outfitters let you in with a M.U.L.E.
        beq     TF8X
        ldx     #3
TF8B:   lda     WX              ; anywhere under the building (DOORLO..HI)
        cmp     DOORLO,x
        bcc     TF8D
        cmp     DOORHI,x
        bcc     TF8C
        beq     TF8C
TF8D:   dex
        bpl     TF8B
TF8X:   jmp     TSPRITE
TF8C:   lda     DOORC,x         ; in through the middle of the door
        sta     WX
        txa                     ; walking in, up
        ora     #$80
        sta     DVBW
        jmp     TSPRITE

; TWALKIN -- a step through a door: 2 a step until inside, then the
; building's action.
TWALKIN: lda    #5
        sta     WCNT
        lda     WFACE
        eor     #$80
        sta     WFACE
        bit     DVBW
        bvc     TWI2
        lda     #2
        jsr     SETFACE
        inc     WY
        inc     WY
        lda     WY
        cmp     #TWYINDN
        bcs     TBLDG
        rts
TWI2:   lda     #3
        sta     WFACE
        dec     WY
        dec     WY
        lda     WY
        cmp     #TWYINUP+1
        bcc     TBLDG
        rts

; TBLDG -- inside: the building's action (develop.c dv_building).
TBLDG:  lda     DVBW
        and     #7
        tax
        lda     #1              ; waiting for the answer, unless nothing is sent
        sta     DVBW
        cpx     #BCORRAL
        bne     TB1
        lda     #RQBUY          ; the corral: a M.U.L.E., or it goes back
        ldy     WMULE
        beq     TBQ
        lda     #RQRET
        bne     TBQ
TB1:    cpx     #BASSAY
        bcs     TB2
        txa                     ; an outfitter: the good
        tax
        lda     #RQOUTF
        bne     TBQX
TB2:    bne     TB3
        ldx     DVSMP           ; the assay: the sample, if we took one
        cpx     #NPLOTS
        bcs     TB2A
        lda     #RQASSAY
        bne     TBQX
TB2A:   lda     #MSGNOSMP
        bne     TBOUT
TB3:    cpx     #BLAND
        bne     TB4
        lda     #1              ; the land office: the next plot pressed
        sta     DVOFF
        lda     #MSGOFFICE
        bne     TBOUT
TB4:    lda     #RQPUB          ; the pub
TBQ:    ldx     #0
TBQX:   sta     DVACT
        ldy     #0
        jmp     QACT
TBOUT:  sta     DMSG
        lda     #1
        sta     MCOMP
        jmp     TSTREET

; TLEAVE -- off one end of the street: the map.
TLEAVE: lda     #ENTOWN
        sta     MENT
        lda     #BKDEV
        sta     MSCR
        jmp     MGOTO

; TSPRITE -- the town's figure (bank 6's shapes) and the M.U.L.E.
TSPRITE: lda    WX
        sec
        sbc     #4
        sta     FIGX
        lda     WY
        sec
        sbc     #MAPYTOP
        sta     P0TOP
; the frame: species * 33 + (0/1 walking, 2 front) * 11
        lda     WFACE
        and     #7
        tax
        lda     FACET,x
        cpx     #2
        bcs     TS2
        bit     WFACE
        bpl     TS2
        clc
        adc     #1
TS2:    tax
        lda     MUL11,x
        sta     MTMP
        lda     WSP
        sta     MNUM            ; * 1
        asl     a
        asl     a
        asl     a
        asl     a
        asl     a               ; * 32
        clc
        adc     MNUM            ; * 33
        clc
        adc     MTMP
        ldx     #(TWFIGS)>>8
        bcc     TS2A
        inx
TS2A:   clc
        adc     #(TWFIGS)&$FF
        bcc     TS2B
        inx
TS2B:   jsr     P0SET
        lda     WFACE
        and     #7
        tax
        lda     #0
        cpx     #1
        bne     TS3
        lda     #8
TS3:    sta     FIGNUS
        sta     MULREF
        lda     WMULE
        beq     TS5
        lda     WTX
        beq     TS5
        sec
        sbc     #4
        sta     MULX
        lda     WTY
        clc
        adc     #3
        sec
        sbc     #MAPYTOP
        sta     P1TOP
        lda     #CWHITE
        sta     MULCOL
        lda     WFACE
        asl     a
        lda     #0
        rol     a
        tax
        lda     MUL12,x
        clc
        adc     #(TWMULE)&$FF
        ldx     #(TWMULE)>>8
        jmp     P1SET
TS5:    lda     #200
        sta     P1TOP
        rts

        INCLUDE "mapx.inc"
        INCLUDE "devcom.inc"
MUL11:  DB      0,11,22,33
MUL12:  DB      0,12
FACET:  DB      0,0,2,2,2

        INCLUDE "mlib.inc"
        INCLUDE "sound.inc"

; ---------------- data: tables, not code (tools/dataranges.py) ----------
DSTART:
; the doors: the outfitters by good (crystite at the left), then assay,
; land, pub, corral (x units). A door takes the figure anywhere under its
; building -- 4 units either side of develop.c's door column -- since a
; step is 4 units and three steps look "under" a 16-clock building; DOORC
; is the column it walks in by.
DOORLO: DB      116,92,68,44,44,64,84,104
DOORHI: DB      124,100,76,52,52,72,92,124
DOORC:  DB      120,96,72,48,48,68,88,116
; the map's plots (mapx.inc XPLACE): a column's POSOBJ x for the double-width
; box, a row's top entry (map.asm's PLOTX, ROWENT)
XPLOTX: DB      12,28,44,60,76,92,108,124,140
XROWENT: DB     0,16,32,48,64

        END
