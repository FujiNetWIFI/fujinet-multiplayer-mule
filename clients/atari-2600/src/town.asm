; town.asm -- bank 6: the town, in your turn: the picture.
;
; The map kernel (dispmap.inc) with the town's tables in ROM
; (tools/mktown.py): 72 entries, the buildings with their letters. Bank 11
; walks the figure (devx.asm), once a frame from this bank's overscan, as it
; does on the map; the town's figures are the species' town frames, every
; other row (11 entries), and the M.U.L.E.'s (12).
;
; The town leaves the planes' map tables alone (rows 2-15): the map comes
; back without the server's help.

        CPU     6502UNDOC
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     1
MHASDEC EQU     0
MHASCMP EQU     1
MHASMSG EQU     1
MHASTXT EQU     0               ; bank 10 draws the text
TICKROW EQU     1

        INCLUDE "../build/tail.inc"
        INCLUDE "devram.inc"
        INCLUDE "devaddr.inc"

MAPENT  EQU     72
P1H     EQU     12
MAPBG   EQU     CBLACK

        ORG     $1000

TENTRY: lda     MENT
        cmp     #ENRUN
        bne     TE1
        jmp     SLOOP
TE1:    cmp     #ENPOST
        bne     TE2
        jmp     SLOOP1
TE2:    cmp     #ENSTATE
        beq     THOP
; ENENTER: walked in (or the turn began here). Only the town's own text
; rows are cleared.
        lda     #BKTOWN
        sta     MSCR
        lda     #2
        sta     VBLANK
        jsr     DINIT
        lda     #0
        jsr     TCLR
        lda     #1
        jsr     TCLR
        lda     #MBOTROW
        jsr     TCLR
        lda     #0
        sta     SNOPOLL
        sta     FIGNUS
        sta     MULREF
        sta     BARPF1
        sta     BARPF2
        sta     BARPF1R
        sta     BARPF2R
        lda     #CWHITE
        sta     TXCOL
        sta     MULCOL
        lda     #11
        sta     P0HV
        lda     #200
        sta     P0TOP
        sta     P1TOP
        lda     #(TWFIGS)&$FF
        sta     P0PTR
        sta     P1PTR
        lda     #(TWFIGS)>>8
        sta     P0PTRH
        sta     P1PTRH
        lda     #OSTIM
        sta     TIM64T
        lda     #0
        sta     VBLANK
THOP:   lda     #BKTOWNX
        jmp     MGOTO

; TCLR -- blank text row A, and wait (bounded) for the cart to have done it.
TCLR:   jsr     FNROWA
        ldx     FNBTXG
        lda     #0
        sta     FNRSEL+FH_TEND
        ldy     #128
TCL1:   cpx     FNBTXG
        bne     TCL2
        dey
        bne     TCL1
TCL2:   rts

; ---------------------------------------------------------------------------
APPVBL: jsr     SNDRUN
        jsr     INSCAN
        jsr     RSTCHK
        jsr     TICKER
        jmp     DBAR

STICK:  lda     #ENRUN
        sta     MENT
        lda     #BKTOWNX
        jmp     MGOTO

        INCLUDE "devbar.inc"
        INCLUDE "ticker.inc"

MBGT:   DB      0

MT0     EQU     TWZERO
MT1     EQU     TWT1
MT2     EQU     TWT2
MT3     EQU     TWT3
MT4     EQU     TWT4
MT5     EQU     TWZERO
MCOLTAB EQU     TWCOL

        INCLUDE "mlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "scrloop.inc"
        INCLUDE "dispmap.inc"

; ---------------- data at fixed addresses (devaddr.inc) ----------------
        IF      * > TWDATA
        ERROR   "town.asm: the code runs into the data at TWDATA"
        ENDIF
DSTART:
        INCLUDE "../build/gen/town.inc"
        ORG     TWMULE
        INCLUDE "../build/gen/town2spr.inc"

        END
