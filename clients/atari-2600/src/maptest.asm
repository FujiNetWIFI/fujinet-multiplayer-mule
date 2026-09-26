; maptest.asm -- the layout build's bank 0: the map kernel with tables baked
; into ROM (build/maptab.inc, from tools/mulemap.py's test map), a figure on
; the stick and the M.U.L.E. behind it. No network.

        CPU     6502UNDOC
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     1
MHASDEC EQU     0
MHASCMP EQU     0

        INCLUDE "../build/tail.inc"

        ORG     $1000

TENTRY: ldx     #$FF
        txs
        lda     #0
TCL1:   sta     $00,x
        dex
        bne     TCL1
        sta     $00
        lda     #2
        sta     VBLANK
        jsr     FNARM
        jsr     DINIT
        jsr     FNCLS
        lda     #0
        jsr     FNROWA
        ldx     #(S1)&$FF
        ldy     #(S1)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        lda     #1
        jsr     FNROWA
        ldx     #(S2)&$FF
        ldy     #(S2)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        lda     #MBOTROW
        jsr     FNROWA
        ldx     #(S3)&$FF
        ldy     #(S3)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        lda     #CWHITE
        sta     TXCOL
        lda     #CPLAY1
        sta     FIGCOL
        lda     #$0A
        sta     MULCOL
        lda     #50
        sta     FIGX
        lda     #40
        sta     MULX
        lda     #200
        sta     P0TOP
        lda     #200
        sta     P1TOP
        lda     #0
        sta     FIGNUS
        lda     #8
        sta     P0HV
        lda     #CBLUE
        sta     BARCOL
        lda     #$FF
        sta     BARPF1
        sta     BARPF2
        lda     #$F0
        sta     BARPF1R
        jsr     SETPTR
        lda     #0
        sta     VBLANK
        jmp     DFRESH

; the pointers: shape - top
SETPTR: lda     #(FIG)&$FF
        sec
        sbc     P0TOP
        sta     P0PTR
        lda     #(FIG)>>8
        sbc     #0
        sta     P0PTRH
        lda     #(MUL)&$FF
        sec
        sbc     P1TOP
        sta     P1PTR
        lda     #(MUL)>>8
        sbc     #0
        sta     P1PTRH
        rts

APPVBL: jsr     INSCAN
        lda     INCUR
        and     #IN_LEFT
        beq     AV1
        dec     FIGX
AV1:    lda     INCUR
        and     #IN_RIGHT
        beq     AV2
        inc     FIGX
AV2:    lda     INCUR
        and     #IN_UP
        beq     AV3
        dec     P0TOP
AV3:    lda     INCUR
        and     #IN_DOWN
        beq     AV4
        inc     P0TOP
AV4:    lda     MINP
        and     #IN_FIRE
        beq     AV5
        lda     FIGNUS
        eor     #5
        sta     FIGNUS
AV5:    jmp     SETPTR

S1:     DB      "DEVELOPMENT",0
S2:     DB      "  ROUND 1",0
S3:     DB      "$1000 F3 E2",0

MBGT:   DB      0

        ALIGN   256
        INCLUDE "../build/maptab.inc"
FIG:    DB      $18,$3C,$18,$7E,$99,$18,$24,$66
MUL:    DB      $00,$06,$FE,$7E,$42,$42,$00,$00

        INCLUDE "mlib.inc"
        INCLUDE "dispmap.inc"

        END
