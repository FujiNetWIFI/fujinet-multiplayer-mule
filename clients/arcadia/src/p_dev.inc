; p_dev.inc -- the development on the map (mekkogx develop.c): whose turn it
; is, the prompt to begin ours, our walk on the map, a computer's walk (its
; land outlined), the turn's time bar. The town (p_town.inc) takes the
; phase over while we walk in it, and gives it back; p_devt.inc holds what
; the frame far-calls (a computer's walk, "<name> takes a turn").
;
; On the map our figure is a sprite 8 x 8 (its top left WX, WY): it keeps
; to columns 16-104, lines 16-88, a step every 6 frames (14 on the river,
; 21 on mountains), a pixel or a line. Walking onto the store goes to town.

        BCTA,UN DVENTER         ; E_ENTER
        BCTA,UN DVSTAT          ; E_STATE
        BCTA,UN DVTICK          ; E_TICK

        INCLUDE "src/devram.inc"

DV_WAIT  EQU    0
DV_PROMPT EQU   1
DV_WALK  EQU    2
DV_OVER  EQU    3

MWXMIN  EQU     16              ; the map walk's bounds
MWXMAX  EQU     104
MWYMIN  EQU     16
MWYMAX  EQU     88
TWXL    EQU     4               ; the town street's ends, its middle, its line
TWXR    EQU     116
TWXST   EQU     60
TWYST   EQU     92

; ---- entering --------------------------------------------------------------------
DVENTER: LODA,R0 H_PHASE
        COMA,R0 LASTPH
        BCTA,EQ DVHAND          ; (the town giving it back)
        LODI,R0 255             ; develop_enter
        LPUT    DVSTATE
        LODI,R0 255
        CPUT    DVMSGS
        EORZ    R0
        LPUT    WMULE
        EORZ    R0
        LPUT    MAPDOTS
        EORZ    R0
        LPUT    DVAI
        EORZ    R0
        LPUT    DVBWAIT
        EORZ    R0
        LPUT    DVDO
        LODI,R0 SCR_NONE
        LPUT    SCRNOW
        BSTA,UN DVPCOPY
        BCTA,UN DVSTAT
; DVHAND: back from the town: walking out onto the map, or the turn over.
DVHAND: LODA,R0 DVSTATE
        COMI,R0 DV_WALK
        BCFA,EQ DVSTAT
        BSTA,UN DVMAP
        LODI,R0 20              ; (the first step waits)
        LPUT    WCNT
        BCTA,UN DVSTAT

; DVPCOPY: DVPMULE = the plots' M.U.L.E.s (a computer's walk finds the one
; it installed against them).
DVPCOPY: CPTR   DVPMULE
        LODI,R0 0
DVPC1:  STRA,R0 T3
        STRZ    R3
        LODA,R0 MP_MULE,R3
        BSTA,UN RAMPUT1
        LODA,R0 T3
        ADDI,R0 1
        COMI,R0 NUM_PLOTS
        BCFR,EQ DVPC1
        BCTA,UN RAMPEND

; DVMAP: the map for the development (dev_map_enter): our sprites' colours
; (8 lines), the wampus's picture; the walk goes on if it was going.
DVMAP:  BSTA,UN MAPSCR
        LODA,R0 DVSEAT          ; the time bar in the player's colour
        COMI,R0 4
        BCFR,LT DVMP0
        BSTA,UN DVREC
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATINK,R3
        LPUT    BARCOL
DVMP0:  LODI,R0 255
        CPUT    WFIGS
        LODI,R0 255
        CPUT    WMULS
        LODI,R1 SL_PL
        LODA,R2 WCOL
        ZBSR    *ZSPRCOL
        LODI,R1 SL_MULE
        LODI,R2 SWHITE
        ZBSR    *ZSPRCOL
        LODI,R1 SL_WAMP
        LODI,R2 SWHITE
        ZBSR    *ZSPRCOL
        LODI,R0 ART_WAMP>>8
        LODI,R1 ART_WAMP&$FF
        LODI,R2 SL_WAMP
        LODI,R3 PG_ART
        ZBSR    *ZUDCQ
        LODA,R0 DVSTATE
        COMI,R0 DV_WALK
        BCFR,EQ DVMP1
        EORZ    R0              ; dv_resume
        LPUT    WHOLD
        LODI,R0 4
        LPUT    WFACE
        BSTA,UN WDRAW
DVMP1:  LODI,R1 T_DEVELOP       ; (the title last: SAY is deep)
        BCTA,UN TITLE

; ---- a state (develop_state) -------------------------------------------------------
DVSTAT: LODA,R0 TN_SEAT
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
        BCFR,EQ DVS1
        LODI,R0 28
DVS1:   LPUT    DVPTU
        EORZ    R0
        STRA,R0 T0
        LODA,R0 TN_SEAT
        COMA,R0 H_MYSEAT
        BCFR,EQ DVS2
        LODI,R0 1
        STRA,R0 T0
DVS2:   LODA,R0 T0
        LPUT    DVMINE
        BSTA,UN DVWALKS         ; our walk goes on?
        BCFR,EQ DVS4
        LODA,R0 DVSTATE
        COMI,R0 DV_WALK
        BCFA,EQ DVSTART         ; it begins: into the town
        LODA,R0 DVFLAGS         ; the server's view of what we lead wins
        ANDI,R0 TF_HASMULE
        BCTR,EQ DVS3
        LODI,R0 1
DVS3:   LPUT    WMULE
        BCTA,UN DVMRES
DVS4:   LODA,R0 DVSTATE         ; anything else happens on the map
        COMI,R0 DV_WALK
        BCFR,EQ DVS5
        BSTA,UN DVSTOP
DVS5:   LODA,R0 SCRNOW
        COMI,R0 SCR_MAP
        BCTR,EQ DVS6
        BSTA,UN DVMAP
DVS6:   LODA,R0 DVMINE          ; ours, not begun: the prompt
        BCTR,EQ DVS8
        LODA,R0 DVFLAGS
        ANDI,R0 TF_STARTED
        BCFR,EQ DVS8
        LODA,R0 DVSTATE
        COMI,R0 DV_PROMPT
        BCTA,EQ DVS20
        LODI,R0 DV_PROMPT
        LPUT    DVSTATE
        LODI,R1 M_START
        ZBSR    *ZSAY
        SND     SND_ALERT
        BCTA,UN DVS20
DVS8:   LODA,R0 DVSEAT          ; someone's turn: said once as it starts,
        COMI,R0 4               ;  once as it ends (a computer's plays out
        BCTA,GT DVS20           ;  on the screen: "taking his turn" until
        BCTA,EQ DVS20           ;  the next seat)
        BSTA,UN DVREC
        LODI,R1 1
        LODA,R0 H_PLAYERS+PL_FLAGS,R3
        TMI,R0  PF_AI
        BCTR,EQ DVS9
        LODI,R1 2
        LODA,R0 DVFLAGS
        TMI,R0  TF_ENDED
        BCTR,EQ DVS9
        LODI,R1 0
DVS9:   STRA,R1 T1
        LODI,R3 DVMSGSO
        LODA,R0 *PRAMH,R3
        COMA,R0 DVSEAT
        BCFR,EQ DVS10
        LODI,R3 DVMSGKO
        LODA,R0 *PRAMH,R3
        COMA,R0 T1
        BCTA,EQ DVS20
DVS10:  LODA,R0 DVSEAT
        CPUT    DVMSGS
        LODA,R0 T1
        CPUT    DVMSGK
        LODA,R0 T1
        COMI,R0 1
        BCFR,EQ DVS11
        LODI,R0 DV_WAIT         ; a computer: its walk (the frame starts it)
        LPUT    DVSTATE
        LODI,R1 M_COMPUTER
        ZBSR    *ZSAY
        LODA,R0 DVDO
        IORI,R0 DO_AIWALK
        LPUT    DVDO
        BCTA,UN DVS20
DVS11:  COMI,R0 2
        BCFR,EQ DVS14
        LODI,R0 DV_OVER         ; over: what the pub paid, time ran out
        LPUT    DVSTATE
        LODA,R0 DVMINE
        BCTR,EQ DVS12
        LODA,R0 ACTSENT
        COMI,R0 ACT_PUB
        BCFR,EQ DVS12
        LODI,R1 M_GAMBLE
        LODA,R2 H_RESULT
        LODA,R3 H_RESULT+1
        ZBSR    *ZSAYV
        BCTR,UN DVS20
DVS12:  LODA,R0 DVLEFTH
        IORA,R0 DVLEFTL
        BCFR,EQ DVS13
        LODI,R1 M_TIMEOUT
        ZBSR    *ZSAY
        BCTR,UN DVS20
DVS13:  LODA,R1 MSGROW
        LODA,R3 MSGN
        ZBSR    *ZDCLR
        BCTR,UN DVS20
DVS14:  LODI,R0 DV_WAIT         ; playing: "<name> takes a turn"
        LPUT    DVSTATE
        LODA,R0 DVDO
        IORI,R0 DO_TURN
        LPUT    DVDO
DVS20:  LODA,R0 DVSEAT          ; the time bar, in the player's colour
        COMI,R0 4
        BCFR,LT DVS21
        BSTA,UN DVREC
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATINK,R3
        LPUT    BARCOL
DVS21:  LODA,R0 DVFLAGS
        ANDI,R0 TF_STARTED|TF_ENDED
        COMI,R0 TF_STARTED
        BCFR,EQ DVS22
        BSTA,UN DVBAR
        BCTR,UN DVS23
DVS22:  EORZ    R0
        LPUT    BARST
DVS23:  LODA,R0 DVDO            ; the M.U.L.E.s as they stand (a computer's
        ANDI,R0 DO_AIWALK       ;  walk wants the last state's: p_devt
        RETC,GT                 ;  copies them after)
        BCTA,UN DVPCOPY

; DVWALKS: CC EQ when our walk goes on: our turn, begun, not over.
DVWALKS: LODA,R0 DVMINE
        COMI,R0 1
        RETC,LT
        LODA,R0 DVFLAGS
        ANDI,R0 TF_STARTED|TF_ENDED
        COMI,R0 TF_STARTED
        RETC,UN

; DVREC: R3 = seat R0's record. Leaf.
DVREC:  STRZ    R3
        LODA,R0 DVRECS,R3
        STRZ    R3
        RETC,UN
DVRECS: DB      0,29,58,87

; DVSTART: our turn begins (dv_start_walk): in the middle of the town.
DVSTART: LODI,R0 DV_WALK
        LPUT    DVSTATE
        LODI,R0 255
        LPUT    DVSAMPLE
        EORZ    R0
        LPUT    DVOFFICE
        EORZ    R0
        LPUT    DVBWAIT
        EORZ    R0
        CPUT    DVEVSH
        LODA,R0 H_MYSEAT
        BSTR,UN DVREC
        LODA,R0 H_PLAYERS+PL_SPECIES,R3
        ANDI,R0 7
        LPUT    WSP
        LODA,R0 H_MYSEAT
        BSTR,UN DVREC
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATSPR,R3
        LPUT    WCOL
        BSTA,UN WCLRH
        LODI,R0 TWXST
        LPUT    WX
        LODI,R0 TWYST
        LPUT    WY
        EORZ    R0
        LPUT WDOOR
        EORZ    R0
        LPUT    WCNT
        EORZ    R0
        LPUT    WHOLD
        LODI,R0 1
        LPUT    WTOWN
        LODA,R0 DVDO
        IORI,R0 DO_EVENT
        LPUT    DVDO
        SND     SND_SELECT
DVTOWN: LODI,R0 PG_TOWN         ; the town takes over
        CPUT    PHPAGE
        LODI,R0 1
        CPUT    PGNEW
        RETC,UN

; WCLRH: the M.U.L.E.'s trail forgotten.
WCLRH:  LODI,R0 7
WCH1:   STRA,R0 T3
        ADDI,R0 WTX&$FF
        STRZ    R3
        EORZ    R0
        ZBSR    *ZLPUT
        LODA,R0 T3
        SUBI,R0 1
        BCTR,GT WCH1
        BCTR,EQ WCH1
        EORZ    R0
        LPUT    WTH
        RETC,UN

; DVSTOP: our walk is over (dv_stop_walk).
DVSTOP: LODI,R0 DV_OVER
        LPUT    DVSTATE
        LODI,R0 1
        LPUT    WHOLD
        LODI,R1 SL_PL
        ZBSR    *ZSPRHID
        LODI,R1 SL_MULE
        ZBSR    *ZSPRHID
        LODI,R1 SL_WAMP
        ZBRR    *ZSPRHID

; DVMRES: what the server said about a map action of ours (dv_map_result).
DVMRES: LODA,R0 ACTSENT
        COMI,R0 ACT_INST
        RETC,LT
        COMI,R0 ACT_WAMPUS
        RETC,GT
        LODA,R0 H_ERR
        BCTR,EQ DVMR1
        SND     SND_ERROR
        BCTA,UN DVMR9
DVMR1:  LODA,R0 ACTSENT
        COMI,R0 ACT_INST
        BCFR,EQ DVMR3
        LODA,R0 H_RESULT+1
        COMI,R0 1
        BCFR,EQ DVMR2
        SND     SND_ERROR       ; it ran off
        LODI,R1 M_RANAWAY
        BCTR,UN DVMR8
DVMR2:  SND     SND_BUY
        BCTR,UN DVMR9
DVMR3:  LODI,R1 M_CAUGHT
        COMI,R0 ACT_WAMPUS
        BCFR,EQ DVMR4
        SND     SND_FOUND
        LODI,R1 M_CAUGHT
        BCTR,UN DVMR8
DVMR4:  LODI,R1 M_FORSALE
        COMI,R0 ACT_SELL
        BCTR,EQ DVMR8
        LODI,R1 M_NOTFORSALE
        COMI,R0 ACT_UNSELL
        BCFR,EQ DVMR9
DVMR8:  LODA,R2 H_RESULT
        LODA,R3 H_RESULT+1
        ZBSR    *ZSAYV
DVMR9:  EORZ    R0
        STRA,R0 ACTSENT
        RETC,UN

; ---- a frame (develop_tick) ------------------------------------------------------
DVTICK: LODA,R0 DVDO            ; far calls the state asked for
        ANDI,R0 DO_AIWALK|DO_TURN
        BCTR,EQ DVT0
        BSTA,UN DVFAR
DVT0:   LODA,R0 DVSTATE
        COMI,R0 DV_PROMPT
        BCFR,EQ DVT2
        LODA,R0 EVBTN           ; the prompt: FIRE begins
        RETC,EQ
        LODI,R0 ACT_BEGIN
        LODI,R1 0
        LODI,R2 0
        ZBSR    *ZACT
        SND     SND_SELECT
        RETC,UN
DVT2:   COMI,R0 DV_WALK
        BCTR,EQ DVT5
        COMI,R0 DV_WAIT         ; watching: a computer's marks, the bar
        BCFR,EQ DVT3
        LODI,R1 PG_DEVT
        LODI,R3 E_EXTRA+6
        BSTA,UN PGFAR
DVT3:   LODA,R0 DVFLAGS
        ANDI,R0 TF_STARTED|TF_ENDED
        COMI,R0 TF_STARTED
        RETC,LT
        RETC,GT
        BCTA,UN DVBAR
DVT5:   BSTA,UN DVBAR           ; our walk on the map
        BSTA,UN DVWALK
        LODI,R1 PG_DEVT         ; the wampus (p_devt.inc)
        LODI,R3 E_EXTRA
        BSTA,UN PGFAR
        LODA,R0 EVBTN
        BCTR,EQ DVT6
        LODI,R1 PG_DEVT         ; the button
        LODI,R3 E_EXTRA+3
        BSTA,UN PGFAR
DVT6:   LODA,R0 EVKEY           ; Start ends the turn
        COMI,R0 K_START
        RETC,LT
        RETC,GT
        LODI,R0 ACT_END
        LODI,R1 0
        LODI,R2 0
        ZBRR    *ZACT

; DVFAR: what the state left for the frame (p_devt.inc).
DVFAR:  LODA,R0 DVDO
        ANDI,R0 DO_AIWALK
        BCTR,EQ DVF1
        LODA,R0 DVDO
        ANDI,R0 $FF-DO_AIWALK
        LPUT    DVDO
        LODI,R1 PG_DEVT
        LODI,R3 E_ENTER
        BSTA,UN PGFAR
DVF1:   LODA,R0 DVDO
        ANDI,R0 DO_TURN
        RETC,EQ
        LODA,R0 DVDO
        ANDI,R0 $FF-DO_TURN
        LPUT    DVDO
        LODI,R1 PG_DEVT
        LODI,R3 E_TICK
        BCTA,UN PGFAR

; DVBAR: the turn's time bar run down from the last state (dv_bar): its
; units left = (left - frames since) / pTU, ticking under 30; on the map's
; bar a step is 5 units. A step more than shown is a later reply's jitter:
; it stays down.
DVBAR:  LODA,R1 RXFRH
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
        BCTR,EQ DVB0
        LODA,R3 DVPTU
        ZBSR    *ZDIV8
        LODZ    R2
        BCTR,UN DVB1
DVB0:   EORZ    R0
DVB1:   STRA,R0 T3
        BSTA,UN BARTICK
        LODI,R1 0               ; / 5
        LODA,R2 T3
        LODI,R3 5
        ZBSR    *ZDIV8
        LODZ    R2
        COMI,R0 20
        BCTR,LT DVB2
        LODI,R0 20
DVB2:   STRA,R0 T2
        LODA,R0 BARST
        ADDI,R0 1
        COMA,R0 T2
        RETC,EQ
        LODA,R0 T2
        LPUT    BARST
        RETC,UN

; ---- our walk on the map (walk_isr, walk_map) ---------------------------------------
DVWALK: LODA,R0 WHOLD
        RETC,GT
        LODA,R0 WCNT            ; (a step when it runs out: 6 frames from 5)
        SUBA,R0 DT
        BCTR,LT DVW0
        LPUT    WCNT
        RETC,UN
DVW0:   LODA,R0 INDIR
        ANDI,R0 $0F
        BCFR,EQ DVW2
        LPUT    WCNT            ; (0: a push steps at once)
        LODA,R0 WSTEP           ; let go: standing
        BCTR,EQ DVW0S
        EORZ    R0
        LPUT    WSTEP
        LODI,R0 4
        LPUT    WFACE
        BCTA,UN WDRAW
DVW0S:    LODI,R3 WFIGSO          ; (standing: until its picture is in, the
        LODA,R0 *PRAMH,R3       ;  queue having had no room)
        COMI,R0 255
        RETC,LT
        BCTA,UN WDRAW
DVW2:   STRA,R0 T3              ; T3: the directions held
        LODI,R0 5
        LPUT    WCNT
        LODA,R0 T3              ; across
        TMI,R0  D_LEFT
        BCFR,EQ DVW3
        LODA,R0 WX
        COMI,R0 MWXMIN
        BCFR,GT DVW4
        SUBI,R0 1
        LPUT    WX
        BCTR,UN DVW4
DVW3:   TMI,R0  D_RIGHT
        BCFR,EQ DVW4
        LODA,R0 WX
        COMI,R0 MWXMAX
        BCFR,LT DVW4
        ADDI,R0 1
        LPUT    WX
DVW4:   LODA,R0 T3              ; up and down
        TMI,R0  D_UP
        BCFR,EQ DVW5
        LODA,R0 WY
        COMI,R0 MWYMIN
        BCFR,GT DVW6
        SUBI,R0 1
        LPUT    WY
        BCTR,UN DVW6
DVW5:   TMI,R0  D_DOWN
        BCFR,EQ DVW6
        LODA,R0 WY
        COMI,R0 MWYMAX
        BCFR,LT DVW6
        ADDI,R0 1
        LPUT    WY
DVW6:   BSTA,UN DVTERR
        LODA,R0 WX              ; into the store (plot 22: 56-63, 48-63)?
        ADDI,R0 5
        COMI,R0 56
        BCTR,LT DVW8
        LODA,R0 WX
        ADDI,R0 2
        COMI,R0 63
        BCTR,GT DVW8
        LODA,R0 WY
        ADDI,R0 3
        COMI,R0 48
        BCTR,LT DVW8
        LODA,R0 WY
        COMI,R0 63
        BCTR,GT DVW8
        LODI,R0 TWXL            ; to the town's end on that side
        LODA,R1 WX
        COMI,R1 60
        BCTR,LT DVW7
        LODI,R0 TWXR
DVW7:   LPUT    WX
        LODI,R0 TWYST
        LPUT    WY
        LODI,R0 1
        LPUT    WTOWN
        BSTA,UN WCLRH
        BSTA,UN DVSPROFF
        BCTA,UN DVTOWN
DVW8:   LODI,R0 1               ; a step: its frame, its sound, the trail
        LPUT    WSTEP
        LODA,R0 WFRAME
        EORI,R0 1
        LPUT    WFRAME
        LODA,R1 T3
        LODI,R0 0               ; right 0-1, left 2-3, down 6-7, up 8-9
        TMI,R1  D_RIGHT
        BCTR,EQ DVW9
        LODI,R0 2
        TMI,R1  D_LEFT
        BCTR,EQ DVW9
        LODI,R0 6
        TMI,R1  D_DOWN
        BCTR,EQ DVW9
        LODI,R0 8
DVW9:   ADDA,R0 WFRAME
        LPUT    WFACE
        SND     SND_STEP
        LODA,R0 WTH             ; remembered for the M.U.L.E.
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
        BCTA,UN WDRAW

; DVSPROFF: our sprites off (a screen change).
DVSPROFF: LODI,R1 SL_PL
        ZBSR    *ZSPRHID
        LODI,R1 SL_MULE
        ZBSR    *ZSPRHID
        LODI,R1 SL_WAMP
        ZBRR    *ZSPRHID

; DVTERR: the step's length by what the figure stands on (walk_terrain):
; the river 8 frames more, a plot's peaks (its upper cell) 15.
DVTERR: LODA,R0 WX              ; the plot under its middle
        ADDI,R0 4
        SUBI,R0 24
        RETC,LT
        STRZ    R2
        RRR,R0
        RRR,R0
        RRR,R0
        ANDI,R0 $1F
        COMI,R0 9
        RETC,GT
        RETC,EQ
        STRA,R0 T2
        LODA,R0 WY
        ADDI,R0 4
        SUBI,R0 16
        RETC,LT
        STRZ    R1              ; (bit 3: the lower cell)
        RRR,R0
        RRR,R0
        RRR,R0
        RRR,R0
        ANDI,R0 $0F
        COMI,R0 5
        RETC,GT
        RETC,EQ
        STRZ    R3              ; plot = row * 9 + column
        RRL,R0
        RRL,R0
        RRL,R0
        ADDZ    R3
        ADDA,R0 T2
        STRZ    R3
        LODA,R0 MP_TERRAIN,R3
        STRA,R0 T2
        ANDI,R0 3
        COMI,R0 TERRAIN_RIVER
        BCFR,EQ DVTR1
        LODI,R0 8
        BCTR,UN DVTR2
DVTR1:  LODA,R0 T2              ; peaks, and the figure in the upper cell
        ANDI,R0 $0C
        RETC,EQ
        TMI,R1  8
        RETC,EQ
        LODI,R0 15
DVTR2:  ADDA,R0 WCNT
        LPUT    WCNT
        RETC,UN

; WDRAW: our figure (8 x 8) and the M.U.L.E. four steps behind it.
WDRAW:  LODA,R0 WSP             ; its picture: species * 10 + face
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
        BCTR,EQ WDR1
        LODA,R0 UDCQN
        COMI,R0 3
        BCFR,LT WDR1
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
WDR1:   LODI,R1 SL_PL
        LODA,R2 WY
        LODA,R3 WX
        ZBSR    *ZSPRSET
        LODA,R0 WMULE           ; the M.U.L.E.
        BCTA,EQ WDR9
        LODA,R3 WTH             ; at the oldest step remembered
        LODA,R0 WTX,R3
        BCFR,EQ WDR2
        LODI,R3 0
        LODA,R0 WTX,R3
        BCTA,EQ WDR9
WDR2:   STRA,R3 T3
        LODA,R0 WFACE           ; facing as we go: left 2, right 0
        COMI,R0 2
        BCTR,LT WDR3A
        COMI,R0 4
        BCFR,LT WDR3A
        LODI,R0 2
        BCTR,UN WDR3
WDR3A:  LODI,R0 0
WDR3:   ADDA,R0 WFRAME
        STRA,R0 T2
        LODI,R3 WMULSO
        LODA,R0 *PRAMH,R3
        COMA,R0 T2
        BCTR,EQ WDR4
        LODA,R0 UDCQN
        COMI,R0 3
        BCFR,LT WDR4
        LODA,R0 T2
        CPUT    WMULS
        LODA,R0 T2              ; ART_MULE + frame * 8
        RRL,R0
        RRL,R0
        RRL,R0
        ADDI,R0 ART_MULE&$FF
        STRZ    R1
        LODI,R0 ART_MULE>>8
        LODI,R2 SL_MULE
        LODI,R3 PG_ART
        ZBSR    *ZUDCQ
WDR4:   LODA,R3 T3
        LODA,R0 WTY,R3
        STRZ    R2
        LODA,R0 WTX,R3
        STRZ    R3
        LODI,R1 SL_MULE
        ZBRR    *ZSPRSET
WDR9:   LODI,R1 SL_MULE
        ZBRR    *ZSPRHID

        INCLUDE "src/mapscr.inc"
        INCLUDE "src/plots.inc"

SEATINK: DB     CBLUE,CMAGEN,CCYAN,CWHITE
SEATSPR: DB     SBLUE,SMAGEN,SCYAN,SWHITE       ; (8 lines: no $80)
