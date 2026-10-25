; p_map.inc -- the planet: 9 x 5 plots and the time bar (mekkogx map.c).
; The main loop calls it (fixb.inc SERVICES): E_ENTER when a phase asks
; for the map (MAPREQ), E_TICK every frame it is up (MAPON). A phase never
; draws a plot itself; it changes the state, or the bookkeeping here
; (MAPDOTS, MAPBLINK/MAPBLON, MAPHI, BARST/BARCOL), and the plots follow
; within five frames.
;
; A plot is two cells, one above the other (8 x 16 pixels, the Atari
; plot's shape on the Arcadia's wide pixels):
;   rows 2-11   plot (c, r) at column 3 + c, rows 2 + 2r (T) and 3 + 2r (B)
;   T           its land: plain, 1-3 peaks (user glyphs $3C-$3E), the
;               river ($3F, flowing: rewritten every 8 frames), the store
;   B           what is on it: a M.U.L.E. (its good's letter), production
;               (the units), owned land without a M.U.L.E. (a flag)
; An owned plot is drawn in its owner's ink, both cells; one outlined in
; white (up for auction, a computer's land) draws white. Column 12 starts
; block graphics and column 13 is the time bar, 20 half cells high.

        BCTA,UN MAPENTER        ; E_ENTER
        RETC,UN                 ; E_STATE
        NOP
        NOP
        BCTA,UN MAPTICK         ; E_TICK

GPEAK1  EQU     GUDC+4          ; the map's user glyphs
GRIVER  EQU     GUDC+7

; MAPENTER: the whole map now: its glyphs, the bar's column, every plot.
MAPENTER:
        LODI,R0 7               ; the peaks' glyphs, as the queue has room
        CPUT    MAPGL
        EORZ    R0              ; (the river comes with its first flow)
        STRA,R0 MAPRV
        LODI,R3 2*16+12-16      ; columns 12-15 of rows 2-11
MPE1:   ADDI,R3 16
        LODI,R0 GMOSAIC
        STRA,R0 SCR0,R3
        EORZ    R0
        STRA,R0 SCR0+1,R3
        STRA,R0 SCR0+2,R3
        STRA,R0 SCR0+3,R3
        COMI,R3 11*16+12
        BCFR,EQ MPE1
        LODI,R0 $FF
        LPUT    BARDRWN
        LODI,R1 0
MPE2:   STRA,R1 MAPK
        BSTA,UN MPLOT
        LODA,R1 MAPK
        ADDI,R1 1
        COMI,R1 NUM_PLOTS
        BCFR,EQ MPE2
        EORZ    R0
        STRA,R0 MAPK
        LODI,R0 1
        LPUT    MAPON
        RETC,UN

; MAPTICK: the peaks' glyphs still to load; nine plots (all 45 in five
; frames), the river's flow, the bar.
MAPTICK:
        LODI,R3 MAPGL-$2900
        LODA,R0 *PRAMH,R3
        BCTR,EQ MPT0
        STRA,R0 T3
        LODI,R2 4               ; the lowest still to load
        LODI,R1 ART_PEAK1&$FF
        TMI,R0  1
        BCTR,EQ MPG1
        LODI,R2 5
        LODI,R1 ART_PEAK2&$FF
        TMI,R0  2
        BCTR,EQ MPG1
        LODI,R2 6
        LODI,R1 ART_PEAK3&$FF
MPG1:   LODA,R0 UDCQN
        COMI,R0 3
        BCFR,LT MPT0
        STRA,R2 T2
        LODI,R0 ART_PEAK1>>8
        LODI,R3 PG_ART
        ZBSR    *ZUDCQ
        LODA,R3 T2              ; its bit off
        LODA,R0 MPGBIT-4,R3
        ANDA,R0 T3
        CPUT    MAPGL
MPT0:   LODI,R0 9
        STRA,R0 T3
MPT1:   LODA,R1 MAPK
        BSTA,UN MPLOT
        LODA,R1 MAPK
        ADDI,R1 1
        COMI,R1 NUM_PLOTS
        BCFR,EQ MPT2
        LODI,R1 0
MPT2:   STRA,R1 MAPK
        LODA,R0 T3
        SUBI,R0 1
        STRA,R0 T3
        BCFR,EQ MPT1
        LODA,R0 MAPRV           ; the river flows: a phase every 8 frames
        SUBA,R0 DT
        STRA,R0 MAPRV
        BCTR,GT MPT3
        LODI,R0 8
        STRA,R0 MAPRV
        LODA,R0 FRML
        ANDI,R0 $18             ; (phase * 8: the art's offset)
        ADDI,R0 ART_RIVER&$FF
        STRZ    R1
        LODI,R0 ART_RIVER>>8
        LODI,R2 7
        LODI,R3 PG_ART
        ZBSR    *ZUDCQ
MPT3:   LODA,R0 BARST           ; the bar, when it changed
        COMA,R0 BARDRWN
        RETC,EQ
; MBAR: column 13, rows 11 up to 2: each cell two steps, full, half or none.
        LPUT    BARDRWN
        LODA,R0 BARST
        STRA,R0 T0
        LODI,R3 11*16+13
MBR1:   LODA,R0 T0
        COMI,R0 2
        BCTR,LT MBR2
        SUBI,R0 2
        STRA,R0 T0
        LODI,R0 $36             ; both halves (two thirds of the width)
        BCTR,UN MBR4
MBR2:   COMI,R0 1
        BCFR,EQ MBR3
        EORZ    R0
        STRA,R0 T0
        LODI,R0 $30             ; the bottom half
        BCTR,UN MBR4
MBR3:   EORZ    R0
        BCTR,UN MBR5
MBR4:   IORA,R0 BARCOL
MBR5:   STRA,R0 SCR0,R3
        SUBI,R3 16
        COMI,R3 2*16+13-16
        BCFR,EQ MBR1
        RETC,UN

; MPLOT: plot R1's two cells, as the state and the bookkeeping say, onto
; the screen where they differ (mekkogx map_draw_plot). Calls one down.
; Clobbers R0-R3, T0-T2.
MPLOT:  LODA,R0 PLOFF,R1        ; T0: its T cell's offset
        STRA,R0 T0
        LODA,R0 MP_TERRAIN,R1
        STRA,R0 T1              ; T1: its land
        LODA,R0 MP_OWNER,R1
        STRA,R0 T2              ; T2: its owner (>3 none)
        LODI,R2 $FF             ; R2: its ink ($FF: neutral)
        LODZ    R1              ; outlined in white? (for sale, a computer's)
        COMA,R0 MAPBLINK
        BCFR,EQ MPL1
        LODA,R0 MAPBLON
        BCFR,EQ MPLW
MPL1:   LODA,R0 T2
        COMI,R0 4
        BCFR,LT MPL2            ; nobody's
        COMA,R0 MAPHI           ; the seat whose land is outlined
        BCTR,EQ MPLW
        STRZ    R3              ; the owner's ink, by its colour
        LODA,R0 PL29,R3
        STRZ    R3
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATINK,R3
        STRZ    R2
        BCTR,UN MPL2
MPLW:   LODI,R2 CWHITE|$01      ; white, and owned however it is (bit 0)
MPL2:   LODA,R0 T1              ; ---- the T cell: the land ----
        ANDI,R0 3
        COMI,R0 TERRAIN_STORE
        BCFR,EQ MPL3
        LODI,R0 GDOLLAR         ; the store: '$' over a building
        LODA,R3 T0
        STRA,R0 SCR0,R3
        LODI,R0 GSOLID
        STRA,R0 SCR0+16,R3
        RETC,UN
MPL3:   COMI,R0 TERRAIN_RIVER
        BCFR,EQ MPL3P
        LODI,R3 GRIVER
        LODI,R0 CBLUE           ; (unowned river: blue)
        BCTR,UN MPL6
MPL3P:  LODA,R0 T1              ; peaks: 1-3, or plain
        RRR,R0
        RRR,R0
        ANDI,R0 3
        BCTR,EQ MPL5
        ADDI,R0 GPEAK1-1
        STRZ    R3
        LODI,R0 CWHITE          ; (unowned peaks: white)
MPL6:   COMI,R2 $FF
        BCTR,EQ MPL7
        LODZ    R2              ; owned: the owner's ink
        ANDI,R0 $C0
MPL7:   IORZ    R3
        BCTR,UN MPL8
MPL5:   EORZ    R0              ; plain: nothing
MPL8:   LODA,R3 T0
        COMA,R0 SCR0,R3
        BCTR,EQ MPL9
        BSTA,UN MPPUT
MPL9:   COMI,R2 $FF             ; ---- the B cell: what is on it ----
        BCTA,EQ MPLB0           ; nobody's: empty
        LODA,R0 T2              ; a M.U.L.E. (only on an owner's land)
        COMI,R0 4
        BCFR,LT MPLB1
        LODA,R0 MP_MULE,R1
        COMI,R0 4
        BCFR,LT MPLB1
        STRA,R0 T1              ; (its good)
        LODA,R0 MAPDOTS         ; production: the units
        BCTR,EQ MPLB2
        LODA,R0 MP_PROD,R1
        TMI,R0  $80             ; (lost: show the M.U.L.E.)
        BCTR,EQ MPLB2
        ANDI,R0 $0F
        COMI,R0 9
        BCTR,LT MPLB3
        LODI,R0 8
MPLB3:  IORI,R0 GDIGIT
        BCTR,UN MPLB4
MPLB2:  LODA,R3 T1              ; the good's letter
        LODA,R0 GOODLET,R3
        BCTR,UN MPLB4
MPLB1:  LODI,R0 GTRILR          ; land, no M.U.L.E.: a flag
MPLB4:  STRZ    R3
        LODZ    R2
        ANDI,R0 $C0
        IORZ    R3
        BCTR,UN MPLB5
MPLB0:  EORZ    R0
MPLB5:  LODA,R3 T0
        COMA,R0 SCR0+16,R3
        RETC,EQ
        STRA,R0 SCR0+16,R3
        RETC,UN
MPPUT:  STRA,R0 SCR0,R3
        RETC,UN

; the T cell of each plot: 2*16 + 3 + 32 * row + column
PLOFF:  DB      $23,$24,$25,$26,$27,$28,$29,$2A,$2B
        DB      $43,$44,$45,$46,$47,$48,$49,$4A,$4B
        DB      $63,$64,$65,$66,$67,$68,$69,$6A,$6B
        DB      $83,$84,$85,$86,$87,$88,$89,$8A,$8B
        DB      $A3,$A4,$A5,$A6,$A7,$A8,$A9,$AA,$AB
PL29:   DB      0,29,58,87      ; a seat's player record
MPGBIT: DB      $FE,$FD,$FB     ; glyph 4-6's bit off
SEATINK: DB     CBLUE,CMAGEN,CCYAN,CWHITE   ; by its colour (mekkogx pl_col)
GOODLET: DB     "FESC"          ; food, energy, smithore, crystite
