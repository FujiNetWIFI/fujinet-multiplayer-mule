; p_sum.inc -- the status summary and the game's end (mekkogx status.c).
;
; Summary: the colonists walk in from the right along the bottom, 30
; frames apart, then climb to their bands in rank order; at 470 frames the
; table: each band its name and score, money, land and goods, and a bar
; of the score against the leader's (block graphics), then the colony's
; total. FIRE says we are ready. Round 0's summary follows the landing:
; the transport ship comes down onto the store first.
;
;   row 0  SUMMARY n
;   band i (rank order) at row 1 + 5i: the colonist at column 0 (two
;   rows), the name at 2 and the score at 10-15; then MONEY, LAND and
;   GOODS a row each, the value at 10-15; the bar under them
;   row 21 COLONY and its total   rows 22-25 the message
;
; Game over: FIRST FOUNDER, the winner standing in the middle, flashing;
; the colony's rating under it at 540 jiffies; FIRE turns to the final
; table and back.

        BCTA,UN SUENTER         ; E_ENTER
        BCTA,UN SUSTATE         ; E_STATE
        BCTA,UN SUTICK          ; E_TICK

        ZONE    UNIONL,UNIONLE
        VAR     SUORD,4         ; the seats in rank order
        VAR     SUSHOW,4        ; each figure's picture in its glyph (255 none)
        VAR     SUSTG,1         ; SU_*
        VAR     SUT0H,1         ; the frame the walk began
        VAR     SUT0L,1
        VAR     SUSIG,1         ; the numbers' signature, as drawn
        VAR     SUVIEW,1        ; game over: 0 the founder, 1 the table
        VAR     SUMAXH,1        ; the leader's score (the bars)
        VAR     SUMAXL,1

SU_LAND EQU     0               ; the ship landing (round 0)
SU_WALK EQU     1
SU_TABLE EQU    2
SU_FOUND EQU    3               ; game over, the founder alone
SU_RATED EQU    4               ; and the rating said

SUBAND  EQU     1               ; band i's first row: 1 + 5i
SUTOPL  EQU     176             ; the walk's line (row 22)

        INCLUDE "src/mapscr.inc"

; ---- entering ---------------------------------------------------------------
SUENTER: LODI,R0 3              ; nothing in the glyphs yet
SUE1:   STRA,R0 T3
        ADDI,R0 SUSHOW&$FF
        STRZ    R3
        LODI,R0 255
        ZBSR    *ZLPUT
        LODA,R0 T3
        SUBI,R0 1
        BCTR,GT SUE1
        BCTR,EQ SUE1
        EORZ    R0
        LPUT    SUVIEW
        BSTA,UN SURANK
        LODA,R0 H_PHASE
        COMI,R0 PHASE_GAMEOVER
        BCTA,EQ GOENTER
        LODA,R0 H_ROUND         ; round 0, early: the ship lands first
        BCFR,EQ SUE2
        BSTA,UN PENOW
        LODZ    R1
        BCFR,EQ SUE2
        COMI,R2 250
        BCFR,LT SUE2
        LODI,R0 SU_LAND
        LPUT    SUSTG
        BSTA,UN MAPSCR
        LODI,R1 T_TRANSPORT
        BSTA,UN TITLE
        LODI,R1 M_LANDING
        ZBSR    *ZSAY
        LODI,R0 FX_LAND
        BCTA,UN FXSTART
SUE2:   LODA,R0 H_ROUND         ; arriving late (past 400): the table at once
        BCTA,EQ SUBEGIN
        BSTA,UN PENOW
        COMI,R1 400>>8
        BCTA,LT SUBEGIN
        BCFR,EQ SUE3
        COMI,R2 400&$FF
        BCTA,LT SUBEGIN
        BCTA,EQ SUBEGIN
SUE3:   BCTA,UN SUTABLE

; SUBEGIN: the walk-in begins (su_begin).
SUBEGIN: BSTA,UN SCRG26
        LODI,R0 SCR_SUM
        LPUT    SCRNOW
        LODI,R0 SU_WALK
        LPUT    SUSTG
        LODA,R0 FRMH
        LPUT    SUT0H
        LODA,R0 FRML
        LPUT    SUT0L
        LODI,R1 3               ; the four in their colours, 16 lines
SUB1:   STRA,R1 T3
        BSTA,UN SUSEAT
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATSPR,R3
        STRZ    R2
        LODA,R1 T3
        ZBSR    *ZSPRCOL
        LODA,R1 T3
        SUBI,R1 1
        BCTR,GT SUB1
        BCTR,EQ SUB1
        RETC,UN

; SURANK: SUORD = the seats by rank (1 first), any without one last.
SURANK: LODI,R0 0
        STRA,R0 T2              ; T2: seats placed (bits); T1: how many
        STRA,R0 T1
        LODI,R0 1
SUR1:   STRA,R0 T0              ; T0: the rank
        LODI,R3 0
SUR2:   LODA,R0 SUREC,R3
        STRZ    R2
        LODA,R0 H_PLAYERS+PL_RANK,R2
        COMA,R0 T0
        BCFR,EQ SUR3
        BSTR,UN SUPUT
SUR3:   ADDI,R3 1
        COMI,R3 4
        BCFR,EQ SUR2
        LODA,R0 T0
        ADDI,R0 1
        COMI,R0 5
        BCFR,EQ SUR1
        LODI,R3 0               ; the rest, in seat order
SUR4:   LODA,R0 T2
        ANDA,R0 SUBIT,R3
        BCFR,EQ SUR5
        BSTR,UN SUPUT
SUR5:   ADDI,R3 1
        COMI,R3 4
        BCFR,EQ SUR4
        RETC,UN
; SUPUT: seat R3 next in SUORD (four at most). Keeps R3.
SUPUT:  LODA,R0 T1
        COMI,R0 4
        RETC,EQ
        LODA,R0 T2
        IORA,R0 SUBIT,R3
        STRA,R0 T2
        STRA,R3 T3
        LODA,R0 T1
        ADDI,R0 SUORD&$FF
        STRZ    R3
        LODA,R0 T3
        ZBSR    *ZLPUT
        LODA,R3 T3
        LODA,R0 T1
        ADDI,R0 1
        STRA,R0 T1
        RETC,UN
SUBIT:  DB      1,2,4,8
SUREC:  DB      0,29,58,87

; SUSEAT: R3 = the record of the seat in rank place R1 (T3 too). Keeps R1.
SUSEAT: LODA,R0 SUORD,R1
        STRZ    R3
        LODA,R0 SUREC,R3
        STRZ    R3
        RETC,UN

; ---- a state --------------------------------------------------------------------
SUSTATE: LODA,R0 SUSTG          ; the table's numbers, when one changed
        COMI,R0 SU_TABLE
        BCTR,EQ SUS1
        COMI,R0 SU_RATED
        RETC,LT
        LODA,R0 SUVIEW
        RETC,EQ
SUS1:   BSTA,UN SUSIGN
        COMA,R0 SUSIG
        RETC,EQ
        BCTA,UN SUVALS

; SUSIGN: R0 = a signature of what the table shows: the players' records,
; the plots' owners and M.U.L.E.s, the prices, the colony's total.
SUSIGN: LODI,R1 0
        LODI,R3 0
SSG1:   LODA,R0 H_PLAYERS,R3
        ADDZ    R1
        RRL,R0
        STRZ    R1
        ADDI,R3 1
        COMI,R3 4*29
        BCFR,EQ SSG1
        LODI,R3 0
SSG2:   LODA,R0 MP_OWNER,R3
        ADDZ    R1
        RRL,R0
        STRZ    R1
        ADDI,R3 1
        COMI,R3 2*NUM_PLOTS
        BCFR,EQ SSG2
        LODI,R3 0
SSG3:   LODA,R0 ST_PRICE,R3
        ADDZ    R1
        RRL,R0
        STRZ    R1
        ADDI,R3 1
        COMI,R3 8
        BCFR,EQ SSG3
        LODA,R0 C_TOTAL+3
        ADDZ    R1
        ADDA,R0 C_TOTAL+2
        ADDA,R0 C_TOTAL+1
        RETC,UN

; ---- a frame --------------------------------------------------------------------
SUTICK: LODA,R0 SUSTG
        COMI,R0 SU_LAND
        BCFR,EQ SUT2
        LODA,R0 FXFLAG          ; landed: the walk
        COMI,R0 1
        BCTA,EQ SUBEGIN
        LODA,R0 FXON
        BCTA,EQ SUBEGIN
        RETC,UN
SUT2:   COMI,R0 SU_WALK
        BCFR,EQ SUT4
        BSTA,UN SUWALK
        LODA,R1 SUT0H           ; 470 frames: the table
        LODA,R2 SUT0L
        ZBSR    *ZSINCE
        COMI,R1 470>>8
        RETC,LT
        COMI,R2 470&$FF
        RETC,LT
        BCTA,UN SUTABLE
SUT4:   COMI,R0 SU_TABLE
        BCFR,EQ SUT6
        LODA,R0 EVBTN           ; ready (again while the server doesn't
        RETC,EQ                 ;  show it: a press it never got mustn't
        LODA,R0 H_MYSEAT        ;  hold the table)
        COMI,R0 4
        RETC,GT
        RETC,EQ
        STRZ    R3
        LODA,R0 SUREC,R3
        STRZ    R3
        LODA,R0 H_PLAYERS+PL_FLAGS,R3
        TMI,R0  PF_READY
        RETC,EQ
        LODI,R0 ACT_READY
        LODI,R1 0
        LODI,R2 0
        ZBSR    *ZACT
        SND     SND_SELECT
        RETC,UN
SUT6:   COMI,R0 SU_FOUND        ; game over
        BCFR,EQ SUT8
        BSTA,UN GOPULSE
        BSTA,UN PENOW           ; 540: the rating
        COMI,R1 540>>8
        RETC,LT
        BCFR,EQ SUT7
        COMI,R2 540&$FF
        RETC,LT
SUT7:   LODI,R0 SU_RATED
        LPUT    SUSTG
        BCTA,UN GORATE
SUT8:   LODA,R0 SUVIEW          ; rated: FIRE turns the page
        BCFR,EQ SUT9
        BSTA,UN GOPULSE
SUT9:   LODA,R0 EVBTN
        RETC,EQ
        SND     SND_SELECT
        LODA,R0 SUVIEW
        EORI,R0 1
        LPUT    SUVIEW
        LODA,R0 SUVIEW
        BCTA,EQ GOVIEW0
        BCTA,UN SUTABLE

; ---- the walk-in -------------------------------------------------------------------
; SUWALK: each figure where the time says (no state but its picture):
; figure i starts 30i frames in, walks left 4 pixels every 6 frames for 34
; steps, then climbs 4 lines every 6 frames to its band.
SUWALK: LODI,R0 0
SUW1:   STRA,R0 T3              ; T3: the rank place
        LODA,R1 SUT0H           ; t = frames - 30i - 1
        LODA,R2 SUT0L
        ZBSR    *ZSINCE
        LODA,R0 T3
        STRZ    R3
        LODZ    R2
        SUBA,R0 SUW30,R3
        STRZ    R2
        PPSL    PSLWC
        LODZ    R1
        SUBI,R0 0
        CPSL    PSLWC
        STRZ    R1
        TMI,R1  $80
        BCTA,EQ SUW9            ; not yet
        LODI,R3 6               ; steps = t / 6
        ZBSR    *ZDIV8
        LODZ    R2
        COMI,R0 34
        BCFR,LT SUW3
        STRA,R0 T2              ; walking: x = 136 - 4 steps, left frames
        RRL,R0
        RRL,R0
        STRA,R0 T0
        LODI,R0 136
        SUBA,R0 T0
        STRA,R0 T0              ; T0: x
        LODI,R0 SUTOPL
        STRA,R0 T1              ; T1: line
        LODA,R0 T2
        ANDI,R0 1
        ADDI,R0 2
        BCTA,UN SUW5
SUW3:   SUBI,R0 34              ; climbing: c = steps - 34, to n = 42 - 10i
        STRA,R0 T2
        LODA,R0 T3
        RRL,R0
        STRA,R0 T1
        RRL,R0
        RRL,R0
        ADDA,R0 T1
        STRA,R0 T1
        LODI,R0 42
        SUBA,R0 T1
        STRA,R0 T1              ; T1: n
        LODA,R0 T2
        COMA,R0 T1
        BCTR,LT SUW4
        LODA,R0 T1              ; there: standing
        STRA,R0 T2
        LODI,R0 4
        BCTR,UN SUW4A
SUW4:   ANDI,R0 1               ; up frames
        ADDI,R0 8
SUW4A:  STRA,R0 T0              ; (the frame, a moment)
        LODA,R0 T2              ; line = 176 - 4c
        RRL,R0
        RRL,R0
        STRA,R0 T1
        LODI,R0 SUTOPL
        SUBA,R0 T1
        STRA,R0 T1
        LODA,R0 T0
        STRA,R0 T2
        EORZ    R0
        STRA,R0 T0              ; x = 0
        LODA,R0 T2
SUW5:   BSTR,UN SUFIG           ; the picture, then the place
        LODA,R0 T0
        COMI,R0 128
        BCTR,LT SUW6
SUW9:   LODA,R1 T3
        ZBSR    *ZSPRHID
        BCTR,UN SUW7
SUW6:   LODA,R1 T3
        LODA,R2 T1
        LODA,R3 T0
        ZBSR    *ZSPRSET
SUW7:   LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFA,EQ SUW1
        RETC,UN
SUW30:  DB      1,31,61,91

; SUFIG: figure T3 (a rank place) shows frame R0 of its species, queued
; when that is not its picture already (and the queue has room).
; Clobbers R0-R3; keeps T0, T1, T3.
SUFIG:  STRA,R0 T2
        LODA,R1 T3
        BSTA,UN SUSEAT
        LODA,R0 H_PLAYERS+PL_SPECIES,R3
        ANDI,R0 7
        STRZ    R1              ; picture = species * 10 + frame
        RRL,R0
        RRL,R0
        ADDZ    R1
        RRL,R0
        ADDA,R0 T2
        STRA,R0 T2
        LODA,R3 T3
        LODA,R0 SUSHOW,R3
        COMA,R0 T2
        RETC,EQ
        LODA,R0 UDCQN
        COMI,R0 3
        RETC,EQ
        RETC,GT
        LODA,R0 T3
        ADDI,R0 SUSHOW&$FF
        STRZ    R3
        LODA,R0 T2
        ZBSR    *ZLPUT
        LODA,R0 T0              ; (UDCQUE takes T0, T1)
        STRA,R0 UNIONB
        LODA,R0 T1
        STRA,R0 UNIONB+1
        LODA,R2 T2              ; ART_FIG + picture * 8
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
        LODA,R2 T3
        LODI,R3 PG_ART
        ZBSR    *ZUDCQ
        LODA,R0 UNIONB
        STRA,R0 T0
        LODA,R0 UNIONB+1
        STRA,R0 T1
        RETC,UN

; ---- the table (su_table) ---------------------------------------------------------
SUTABLE: BSTA,UN SCRG26
        LODI,R0 SCR_SUM
        LPUT    SCRNOW
        LODA,R0 SUSTG           ; (game over keeps its stage)
        COMI,R0 SU_FOUND
        BCFR,LT SUTB1
        LODI,R0 SU_TABLE
        LPUT    SUSTG
SUTB1:  DCOL    CWHITE
        LODI,R1 T_SUMMARY       ; the title
        BSTA,UN TITLE
        DAT     21,1
        DCOL    CMAGEN
        TXTAT   SCOLONY
        LODI,R1 3               ; the colonists stand at their bands
SUTB2:  STRA,R1 T3
        LODA,R0 T3              ; line (1 + 5i) * 8
        RRL,R0
        RRL,R0
        ADDA,R0 T3
        ADDI,R0 SUBAND
        RRL,R0
        RRL,R0
        RRL,R0
        STRA,R0 T1
        EORZ    R0
        STRA,R0 T0
        LODI,R0 4
        BSTA,UN SUFIG
        LODA,R1 T3
        BSTA,UN SUSEAT
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATSPR,R3
        STRZ    R2
        LODA,R1 T3
        ZBSR    *ZSPRCOL
        LODA,R1 T3
        LODA,R2 T1
        LODI,R3 0
        ZBSR    *ZSPRSET
        LODA,R1 T3
        SUBI,R1 1
        BCTA,GT SUTB2
        BCTA,EQ SUTB2
        BSTA,UN SUVALS
        LODA,R0 H_PHASE         ; the summary: everyone presses on
        COMI,R0 PHASE_SUMMARY
        RETC,GT
        LODI,R0 22
        LPUT    MSGROW
        LODI,R0 4
        LPUT    MSGN
        DCOL    CWHITE
        LODI,R1 M_PRESSALL
        ZBSR    *ZSAY
        LODA,R0 H_MYSEAT
        COMI,R0 4
        RETC,GT
        RETC,EQ
        SND     SND_ALERT
        RETC,UN

; SUVALS: the bands' numbers and bars, the colony's total.
SUVALS: BSTA,UN SUSIGN
        LPUT    SUSIG
        BSTA,UN SUMAX
        LODI,R0 0
SUV1:   STRA,R0 T3
        BSTR,UN SUBANDV
        LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFR,EQ SUV1
        DAT     21,9            ; the colony: 7 digits
        DCOL    CMAGEN
        LODA,R0 C_TOTAL+1
        STRA,R0 NUMH
        LODA,R0 C_TOTAL+2
        STRA,R0 NUMM
        LODA,R0 C_TOTAL+3
        STRA,R0 NUML
        LODI,R1 7
        ZBRR    *ZDNUM

; SUBANDV: band T3 (a rank place): name and score, money, land, goods,
; the bar. DNUM takes T0 and T1: the band's own are in UNIONB+4, +5.
; Clobbers R0-R3, T0-T2, NUM*, UNIONB.
SUBANDV: LODA,R0 T3             ; T2: its first row
        RRL,R0
        RRL,R0
        ADDA,R0 T3
        ADDI,R0 SUBAND
        STRA,R0 T2
        LODA,R1 T3
        BSTA,UN SUSEAT
        STRA,R3 SUREC4          ; its record
        LODA,R0 SUORD,R1
        STRA,R0 SUSEAT5         ; its seat
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATINK,R3
        STRA,R0 DCOLOR
        LODA,R1 T2              ; the name
        LODI,R2 1
        ZBSR    *ZDPOS
        LODA,R1 SUSEAT5
        ZBSR    *ZDNAME
        LODA,R3 SUREC4          ; the score: columns 10-15
        LODA,R0 H_PLAYERS+PL_SCORE+1,R3
        STRZ    R1
        LODA,R0 H_PLAYERS+PL_SCORE,R3
        LODI,R3 6
        LODI,R2 9
        BSTA,UN SUNUM16
        LODI,R1 1               ; money, land, goods: a row each, the word
        LODI,R0 CWHITE          ;  at 2, the value at 10-15
        BSTA,UN SUWORD
        LODI,R2 9
        LODA,R3 SUREC4
        LODA,R0 H_PLAYERS+PL_MONEY+1,R3
        STRZ    R1
        LODA,R0 H_PLAYERS+PL_MONEY,R3
        LODI,R3 6
        ZBSR    *ZDNUM16
        LODI,R1 2
        LODI,R0 CCYAN
        BSTA,UN SUWORD
        LODI,R2 9
        BSTA,UN SULAND
        LODI,R3 6
        ZBSR    *ZDNUM16
        LODI,R1 3
        LODI,R0 CWHITE
        BSTA,UN SUWORD
        LODI,R2 9
        BSTA,UN SUGOODS
        LODI,R1 6
        ZBSR    *ZDNUM
        BCTA,UN SUBAR

; SUWORD: row T2 + R1 in ink R0: its word (MONEY, LAND, GOODS by R1) at
; column 2 (then R2 = 9 for its value at 10-15).
SUWORD: STRA,R0 DCOLOR
        LODA,R0 SUWDS-1,R1
        STRA,R0 UNIONB+3
        LODZ    R1
        ADDA,R0 T2
        STRZ    R1
        LODI,R2 1
        ZBSR    *ZDPOS
        LODI,R0 SMONEY>>8
        LODA,R1 UNIONB+3
        ZBRR    *ZDTXT          ; (tail: the caller sets column 10)
SUWDS:  DB      SMONEY&$FF,SLAND&$FF,SGOODS&$FF

SUREC4  EQU     UNIONB+4
SUSEAT5 EQU     UNIONB+5

; SUNUM16: R0:R1 in R3 cells after column R2 of row T2. Calls two down.
SUNUM16: STRA,R0 UNIONB
        STRA,R1 UNIONB+1
        STRA,R3 UNIONB+2
        LODA,R1 T2
        ZBSR    *ZDPOS
        LODA,R0 UNIONB
        LODA,R1 UNIONB+1
        LODA,R3 UNIONB+2
        ZBRR    *ZDNUM16

; SULAND: R0:R1 = the land value of seat SUSEAT5: 500 a plot, and an
; outfitted M.U.L.E.'s cost + 35 (the server's ScoreOf). Keeps R2.
SULAND: STRA,R2 UNIONB+2
        EORZ    R0
        STRA,R0 UNIONB          ; UNIONB:UNIONB+1, the sum
        STRA,R0 UNIONB+1
        LODI,R3 0
SUL1:   LODA,R0 MP_OWNER,R3
        COMA,R0 SUSEAT5
        BCFR,EQ SUL3
        LODI,R1 500>>8          ; a plot
        LODI,R2 500&$FF
        LODA,R0 MP_MULE,R3
        COMI,R0 4
        BCFR,LT SUL2
        STRZ    R2              ; and its M.U.L.E.
        LODA,R0 SULMV,R2
        ADDI,R0 500&$FF
        STRZ    R2
        PPSL    PSLWC
        LODI,R0 500>>8
        ADDI,R0 0
        CPSL    PSLWC
        STRZ    R1
SUL2:   LODA,R0 UNIONB+1
        ADDZ    R2
        STRA,R0 UNIONB+1
        PPSL    PSLWC
        LODA,R0 UNIONB
        ADDZ    R1
        STRA,R0 UNIONB
        CPSL    PSLWC
SUL3:   ADDI,R3 1
        COMI,R3 NUM_PLOTS
        BCFR,EQ SUL1
        LODA,R2 UNIONB+2
        LODA,R0 UNIONB+1
        STRZ    R1
        LODA,R0 UNIONB
        RETC,UN
SULMV:  DB      60,85,110,135   ; (g + 1) * 25 + 35

; SUGOODS: NUM = record SUREC4's goods at the store's prices. Keeps R2.
SUGOODS: STRA,R2 UNIONB+2
        EORZ    R0
        STRA,R0 NUMH
        STRA,R0 NUMM
        STRA,R0 NUML
        LODI,R0 0
SUG1:   STRA,R0 UNIONB+3        ; the good
        ADDA,R0 SUREC4
        STRZ    R3
        LODA,R0 H_PLAYERS+PL_GOODS,R3
        STRA,R0 UNIONB          ; its units
        LODA,R0 UNIONB+3        ; x the price's low byte
        RRL,R0
        STRZ    R3
        LODA,R0 ST_PRICE+1,R3
        STRZ    R2
        LODA,R1 UNIONB
        ZBSR    *ZMUL8
        LODA,R0 NUML
        ADDZ    R2
        STRA,R0 NUML
        PPSL    PSLWC
        LODA,R0 NUMM
        ADDZ    R1
        STRA,R0 NUMM
        LODA,R0 NUMH
        ADDI,R0 0
        STRA,R0 NUMH
        CPSL    PSLWC
        LODA,R0 UNIONB+3        ; x the high byte, a byte up
        RRL,R0
        STRZ    R3
        LODA,R0 ST_PRICE,R3
        STRZ    R2
        LODA,R1 UNIONB
        ZBSR    *ZMUL8
        LODA,R0 NUMM
        ADDZ    R2
        STRA,R0 NUMM
        PPSL    PSLWC
        LODA,R0 NUMH
        ADDZ    R1
        STRA,R0 NUMH
        CPSL    PSLWC
        LODA,R0 UNIONB+3
        ADDI,R0 1
        COMI,R0 4
        BCFA,EQ SUG1
        LODA,R2 UNIONB+2
        RETC,UN

; SUMAX: SUMAXH:SUMAXL = the leader's score (1 at least).
SUMAX:  EORZ    R0
        STRA,R0 UNIONB
        LODI,R0 1
        STRA,R0 UNIONB+1
        LODI,R3 0
SUM1:   LODA,R0 H_PLAYERS+PL_SCORE,R3
        COMA,R0 UNIONB
        BCTR,LT SUM3
        BCFR,EQ SUM2
        LODA,R0 H_PLAYERS+PL_SCORE+1,R3
        COMA,R0 UNIONB+1
        BCTR,LT SUM3
SUM2:   LODA,R0 H_PLAYERS+PL_SCORE,R3
        STRA,R0 UNIONB
        LODA,R0 H_PLAYERS+PL_SCORE+1,R3
        STRA,R0 UNIONB+1
SUM3:   ADDI,R3 29
        COMI,R3 4*29
        BCFR,EQ SUM1
        LODA,R0 UNIONB
        LPUT    SUMAXH
        LODA,R0 UNIONB+1
        LPUT    SUMAXL
        RETC,UN

; SUBAR: band T3's bar on row T2 + 2: block graphics from column 1, 42
; blocks (14 cells of 3) at the leader's score, in the seat's ink.
SUBAR:  LODA,R3 SUREC4          ; the score and the leader's, both halved
        LODA,R0 H_PLAYERS+PL_SCORE,R3   ;  until the leader's fits a byte
        STRA,R0 UNIONB
        LODA,R0 H_PLAYERS+PL_SCORE+1,R3
        STRA,R0 UNIONB+1
        LODA,R0 SUMAXH
        STRA,R0 UNIONB+2
        LODA,R0 SUMAXL
        STRA,R0 UNIONB+3
SBR1:   LODA,R0 UNIONB+2
        BCTR,EQ SBR2
        CPSL    PSLC            ; (each pair a 16-bit shift right, through C)
        PPSL    PSLWC
        RRR,R0
        STRA,R0 UNIONB+2
        LODA,R0 UNIONB+3
        RRR,R0
        STRA,R0 UNIONB+3
        CPSL    PSLC
        LODA,R0 UNIONB
        RRR,R0
        STRA,R0 UNIONB
        LODA,R0 UNIONB+1
        RRR,R0
        STRA,R0 UNIONB+1
        CPSL    PSLWC
        BCTR,UN SBR1
SBR2:   LODA,R0 UNIONB+3        ; (0 only if the leader has 0: then no bar)
        BCFR,EQ SBR3
        LODI,R0 1
        STRA,R0 UNIONB+3
SBR3:   LODA,R1 UNIONB+1        ; blocks = score * 42 / leader
        LODI,R2 42
        ZBSR    *ZMUL8
        LODA,R3 UNIONB+3
        ZBSR    *ZDIV8
        LODZ    R2
        STRA,R0 UNIONB          ; UNIONB: blocks left
        LODA,R0 SUREC4          ; UNIONB+1: the ink
        STRZ    R3
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATINK,R3
        STRA,R0 UNIONB+1
        LODA,R1 T2
        ADDI,R1 4
        LODI,R2 0
        ZBSR    *ZDPOS
        LODI,R0 GMOSAIC         ; column 1 starts block graphics
        LODI,R3 1
        ZBSR    *ZDFILL
        LODI,R0 14
        STRA,R0 UNIONB+2
SBR4:   LODA,R0 UNIONB          ; this cell: 0-3 columns of blocks
        COMI,R0 3
        BCTR,LT SBR5
        LODI,R0 3
SBR5:   STRZ    R3
        LODA,R0 UNIONB
        SUBZ    R3
        STRA,R0 UNIONB
        LODA,R0 SBRCELL,R3
        BCTR,EQ SBR6
        IORA,R0 UNIONB+1
SBR6:   LODI,R3 1
        ZBSR    *ZDFILL
        LODA,R0 UNIONB+2
        SUBI,R0 1
        STRA,R0 UNIONB+2
        BCFR,EQ SBR4
        RETC,UN
SBRCELL: DB     $00,$24,$36,$3F ; left column, two, all three (both rows)

; ---- game over (gameover_*) -------------------------------------------------------
GOENTER: LODI,R0 SU_FOUND
        LPUT    SUSTG
        SND     SND_FOUND
; GOVIEW0: FIRST FOUNDER: the winner in the middle, its name under it, the
; colony's rating under that once it is said.
GOVIEW0: BSTA,UN SCRG26
        LODI,R0 SCR_FOUND
        LPUT    SCRNOW
        DAT     2,2
        DCOL    CMAGEN
        TXTAT   SFIRST
        LODI,R0 60              ; the winner, standing
        STRA,R0 T0
        LODI,R0 40
        STRA,R0 T1
        LODI,R0 0
        STRA,R0 T3
        LODI,R0 4
        BSTA,UN SUFIG
        LODI,R1 0
        LODI,R2 40
        LODI,R3 60
        ZBSR    *ZSPRSET
        BSTA,UN GOPULSE
        LODI,R1 0               ; its name
        BSTA,UN SUSEAT
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATINK,R3
        STRA,R0 DCOLOR
        DAT     8,4
        LODA,R1 SUORD
        ZBSR    *ZDNAME
        LODA,R0 SUSTG
        COMI,R0 SU_RATED
        RETC,LT
; GORATE: "OVERALL, ..." under it.
GORATE: LODA,R0 SUVIEW
        RETC,GT
        LODA,R0 C_RATING
        COMI,R0 7
        RETC,GT
        RETC,EQ
        LODI,R0 11
        LPUT    MSGROW
        LODI,R0 15
        LPUT    MSGN
        DCOL    CWHITE
        LODA,R0 C_RATING
        ADDI,R0 R_0
        STRZ    R1
        ZBRR    *ZSAY

; GOPULSE: the winner flashes, white and its colour, 32 frames each.
GOPULSE: LODI,R1 0               ; (rank place 0: sprite 0)
        BSTA,UN SUSEAT
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATSPR,R3
        STRZ    R2
        LODA,R0 FRML
        ANDI,R0 32
        BCTR,EQ GOP1
        LODI,R2 SWHITE|$80
GOP1:   LODI,R1 0
        ZBRR    *ZSPRCOL

SEATINK: DB     CBLUE,CMAGEN,CCYAN,CWHITE
SEATSPR: DB     SBLUE|$80,SMAGEN|$80,SCYAN|$80,SWHITE|$80
SMONEY: DB      "MONEY",$FF       ; (one 256-byte page with SGOODS)
SLAND:  DB      "LAND",$FF
SGOODS: DB      "GOODS",$FF
SCOLONY: DB     "COLONY",$FF
        IF      (SMONEY>>8) <> (SGOODS>>8)
        ERROR   "the summary's words cross a 256-byte page"
        ENDIF
SFIRST: DB      "FIRST FOUNDER",$FF
