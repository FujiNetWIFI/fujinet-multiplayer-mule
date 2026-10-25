; mapscr.inc -- what every page that shows the map shares (each includes
; its own copy: a page reaches only fixed code and itself).

; MAPSCR: the map screen: cleared, the map asked for (the map page draws
; it, fixb.inc SERVICES), its title row, the message box rows 13-20, no
; sprites (SCRG26), nothing outlined, no time bar. Clobbers R0-R3.
MAPSCR: BSTA,UN SCRG26
        LODI,R0 1
        LPUT    MAPREQ
        LODI,R0 SCR_MAP
        LPUT    SCRNOW
        LODI,R0 13
        LPUT    MSGROW
        LODI,R0 8
        LPUT    MSGN
        LODI,R0 255
        LPUT    MAPBLINK
        EORZ    R0
        LPUT    MAPBLON
        LODI,R0 255
        LPUT    MAPHI
        EORZ    R0
        LPUT    BARST
        RETC,UN

; TITLE: title R1 on row 0, its '%' the round. Clobbers R0-R3, T*.
TITLE:  STRA,R1 T2
        EORZ    R0
        LPUT    SAYV1H
        LODA,R0 H_ROUND
        LPUT    SAYV1L
        LODA,R1 T2
        LODI,R2 0
        LODI,R3 1
        ZBRR    *ZSAYBX

; PENOW: R1:R2 = the phase's jiffies now: rx_pe + (frame - rx_frame)
; (mekkogx pe_now). Clobbers R0.
PENOW:  LODA,R1 RXFRH
        LODA,R2 RXFRL
        ZBSR    *ZSINCE
        LODZ    R2
        ADDA,R0 RXPEL
        STRZ    R2
        PPSL    PSLWC
        LODZ    R1
        ADDA,R0 RXPEH
        CPSL    PSLWC
        STRZ    R1
        RETC,UN

; FXSTART: start sprite script R0 (FX_*) on the fx page; FXFLAG 0 until it
; signals. Clobbers R0-R3.
FXSTART: LPUT   FXREQ
        EORZ    R0
        LPUT    FXFLAG
        LODI,R0 1
        LPUT    FXON
        RETC,UN
