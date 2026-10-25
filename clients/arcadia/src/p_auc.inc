; p_auc.inc -- the auctions' first stage (mekkogx auction.c, STAGE_SHOW):
; a land auction's plot blinking on the map; a goods auction's status, each
; player's stock as a bar over them, stepping through previous amount,
; usage, spoilage and production to what the store has, then what each
; needs next month. From STAGE_DECLARE on the arena page (p_arena.inc)
; takes the phase over, and gives it back for the next plot or good.
;
; Status: rows 3-12 are block graphics (column 0 starts it): seat i's bar
; in columns 4i+1 and 4i+2, a block row (4 lines) a unit, up to 20; its
; need, a white block in column 4i+3. The players stand under their bars
; (sprites, lines 104-119); row 17 is each one's +/- against its need.

        BCTA,UN AUENTER         ; E_ENTER
        BCTA,UN AUSTATE         ; E_STATE
        BCTA,UN AUTICK          ; E_TICK

        INCLUDE "src/auram.inc"
        ZONE    AUMSIG+1,UNIONHE
        VAR     AUFIG,4         ; status: the species in each player's glyph (255 none)
AUFIGO  EQU     AUFIG-$2900

AUBTOP  EQU     3               ; the bars' rows (all in the screen's first block)
AUBBOT  EQU     12
AUFEET  EQU     104             ; the players' line (rows 13-14)
GDASH   EQU     GUDC+4          ; a minus (the ROM has none)
AUMROW  EQU     22              ; the message
AULROW  EQU     17              ; each player's +/- under it (row 16: WATCH)

; ---- entering: a new phase, or the arena handing back -----------------------------
AUENTER: LODA,R0 H_PHASE
        COMA,R0 LASTPH
        BCTA,EQ AUSTATE         ; (the arena giving it back)
        LODI,R0 255             ; landauc_enter, goods_enter
        LPUT    AUSTG
        EORZ    R0
        LPUT    AUPUSH
        EORZ    R0
        LPUT    AUTRD
        EORZ    R0
        LPUT    ARFLAGS
        LODA,R0 H_PHASE
        COMI,R0 PHASE_LANDAUCTION
        BCFR,EQ AUE1
        EORZ    R0
        LPUT    MAPDOTS
AUE1:   LODA,R0 H_PHASE         ; declaring begins a good afresh
        COMI,R0 PHASE_DECLARE
        BCFR,EQ AUSTATE
        LODI,R0 SCR_NONE
        LPUT    SCRNOW

; ---- a state (auction_state, as far as the first stage goes) -----------------------
AUSTATE: LODA,R0 A_KIND
        LPUT    AUKIND
        LODA,R0 A_GOOD
        ANDI,R0 3
        LPUT    AUGOOD
        LODA,R0 A_STEP
        BCFR,EQ AUS1
        LODI,R0 1
AUS1:   LPUT    AUSTEPV
        LODA,R0 A_STAGE         ; past the first stage: the arena's
        BCTR,EQ AUS2
        LODI,R0 255
        LPUT    MAPBLINK
        LODI,R0 PG_ARENA
        CPUT    PHPAGE
        LODI,R0 1
        CPUT    PGNEW
        RETC,UN
AUS2:   LODA,R0 AUKIND
        COMI,R0 1
        BCFA,EQ AUGOODS
        LODA,R0 SCRNOW          ; land: the plot blinks on the map
        COMI,R0 SCR_MAP
        BCFR,EQ AUS3
        LODA,R0 AUSTG
        BCTR,EQ AUS5
        LODI,R1 3               ; (the grant's cursor off: its map stays)
        ZBSR    *ZSPRHID
        BSTA,UN TITLAND
        BCTR,UN AUS4
AUS3:   BSTA,UN MAPSCR
        BSTA,UN TITLAND
AUS4:   LODI,R0 AUMROW
        LPUT    MSGROW
        LODI,R0 4
        LPUT    MSGN
        LODI,R1 M_BIDSTART
        LODA,R2 A_START
        LODA,R3 A_START+1
        ZBSR    *ZSAYV
AUS5:   EORZ    R0
        LPUT    AUSTG
        LODA,R0 A_PLOT
        LPUT    MAPBLINK
        RETC,UN
TITLAND: LODI,R1 T_LANDAUC
        BCTA,UN TITLE

; ---- a frame ---------------------------------------------------------------------
AUTICK: LODA,R0 AUSTG
        RETC,LT                 ; (255: nothing shown)
        RETC,GT
        LODA,R0 AUKIND
        COMI,R0 1
        BCFA,EQ AUSTICK
        LODA,R0 FRML            ; land: the plot outlined 16 frames, 16 not
        RRR,R0
        RRR,R0
        RRR,R0
        RRR,R0
        ANDI,R0 1
        COMA,R0 MAPBLON
        BCTR,EQ AUT1
        LPUT    MAPBLON
AUT1:   LODA,R0 H_MYSEAT        ; the button skips the wait; the stick is
        COMI,R0 4               ;  sent ahead (the server holds it)
        RETC,GT
        RETC,EQ
        LODA,R0 EVBTN
        BCTR,EQ AUSTICKS
        LODI,R0 ACT_READY
        LODI,R1 0
        LODI,R2 0
        ZBSR    *ZACT
; AUSTICKS: our stick to the server when it changes (au_stick, before the
; lines go: a push only).
AUSTICKS: LODI,R2 0
        LODA,R0 INDIR
        TMI,R0  D_UP
        BCFR,EQ AUK1
        LODI,R2 1
AUK1:   TMI,R0  D_DOWN
        BCFR,EQ AUK2
        LODI,R2 2
AUK2:   LODZ    R2
        COMA,R0 AUPUSH
        RETC,EQ
        LODA,R0 ACT
        RETC,GT
        LODZ    R2
        STRA,R0 T3
        LPUT    AUPUSH
        LODI,R0 ACT_PUSH
        LODI,R1 0
        LODA,R2 T3
        ZBRR    *ZACT

; ---- the goods' status ------------------------------------------------------------
AUGOODS: LODA,R0 SCRNOW
        COMI,R0 SCR_AUC
        BCFR,EQ AUG1
        LODA,R0 AUSTG
        RETC,EQ                 ; (shown: the tick moves it on)
AUG1:   BSTA,UN SCRG26          ; auction_enter, the title, au_status_setup
        LODI,R0 SCR_AUC
        LPUT    SCRNOW
        LODI,R1 T_STATUS
        BSTA,UN AUTITLE
        LODI,R3 AUBTOP*16       ; block graphics in the bars' rows
AUG2:   LODI,R0 GMOSAIC
        STRA,R0 SCR0,R3
        ADDI,R3 16
        COMI,R3 (AUBBOT+1)*16
        BCFR,EQ AUG2
        LODI,R0 3               ; bars down, figures to load
AUG3:   STRA,R0 T3
        ADDI,R0 AUBARO
        STRZ    R3
        EORZ    R0
        LODI,R1 FRRAMP2
        BSTA,UN RAMPUT
        LODA,R0 T3
        ADDI,R0 AUBARTO
        STRZ    R3
        EORZ    R0
        LODI,R1 FRRAMP2
        BSTA,UN RAMPUT
        LODA,R0 T3
        ADDI,R0 AUFIGO
        STRZ    R3
        LODI,R0 255
        LODI,R1 FRRAMP2
        BSTA,UN RAMPUT
        LODA,R1 T3              ; standing at the bottom, in its colour
        BSTA,UN AUREC
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATSPR,R3
        STRZ    R2
        LODA,R1 T3
        ZBSR    *ZSPRCOL
        LODA,R1 T3
        LODI,R2 AUFEET
        LODA,R0 AUCOLX,R1
        STRZ    R3
        ZBSR    *ZSPRSET
        LODA,R0 T3
        SUBI,R0 1
        BCTA,GT AUG3
        BCTA,EQ AUG3
        LODI,R0 MINUS>>8        ; the minus
        LODI,R1 MINUS&$FF
        LODI,R2 4
        LODI,R3 PG_AUC
        ZBSR    *ZUDCQ
        LODI,R0 AUMROW
        LPUT    MSGROW
        LODI,R0 4
        LPUT    MSGN
        LODI,R0 255
        LPUT    AUSHOWT
        EORZ    R0
        LPUT    AUSTG
        RETC,UN

; AUTITLE: title R1 (its '%' the round) on row 0, the good under it.
AUTITLE: STRA,R1 T2
        LODA,R3 AUGOOD          ; the good, centred
        LODA,R0 GOODCOL,R3
        STRZ    R2
        LODI,R1 1
        ZBSR    *ZDPOS
        DCOL    CCYAN
        LODA,R3 AUGOOD
        LODA,R0 GOODNM,R3
        STRZ    R1
        LODI,R0 SFOOD>>8
        ZBSR    *ZDTXT
        DCOL    CWHITE
        EORZ    R0
        LPUT    SAYV1H
        LODA,R0 H_ROUND
        LPUT    SAYV1L
        LODA,R1 T2
        LODI,R2 0
        LODI,R3 1
        ZBRR    *ZSAYBX

; AUREC: R3 = seat R1's record. Keeps R1.
AUREC:  LODA,R0 AUREC4,R1
        STRZ    R3
        RETC,UN
AUREC4: DB      0,29,58,87
AUCOLX: DB      12,44,76,108    ; a player's column (its sprite)

; AUSTICK: the status steps on (au_status_tick): previous amount, usage,
; spoilage, production, then the store's stock and the needs; the bars
; walk a unit every other frame; the figures load as the queue allows.
AUSTICK: BSTA,UN AUFIGS
        BSTA,UN PENOW           ; the step: 0-4 at 127, 250, 373, 496
        LODI,R3 0
AUST1:  LODZ    R1
        COMA,R0 AUSTEPH,R3
        BCTR,LT AUST3
        BCFR,EQ AUST2
        LODZ    R2
        COMA,R0 AUSTEPL,R3
        BCTR,LT AUST3
AUST2:  ADDI,R3 1
        COMI,R3 4
        BCFR,EQ AUST1
AUST3:  LODZ    R3
        COMA,R0 AUSHOWT
        BCTA,EQ AUBARS
        STRA,R0 T3
        LPUT    AUSHOWT
        BSTA,UN AUTGTS
        LODA,R0 AUSHOWT
        COMI,R0 4
        BCTR,EQ AUST4
        ADDI,R0 M_PREVIOUS
        STRZ    R1
        ZBSR    *ZSAY
        BCTA,UN AUBARS
AUST4:  LODA,R3 AUGOOD          ; the store's stock
        LODA,R0 ST_STOCK,R3
        STRZ    R3
        LODI,R2 0
        LODI,R1 M_STOREHAS
        COMI,R3 0
        BCFR,EQ AUST5
        LODI,R1 M_STORENO
AUST5:  ZBSR    *ZSAYV
        BSTA,UN AUNEEDS
        BCTA,UN AUBARS
AUSTEPH: DB     127>>8,250>>8,373>>8,496>>8
AUSTEPL: DB     127&$FF,250&$FF,373&$FF,496&$FF

; AUTGTS: each bar's target for step AUSHOWT (au_status_targets).
AUTGTS: LODI,R0 0
AUTG1:  STRA,R0 T3              ; T3: the seat
        LODA,R1 T3
        BSTA,UN AUREC
        LODZ    R3
        ADDA,R0 AUGOOD
        STRZ    R3
        LODA,R0 H_PLAYERS+PL_GOODS,R3
        STRA,R0 UNIONB          ; w
        LODA,R0 H_PLAYERS+PL_PROD,R3
        STRA,R0 UNIONB+1        ; c
        LODA,R0 H_PLAYERS+PL_SPOIL,R3
        STRA,R0 UNIONB+2        ; j
        LODA,R1 T3
        LODA,R2 H_ROUND
        BSTA,UN AUNEED
        STRA,R0 UNIONB+3        ; n
        LODA,R0 AUSHOWT
        COMI,R0 2
        BCTR,EQ AUTG4
        BCTR,GT AUTG5
        LODA,R0 UNIONB          ; previous: w + j + n - c (0 at least)
        ADDA,R0 UNIONB+2
        ADDA,R0 UNIONB+3
        SUBA,R0 UNIONB+1
        BCTR,LT AUTG2
        LODA,R1 AUSHOWT         ; usage: less n
        BCTR,EQ AUTG6
        SUBA,R0 UNIONB+3
        BCFR,LT AUTG6
AUTG2:  EORZ    R0
        BCTR,UN AUTG6
AUTG4:  LODA,R0 UNIONB          ; spoilage: w - c
        SUBA,R0 UNIONB+1
        BCTR,LT AUTG2
        BCTR,UN AUTG6
AUTG5:  LODA,R0 UNIONB          ; production on: w
AUTG6:  COMI,R0 20
        BCTR,LT AUTG7
        LODI,R0 20
AUTG7:  STRZ    R2
        LODA,R0 T3
        ADDI,R0 AUBARTO
        STRZ    R3
        LODZ    R2
        LODI,R1 FRRAMP2
        BSTA,UN RAMPUT
        LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFA,EQ AUTG1
        RETC,UN

; AUNEED: R0 = the units of AUGOOD seat R1 needs in round R2 (au_need):
; food by the round (none after 12); energy, one for each other M.U.L.E.
AUNEED: LODA,R0 AUGOOD
        BCFR,EQ AUN3
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
AUN3:   COMI,R0 GOOD_ENERGY
        BCTR,EQ AUN4
        EORZ    R0
        RETC,UN
AUN4:   STRA,R1 UNIONB+4        ; energy: count
        EORZ    R0
        STRA,R0 UNIONB+5
        LODI,R3 0
AUN5:   LODA,R0 MP_OWNER,R3
        COMA,R0 UNIONB+4
        BCFR,EQ AUN6
        LODA,R0 MP_MULE,R3
        COMI,R0 4
        BCFR,LT AUN6
        COMI,R0 GOOD_ENERGY
        BCTR,EQ AUN6
        LODA,R0 UNIONB+5
        ADDI,R0 1
        STRA,R0 UNIONB+5
AUN6:   ADDI,R3 1
        COMI,R3 NUM_PLOTS
        BCFR,EQ AUN5
        LODA,R0 UNIONB+5
        RETC,UN

; AUNEEDS: next month's need of each player that has one: a white block
; at that height by its bar, and under it how far short or over.
AUNEEDS: LODI,R0 0
AUND1:  STRA,R0 T3
        LODA,R1 T3
        LODA,R2 H_ROUND
        ADDI,R2 1
        BSTA,UN AUNEED
        COMI,R0 0
        BCTA,EQ AUND9
        STRA,R0 T2              ; T2: the need
        LODA,R1 T3              ; T1: what it has
        BSTA,UN AUREC
        LODZ    R3
        ADDA,R0 AUGOOD
        STRZ    R3
        LODA,R0 H_PLAYERS+PL_GOODS,R3
        STRA,R0 T1
        LODA,R0 T2              ; the mark: unit n's block row
        COMI,R0 20
        BCTR,LT AUND2
        LODI,R0 20
AUND2:  SUBI,R0 1               ; row 13 - (n - 1) / 2, the bottom block
        STRZ    R2              ;  for odd n, the top for even
        RRR,R0
        ANDI,R0 $7F
        STRZ    R1
        LODI,R0 AUBBOT
        SUBZ    R1
        STRZ    R1
        LODI,R0 $20             ; (the left column: bottom $20, top $04)
        TMI,R2  1
        BCFR,EQ AUND3
        LODI,R0 $04
AUND3:  STRA,R0 UNIONB
        LODA,R0 T3              ; column 4i + 3
        RRL,R0
        RRL,R0
        ADDI,R0 2
        STRZ    R2
        ZBSR    *ZDPOS
        LODA,R0 UNIONB
        LODI,R3 1
        ZBSR    *ZDFILL
        LODI,R1 AULROW          ; the label: +over or -short
        LODA,R0 T3
        RRL,R0
        RRL,R0
        SUBI,R0 1
        STRZ    R2
        ZBSR    *ZDPOS
        LODA,R0 T1
        SUBA,R0 T2
        BCTR,LT AUND4
        STRA,R0 T1
        DCOL    CWHITE
        LODI,R0 GPLUS
        BCTR,UN AUND5
AUND4:  LODA,R0 T2
        SUBA,R0 T1
        STRA,R0 T1
        DCOL    CMAGEN
        LODI,R0 GDASH
AUND5:  ZBSR    *ZDPUTC
        LODI,R0 0
        LODA,R1 T1
        LODI,R3 3
        ZBSR    *ZDNUM16
AUND9:  LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFA,EQ AUND1
        RETC,UN

; AUBARS: each bar a unit towards its target, every other frame; the cell
; row that changed drawn.
AUBARS: LODA,R0 FRML
        ANDI,R0 1
        RETC,GT
        LODI,R0 0
AUB1:   STRA,R0 T3
        ADDI,R0 AUBARO
        STRZ    R3
        LODA,R0 *PRAMH,R3
        STRA,R0 T2              ; T2: shown
        LODA,R0 T3
        ADDI,R0 AUBARTO
        STRZ    R3
        LODA,R0 *PRAMH,R3
        COMA,R0 T2
        BCTR,EQ AUB9
        BCTR,LT AUB2
        LODA,R0 T2              ; up: the new unit's row
        ADDI,R0 1
        STRA,R0 T2
        STRA,R0 T1
        BCTR,UN AUB3
AUB2:   LODA,R0 T2              ; down: the old unit's row
        STRA,R0 T1
        SUBI,R0 1
        STRA,R0 T2
AUB3:   LODA,R0 T3
        ADDI,R0 AUBARO
        STRZ    R3
        LODA,R0 T2
        LODI,R1 FRRAMP2
        BSTA,UN RAMPUT
        BSTR,UN AUCELL
AUB9:   LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFA,EQ AUB1
        RETC,UN

; AUCELL: bar T3's cell row holding unit T1 (1-20), as T2 units show it:
; columns 4i+1 (its middle and right blocks) and 4i+2 (its left).
AUCELL: LODA,R0 T1
        SUBI,R0 1
        STRZ    R2
        RRR,R0
        ANDI,R0 $7F
        STRZ    R1              ; k: the cell row from the bottom
        LODI,R0 AUBBOT
        SUBZ    R1
        STRZ    R1              ; R1: the row
        LODZ    R2              ; units 2k+1 (bottom) and 2k+2 (top)
        ANDI,R0 $FE
        STRA,R0 UNIONB          ; 2k
        LODA,R0 T2
        SUBA,R0 UNIONB          ; units in this row: 0, 1, 2+
        BCTR,GT AUC1
        EORZ    R0
AUC1:   COMI,R0 2
        BCTR,LT AUC2
        LODI,R0 2
AUC2:   STRA,R0 UNIONB
        LODA,R0 T3              ; column 4i+1
        RRL,R0
        RRL,R0
        STRZ    R2
        ZBSR    *ZDPOS
        LODA,R1 T3              ; the ink
        BSTA,UN AUREC
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATINK,R3
        STRA,R0 UNIONB+1
        LODA,R3 UNIONB
        LODA,R0 AUCA,R3
        BCTR,EQ AUC3
        IORA,R0 UNIONB+1
AUC3:   LODI,R3 1
        ZBSR    *ZDFILL
        LODA,R3 UNIONB
        LODA,R0 AUCB,R3
        BCTR,EQ AUC4
        IORA,R0 UNIONB+1
AUC4:   LODI,R3 1
        ZBRR    *ZDFILL
AUCA:   DB      $00,$18,$1B     ; middle and right: none, bottom, both
AUCB:   DB      $00,$20,$24     ; left

; AUFIGS: each player's figure into its glyph, as the queue has room.
AUFIGS: LODI,R0 0
AUF1:   STRA,R0 T3
        LODA,R0 UDCQN
        COMI,R0 3
        RETC,EQ
        RETC,GT
        LODA,R1 T3
        BSTA,UN AUREC
        LODA,R0 H_PLAYERS+PL_SPECIES,R3
        ANDI,R0 7
        STRA,R0 T2
        LODA,R0 T3
        ADDI,R0 AUFIGO
        STRZ    R3
        LODA,R0 *PRAMH,R3
        COMA,R0 T2
        BCTR,EQ AUF2
        LODA,R0 T2
        LODI,R1 FRRAMP2
        BSTA,UN RAMPUT
        LODA,R2 T2              ; ART_FIG + (species * 10 + 4) * 8
        LODI,R1 80
        ZBSR    *ZMUL8
        LODZ    R2
        ADDI,R0 (ART_FIG+32)&$FF
        STRZ    R2
        PPSL    PSLWC
        LODZ    R1
        ADDI,R0 (ART_FIG+32)>>8
        CPSL    PSLWC
        STRZ    R3
        LODZ    R2
        STRZ    R1
        LODZ    R3
        LODA,R2 T3
        LODI,R3 PG_ART
        ZBSR    *ZUDCQ
AUF2:   LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFA,EQ AUF1
        RETC,UN

        INCLUDE "src/mapscr.inc"

MINUS:  DB      0,0,0,$7E,0,0,0,0
SEATINK: DB     CBLUE,CMAGEN,CCYAN,CWHITE
SEATSPR: DB     SBLUE|$80,SMAGEN|$80,SCYAN|$80,SWHITE|$80
GOODCOL: DB     5,4,3,3         ; the good's name centred: its column - 1
GOODNM: DB      SFOOD&$FF,SENERGY&$FF,SSMITH&$FF,SCRYST&$FF
SFOOD:  DB      "FOOD",$FF
SENERGY: DB     "ENERGY",$FF
SSMITH: DB      "SMITHORE",$FF
SCRYST: DB      "CRYSTITE",$FF
        IF      (SCRYST>>8) <> (SFOOD>>8)
        ERROR   "the goods' names cross a 256-byte page"
        ENDIF
