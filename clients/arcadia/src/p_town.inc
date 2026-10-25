; p_town.inc -- the town (mekkogx develop.c, the town's half): it takes the
; development over while we walk in it (p_dev.inc hands it over, and takes
; it back when we walk out of either end, or the turn ends).
;
;   row 0      the title
;   rows 1-6   the outfitters: crystite, smithore, energy, food (doors below)
;   rows 7-15  the street: our colonist (16 lines), the M.U.L.E. behind;
;              our turn's event is said here before the first step
;   rows 16-21 the assay office, the land office, the pub, the corral
;   row 22     our money; the M.U.L.E.s left and their price
;   rows 23-24 the message      row 25 the time bar (block graphics)
;
; The street runs from column 4 to 116 (our figure's left), 4 pixels a
; step, two steps deep (lines 88-96). A building's door is at 12, 44, 76
; or 108 (the corral's 108-116): pushing on into it walks us in (an
; outfitter only with a M.U.L.E.), 8 steps of 4 lines.

        BCTA,UN TWENTER         ; E_ENTER
        BCTA,UN TWSTATE         ; E_STATE
        BCTA,UN TWTICK          ; E_TICK

        INCLUDE "src/devram.inc"

DV_WALK  EQU    2
DV_OVER  EQU    3
TWXL    EQU     4
TWXR    EQU     116
TWYTOP  EQU     88
TWYBOT  EQU     96
TWYST   EQU     92
TWYUP   EQU     56              ; through an outfitter's door
TWYDN   EQU     128             ; through a lower building's
GDOOR   EQU     GUDC+4
GWIN    EQU     GUDC+5
GFENCE  EQU     GUDC+6
TWMSG   EQU     23

; ---- the town screen (town_enter) --------------------------------------------------
TWENTER: BSTA,UN SCRG26
        LODI,R0 SCR_TOWN
        LPUT    SCRNOW
        LODI,R1 T_DEVELOP
        BSTA,UN TITLE
        LODI,R0 TWMSG
        LPUT    MSGROW
        LODI,R0 2
        LPUT    MSGN
        LODI,R0 255
        LPUT    DVBARU
        LODI,R0 255
        CPUT    WFIGS
        LODI,R0 255
        CPUT    WMULS
        LODI,R0 TWDOORA>>8      ; the town's glyphs
        LODI,R1 TWDOORA&$FF
        LODI,R2 4
        LODI,R3 PG_TOWN
        ZBSR    *ZUDCQ
        LODI,R0 TWWINA>>8
        LODI,R1 TWWINA&$FF
        LODI,R2 5
        LODI,R3 PG_TOWN
        ZBSR    *ZUDCQ
        LODI,R0 TWFENA>>8
        LODI,R1 TWFENA&$FF
        LODI,R2 6
        LODI,R3 PG_TOWN
        ZBSR    *ZUDCQ
        LODA,R0 WCOL            ; 16 lines in the town
        IORI,R0 $80
        STRZ    R2
        LODI,R1 SL_PL
        ZBSR    *ZSPRCOL
        LODI,R1 SL_MULE
        LODI,R2 SWHITE|$80
        ZBSR    *ZSPRCOL
        LODI,R0 0               ; the buildings
TWE1:   STRA,R0 T3
        BSTR,UN TWBLDG
        LODA,R0 T3
        ADDI,R0 1
        COMI,R0 8
        BCFR,EQ TWE1
        LODI,R1 25              ; the time bar's row: block graphics
        LODI,R2 $FF
        ZBSR    *ZDPOS
        LODI,R0 GMOSAIC
        LODI,R3 1
        ZBSR    *ZDFILL
        BSTA,UN TWMONEY
        EORZ    R0              ; walking on (dv_resume)
        LPUT    WHOLD
        LODI,R0 4
        LPUT    WFACE
        BCTA,UN TWDRAW

; TWBLDG: building T3 (0-3 the top row, 4-7 the bottom) from its template:
; corners and edges, its sign's letter, windows, the door; the corral a
; fence. Clobbers R0-R3, T0-T2.
TWBLDG: LODA,R3 T3
        LODA,R0 TWINK,R3
        STRA,R0 DCOLOR
        LODA,R0 TWTPL,R3        ; T2: its template's first cell
        STRA,R0 T2
        LODI,R0 0               ; T0: the row within it
TWB1:   STRA,R0 T0
        LODI,R1 1               ; its screen row: 1 or 16, + T0
        LODA,R0 T3
        COMI,R0 4
        BCTR,LT TWB2
        LODI,R1 16
TWB2:   LODZ    R1
        ADDA,R0 T0
        STRZ    R1
        LODA,R0 T3              ; its column: 4 * (T3 & 3), less 1
        ANDI,R0 3
        RRL,R0
        RRL,R0
        SUBI,R0 1
        STRZ    R2
        ZBSR    *ZDPOS
        LODI,R0 4
        STRA,R0 T1              ; T1: cells left in the row
TWB3:   LODA,R3 T2
        LODA,R0 TWCELLS,R3
        COMI,R0 $FE             ; $FE: the sign's letter
        BCFR,EQ TWB4
        LODA,R3 T3
        LODA,R0 TWSIGN,R3
TWB4:   ZBSR    *ZDPUTC
        LODA,R0 T2
        ADDI,R0 1
        STRA,R0 T2
        LODA,R0 T1
        SUBI,R0 1
        STRA,R0 T1
        BCFR,EQ TWB3
        LODA,R0 T0
        ADDI,R0 1
        COMI,R0 6
        BCFA,EQ TWB1
        RETC,UN

; the templates, 6 rows of 4: an outfitter (its door on the street, below),
; a lower building (its door above), the corral
TWCELLS:
TPUP:   DB      GCNRTL,GTOPBAR,GTOPBAR,GCNRTR
        DB      GLFBAR,$FE,GWIN,GRTBAR
        DB      GLFBAR,GWIN,GWIN,GRTBAR
        DB      GLFBAR,GSPACE,GSPACE,GRTBAR
        DB      GLFBAR,GDOOR,GDOOR,GRTBAR
        DB      GCNRBL,GDOOR,GDOOR,GCNRBR
TPDN:   DB      GCNRTL,GDOOR,GDOOR,GCNRTR
        DB      GLFBAR,GDOOR,GDOOR,GRTBAR
        DB      GLFBAR,GSPACE,GSPACE,GRTBAR
        DB      GLFBAR,GWIN,GWIN,GRTBAR
        DB      GLFBAR,$FE,GWIN,GRTBAR
        DB      GCNRBL,GBOTBAR,GBOTBAR,GCNRBR
TPMU:   DB      GFENCE,GSPACE,GSPACE,GFENCE
        DB      GFENCE,GSPACE,GSPACE,GFENCE
        DB      GFENCE,GSPACE,GSPACE,GFENCE
        DB      GFENCE,GSPACE,$FE,GFENCE
        DB      GFENCE,GSPACE,GSPACE,GFENCE
        DB      GFENCE,GFENCE,GFENCE,GFENCE
TWTPL:  DB      TPUP-TWCELLS,TPUP-TWCELLS,TPUP-TWCELLS,TPUP-TWCELLS
        DB      TPDN-TWCELLS,TPDN-TWCELLS,TPDN-TWCELLS,TPMU-TWCELLS
TWINK:  DB      CCYAN,CWHITE,CMAGEN,CBLUE,CWHITE,CCYAN,CMAGEN,CWHITE
TWSIGN: DB      GALPHA+2,GALPHA+18,GALPHA+4,GALPHA+5    ; C S E F
        DB      GALPHA,GALPHA+11,GALPHA+15,GALPHA+12    ; A L P M
TWDOORA: DB     $3C,$42,$42,$42,$4A,$42,$42,$42
TWWINA: DB      $00,$7E,$5A,$7E,$5A,$7E,$00,$00
TWFENA: DB      $AA,$FF,$AA,$AA,$AA,$FF,$AA,$AA

; TWMONEY: our money; the M.U.L.E.s left and what one costs (town_money).
TWMONEY: LODI,R1 22
        LODI,R3 1
        ZBSR    *ZDCLR
        LODA,R0 H_MYSEAT
        COMI,R0 4
        RETC,GT
        RETC,EQ
        STRZ    R3
        LODA,R0 TWRECS,R3
        STRZ    R3
        STRA,R3 T3
        LODI,R1 22
        LODI,R2 $FF
        ZBSR    *ZDPOS
        DCOL    CWHITE
        LODI,R0 GDOLLAR
        ZBSR    *ZDPUTC
        LODA,R3 T3
        LODA,R0 H_PLAYERS+PL_MONEY+1,R3
        STRZ    R1
        LODA,R0 H_PLAYERS+PL_MONEY,R3
        LODI,R3 5
        ZBSR    *ZDNUM16
        LODI,R2 7               ; columns 8-9: the M.U.L.E.s; $ and their price
        EORZ    R0
        LODA,R1 ST_MULES
        LODI,R3 2
        ZBSR    *ZDNUM16
        LODI,R2 10
        LODI,R0 GDOLLAR
        ZBSR    *ZDPUTC
        LODA,R0 ST_MULEPRICE
        LODA,R1 ST_MULEPRICE+1
        LODI,R3 4
        ZBRR    *ZDNUM16
TWRECS: DB      0,29,58,87

; ---- a state (develop_state, the town's part) -------------------------------------
TWSTATE: LODA,R0 TN_SEAT
        LPUT    DVSEAT
        LODA,R0 TN_FLAGS
        LPUT    DVFLAGS
        LODA,R0 TN_LEFT
        LPUT    DVLEFTH
        LODA,R0 TN_LEFT+1
        LPUT    DVLEFTL
        LODA,R0 TN_ELAPSED
        LPUT    DVELH
        LODA,R0 TN_ELAPSED+1
        LPUT    DVELL
        LODA,R0 TN_PTU
        BCFR,EQ TWS1
        LODI,R0 28
TWS1:   LPUT    DVPTU
        LODA,R0 TN_SEAT         ; still our walk?
        COMA,R0 H_MYSEAT
        BCFR,EQ TWS5
        LODA,R0 DVFLAGS
        ANDI,R0 TF_STARTED|TF_ENDED
        COMI,R0 TF_STARTED
        BCFR,EQ TWS5
        LODA,R0 DVFLAGS         ; what we lead, as the server sees it
        ANDI,R0 TF_HASMULE
        BCTR,EQ TWS2
        LODI,R0 1
TWS2:   LPUT    WMULE
        LODA,R0 DVBWAIT         ; in a doorway: the building's answer
        BCTR,EQ TWS4
        LODA,R0 ACTSENT
        COMI,R0 ACT_BUYMU
        BCTR,LT TWS4
        COMI,R0 ACT_PUB
        BCTR,GT TWS4
        COMI,R0 ACT_INST
        BCTR,EQ TWS4
        COMI,R0 ACT_SELL
        BCTR,EQ TWS4
        COMI,R0 ACT_UNSELL
        BCTR,EQ TWS4
        COMI,R0 ACT_WAMPUS
        BCTR,EQ TWS4
        BSTA,UN TWRESULT
        BSTA,UN TWSTREET
TWS4:   BCTA,UN TWMONEY
TWS5:   LODI,R0 DV_OVER         ; the turn is over: the map's (dv_stop_walk)
        LPUT    DVSTATE
        LODI,R0 1
        LPUT    WHOLD
        BSTA,UN TWSPROFF
TWMAP:  LODI,R0 PG_DEV
        CPUT    PHPAGE
        LODI,R0 1
        CPUT    PGNEW
        RETC,UN

; TWRESULT: what the building's action came to (dv_building_result).
TWRESULT: LODA,R0 H_ERR
        COMI,R0 ERR_MONEY
        BCFR,EQ TWR1
        LODI,R1 M_NOMONEY
        ZBRR    *ZSAY
TWR1:   COMI,R0 ERR_NOSTOCK
        BCFR,EQ TWR2
        LODI,R1 M_NOMULES
        ZBRR    *ZSAY
TWR2:   COMI,R0 0
        BCTR,EQ TWR3
        SND     SND_ERROR
        RETC,UN
TWR3:   LODA,R0 ACTSENT
        COMI,R0 ACT_ASSAY
        BCFR,EQ TWR4
        LODI,R0 255
        LPUT    DVSAMPLE
        LODA,R0 H_RESULT+1
        ANDI,R0 7
        ADDI,R0 M_ASSAY
        STRZ    R1
        ZBRR    *ZSAY
TWR4:   COMI,R0 ACT_BUYMU
        BCTR,EQ TWR5
        COMI,R0 ACT_OUTFIT
        RETC,LT
        RETC,GT
TWR5:   SND     SND_BUY
        RETC,UN

; TWSTREET: out of the doorway onto the street (back_to_street).
TWSTREET: EORZ  R0
        LPUT    DVBWAIT
        LODA,R0 DVSTATE
        COMI,R0 DV_WALK
        RETC,LT
        RETC,GT
        LODI,R0 TWYST
        LPUT    WY
        SND     SND_SELECT
        EORZ    R0
        LPUT    WHOLD
        LODI,R0 4
        LPUT    WFACE
        BCTA,UN TWDRAW

; TWSPROFF: our sprites off.
TWSPROFF: LODI,R1 SL_PL
        ZBSR    *ZSPRHID
        LODI,R1 SL_MULE
        ZBRR    *ZSPRHID

; ---- a frame ---------------------------------------------------------------------
TWTICK: LODA,R0 DVDO            ; our turn's event: said in the street
        ANDI,R0 DO_EVENT
        BCTR,EQ TWT1
        LODA,R0 DVDO
        ANDI,R0 $FF-DO_EVENT
        LPUT    DVDO
        LODI,R1 PG_DEVT
        LODI,R3 E_STATE
        BSTA,UN PGFAR
TWT1:   BSTA,UN TWBAR
        LODA,R0 DVBWAIT         ; in a doorway: 10 seconds at most
        BCTR,EQ TWT2
        LODA,R1 DVBT0H
        LODA,R2 DVBT0L
        ZBSR    *ZSINCE
        COMI,R1 600>>8
        RETC,LT
        BCFA,EQ TWSTREET
        COMI,R2 600&$FF
        RETC,LT
        BCTA,UN TWSTREET
TWT2:   BSTA,UN TWWALK
        LODA,R0 EVKEY           ; Start ends the turn
        COMI,R0 K_START
        RETC,LT
        RETC,GT
        LODI,R0 ACT_END
        LODI,R1 0
        LODI,R2 0
        ZBRR    *ZACT

; TWBAR: the turn's time bar on row 25 (dv_bar): units left, a block
; column each two (45 at most), ticking under 30.
TWBAR:  LODA,R1 RXFRH
        LODA,R2 RXFRL
        ZBSR    *ZSINCE
        LODA,R0 DVLEFTL
        SUBZ    R2
        STRZ    R2
        PPSL    PSLWC
        LODA,R0 DVLEFTH
        SUBZ    R1
        CPSL    PSLWC
        STRZ    R1
        TMI,R1  $80
        BCTR,EQ TWBR0
        LODA,R3 DVPTU
        ZBSR    *ZDIV8
        LODZ    R2
        BCTR,UN TWBR1
TWBR0:  EORZ    R0
TWBR1:  STRA,R0 T3
        BSTA,UN BARTICK
        LODA,R0 T3              ; / 2, 45 at most
        RRR,R0
        ANDI,R0 $7F
        COMI,R0 45
        BCTR,LT TWBR2
        LODI,R0 45
TWBR2:  STRA,R0 T3
        COMA,R0 DVBARU          ; (one more than shown: jitter)
        RETC,EQ
        SUBI,R0 1
        COMA,R0 DVBARU
        RETC,EQ
        LODA,R0 T3
        LPUT    DVBARU
        LODI,R1 25
        LODI,R2 0
        ZBSR    *ZDPOS
        LODA,R1 DVSEAT          ; the player's ink
        LODA,R0 TWRECS,R1
        STRZ    R3
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 TWSINK,R3
        STRA,R0 T2
        LODI,R0 15
        STRA,R0 T1
TWBR3:  LODA,R0 T3
        COMI,R0 3
        BCTR,LT TWBR4
        LODI,R0 3
TWBR4:  STRZ    R3
        LODA,R0 T3
        SUBZ    R3
        STRA,R0 T3
        LODA,R0 TWBCELL,R3
        BCTR,EQ TWBR5
        IORA,R0 T2
TWBR5:  LODI,R3 1
        ZBSR    *ZDFILL
        LODA,R0 T1
        SUBI,R0 1
        STRA,R0 T1
        BCFR,EQ TWBR3
        RETC,UN
TWBCELL: DB     $00,$24,$36,$3F
TWSINK: DB      CBLUE,CMAGEN,CCYAN,CWHITE

; ---- our walk in the town (walk_isr, walk_town, walk_building) ----------------------
TWWALK: LODA,R0 WHOLD
        RETC,GT
        LODA,R0 WCNT            ; (a step when it runs out: 6 frames from 5)
        SUBA,R0 DT
        BCTR,LT TWW0
        LPUT    WCNT
        RETC,UN
TWW0:   LODA,R0 WDOOR             ; through a door
        BCTA,GT TWDOOR
        LODA,R0 INDIR
        ANDI,R0 $0F
        BCFR,EQ TWW2
        LPUT    WCNT
        LODA,R0 WSTEP           ; let go: standing
        BCTR,EQ TWW0S
        EORZ    R0
        LPUT    WSTEP
        LODI,R0 4
        LPUT    WFACE
        BCTA,UN TWDRAW
TWW0S:    LODI,R3 WFIGSO          ; (standing: until its picture is in, the
        LODA,R0 *PRAMH,R3       ;  queue having had no room)
        COMI,R0 255
        RETC,LT
        BCTA,UN TWDRAW
TWW2:   STRA,R0 T3              ; T3: the directions held
        LODI,R0 5
        LPUT    WCNT
        LODI,R3 DVEVSHO         ; (the event's words off the street, the
                                ;  first step on)
        LODA,R0 *PRAMH,R3
        COMI,R0 1
        BCFR,EQ TWW2A
        LODI,R0 2
        CPUT    DVEVSH
        LODI,R1 7
        LODI,R3 9
        ZBSR    *ZDCLR
TWW2A:  LODA,R0 T3              ; along the street; out of either end
        TMI,R0  D_LEFT
        BCFR,EQ TWW4
        LODA,R0 WX
        COMI,R0 TWXL
        BCTA,LT TWOUTL
        BCTA,EQ TWOUTL
        SUBI,R0 4
        LPUT    WX
        BCTR,UN TWW5
TWW4:   TMI,R0  D_RIGHT
        BCFR,EQ TWW5
        LODA,R0 WX
        COMI,R0 TWXR
        BCTA,GT TWOUTR
        BCTA,EQ TWOUTR
        ADDI,R0 4
        LPUT    WX
TWW5:   LODA,R0 T3              ; down: the street, then a lower door
        TMI,R0  D_DOWN
        BCFA,EQ TWW7
        LODA,R0 WY
        COMI,R0 TWYBOT
        BCFR,LT TWW6
        ADDI,R0 4
        LPUT    WY
        BCTA,UN TWW9
TWW6:   LODA,R0 WX              ; the assay office, land office, pub
        LODI,R1 B_ASSAY
        COMI,R0 12
        BCTR,EQ TWW6A
        LODI,R1 B_LAND
        COMI,R0 44
        BCTR,EQ TWW6A
        LODI,R1 B_PUB
        COMI,R0 76
        BCTR,EQ TWW6A
        LODI,R1 B_CORRAL        ; the corral (108-116)
        COMI,R0 108
        BCTA,LT TWW9
        LODI,R0 1
        BCTR,UN TWW6B
TWW6A:  LODI,R0 1
TWW6B:  STRA,R1 T2
        LPUT WDOOR
        LODA,R0 T2
        LPUT    WBLDG
        BCTA,UN TWW9
TWW7:   TMI,R0  D_UP            ; up: the street, then an outfitter's door
        BCFA,EQ TWW9            ;  (leading a M.U.L.E.)
        LODA,R0 WY
        COMI,R0 TWYTOP
        BCFR,GT TWW8
        SUBI,R0 4
        LPUT    WY
        BCTA,UN TWW9
TWW8:   LODA,R0 WMULE
        BCTA,EQ TWW9
        LODA,R0 WX              ; crystite, smithore, energy, food
        LODI,R1 GOOD_CRYSTITE
        COMI,R0 12
        BCTR,EQ TWW8A
        LODI,R1 GOOD_SMITHORE
        COMI,R0 44
        BCTR,EQ TWW8A
        LODI,R1 GOOD_ENERGY
        COMI,R0 76
        BCTR,EQ TWW8A
        LODI,R1 GOOD_FOOD
        COMI,R0 108
        BCFA,EQ TWW9
TWW8A:  STRA,R1 T2
        LODI,R0 2
        LPUT WDOOR
        LODA,R0 T2
        LPUT    WBLDG
TWW9:   LODI,R0 1               ; a step: its frame, sound, the trail
        LPUT    WSTEP
        LODA,R0 WFRAME
        EORI,R0 1
        LPUT    WFRAME
        LODA,R1 T3
        LODI,R0 0
        TMI,R1  D_RIGHT
        BCTR,EQ TWW10
        LODI,R0 2
        TMI,R1  D_LEFT
        BCTR,EQ TWW10
        LODI,R0 6
        TMI,R1  D_DOWN
        BCTR,EQ TWW10
        LODI,R0 8
TWW10:  ADDA,R0 WFRAME
        LPUT    WFACE
        SND     SND_STEP
        LODA,R0 WTH
        ADDI,R0 WTX&$FF
        STRZ    R3
        LODA,R0 WX
        ZBSR    *ZLPUT
        LODA,R0 WTH
        ADDI,R0 WTY&$FF
        STRZ    R3
        LODA,R0 WY
        ZBSR    *ZLPUT
        LODA,R0 WTH
        ADDI,R0 1
        ANDI,R0 3
        LPUT    WTH
        BCTA,UN TWDRAW

; TWDOOR: through a door, 4 lines a step until inside (walk_building);
; then the building's action.
TWDOOR: LODI,R0 5
        LPUT    WCNT
        LODA,R0 WFRAME
        EORI,R0 1
        LPUT    WFRAME
        LODA,R0 WDOOR
        COMI,R0 1
        BCFR,EQ TWD2
        LODA,R0 WY              ; down
        ADDI,R0 4
        LPUT    WY
        LODA,R0 WFRAME
        ADDI,R0 6
        LPUT    WFACE
        LODA,R0 WY
        COMI,R0 TWYDN
        BCTA,LT TWDRAW
        BCTR,UN TWD3
TWD2:   LODA,R0 WY              ; up
        SUBI,R0 4
        LPUT    WY
        LODA,R0 WFRAME
        ADDI,R0 8
        LPUT    WFACE
        LODA,R0 WY
        COMI,R0 TWYUP
        BCTA,GT TWDRAW
TWD3:   EORZ    R0              ; inside
        LPUT WDOOR
        LODI,R0 1
        LPUT    WHOLD
        BSTA,UN TWDRAW
; TWACT: the building's action (dv_building): to the server, standing in
; its door until it answers; the assay office without a sample and the
; land office say so and send us out.
TWACT:  LODA,R0 WBLDG
        COMI,R0 B_CORRAL
        BCFR,EQ TWA2
        LODI,R0 ACT_BUYMU
        LODA,R1 WMULE
        BCTR,EQ TWA9
        LODI,R0 ACT_RETURN
        BCTR,UN TWA9
TWA2:   COMI,R0 GOOD_CRYSTITE
        BCTR,GT TWA3
        STRZ    R2
        LODI,R0 ACT_OUTFIT
        BCTR,UN TWA9A
TWA3:   COMI,R0 B_ASSAY
        BCFR,EQ TWA5
        LODA,R2 DVSAMPLE
        COMI,R2 NUM_PLOTS
        BCTR,LT TWA4
        LODI,R1 M_NOSAMPLE
        ZBSR    *ZSAY
        BCTA,UN TWSTREET
TWA4:   LODI,R0 ACT_ASSAY
        BCTR,UN TWA9A
TWA5:   COMI,R0 B_LAND
        BCFR,EQ TWA6
        LODI,R0 1
        LPUT    DVOFFICE
        LODI,R1 M_LANDOFFICE
        ZBSR    *ZSAY
        BCTA,UN TWSTREET
TWA6:   LODI,R0 ACT_PUB
TWA9:   LODI,R2 0
TWA9A:  LODI,R1 0
        ZBSR    *ZACT
        LODI,R0 1
        LPUT    DVBWAIT
        LODA,R0 FRMH
        LPUT    DVBT0H
        LODA,R0 FRML
        LPUT    DVBT0L
        RETC,UN

; TWOUTL, TWOUTR: out of the town's end, onto the map beside the store.
TWOUTL: LODI,R0 46
        BCTR,UN TWOUT
TWOUTR: LODI,R0 66
TWOUT:  LPUT    WX
        LODI,R0 52
        LPUT    WY
        EORZ    R0
        LPUT    WTOWN
        BSTA,UN TWSPROFF
        LODI,R0 7               ; the trail forgotten
TWO1:   STRA,R0 T3
        ADDI,R0 WTX&$FF
        STRZ    R3
        EORZ    R0
        ZBSR    *ZLPUT
        LODA,R0 T3
        SUBI,R0 1
        BCTR,GT TWO1
        BCTR,EQ TWO1
        EORZ    R0
        LPUT    WTH
        BCTA,UN TWMAP

; TWDRAW: our colonist (16 lines) and the M.U.L.E. four steps behind, its
; feet with ours.
TWDRAW: LODA,R0 WSP
        STRZ    R1
        RRL,R0
        RRL,R0
        ADDZ    R1
        RRL,R0
        ADDA,R0 WFACE
        STRA,R0 T2
        LODI,R3 WFIGSO
        LODA,R0 *PRAMH,R3
        COMA,R0 T2
        BCTR,EQ TWDR1
        LODA,R0 UDCQN
        COMI,R0 3
        BCFR,LT TWDR1
        LODA,R0 T2
        CPUT    WFIGS
        LODA,R2 T2
        LODI,R1 8
        ZBSR    *ZMUL8
        LODZ    R2
        ADDI,R0 ART_FIG&$FF
        STRZ    R2
        PPSL    PSLWC
        LODZ    R1
        ADDI,R0 ART_FIG>>8
        CPSL    PSLWC
        STRZ    R3
        LODZ    R2
        STRZ    R1
        LODZ    R3
        LODI,R2 SL_PL
        LODI,R3 PG_ART
        ZBSR    *ZUDCQ
TWDR1:  LODI,R1 SL_PL
        LODA,R2 WY
        LODA,R3 WX
        ZBSR    *ZSPRSET
        LODA,R0 WMULE
        BCTA,EQ TWDR9
        LODA,R3 WTH
        LODA,R0 WTX,R3
        BCFR,EQ TWDR2
        LODI,R3 0
        LODA,R0 WTX,R3
        BCTA,EQ TWDR9
TWDR2:  STRA,R3 T3
        LODA,R0 WFACE           ; facing as we go: left 2, right 0
        COMI,R0 2
        BCTR,LT TWDR3
        COMI,R0 4
        BCFR,LT TWDR3
        LODI,R0 2
        BCTR,UN TWDR3A
TWDR3:  LODI,R0 0
TWDR3A: ADDA,R0 WFRAME
        STRA,R0 T2
        LODI,R3 WMULSO
        LODA,R0 *PRAMH,R3
        COMA,R0 T2
        BCTR,EQ TWDR4
        LODA,R0 UDCQN
        COMI,R0 3
        BCFR,LT TWDR4
        LODA,R0 T2
        CPUT    WMULS
        LODA,R0 T2
        RRL,R0
        RRL,R0
        RRL,R0
        ADDI,R0 ART_MULE&$FF
        STRZ    R1
        LODI,R0 ART_MULE>>8
        LODI,R2 SL_MULE
        LODI,R3 PG_ART
        ZBSR    *ZUDCQ
TWDR4:  LODA,R3 T3
        LODA,R0 WTY,R3
        STRZ    R2
        LODA,R0 WTX,R3
        STRZ    R3
        LODI,R1 SL_MULE
        ZBRR    *ZSPRSET
TWDR9:  LODI,R1 SL_MULE
        ZBRR    *ZSPRHID

        INCLUDE "src/mapscr.inc"
