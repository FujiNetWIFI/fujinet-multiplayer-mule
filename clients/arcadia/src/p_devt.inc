; p_devt.inc -- what the development's pages far-call (PGFAR) from their
; frames: a computer's walk (dv_ai_walk), our turn's player event
; (dv_event_msg), "<name> takes a turn" (say_turn).

        BCTA,UN DTAIWALK        ; E_ENTER: a computer's walk
        BCTA,UN DTEVENT         ; E_STATE: our turn's event
        BCTA,UN DTTURN          ; E_TICK: whose turn
        BCTA,UN DVWAMP          ; E_EXTRA: the wampus, walking on the map
        BCTA,UN DVBUTTON        ; E_EXTRA+3: the button on the map
        BCTA,UN DVMARKS         ; E_EXTRA+6: a computer's marks

        INCLUDE "src/devram.inc"
        INCLUDE "src/fxops.inc"

; ---- a computer's turn (dv_ai_walk) -----------------------------------------------
; The server plays it at once, so the map already shows what it did. Its
; figure walks out of the store to the plot that gained a M.U.L.E. (or one
; of its plots), the M.U.L.E. behind, which stays there; then back into
; the store, to wait. Scripts for the fx page, written into RAM-H: across
; a pixel every 6 frames, then a line every 6; 90 frames on the plot
; (signals 3 and 4: outlined, then not).
DTAIWALK: LODI,R0 255           ; T2: the plot; T1: 1 a M.U.L.E. went in
        STRA,R0 T2
        EORZ    R0
        STRA,R0 T1
        LODI,R3 0
DTA1:   LODA,R0 MP_OWNER,R3
        COMA,R0 DVSEAT
        BCFR,EQ DTA3
        COMI,R3 STORE_PLOT
        BCTR,EQ DTA3
        LODA,R0 T2
        COMI,R0 255
        BCFR,EQ DTA2
        STRA,R3 T2
DTA2:   LODA,R0 MP_MULE,R3
        COMI,R0 4
        BCFR,LT DTA3
        STRA,R3 UNIONB          ; (the last state's, through PRAMH)
        LODZ    R3
        ADDI,R0 DVPMULEO
        STRZ    R3
        LODA,R0 *PRAMH,R3
        LODA,R3 UNIONB
        COMA,R0 MP_MULE,R3
        BCTR,EQ DTA3
        STRA,R3 T2
        LODI,R0 1
        STRA,R0 T1
DTA3:   ADDI,R3 1
        COMI,R3 NUM_PLOTS
        BCFR,EQ DTA1
        LODA,R0 T2              ; no land: out and back
        COMI,R0 NUM_PLOTS
        BCTR,LT DTA4
        LODI,R0 13
        STRA,R0 T2
DTA4:   LODA,R0 T2
        LPUT    DVAIP
        LODI,R0 1               ; its land outlined (DVMARKS)
        LPUT    DVAI
        LODI,R0 255             ; (the last one's off)
        LPUT    MAPHI
        LODA,R3 T2              ; where: the plot's middle, from the store's
        LODA,R0 PLCOL,R3
        STRA,R0 UNIONB+1        ; x
        LODA,R0 PLROW,R3
        ADDI,R0 4
        STRA,R0 UNIONB+2        ; line
        LODI,R0 1               ; dx, steps across
        STRA,R0 UNIONB+3
        LODA,R0 UNIONB+1
        SUBI,R0 DTSTX
        BCFR,LT DTA5
        STRZ    R1
        LODI,R0 $FF
        STRA,R0 UNIONB+3
        EORZ    R0
        SUBZ    R1
DTA5:   ADDI,R0 1
        STRA,R0 UNIONB+4
        LODI,R0 1               ; dy, steps down
        STRA,R0 UNIONB+5
        LODA,R0 UNIONB+2
        SUBI,R0 DTSTY
        BCFR,LT DTA6
        STRZ    R1
        LODI,R0 $FF
        STRA,R0 UNIONB+5
        EORZ    R0
        SUBZ    R1
DTA6:   ADDI,R0 1
        STRA,R0 UNIONB+6
        LODA,R0 DVSEAT          ; its species, its colour
        STRZ    R3
        LODA,R0 DTRECS,R3
        STRZ    R3
        LODA,R0 H_PLAYERS+PL_SPECIES,R3
        ANDI,R0 7
        STRZ    R1              ; * 10: its pictures
        RRL,R0
        RRL,R0
        ADDZ    R1
        RRL,R0
        STRA,R0 UNIONB+7
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 DTSPR,R3
        STRA,R0 UNIONB+8
; the figure's script
        CPTR    DVAIS0
        LODI,R0 FO_LOOK
        BSTA,UN RAMPUT1
        LODI,R0 FK_SINGLE
        BSTA,UN RAMPUT1
        LODI,R0 SL_PL
        BSTA,UN RAMPUT1
        LODA,R0 UNIONB+7        ; its pictures (ART_FIG is index 0)
        BSTA,UN RAMPUT1
        LODI,R0 0               ; facing its way: right 0, left 2
        LODA,R1 UNIONB+3
        TMI,R1  $80
        BCFR,EQ DTA7
        LODI,R0 2
DTA7:   BSTA,UN RAMPUT1
        LODI,R0 1               ; its two walking frames
        BSTA,UN RAMPUT1
        LODA,R0 UNIONB+8
        BSTA,UN RAMPUT1
        BSTA,UN DTPOS
        BSTA,UN DTOUT
        LODI,R0 FO_SIG
        BSTA,UN RAMPUT1
        LODI,R0 3
        BSTA,UN RAMPUT1
        LODI,R0 FO_WAIT
        BSTA,UN RAMPUT1
        LODI,R0 90
        BSTA,UN RAMPUT1
        LODI,R0 FO_SIG
        BSTA,UN RAMPUT1
        LODI,R0 4
        BSTA,UN RAMPUT1
        LODI,R0 FO_MOVE         ; back: up (down), then across, facing home
        BSTA,UN RAMPUT1
        LODI,R0 0
        BSTA,UN RAMPUT1
        LODA,R0 UNIONB+5
        EORI,R0 $FF
        ADDI,R0 1
        BSTA,UN RAMPUT1
        LODI,R0 6
        BSTA,UN RAMPUT1
        LODA,R0 UNIONB+6
        BSTA,UN RAMPUT1
        LODI,R0 FO_FRAME
        BSTA,UN RAMPUT1
        LODA,R1 UNIONB+3        ; (home: the other way)
        LODI,R0 2
        TMI,R1  $80
        BCFR,EQ DTA8
        LODI,R0 0
DTA8:   BSTA,UN RAMPUT1
        LODI,R0 FO_MOVE
        BSTA,UN RAMPUT1
        LODA,R0 UNIONB+3
        EORI,R0 $FF
        ADDI,R0 1
        BSTA,UN RAMPUT1
        LODI,R0 0
        BSTA,UN RAMPUT1
        LODI,R0 6
        BSTA,UN RAMPUT1
        LODA,R0 UNIONB+4
        BSTA,UN RAMPUT1
        LODI,R0 FO_WAIT         ; and waits in the store
        BSTA,UN RAMPUT1
        LODI,R0 120
        BSTA,UN RAMPUT1
        LODI,R0 FO_LOOP
        BSTA,UN RAMPUT1
        LODI,R0 200
        BSTA,UN RAMPUT1
        LODI,R0 2
        BSTA,UN RAMPUT1
        BSTA,UN RAMPEND
; its M.U.L.E.'s, when one went in (else nothing)
        CPTR    DVAIS1
        LODA,R0 T1
        BCTA,EQ DTA9
        LODI,R0 FO_WAIT
        BSTA,UN RAMPUT1
        LODI,R0 24
        BSTA,UN RAMPUT1
        LODI,R0 FO_LOOK
        BSTA,UN RAMPUT1
        LODI,R0 FK_SINGLE
        BSTA,UN RAMPUT1
        LODI,R0 SL_MULE
        BSTA,UN RAMPUT1
        LODI,R0 (ART_MULE-$0800)>>3
        LODA,R1 UNIONB+3
        TMI,R1  $80
        BCFR,EQ DTA8A
        ADDI,R0 2
DTA8A:  BSTA,UN RAMPUT1
        LODI,R0 0
        BSTA,UN RAMPUT1
        LODI,R0 1
        BSTA,UN RAMPUT1
        LODI,R0 SWHITE
        BSTA,UN RAMPUT1
        BSTA,UN DTPOS
        BSTA,UN DTOUT
        LODI,R0 FO_WAIT
        BSTA,UN RAMPUT1
        LODI,R0 20
        BSTA,UN RAMPUT1
        LODI,R0 FO_HIDE
        BSTA,UN RAMPUT1
DTA9:   LODI,R0 FO_END
        BSTA,UN RAMPUT1
        BSTA,UN RAMPEND
        LODI,R0 FX_AIWALK       ; the fx page runs them
        LPUT    FXREQ
        EORZ    R0
        LPUT    FXFLAG
        LODI,R0 1
        LPUT    FXON
; DTPCOPY: DVPMULE = the plots' M.U.L.E.s as they stand now.
DTPCOPY: CPTR   DVPMULE
        LODI,R0 0
DTPC1:  STRA,R0 T3
        STRZ    R3
        LODA,R0 MP_MULE,R3
        BSTA,UN RAMPUT1
        LODA,R0 T3
        ADDI,R0 1
        COMI,R0 NUM_PLOTS
        BCFR,EQ DTPC1
        BCTA,UN RAMPEND

; DTPOS: FO_POS at the store. DTOUT: across, then down (or up), to the
; plot. (Into the open burst.)
DTPOS:  LODI,R0 FO_POS
        BSTA,UN RAMPUT1
        LODI,R0 DTSTX+64
        BSTA,UN RAMPUT1
        LODI,R0 DTSTY+48
        BCTA,UN RAMPUT1
DTOUT:  LODI,R0 FO_MOVE
        BSTA,UN RAMPUT1
        LODA,R0 UNIONB+3
        BSTA,UN RAMPUT1
        LODI,R0 0
        BSTA,UN RAMPUT1
        LODI,R0 6
        BSTA,UN RAMPUT1
        LODA,R0 UNIONB+4
        BSTA,UN RAMPUT1
        LODI,R0 FO_MOVE
        BSTA,UN RAMPUT1
        LODI,R0 0
        BSTA,UN RAMPUT1
        LODA,R0 UNIONB+5
        BSTA,UN RAMPUT1
        LODI,R0 6
        BSTA,UN RAMPUT1
        LODA,R0 UNIONB+6
        BCTA,UN RAMPUT1

DTSTX   EQU     56              ; in the store: its column, its line
DTSTY   EQU     52
DTRECS: DB      0,29,58,87
DTSPR:  DB      SBLUE,SMAGEN,SCYAN,SWHITE

; ---- our turn's player event (dv_event_msg) -----------------------------------------
; Said in the town's street as the turn starts. Its amount: x * 25 *
; (round / 4 + 1), or per M.U.L.E. for three of them (x 100: food ones,
; 101: mining ones, twice, both numbers; 102: energy ones).
DTEVENT: LODA,R0 TN_EVENT
        COMI,R0 1
        RETC,LT
        COMI,R0 22
        RETC,GT
        SUBI,R0 1
        STRA,R0 T3              ; T3: the event - 1
        LODA,R0 H_ROUND         ; v = 25 * (round / 4 + 1)
        RRR,R0
        RRR,R0
        ANDI,R0 $3F
        ADDI,R0 1
        STRZ    R2
        LODI,R1 25
        ZBSR    *ZMUL8
        STRA,R2 T2              ; T2: v (under 256)
        EORZ    R0
        STRA,R0 UNIONB          ; UNIONB:+1 v2
        STRA,R0 UNIONB+1
        LODA,R3 T3
        LODA,R0 DTPEVX,R3
        STRA,R0 T1              ; T1: x
        COMI,R0 100
        BCTA,LT DTE6
        EORZ    R0              ; per M.U.L.E.: count ours
        STRA,R0 UNIONB+2
        LODI,R3 0
DTE1:   LODA,R0 MP_OWNER,R3
        COMA,R0 H_MYSEAT
        BCFR,EQ DTE3
        LODA,R0 MP_MULE,R3
        STRZ    R2
        LODA,R0 T1
        COMI,R0 100
        BCFR,EQ DTE1A
        COMI,R2 GOOD_FOOD
        BCTR,EQ DTE2
        BCTR,UN DTE3
DTE1A:  COMI,R0 102
        BCFR,EQ DTE1B
        COMI,R2 GOOD_ENERGY
        BCTR,EQ DTE2
        BCTR,UN DTE3
DTE1B:  COMI,R2 GOOD_SMITHORE
        BCTR,EQ DTE2
        COMI,R2 GOOD_CRYSTITE
        BCFR,EQ DTE3
DTE2:   LODA,R0 UNIONB+2
        ADDI,R0 1
        STRA,R0 UNIONB+2
DTE3:   ADDI,R3 1
        COMI,R3 NUM_PLOTS
        BCFR,EQ DTE1
        LODA,R0 T1
        COMI,R0 102
        BCFR,EQ DTE4
        LODA,R1 T2              ; energy: v * n
        LODA,R2 UNIONB+2
        BCTR,UN DTE7
DTE4:   LODA,R0 T2              ; v2 = 2v * n, v = 2v
        RRL,R0
        STRA,R0 T2
        STRZ    R1
        LODA,R2 UNIONB+2
        ZBSR    *ZMUL8
        STRA,R1 UNIONB
        STRA,R2 UNIONB+1
        LODA,R0 T1
        COMI,R0 101
        BCFR,EQ DTE5
        LODA,R0 UNIONB          ; mining: v = v2
        STRZ    R1
        LODA,R0 UNIONB+1
        STRZ    R2
        BCTR,UN DTE8
DTE5:   LODI,R1 0
        LODA,R2 T2
        BCTR,UN DTE8
DTE6:   LODA,R1 T2              ; v * x
        LODA,R2 T1
DTE7:   ZBSR    *ZMUL8
DTE8:   STRA,R1 UNIONB+3        ; v, v2: SAY's numbers
        STRA,R2 UNIONB+4
        LODA,R0 UNIONB+3
        LPUT    SAYV1H
        LODA,R0 UNIONB+4
        LPUT    SAYV1L
        LODA,R0 UNIONB
        LPUT    SAYV2H
        LODA,R0 UNIONB+1
        LPUT    SAYV2L
        LODI,R0 1               ; said (the first step clears it)
        CPUT    DVEVSH
        SND     SND_EVENT
        DCOL    CWHITE
        LODA,R0 T3              ; in the street, rows 7-15
        ADDI,R0 E_1
        STRZ    R1
        LODI,R2 7
        LODI,R3 9
        ZBRR    *ZSAYBX

; DTPEVX: x for each event (mekkogx pevent_x)
DTPEVX: DB      0,0,2,4,100,4,8,2,3,6,4,2,0,0,3,101,102,6,4,4,4,0

; ---- whose turn (say_turn) --------------------------------------------------------
; "<name> takes a turn" in the map's message box: the name on its first
; row, in the player's ink, the words under it.
DTTURN: LODA,R1 MSGROW
        LODA,R3 MSGN
        ZBSR    *ZDCLR
        LODA,R0 DVSEAT
        COMI,R0 4
        RETC,GT
        RETC,EQ
        STRZ    R3
        LODA,R0 DTRECS,R3
        STRZ    R3
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 DTINK,R3
        STRA,R0 DCOLOR
        LODA,R1 MSGROW
        LODI,R2 3
        ZBSR    *ZDPOS
        LODA,R1 DVSEAT
        ZBSR    *ZDNAME
        DCOL    CWHITE
        LODI,R1 M_TURN
        LODA,R2 MSGROW
        ADDI,R2 1
        LODI,R3 2
        ZBRR    *ZSAYBX
DTINK:  DB      CBLUE,CMAGEN,CCYAN,CWHITE

; ---- the button on the map (dv_button) -----------------------------------------------
; Catch a showing wampus (within 8, leading no M.U.L.E.); install the
; M.U.L.E. we lead; list land after the land office; else take a sample.
DVBUTTON: LODI,R0 255           ; T3: the plot under the figure's middle
        STRA,R0 T3
        LODA,R0 WX
        ADDI,R0 4
        SUBI,R0 24
        BCTR,LT DVBN1
        RRR,R0
        RRR,R0
        RRR,R0
        ANDI,R0 $1F
        COMI,R0 9
        BCFR,LT DVBN1
        STRA,R0 T2
        LODA,R0 WY
        ADDI,R0 4
        SUBI,R0 16
        BCTR,LT DVBN1
        RRR,R0
        RRR,R0
        RRR,R0
        RRR,R0
        ANDI,R0 $0F
        COMI,R0 5
        BCFR,LT DVBN1
        STRZ    R1
        RRL,R0
        RRL,R0
        RRL,R0
        ADDZ    R1
        ADDA,R0 T2
        STRA,R0 T3
DVBN1:  LODA,R0 DVWP            ; the wampus first
        COMI,R0 12
        BCFR,LT DVBN2
        LODA,R0 WMULE
        BCFR,EQ DVBN2
        LODA,R0 WX
        SUBA,R0 WPX
        BSTA,UN DVABS
        COMI,R0 9
        BCFR,LT DVBN2
        LODA,R0 WY
        SUBA,R0 WPY
        BSTA,UN DVABS
        COMI,R0 9
        BCFR,LT DVBN2
        LODI,R0 ACT_WAMPUS
        LODI,R1 0
        LODI,R2 0
        ZBRR    *ZACT
DVBN2:  LODA,R0 T3
        COMI,R0 NUM_PLOTS
        RETC,GT
        RETC,EQ
        LODA,R0 WMULE           ; install
        BCTR,EQ DVBN3
        LODI,R0 ACT_INST
        LODI,R1 0
        LODA,R2 T3
        ZBSR    *ZACT
        SND     SND_SELECT
        RETC,UN
DVBN3:  LODA,R0 DVOFFICE        ; list (or withdraw) our land
        BCTR,EQ DVBN5
        LODA,R3 T3
        LODA,R0 MP_OWNER,R3
        COMA,R0 H_MYSEAT
        RETC,LT
        RETC,GT
        LODI,R1 ACT_SELL
        LODA,R0 MP_PROD,R3
        TMI,R0  $40
        BCFR,EQ DVBN4
        LODI,R1 ACT_UNSELL
DVBN4:  LODZ    R1
        LODI,R1 0
        LODA,R2 T3
        ZBSR    *ZACT
        EORZ    R0
        LPUT    DVOFFICE
        RETC,UN
DVBN5:  LODA,R0 T3              ; a sample
        LPUT    DVSAMPLE
        SND     SND_SAMPLE
        RETC,UN
; DVABS: R0 = |R0| (a signed difference). Leaf.
DVABS:  TMI,R0  $80
        RETC,LT
        RETC,GT
        STRZ    R1
        EORZ    R0
        SUBZ    R1
        RETC,UN

; ---- the wampus (dv_wampus) -----------------------------------------------------------
; turn.wampus lists when (the turn's jiffies) and where it shows: on a
; plot's peaks, flashing 8 frames on, 8 off.
DVWAMP: LODA,R1 RXFRH           ; T0:T1 = the turn's jiffies now
        LODA,R2 RXFRL
        ZBSR    *ZSINCE
        LODA,R0 DVELL
        ADDZ    R2
        STRA,R0 T1
        PPSL    PSLWC
        LODA,R0 DVELH
        ADDZ    R1
        CPSL    PSLWC
        STRA,R0 T0
        LODI,R0 255
        STRA,R0 T2              ; T2: the one showing
        LODA,R0 TN_NWAMPUS
        COMI,R0 MAX_WAMPUS
        BCTR,LT DVWP1
        LODI,R0 MAX_WAMPUS
DVWP1:  STRA,R0 T3              ; T3: how many to look at
        LODI,R3 0               ; R3: the entry's offset
DVWP2:  LODA,R0 T3
        BCTA,EQ DVWP6
        SUBI,R0 1
        STRA,R0 T3
        STRA,R3 UNIONB          ; from <= now?
        ADDI,R3 S_WAMPUS_FROM
        LODA,R0 *PRAMH,R3
        COMA,R0 T0
        BCTR,GT DVWP5
        BCTR,LT DVWP3
        ADDI,R3 1
        LODA,R0 *PRAMH,R3
        COMA,R0 T1
        BCTR,GT DVWP5
DVWP3:  LODA,R3 UNIONB          ; now < to?
        ADDI,R3 S_WAMPUS_TO
        LODA,R0 *PRAMH,R3
        COMA,R0 T0
        BCTR,LT DVWP5
        BCTR,GT DVWP4
        ADDI,R3 1
        LODA,R0 *PRAMH,R3
        COMA,R0 T1
        BCTR,LT DVWP5
        BCTR,EQ DVWP5
DVWP4:  LODA,R0 UNIONB          ; this one
        STRA,R0 T2
DVWP5:  LODA,R3 UNIONB
        ADDI,R3 SZ_MULEWAMPUS
        BCTA,UN DVWP2
DVWP6:  LODA,R0 T2              ; (as an index: offset / 6, or 255)
        COMI,R0 255
        BCTR,EQ DVWP7
        LODI,R1 0
        LODA,R2 T2
        LODI,R3 SZ_MULEWAMPUS
        ZBSR    *ZDIV8
        LODZ    R2
DVWP7:  LPUT    DVWP
        LODA,R0 T2
        COMI,R0 255
        BCFR,EQ DVWP8
        LODI,R1 SL_WAMP
        ZBRR    *ZSPRHID
DVWP8:  LODA,R3 T2              ; on its plot's peaks: plot, mountain
        LODA,R0 *PRAMH,R3
        STRA,R0 UNIONB
        ADDI,R3 1
        LODA,R0 *PRAMH,R3
        ANDI,R0 3
        RRL,R0
        STRA,R0 UNIONB+1        ; (mountain * 2)
        LODA,R3 UNIONB
        LODA,R0 PLCOL,R3
        ADDA,R0 UNIONB+1
        LPUT    WPX
        LODA,R3 UNIONB
        LODA,R0 PLROW,R3
        ADDA,R0 UNIONB+1
        LPUT    WPY
        LODA,R0 FRML            ; flashing
        ANDI,R0 8
        BCFR,EQ DVWP9
        LODI,R1 SL_WAMP
        ZBRR    *ZSPRHID
DVWP9:  LODI,R1 SL_WAMP
        LODA,R2 WPY
        LODA,R3 WPX
        ZBRR    *ZSPRSET


; DVMARKS: a computer's land outlined in white for 40 frames as its turn
; starts, and the plot it works while it stands there (dv_ai_marks).
DVMARKS: LODA,R0 DVAI
        COMI,R0 1
        BCFR,EQ DVM2
        LODA,R0 DVSEAT
        LPUT    MAPHI
        LODI,R0 2
        LPUT    DVAI
        LODA,R0 FRML
        LPUT    DVAIT
        BCTR,UN DVM3
DVM2:   COMI,R0 2
        BCFR,EQ DVM3
        LODA,R0 FRML
        SUBA,R0 DVAIT
        COMI,R0 40
        BCTR,LT DVM3
        LODI,R0 255
        LPUT    MAPHI
        EORZ    R0
        LPUT    DVAI
DVM3:   LODA,R0 FXFLAG          ; its script: 3 there, 4 leaving
        COMI,R0 3
        BCFR,EQ DVM4
        LODA,R0 DVAIP
        LPUT    MAPBLINK
        LODI,R0 1
        LPUT    MAPBLON
        BCTR,UN DVM5
DVM4:   COMI,R0 4
        RETC,LT
        RETC,GT
        LODI,R0 255
        LPUT    MAPBLINK
DVM5:   EORZ    R0
        LPUT    FXFLAG
        RETC,UN


        INCLUDE "src/plots.inc"
