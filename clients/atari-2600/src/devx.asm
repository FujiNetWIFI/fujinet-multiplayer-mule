; devx.asm -- bank 11: the walker's logic on the map (bank 5); bank 12 is
; the town's. MekkoGX develop.c.
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

XENTRY: lda     MENT
        cmp     #ENRUN
        bne     XE1
        jsr     MFRAMEX
        jmp     XBACK
XE1:    cmp     #ENSTATE
        bne     XE2
        jsr     MREADX
        jmp     XBACK
XE2:    cmp     #ENTOWN
        bne     XENTER
        jmp     XFROMT
; ENENTER: my turn has come -- or it ended in the town (DVOVER: the town
; has said why, and that stands)
XENTER: lda     DVST
        cmp     #DVOVER
        beq     XEN1
        lda     #0
        sta     DVST
        sta     WTOWN
        sta     WMULE
        sta     DVBW
        sta     DVOFF
        sta     DVACT
        sta     DMSG
        lda     #$FF
        sta     DVSMP
        sta     WPPLOT
        sta     DVLIST
XEN1:   jsr     MREADX
        jmp     XBACK

; ENTOWN: out of the town at one end: beside the store, and the first step
; waits 20 frames.
XFROMT: lda     #0
        sta     WTOWN
        sta     WTX
        lda     #52
        sta     WY
        lda     #68
        ldx     WX
        cpx     #TWXLEFT+1
        bcc     XFT1
        lda     #99
XFT1:   sta     WX
        lda     #20
        sta     WCNT
        lda     #4
        sta     WFACE
        lda     #8
        sta     P0HV
        jsr     MSPRITE
        lda     #1
        sta     MCOMP
        jmp     XBACK

; ===========================================================================
; The map.
; ===========================================================================

; MREADX -- a state has arrived, on the map.
MREADX: lda     #0              ; the map, composed by the cartridge
        tax
        lda     #(SMAP)&$FF
        ldy     #0
        jsr     FNBARG
        lda     #FB_MULEMAP
        jsr     FNBLIT
        jsr     TURNX
        lda     FNRPLY+STFLAGS
        and     #TF_START
        bne     MR3
; not begun: the prompt, and the bell
        lda     DVST
        cmp     #DVPROMPT
        beq     MR9
        lda     #DVPROMPT
        sta     DVST
        lda     #MSGSTART
        sta     DMSG
        lda     #SNDBELL
        jsr     SNDPLAY
        jmp     MR9
MR3:    lda     FNRPLY+STFLAGS
        and     #TF_ENDED
        beq     MR4
        lda     DVST            ; over: the map, and nothing to walk
        cmp     #DVOVER
        beq     MR3A
        lda     #DVOVER
        sta     DVST
        lda     DMSG            ; the pub ended it: its winnings stand
        cmp     #MSGPUB
        beq     MR3A
        lda     FNRPLY+STLEFT   ; out of time, not the pub or SELECT
        ora     FNRPLY+STLEFT+1
        bne     MR3A
        lda     #MSTIME
        sta     MSGREQ
MR3A:   jsr     MSPRITE
        jmp     MR9
MR4:    lda     DVST
        cmp     #DVWALK
        beq     MR6
; the walk starts in the middle of the town, with the turn's event
        lda     #DVWALK
        sta     DVST
        lda     #MSGNONE
        sta     DMSG
        lda     FNRPLY+STEVENT
        beq     MR4A
        cmp     #23
        bcs     MR4A
        cmp     #20
        bcs     MR4B
        ora     #MSEVENT        ; events 1-19: bank 13
        bne     MR4C
MR4B:   adc     #MSEV20-20-1    ; 20-22: bank 14's 27-29 (carry is set)
MR4C:   sta     MSGREQ
MR4A:
        lda     #TWXSTRT
        sta     WX
        lda     #TWYTOP+2
        sta     WY
        lda     #1
        sta     WTOWN
        lda     #0
        sta     WTX
        lda     #ENENTER
        sta     MENT
        lda     #BKTOWN
        sta     MSCR
        jmp     MGOTO
; what the server said about a map action
MR6:    jsr     ACTRES
        jsr     MSPRITE
MR9:    lda     #1
        sta     MCOMP
        rts

; MFRAMEX -- a frame on the map.
MFRAMEX: lda    DVST
        cmp     #DVPROMPT
        bne     MF1
        lda     MINP            ; the prompt: the button begins
        and     #IN_FIRE
        beq     MF0
        lda     #RQBEGIN
        ldx     #0
        ldy     #0
        jsr     QACT
MF0:    rts
MF1:    cmp     #DVWALK
        bne     MF9
        lda     MINP            ; SELECT ends the turn
        and     #IN_SEL
        beq     MF2
        lda     #RQEND
        ldx     #0
        ldy     #0
        jsr     QACT
MF2:    jsr     MWALK
        lda     MINP
        and     #IN_FIRE
        beq     MF9
        jsr     MBUTTON
MF9:    jmp     MSPRITE

; MWALK -- a step on the map, when its time comes.
MWALK:  lda     WCNT
        beq     MW1
        dec     WCNT
        rts
MW1:    lda     INCUR
        and     #INDIRS
        bne     MW2
        lda     #4              ; standing
        sta     WFACE
        rts
MW2:    jsr     STEPX           ; the M.U.L.E.'s place, the walk's toggle
        lda     INCUR
        and     #IN_LEFT
        beq     MW3
        lda     WX
        cmp     #16
        bcc     MW3
        dec     WX
        dec     WX
        lda     #1
        jsr     SETFACE
MW3:    lda     INCUR
        and     #IN_RIGHT
        beq     MW4
        lda     WX
        cmp     #157
        bcs     MW4
        inc     WX
        inc     WX
        lda     #0
        jsr     SETFACE
MW4:    lda     INCUR
        and     #IN_UP
        beq     MW5
        lda     WY
        cmp     #17
        bcc     MW5
        dec     WY
        lda     INCUR
        and     #IN_LEFT|IN_RIGHT
        bne     MW5
        lda     #3
        jsr     SETFACE
MW5:    lda     INCUR
        and     #IN_DOWN
        beq     MW6
        lda     WY
        cmp     #92
        bcs     MW6
        inc     WY
        lda     INCUR
        and     #IN_LEFT|IN_RIGHT
        bne     MW6
        lda     #2
        jsr     SETFACE
MW6:    jsr     DTERR
; into the store (plot 22: x 80..95, y 48..63): the town
        lda     WX
        clc
        adc     #5
        cmp     #80
        bcc     MW9
        lda     WX
        clc
        adc     #2
        cmp     #96
        bcs     MW9
        lda     WY
        clc
        adc     #3
        cmp     #48
        bcc     MW9
        lda     WY
        cmp     #64
        bcs     MW9
        lda     #TWXLEFT        ; in at the side we came from
        ldx     WX
        cpx     #88
        bcc     MW7
        lda     #TWXRGHT
MW7:    sta     WX
        lda     #TWYTOP+2
        sta     WY
        lda     #1
        sta     WTOWN
        lda     #0
        sta     WTX
        lda     #ENENTER
        sta     MENT
        lda     #BKTOWN
        sta     MSCR
        jmp     MGOTO           ; (the return address is left: GOTO resets SP)
MW9:    rts

; DTERR -- the next step is slow over the river (+8) and a plot's mountains
; (+15): the figure's middle, against the river's band and against the
; corner of a plot the peaks stand in (develop.c walk_terrain). Read from
; the map's playfield tables, which hold still (the reply window does not).
DTR0:   rts
DTERR:  lda     WX              ; cx = x - 6: colour clocks
        sec
        sbc     #6
        cmp     #8
        bcc     DTR0
        sta     MTMP
        lda     WY              ; cy = (y - 8) * 2 + 2: scanlines
        sec
        sbc     #8
        asl     a
        clc
        adc     #2
        cmp     #16
        bcc     DTR0
        sta     MTMP2
        lda     MTMP            ; plot column (cx - 8) / 16
        sec
        sbc     #8
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        cmp     #9
        bcs     DTR0
        sta     MNUM
        lda     MTMP2           ; plot row (cy - 16) / 32
        sec
        sbc     #16
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        cmp     #5
        bcs     DTR0
        tax
        lda     ROW9,x
        clc
        adc     MNUM
        tax                     ; the plot
        lda     MNUM
        cmp     #4
        bne     DTR5
        cpx     #STOREPL        ; column 4 is the river, but for the store
        beq     DTR9
        lda     MTMP            ; the river: cx + 4 >= 76 and cx <= 83
        clc
        adc     #4
        cmp     #76
        bcc     DTR9
        lda     MTMP
        cmp     #84
        bcs     DTR9
        lda     WCNT
        clc
        adc     #8
        sta     WCNT
        rts
; mountains: the plot's lowest ground row has ink
DTR5:   ldy     #11
        jsr     PLOTPIX
        beq     DTR9
        lda     MNUM            ; plot x = col * 16 + 8
        asl     a
        asl     a
        asl     a
        asl     a
        clc
        adc     #8
        sta     MNUMH
        lda     MTMP            ; cx + 4 > px + 1 and cx < px + 9
        clc
        adc     #4
        sec
        sbc     MNUMH
        bcc     DTR9
        cmp     #2
        bcc     DTR9
        lda     MTMP
        sec
        sbc     MNUMH
        bcs     DTR6
        lda     #0
DTR6:   cmp     #9
        bcs     DTR9
        lda     WCNT
        clc
        adc     #15
        sta     WCNT
DTR9:   rts

; PLOTPIX -- Z clear if plot X has ink at its entry Y (0-15) in the map's
; playfield tables. A plot is 4 playfield pixels, which may straddle two
; registers: PFREGA/PFMSKA and PFREGB/PFMSKB say where, by column.
PLOTPIX: sty    MTMP2
        jsr     PLOTCR          ; X: the column, A: the row
        stx     MNUMH
        asl     a
        asl     a
        asl     a
        asl     a
        clc
        adc     MTMP2
        tay                     ; the entry in the tables
        ldx     MNUMH
        lda     PFREGA,x
        jsr     PLPTR
        lda     (FNPTRL),y
        and     PFMSKA,x
        bne     PLX
        lda     PFREGB,x
        jsr     PLPTR
        lda     (FNPTRL),y
        and     PFMSKB,x
PLX:    rts
PLPTR:  lsr     a               ; register r: plane r at $1800 + r * $80
        pha
        lda     #0
        ror     a               ; (r & 1) * $80
        adc     #MAP0
        sta     FNPTRL
        pla
        adc     #(FNTEXT)>>8
        sta     FNPTRH
        rts
MAP0    EQU     12

; MBUTTON -- the button on the map (develop.c dv_button).
MBUTTON: lda    #$FF            ; the plot under the figure's middle
        sta     MNUM
        lda     WX
        sec
        sbc     #12
        bcc     MB2
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        cmp     #9
        bcs     MB2
        sta     MTMP
        lda     WY
        clc
        adc     #2
        sec
        sbc     #16
        bcc     MB2
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        cmp     #5
        bcs     MB2
        tax
        lda     ROW9,x
        clc
        adc     MTMP
        sta     MNUM
MB2:
; the wampus, if it shows and we lead nothing: within 8 of it
        lda     WMULE
        bne     MB4
        jsr     DWSHOW
        bcc     MB4
        lda     WX
        clc
        adc     #8
        cmp     MNUMH           ; the wampus's x (DWSHOW)
        bcc     MB4
        lda     MNUMH
        clc
        adc     #8
        cmp     WX
        bcc     MB4
        lda     #RQWAMP
        bne     MBQ0
MB4:    ldx     MNUM
        cpx     #NPLOTS
        bcs     MB9
        lda     WMULE
        beq     MB5
        lda     #RQINST         ; install the M.U.L.E. here
        bne     MBQ
MB5:    lda     DVOFF
        beq     MB6
        lda     #0              ; the land office: list, or withdraw the one
        sta     DVOFF           ;   listed before
        stx     MTMP
        ldy     HSEAT           ; mine: my bracket's row has ink
        jsr     PLOTPIX
        beq     MB9
        ldx     MTMP
        lda     #RQSELL
        cpx     DVLIST
        bne     MB5A
        lda     #RQUNSEL
        ldy     #$FF
        sty     DVLIST
        bne     MBQ
MB5A:   stx     DVLIST
        bne     MBQ
MB6:    stx     DVSMP           ; a sample, for the assay office
        lda     #MSGSMP
        sta     DMSG
        lda     #1
        sta     MCOMP
MB9:    rts
MBQ0:   ldx     #0
MBQ:    sta     DVACT
        ldy     #0
        jmp     QACT

; DWSHOW -- carry set if the wampus shows now; MNUMH its x, MTMP2 its y
; (units): on the plot's mountains, 2 a mountain in.
DWSHOW: lda     WPPLOT
        cmp     #NPLOTS
        bcs     DWS9
        jsr     DELAPSED
        lda     MTMP            ; from <= now
        cmp     WPFROM
        lda     MTMP2
        sbc     WPFROMH
        bcc     DWS9
        lda     MTMP            ; now < to
        cmp     WPTO
        lda     MTMP2
        sbc     WPTOH
        bcs     DWS9
        ldx     WPPLOT
        jsr     PLOTCR
        sta     MTMP2           ; (the row, for below)
        txa                     ; x = col * 16 + 17 + mount * 2
        asl     a
        asl     a
        asl     a
        asl     a
        clc
        adc     #17
        adc     WPMNT
        adc     WPMNT
        sta     MNUMH
        lda     MTMP2           ; y = row * 16 + 17 + mount * 2
        asl     a
        asl     a
        asl     a
        asl     a
        clc
        adc     #17
        adc     WPMNT
        adc     WPMNT
        sta     MTMP2
        sec
        rts
DWS9:   clc
        rts

; MSPRITE -- the map's figure (bank 5's shapes, devaddr.inc) and what player
; 1 shows: the M.U.L.E. a step behind, or the wampus flashing, or nothing.
MSPRITE: lda    DVST
        cmp     #DVWALK
        beq     MS1
        lda     #200
        sta     P0TOP
        sta     P1TOP
        rts
MS1:    lda     WX
        sec
        sbc     #4              ; POSOBJ's x is 4 clocks right: x units - 8 + 4
        sta     FIGX
        lda     WY
        sec
        sbc     #MAPYTOP
        sta     P0TOP
; the frame: species * 56 + (0/1 walking right, 2 standing, 3/4 down, 5/6 up)
        lda     WFACE
        and     #7
        tax
        lda     FACEM,x
        cpx     #4
        beq     MS2
        bit     WFACE           ; the toggle: the second frame
        bpl     MS2
        clc
        adc     #1
MS2:    asl     a
        asl     a
        asl     a
        sta     MTMP
        lda     WSP
        asl     a
        asl     a
        asl     a
        sta     MNUM            ; * 8
        asl     a
        asl     a
        asl     a               ; * 64
        sec
        sbc     MNUM            ; * 56
        clc
        adc     MTMP
        ldx     #(DVFIGS)>>8
        bcc     MS2A
        inx
MS2A:   clc
        adc     #(DVFIGS)&$FF
        bcc     MS2B
        inx
MS2B:   jsr     P0SET
        lda     WFACE           ; a left-facing figure is mirrored
        and     #7
        tax
        lda     #0
        cpx     #1
        bne     MS3
        lda     #8
MS3:    sta     FIGNUS
        sta     MULREF
; player 1
        lda     WMULE
        beq     MS6
        lda     WTX
        bne     MS4
MS3A:   lda     #200
        sta     P1TOP
        rts
MS4:    sec
        sbc     #4
        sta     MULX
        lda     WTY
        sec
        sbc     #MAPYTOP
        sta     P1TOP
        lda     #CWHITE
        sta     MULCOL
        lda     WFACE           ; its step: the figure's toggle
        asl     a
        lda     #0
        rol     a
        asl     a
        asl     a
        asl     a
        clc
        adc     #(DVMULE)&$FF
        ldx     #(DVMULE)>>8
        jmp     P1SET
MS6:    jsr     DWSHOW
        bcc     MS3A
        lda     MFRAME          ; flashing: 8 frames on, 8 off
        and     #8
        beq     MS3A
        lda     MNUMH
        sec
        sbc     #12
        sta     MULX
        lda     MTMP2
        sec
        sbc     #MAPYTOP
        sta     P1TOP
        lda     #CWHITE
        sta     MULCOL
        lda     #(DVWAMP)&$FF
        ldx     #(DVWAMP)>>8
        jmp     P1SET

; PLOTCR -- plot X: its column in X, its row in A.
PLOTCR: txa
        ldx     #0
PCR1:   cmp     #9
        bcc     PCR2
        sbc     #9
        inx
        bne     PCR1
PCR2:   pha                     ; the column
        txa                     ; the row
        tay
        pla
        tax
        tya
        rts

        INCLUDE "devcom.inc"
ROW9:   DB      0,9,18,27,36
FACEM:  DB      0,0,3,5,2

        INCLUDE "mlib.inc"
        INCLUDE "sound.inc"

; ---------------- data: tables, not code (tools/dataranges.py) ----------
DSTART:
; A plot column's four playfield pixels (tools/mulemap.py pixel_reg):
; register 0-5 (PF0/1/2 left, then right) and the bits, twice.
PFREGA: DB      0,1,1,2,2,3,4,4,5
PFMSKA: DB      $C0,$3C,$03,$3C,$C0,$C0,$3C,$03,$3C
PFREGB: DB      1,1,2,2,3,4,4,5,5
PFMSKB: DB      $C0,$00,$03,$00,$30,$C0,$00,$03,$00

        END
