; p_auto.inc -- AUTOPLAY builds play by themselves (mekkogx autoplay.c and
; each phase's ap_*), for soak tests and filmed games against a real
; server. Only what a player would do, through the disc and the button, a
; frame at a time, so the screen shows a real game: ready (once four sit,
; or after 10 seconds), claim land, walk a turn (corral, outfitter, out to
; our plot, install, the pub), declare and trade. The main loop calls it
; before the network and the phase (fixb.inc, IF AUTOPLAY).

        BCTA,UN APMAIN          ; E_ENTER
        RETC,UN
        NOP
        NOP
        RETC,UN
        NOP
        NOP

        INCLUDE "src/grram.inc"
        INCLUDE "src/auram.inc"
        INCLUDE "src/devram.inc"

APPH    EQU     0               ; APV: the phase we began
APT0H   EQU     1               ;  as of this frame
APT0L   EQU     2
APPF    EQU     3               ;  the frame (low byte) of our last press
APST    EQU     4               ;  the turn's step (AP_*)
APGOOD  EQU     5               ;  the good we outfit for
APIN    EQU     6               ;  1: we went into a building
APTX    EQU     7               ;  our plot: where to stand
APTY    EQU     8
APSIDE  EQU     9               ;  the town's end to leave by (D_LEFT, D_RIGHT)
APGET   MACRO   k
        LODI,R3 APV-$2900+(k)
        LODA,R0 *PRAMH,R3
        ENDM
APPUT   MACRO   k
        CPUT    APV+(k)
        ENDM
AP_CORRAL EQU   0
AP_SHOP EQU     1
AP_OUT  EQU     2
AP_PLOT EQU     3
AP_HOME EQU     4
AP_PUB  EQU     5
AP_END  EQU     6
AP_DONE EQU     7
AP_WAIT EQU     8
AP_PROMPT EQU   9
AP_OFF  EQU     10
DV_PROMPT EQU   1
DV_WALK EQU     2

APMAIN: LODA,R0 H_MYSEAT        ; a player's game only
        COMI,R0 4
        RETC,GT
        RETC,EQ
        APGET   APPH            ; a new phase: its start
        COMA,R0 H_PHASE
        BCTR,EQ APM1
        LODA,R0 H_PHASE
        APPUT   APPH
        BSTA,UN APNOW
APM1:   EORZ    R0              ; the disc is ours
        STRA,R0 INDIR
        LODA,R0 H_PHASE
        BCTA,EQ APSTAT
        COMI,R0 PHASE_SUMMARY
        BCTA,EQ APSTAT
        COMI,R0 PHASE_GRANT
        BCTA,EQ APGRANT
        COMI,R0 PHASE_DEVELOP
        BCTA,EQ APDEV
        COMI,R0 PHASE_PRODUCTION
        RETC,EQ
        COMI,R0 PHASE_GAMEOVER
        RETC,EQ
        BCTA,UN APAUC

; APNOW: the phase's (or a step's) start is now.
APNOW:  LODA,R0 FRMH
        APPUT   APT0H
        LODA,R0 FRML
        APPUT   APT0L
        RETC,UN
; APSINCE: R1:R2 = frames since then.
APSINCE: APGET  APT0H
        STRZ    R1
        APGET   APT0L
        STRZ    R2
        ZBRR    *ZSINCE
; APEVERY: CC EQ (and noted) at most every R0 frames: a press the server
; may not have seen yet isn't repeated at once.
APEVERY: STRA,R0 T0
        APGET   APPF
        STRZ    R1
        LODA,R0 FRML
        SUBZ    R1
        COMA,R0 T0
        BCTR,LT APE9
        LODA,R0 FRML
        APPUT   APPF
        EORZ    R0              ; (EQ)
        RETC,UN
APE9:   LODI,R0 1               ; (not EQ)
        RETC,UN
APBTN:  LODI,R0 1
        STRA,R0 EVBTN
        RETC,UN
APREC:  STRZ    R3              ; R3 = seat R0's record
        LODA,R0 APRECS,R3
        STRZ    R3
        RETC,UN
APRECS: DB      0,29,58,87

; ---- the lobby, the summary (ap_status) ------------------------------------------
APSTAT: LODA,R0 H_MYSEAT        ; ready already: nothing
        BSTR,UN APREC
        LODA,R0 H_PLAYERS+PL_FLAGS,R3
        TMI,R0  PF_READY
        RETC,EQ
        BSTA,UN APSINCE
        LODA,R0 H_PHASE
        BCFR,EQ APS4
        LODI,R3 0               ; the lobby: players sitting
        EORZ    R0
        STRA,R0 T1
APS1:   LODA,R0 H_PLAYERS+PL_NAME,R3
        BCTR,EQ APS2
        LODA,R0 H_PLAYERS+PL_FLAGS,R3
        TMI,R0  PF_AI
        BCTR,EQ APS2
        LODA,R0 T1
        ADDI,R0 1
        STRA,R0 T1
APS2:   ADDI,R3 29
        COMI,R3 4*29
        BCFR,EQ APS1
        LODA,R0 T1              ; four, or 10 seconds on
        COMI,R0 4
        BCTR,EQ APS5
        COMI,R1 600>>8
        RETC,LT
        BCFR,EQ APS5
        COMI,R2 600&$FF
        RETC,LT
        BCTR,UN APS5
APS4:   COMI,R1 560>>8          ; the summary: its table up a while
        RETC,LT
        BCFR,EQ APS5
        COMI,R2 560&$FF
        RETC,LT
APS5:   LODI,R0 45
        BSTA,UN APEVERY
        RETC,GT
        BCTA,UN APBTN

; ---- the grant (ap_grant) ---------------------------------------------------------
; Press on our seat's kind of land (river, mountains, plains, plains), or
; plains from row 2, or anything from row 4: 3 frames into a step.
APGRANT: LODA,R0 GCLAIMED
        RETC,GT
        LODA,R0 GDEPART
        RETC,GT
        LODA,R0 GSHOW
        RETC,EQ
        LODA,R3 GCUR
        COMI,R3 NUM_PLOTS
        RETC,GT
        RETC,EQ
        LODA,R0 FRML
        SUBA,R0 GSTEPF
        COMI,R0 3
        RETC,LT
        LODA,R0 MP_TERRAIN,R3
        ANDI,R0 3
        STRA,R0 T1
        LODA,R3 H_MYSEAT
        LODA,R0 APLAND,R3
        COMA,R0 T1
        BCTA,EQ APBTN
        LODA,R0 GCUR
        COMI,R0 36
        BCTA,GT APBTN
        BCTA,EQ APBTN
        COMI,R0 18
        RETC,LT
        LODA,R0 T1
        COMI,R0 TERRAIN_PLAIN
        BCTA,EQ APBTN
        RETC,UN
APLAND: DB      TERRAIN_RIVER,TERRAIN_MOUNTAIN,TERRAIN_PLAIN,TERRAIN_PLAIN

; ---- the auctions (ap_auction) --------------------------------------------------------
; Land: press on through the plot's showing; then bid a little over the
; start (with fewer than six plots and the money), each seat stopping at
; its own limit. Goods: declare a seller with more than next month needs,
; a buyer with less; a seller walks its line down until its surplus has
; gone, a buyer up (to the store's price) until it has enough.
APAUC:  LODA,R0 AUKIND
        COMI,R0 1
        BCFA,EQ APAG
        LODA,R0 AUSTG
        BCFR,EQ APAL1
        BSTA,UN APSINCE         ; the plot showing: on after 90 frames
        LODZ    R1
        BCFR,EQ APAL0
        COMI,R2 90
        RETC,LT
APAL0:  LODI,R0 45
        BSTA,UN APEVERY
        RETC,GT
        BCTA,UN APBTN
APAL1:  COMI,R0 STAGE_LEAD
        RETC,LT
        LODA,R3 H_MYSEAT
        LODA,R0 AUROLE,R3
        COMI,R0 ROLE_BUYER
        RETC,LT
        RETC,GT
        LODI,R3 0               ; our plots: fewer than 6
        EORZ    R0
        STRA,R0 T1
APAL2:  LODA,R0 MP_OWNER,R3
        COMA,R0 H_MYSEAT
        BCFR,EQ APAL3
        LODA,R0 T1
        ADDI,R0 1
        STRA,R0 T1
APAL3:  ADDI,R3 1
        COMI,R3 NUM_PLOTS
        BCFR,EQ APAL2
        LODA,R0 T1
        COMI,R0 6
        RETC,GT
        RETC,EQ
        LODA,R0 H_MYSEAT        ; money > start + 300
        BSTA,UN APREC
        LODA,R0 A_START+1
        ADDI,R0 300&$FF
        STRZ    R2
        PPSL    PSLWC
        LODA,R0 A_START
        ADDI,R0 300>>8
        CPSL    PSLWC
        COMA,R0 H_PLAYERS+PL_MONEY,R3
        RETC,GT
        BCFR,EQ APAL4
        LODZ    R2
        COMA,R0 H_PLAYERS+PL_MONEY+1,R3
        RETC,GT
        RETC,EQ
APAL4:  LODA,R0 H_MYSEAT        ; our line under start + (seat + 2) * 4 steps
        ADDI,R0 2
        RRL,R0
        RRL,R0
        STRZ    R1
        LODA,R2 AUSTEPV
        ZBSR    *ZMUL8
        LODA,R0 A_START+1
        ADDZ    R2
        STRZ    R2
        PPSL    PSLWC
        LODA,R0 A_START
        ADDZ    R1
        CPSL    PSLWC
        STRZ    R1
        LODA,R0 H_MYSEAT
        RRL,R0
        STRZ    R3
        LODA,R0 AUPOS,R3
        COMZ    R1
        RETC,GT
        BCFR,EQ APUP
        LODA,R0 AUPOS+1,R3
        COMZ    R2
        RETC,GT
        RETC,EQ
APUP:   LODI,R0 D_UP
        STRA,R0 INDIR
        RETC,UN
APDOWN: LODI,R0 D_DOWN
        STRA,R0 INDIR
        RETC,UN
APAG:   LODA,R0 H_MYSEAT        ; goods: T1 what we have, T2 next month's need
        BSTA,UN APREC
        LODZ    R3
        ADDA,R0 AUGOOD
        STRZ    R3
        LODA,R0 H_PLAYERS+PL_GOODS,R3
        STRA,R0 T1
        BSTA,UN APNEED
        STRA,R0 T2
        LODA,R0 AUSTG
        COMI,R0 STAGE_DECLARE
        BCFR,EQ APAG5
        BSTA,UN APSINCE         ; declaring: 40 frames in
        LODZ    R1
        BCFR,EQ APAG1
        COMI,R2 40
        RETC,LT
APAG1:  LODI,R1 ROLE_NONE       ; want: seller over, buyer under
        LODA,R0 T1
        COMA,R0 T2
        BCTR,EQ APAG2
        BCTR,LT APAG1B
        LODI,R1 ROLE_SELLER
        BCTR,UN APAG2
APAG1B: LODI,R1 ROLE_BUYER
APAG2:  LODA,R3 H_MYSEAT
        LODA,R0 AUROLE,R3
        COMZ    R1
        RETC,EQ
        COMI,R1 ROLE_SELLER
        BCTA,EQ APUP
        COMI,R1 ROLE_BUYER
        BCTA,EQ APDOWN
        LODI,R0 45              ; none: sit out (0)
        BSTA,UN APEVERY
        RETC,GT
        EORZ    R0
        STRA,R0 EVKEY
        RETC,UN
APAG5:  COMI,R0 STAGE_LEAD      ; trading
        RETC,LT
        COMI,R0 STAGE_DONE
        RETC,EQ
        LODA,R3 H_MYSEAT
        LODA,R0 AUROLE,R3
        COMI,R0 ROLE_SELLER
        BCFR,EQ APAG6
        LODA,R0 T1              ; a seller with surplus: down
        COMA,R0 T2
        BCTA,GT APDOWN
        RETC,UN
APAG6:  COMI,R0 ROLE_BUYER
        RETC,LT
        RETC,GT
        LODA,R0 T2              ; a buyer short: up, to the store's price
        COMA,R0 T1
        RETC,LT
        RETC,EQ
        LODA,R0 H_MYSEAT
        RRL,R0
        STRZ    R3
        LODA,R0 AUPOS,R3
        COMA,R0 A_STORESELL
        BCTA,LT APUP
        RETC,GT
        LODA,R0 AUPOS+1,R3
        COMA,R0 A_STORESELL+1
        BCTA,LT APUP
        RETC,UN

; APNEED: R0 = the units of AUGOOD we need next round (au_need).
APNEED: LODA,R2 H_ROUND
        ADDI,R2 1
        LODA,R0 AUGOOD
        BCFR,EQ APN3
        EORZ    R0              ; food
        COMI,R2 12
        RETC,GT
        LODI,R0 3
        COMI,R2 4
        RETC,LT
        LODI,R0 4
        COMI,R2 8
        RETC,LT
        LODI,R0 5
        RETC,UN
APN3:   COMI,R0 GOOD_ENERGY
        BCTR,EQ APN4
        EORZ    R0
        RETC,UN
APN4:   EORZ    R0
        STRA,R0 T3
        LODI,R3 0
APN5:   LODA,R0 MP_OWNER,R3
        COMA,R0 H_MYSEAT
        BCFR,EQ APN6
        LODA,R0 MP_MULE,R3
        COMI,R0 4
        BCFR,LT APN6
        COMI,R0 GOOD_ENERGY
        BCTR,EQ APN6
        LODA,R0 T3
        ADDI,R0 1
        STRA,R0 T3
APN6:   ADDI,R3 1
        COMI,R3 NUM_PLOTS
        BCFR,EQ APN5
        LODA,R0 T3
        RETC,UN

; ---- the development (ap_develop) ------------------------------------------------------
; Buy a M.U.L.E. at the corral, outfit it for our first empty plot (river:
; food, mountains: smithore, plains: food on the odd seats, energy on the
; even), leave town on the plot's side, walk down or up to its row and
; along to it, install, then back into town to the pub if there is time,
; else end the turn. Short of time, money, M.U.L.E.s or land: the pub.
APDEV:  LODA,R0 DVSTATE
        COMI,R0 DV_PROMPT
        BCFR,EQ APD2
        APGET   APST            ; the prompt: the button, a second on
        COMI,R0 AP_PROMPT
        BCTR,EQ APD1
        LODI,R0 AP_PROMPT
        APPUT   APST
        BCTA,UN APNOW
APD1:   BSTA,UN APSINCE
        LODZ    R1
        BCFR,EQ APD1A
        COMI,R2 60
        RETC,LT
APD1A:  LODI,R0 45
        BSTA,UN APEVERY
        RETC,GT
        BCTA,UN APBTN
APD2:   COMI,R0 DV_WALK
        BCTR,EQ APD3
        LODI,R0 AP_OFF
        APPUT   APST
        RETC,UN
APD3:   LODA,R0 DVBWAIT         ; in a doorway: the server has it
        BCTR,EQ APD4
        LODI,R0 1
        APPUT   APIN
        RETC,UN
APD4:   LODA,R0 WHOLD
        RETC,GT
        LODA,R0 WDOOR
        RETC,GT
        LODA,R1 RXFRH           ; T3: the time left, in 256ths
        LODA,R2 RXFRL
        ZBSR    *ZSINCE
        LODA,R0 DVLEFTL
        SUBZ    R2
        PPSL    PSLWC
        LODA,R0 DVLEFTH
        SUBZ    R1
        CPSL    PSLWC
        BCFR,LT APD5
        EORZ    R0
APD5:   STRA,R0 T3
        APGET   APST
        COMI,R0 AP_WAIT
        BCTR,LT APD6
        BSTA,UN APPLAN
        RETC,GT                 ; (standing a moment)
        APGET   APST
APD6:   EORZ    R0              ; T2: the stick
        STRA,R0 T2
        APGET   APST
        COMI,R0 AP_CORRAL
        BCFR,EQ APD10
        LODA,R0 WMULE           ; the corral
        BCTR,EQ APD7
        LODI,R0 AP_SHOP
        APPUT   APST
        EORZ    R0
        APPUT   APIN
        BCTA,UN APDGO
APD7:   APGET   APIN
        BCTR,EQ APD8
        LODI,R0 AP_PUB          ; none for us
        APPUT   APST
        BCTA,UN APDGO
APD8:   LODI,R1 108
        LODI,R2 D_DOWN
        BSTA,UN APDOOR
        BCTA,UN APDGO
APD10:  COMI,R0 AP_SHOP
        BCFA,EQ APD14
        APGET   APGOOD          ; the outfitter
        COMA,R0 TN_CARRYING
        BCFR,EQ APD11
        LODI,R0 AP_OUT
        APPUT   APST
        BCTA,UN APDGO
APD11:  APGET   APIN
        BCTR,EQ APD12
        LODI,R0 AP_PUB
        APPUT   APST
        BCTA,UN APDGO
APD12:  APGET   APGOOD          ; its door: 12 + 32 * (3 - good)
        STRZ    R1
        LODI,R0 3
        SUBZ    R1
        RRL,R0
        RRL,R0
        RRL,R0
        RRL,R0
        RRL,R0
        ADDI,R0 12
        STRZ    R1
        LODI,R2 D_UP
        BSTA,UN APDOOR
        BCTA,UN APDGO
APD14:  COMI,R0 AP_OUT
        BCFR,EQ APD16
        LODA,R0 WTOWN           ; out of town on the plot's side
        BCFR,EQ APD15
        LODI,R0 AP_PLOT
        APPUT   APST
APD15:  APGET   APSIDE
        STRA,R0 T2
        BCTA,UN APDGO
APD16:  COMI,R0 AP_PLOT
        BCFA,EQ APD24
        LODA,R0 WMULE           ; to the plot; installed: home or end
        BCFR,EQ APD18
        LODI,R0 AP_HOME
        LODA,R1 T3
        COMI,R1 3
        BCFR,LT APD17
        LODI,R0 AP_END
APD17:  APPUT   APST
        BCTA,UN APDGO
APD18:  APGET   APTY
        LODI,R1 D_DOWN
        COMA,R0 WY
        BCTR,EQ APD20
        BCTR,GT APD19
        LODI,R1 D_UP
APD19:  STRA,R1 T2
        BCTA,UN APDGO
APD20:  LODA,R0 WX              ; along to it
        ADDI,R0 1
        STRZ    R1
        APGET   APTX
        COMZ    R1
        BCTR,LT APD21
        BCTR,EQ APD21
        LODI,R0 D_RIGHT
        STRA,R0 T2
        BCTA,UN APDGO
APD21:  APGET   APTX
        ADDI,R0 1
        COMA,R0 WX
        BCFR,LT APD22
        LODI,R0 D_LEFT
        STRA,R0 T2
        BCTA,UN APDGO
APD22:  LODI,R0 45              ; there: install
        BSTA,UN APEVERY
        BCFA,EQ APDGO
        BSTA,UN APBTN
        BCTA,UN APDGO
APD24:  COMI,R0 AP_HOME
        BCFR,EQ APD28
        LODA,R0 WTOWN           ; home: the store's row, then into it
        BCTR,EQ APD25
        LODI,R0 AP_PUB
        APPUT   APST
        BCTA,UN APDGO
APD25:  LODI,R1 D_DOWN
        LODA,R0 WY
        COMI,R0 52
        BCTR,EQ APD26
        BCTR,LT APD25A
        LODI,R1 D_UP
APD25A: STRA,R1 T2
        BCTA,UN APDGO
APD26:  LODI,R1 D_RIGHT
        LODA,R0 WX
        COMI,R0 60
        BCTR,LT APD27
        LODI,R1 D_LEFT
APD27:  STRA,R1 T2
        BCTA,UN APDGO
APD28:  COMI,R0 AP_PUB
        BCFR,EQ APD30
        LODA,R0 WTOWN           ; the pub (out on the map: home first)
        BCFR,EQ APD29
        LODI,R0 AP_HOME
        APPUT   APST
        BCTA,UN APDGO
APD29:  LODI,R1 76
        LODI,R2 D_DOWN
        BSTA,UN APDOOR
        BCTA,UN APDGO
APD30:  COMI,R0 AP_END
        BCFR,EQ APDGO
        LODI,R0 K_START         ; end the turn
        STRA,R0 EVKEY
        LODI,R0 AP_DONE
        APPUT   APST
APDGO:  LODA,R0 T2
        STRA,R0 INDIR
        RETC,UN

; APDOOR: T2 = the way to door R1 along the street, then R2 into it.
APDOOR: LODA,R0 WX
        COMZ    R1
        BCTR,EQ APDR2
        BCTR,LT APDR3
        LODI,R0 D_LEFT
        BCTR,UN APDR1
APDR3:  LODI,R0 D_RIGHT
        BCTR,UN APDR1
APDR2:  LODZ    R2
APDR1:  STRA,R0 T2
        RETC,UN

; APPLAN: the turn begins (ap_plan, T3 its time left in 256ths): standing
; a moment (CC GT while it lasts), then our first empty plot and its good.
APPLAN: APGET   APST
        COMI,R0 AP_WAIT
        BCTR,EQ APP1
        LODI,R0 AP_WAIT
        APPUT   APST
        BSTA,UN APNOW
APP1:   BSTA,UN APSINCE
        LODZ    R1
        BCFR,EQ APP2
        COMI,R2 40
        BCFR,LT APP2
        LODI,R0 1               ; (GT: standing)
        RETC,UN
APP2:   LODI,R0 AP_PUB          ; the pub, unless all is there
        APPUT   APST
        EORZ    R0
        APPUT   APIN
        LODI,R3 0               ; our first plot with no M.U.L.E.
APP3:   LODA,R0 MP_OWNER,R3
        COMA,R0 H_MYSEAT
        BCFR,EQ APP4
        COMI,R3 STORE_PLOT
        BCTR,EQ APP4
        LODA,R0 MP_MULE,R3
        COMI,R0 4
        BCFR,LT APP5
APP4:   ADDI,R3 1
        COMI,R3 NUM_PLOTS
        BCFR,EQ APP3
        EORZ    R0              ; none
        RETC,UN
APP5:   STRA,R3 T1              ; T1: it
        LODA,R0 T3              ; time and M.U.L.E.s?
        COMI,R0 4
        BCTA,LT APP9
        LODA,R0 ST_MULES
        BCTA,EQ APP9
        LODA,R3 T1              ; its good
        LODA,R0 MP_TERRAIN,R3
        ANDI,R0 3
        LODI,R1 GOOD_FOOD
        COMI,R0 TERRAIN_RIVER
        BCTR,EQ APP6
        LODI,R1 GOOD_SMITHORE
        COMI,R0 TERRAIN_MOUNTAIN
        BCTR,EQ APP6
        LODI,R1 GOOD_FOOD
        LODA,R0 H_MYSEAT
        ANDI,R0 1
        BCFR,EQ APP6
        LODI,R1 GOOD_ENERGY
APP6:   STRA,R1 T2
        LODA,R0 T2
        APPUT   APGOOD
        LODA,R0 T2              ; money for it: the price + 25 (good + 1)
        ADDI,R0 1
        STRZ    R2
        LODI,R1 25
        ZBSR    *ZMUL8
        LODA,R0 ST_MULEPRICE+1
        ADDZ    R2
        STRZ    R2
        PPSL    PSLWC
        LODA,R0 ST_MULEPRICE
        ADDZ    R1
        CPSL    PSLWC
        STRZ    R1
        LODA,R0 H_MYSEAT
        BSTA,UN APREC
        LODA,R0 H_PLAYERS+PL_MONEY,R3
        COMZ    R1
        BCTA,LT APP9
        BCFR,EQ APP7
        LODA,R0 H_PLAYERS+PL_MONEY+1,R3
        COMZ    R2
        BCTR,LT APP9
APP7:   LODA,R3 T1              ; leave by its side; stand on it
        LODA,R0 PLCOL,R3
        APPUT   APTX
        LODA,R3 T1
        LODA,R0 PLROW,R3
        ADDI,R0 4
        APPUT   APTY
        LODI,R1 D_LEFT
        LODA,R3 T1
        LODA,R0 APCOL9,R3
        COMI,R0 4
        BCFR,GT APP8
        LODI,R1 D_RIGHT
APP8:   LODZ    R1
        APPUT   APSIDE
        LODI,R0 AP_CORRAL
        APPUT   APST
APP9:   EORZ    R0
        RETC,UN
APCOL9: DB      0,1,2,3,4,5,6,7,8, 0,1,2,3,4,5,6,7,8, 0,1,2,3,4,5,6,7,8
        DB      0,1,2,3,4,5,6,7,8, 0,1,2,3,4,5,6,7,8

        INCLUDE "src/plots.inc"
