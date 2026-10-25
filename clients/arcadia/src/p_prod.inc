; p_prod.inc -- production and the colony events (mekkogx events.c): the
; map with the round's event played out (the fx page's actors; the
; planetquake here: the screen shakes and plots jolt for 300 frames), then
; "PRODUCTION n" and each plot's units.

        BCTA,UN PRENTER         ; E_ENTER
        RETC,UN                 ; E_STATE (the map keeps the plots current)
        NOP
        NOP
        BCTA,UN PRTICK          ; E_TICK

        ZONE    UNIONL,UNIONLE
        VAR     PREV,1          ; the event (CE_*)
        VAR     PRDONE,1        ; 1: over, production shown
        VAR     PRT0H,1         ; the quake's start (frame)
        VAR     PRT0L,1
        VAR     PRLAST,1        ; its last jolt (frame, low byte)

        INCLUDE "src/mapscr.inc"

PRENTER: EORZ   R0
        LPUT    MAPDOTS
        BSTA,UN MAPSCR
        EORZ    R0
        LPUT    PRDONE
        LODA,R0 C_EVENT
        LPUT    PREV
        LODA,R0 PREV
        BCTR,EQ PRE2
        COMI,R0 CE_SHIPRETURNS
        BCTR,GT PRE2
        SUBI,R0 1               ; its title, what it did
        ADDI,R0 T_EVENT
        STRZ    R1
        BSTA,UN TITLE
        LODA,R0 PREV
        SUBI,R0 1
        ADDI,R0 M_EVENT
        STRZ    R1
        ZBSR    *ZSAY
        SND     SND_EVENT
        LODA,R0 PREV
        COMI,R0 CE_QUAKE
        BCFR,EQ PRE1
        LODA,R0 FRMH            ; the quake: ours
        LPUT    PRT0H
        LODA,R0 FRML
        LPUT    PRT0L
        RETC,UN
PRE1:   STRZ    R3              ; the others: the fx page's
        LODA,R0 PRFX,R3
        BCTA,UN FXSTART
PRE2:   LODI,R0 1               ; none: production straight away
        LPUT    PRDONE
        LODI,R1 T_PRODUCT
        BCTA,UN TITLE
; the fx script for each event (the quake has none)
PRFX:   DB      0,FX_PEST,FX_PIRATE,FX_ACID,0,FX_SUN,FX_METEOR,FX_RADIAT,FX_FIRE,FX_LAND

PRTICK: LODA,R0 PRDONE
        BCTR,EQ PRT1
        LODA,R0 MAPDOTS         ; the units on the plots (the map draws them)
        RETC,GT
        LODI,R0 1
        LPUT    MAPDOTS
        RETC,UN
PRT1:   LODA,R0 PREV
        COMI,R0 CE_QUAKE
        BCTA,EQ PRQUAKE
        LODA,R0 FXFLAG          ; over when the script says, or stops
        COMI,R0 1
        BCTR,EQ PRT2
        LODA,R0 FXON
        RETC,GT
PRT2:   EORZ    R0              ; production_tick's end
        LPUT    FXON
        LODI,R0 1
        LPUT    PRDONE
        LODI,R1 T_PRODUCT
        BCTA,UN TITLE

; PRQUAKE: 300 frames: the screen shifts and jolts, a plot outlined every
; 4 frames; then it settles.
PRQUAKE: LODA,R1 PRT0H
        LODA,R2 PRT0L
        ZBSR    *ZSINCE
        COMI,R1 300>>8
        BCTR,LT PRQ1
        BCFA,EQ PRQ5
        COMI,R2 300&$FF
        BCFR,LT PRQ5
PRQ1:   LODA,R0 FRML            ; every 4 frames
        SUBA,R0 PRLAST
        COMI,R0 4
        RETC,LT
        LODA,R0 FRML
        LPUT    PRLAST
        LODA,R0 FRML            ; a plot, near enough at random: (frame *
        STRZ    R1              ;  37 + frame / 8) mod 45
        LODI,R2 37
        ZBSR    *ZMUL8
        LODA,R0 FRML
        RRR,R0
        RRR,R0
        RRR,R0
        ANDI,R0 $1F
        ADDZ    R2
        STRZ    R2
        LODI,R1 0
        LODI,R3 NUM_PLOTS
        ZBSR    *ZDIV8
        LODZ    R1              ; (the remainder)
        LPUT    MAPBLINK
        LODI,R0 1
        LPUT    MAPBLON
        LODA,R0 FRML            ; the screen's shift: VOLUME's top bits
        ANDI,R0 $E0
        STRA,R0 SHAKE
        RETC,UN
PRQ5:   LODI,R0 255             ; settled
        LPUT    MAPBLINK
        EORZ    R0
        STRA,R0 SHAKE
        BCTA,UN PRT2
