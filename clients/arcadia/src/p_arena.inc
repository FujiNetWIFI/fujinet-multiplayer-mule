; p_arena.inc -- the auctions' arena (mekkogx auction.c from STAGE_DECLARE
; on): declaring, the lead-in, the trading, the end; land and goods alike.
; The status page (p_auc.inc) hands it over, and takes it back for the
; next plot or good.
;
;   row 0   the title        row 1  the store pays .. the good .. it charges
;   row 2   land: the price line (bids start at, high bid, sale price)
;   lines 20-128 the arena: the base price at line 102, a step 2 lines;
;           sellers start at line 32, buyers at 128. Each player (a
;           sprite) stands with its feet on its price, in its column.
;           The sellers' and the buyers' price lines are a row of a user
;           glyph whose 8 bytes hold a dotted line at the line's pixel row
;           ($3C, $3D; $3E when both fall in one row).
;   row 17  each player's BUY / SELL, or its price (4 cells each)
;   row 18  money ("15K" past 9999)    row 19  units (goods)
;   rows 20-23 the message           row 25  the clock (block graphics)

        BCTA,UN ARSTATE         ; E_ENTER (handed over: the state as it is)
        BCTA,UN ARSTATE         ; E_STATE
        BCTA,UN ARTICK          ; E_TICK

        INCLUDE "src/auram.inc"
        INCLUDE "src/arena.inc"

; ---- a state (auction_state) -------------------------------------------------------
ARSTATE: LODA,R0 A_STAGE
        BCFR,EQ ARS1
        EORZ    R0              ; the first stage again: the status page's
        LPUT    AUON
        LODI,R0 PG_AUC
        CPUT    PHPAGE
        LODI,R0 1
        CPUT    PGNEW
        RETC,UN
ARS1:   LODA,R0 A_KIND
        LPUT    AUKIND
        LODA,R0 A_GOOD
        ANDI,R0 3
        LPUT    AUGOOD
        LODA,R0 A_STEP
        BCFR,EQ ARS2
        LODI,R0 1
ARS2:   LPUT    AUSTEPV
        LODI,R3 0               ; the roles
ARS3:   STRA,R3 T3
        LODA,R0 A_ROLE,R3
        ADDI,R3 AUROLE&$FF
        ZBSR    *ZLPUT
        LODA,R3 T3
        ADDI,R3 1
        COMI,R3 4
        BCFR,EQ ARS3
        LODA,R0 SCRNOW          ; a new screen, a new stage: the frame
        COMI,R0 SCR_AUC         ;  draws them (p_arc.inc)
        BCTR,EQ ARS4
        LODI,R0 SCR_AUC
        LPUT    SCRNOW
        LODI,R0 255
        LPUT    AUSTG
        EORZ    R0
        LPUT    AUON
        LODI,R0 ARF_SCR
        LPUT    ARFLAGS
ARS4:   LODA,R0 A_STAGE
        COMA,R0 AUSTG
        BCTR,EQ ARS5
        LPUT    AUSTG
        LODA,R0 ARFLAGS
        IORI,R0 ARF_BEGIN
        LPUT    ARFLAGS
; the base price: land, the start (it scrolls up with the bids, so only a
; new plot resets it); goods, what the store pays
ARS5:   LODA,R0 AUKIND
        COMI,R0 1
        BCFR,EQ ARS6
        LODA,R0 AUSTG
        COMI,R0 STAGE_ACTIVE
        BCTR,LT ARS7
        LODA,R0 AUBASEH
        COMA,R0 A_START
        BCTR,LT ARS7
        BCFR,EQ ARS8
        LODA,R0 AUBASEL
        COMA,R0 A_START+1
        BCFR,LT ARS8
ARS7:   LODA,R0 A_START
        LPUT    AUBASEH
        LODA,R0 A_START+1
        LPUT    AUBASEL
        BCTR,UN ARS8
ARS6:   LODA,R0 A_STOREBUY
        LPUT    AUBASEH
        LODA,R0 A_STOREBUY+1
        LPUT    AUBASEL
; the lines: snapped to the state, then walked on by the steps the server
; took since (it steps every 8th jiffy of its clock; the state is rx_age
; old); our own line, let go a moment ago, stays rather than jump back
ARS8:   LODI,R0 255
        STRA,R0 ARK
        LODA,R0 AUSTG
        COMI,R0 STAGE_ACTIVE
        BCFR,EQ ARS9
        LODA,R0 AUPUSH
        BCFR,EQ ARS9
        LODA,R0 FRML
        SUBA,R0 AUHELD
        COMI,R0 60
        BCFR,LT ARS9
        LODA,R0 H_MYSEAT
        STRA,R0 ARK
ARS9:   LODA,R0 H_T+1
        ANDI,R0 7
        ADDA,R0 RXAGE
        STRA,R0 ARJ
        ANDI,R0 7
        LPUT    AUTKS
        LODA,R0 AUSTG
        COMI,R0 STAGE_ACTIVE
        BCTR,EQ ARS10
        EORZ    R0
        STRA,R0 ARJ
        BCTR,UN ARS11
ARS10:  LODA,R0 ARJ
        RRR,R0
        RRR,R0
        RRR,R0
        ANDI,R0 $1F
        STRA,R0 ARJ
ARS11:  LODI,R0 255             ; a seller at critical level is held
        STRA,R0 ARC
        LODA,R0 AUKIND
        COMI,R0 2
        BCFR,EQ ARS12
        LODA,R0 A_FLAGS
        TMI,R0  AF_CRITICAL
        BCFR,EQ ARS12
        LODA,R0 A_SELLER
        STRA,R0 ARC
ARS12:  LODI,R0 0
ARS13:  STRA,R0 T3
        COMA,R0 ARK
        BCFR,EQ ARS14
        STRZ    R3              ; ours, still pushed or still walking: kept
        LODA,R0 A_PUSH,R3
        BCFA,EQ ARS17
        LODZ    R3
        RRL,R0
        STRZ    R3
        LODA,R0 A_POS,R3
        COMA,R0 A_TARGET,R3
        BCFA,EQ ARS17
        LODA,R0 A_POS+1,R3
        COMA,R0 A_TARGET+1,R3
        BCFA,EQ ARS17
ARS14:  LODA,R0 T3              ; pos = the state's
        RRL,R0
        STRZ    R3
        LODA,R0 A_POS,R3
        STRA,R0 ARWH
        LODA,R0 A_POS+1,R3
        STRA,R0 ARWL
        LODI,R0 AUPOS&$FF
        BSTA,UN PUTW
        LODA,R0 T3              ; target: the state's (held: its pos)
        COMA,R0 ARC
        BCTR,EQ ARS15
        RRL,R0
        STRZ    R3
        LODA,R0 A_TARGET,R3
        STRA,R0 ARWH
        LODA,R0 A_TARGET+1,R3
        STRA,R0 ARWL
ARS15:  LODI,R0 AUTGT&$FF
        BSTA,UN PUTW
        LODA,R0 ARJ             ; the steps missed
        STRA,R0 T1
ARS16:  LODA,R0 T1
        BCTR,EQ ARS17
        SUBI,R0 1
        STRA,R0 T1
        BSTA,UN AUSTEP
        BCTR,UN ARS16
ARS17:  LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFA,EQ ARS13
        LODA,R0 AUSTG
        COMI,R0 STAGE_LEAD
        BCTR,LT ARS18
        LODI,R0 1
        LPUT    AUON
ARS18:  LODA,R0 ARFLAGS         ; the frame draws it all
        IORI,R0 ARF_DIRTY
        LPUT    ARFLAGS
        LODA,R0 A_TRADED        ; a trade: its sound
        COMA,R0 AUTRD
        RETC,EQ
        BCTR,LT ARS19
        SND     SND_TRADE
ARS19:  LODA,R0 A_TRADED
        LPUT    AUTRD
        RETC,UN
; PUTW: the word ARWH:ARWL into the RAM-L pair at (R0 + 2 * T3). Clobbers
; R0-R3, T0.
PUTW:   STRA,R0 T0
        LODA,R0 T3
        RRL,R0
        ADDA,R0 T0
        STRA,R0 T0
        STRZ    R3
        LODA,R0 ARWH
        ZBSR    *ZLPUT
        LODA,R3 T0
        ADDI,R3 1
        LODA,R0 ARWL
        ZBRR    *ZLPUT

; AUSTEP: seat T3's line a step towards its target; land: a bid past 30
; steps over the base scrolls the scale up. Clobbers R0-R3, T0, ARW*.
AUSTEP: LODA,R0 T3
        RRL,R0
        STRZ    R3
        LODA,R0 AUPOS,R3
        STRA,R0 ARWH
        LODA,R0 AUPOS+1,R3
        STRA,R0 ARWL
        LODA,R0 AUPOS,R3        ; p against t
        COMA,R0 AUTGT,R3
        BCTA,LT AST1
        BCFR,EQ AST3
        LODA,R0 AUPOS+1,R3
        COMA,R0 AUTGT+1,R3
        RETC,EQ
        BCTR,LT AST1
AST3:   LODA,R0 ARWL            ; over: down a step, not past it
        SUBA,R0 AUSTEPV
        STRA,R0 ARWL
        PPSL    PSLWC
        LODA,R0 ARWH
        SUBI,R0 0
        STRA,R0 ARWH
        CPSL    PSLWC
        LODA,R0 ARWH
        COMA,R0 AUTGT,R3
        BCTR,LT AST4
        BCFR,EQ AST5
        LODA,R0 ARWL
        COMA,R0 AUTGT+1,R3
        BCFR,LT AST5
AST4:   LODA,R0 AUTGT,R3
        STRA,R0 ARWH
        LODA,R0 AUTGT+1,R3
        STRA,R0 ARWL
        BCTR,UN AST5
AST1:   LODA,R0 ARWL            ; under: up a step, not past it
        ADDA,R0 AUSTEPV
        STRA,R0 ARWL
        PPSL    PSLWC
        LODA,R0 ARWH
        ADDI,R0 0
        STRA,R0 ARWH
        CPSL    PSLWC
        LODA,R0 ARWH
        COMA,R0 AUTGT,R3
        BCTR,GT AST4
        BCFR,EQ AST5
        LODA,R0 ARWL
        COMA,R0 AUTGT+1,R3
        BCTR,GT AST4
AST5:   LODI,R0 AUPOS&$FF
        BSTA,UN PUTW
        LODA,R0 AUKIND          ; land, a buyer: the scale's top
        COMI,R0 1
        RETC,GT
        RETC,LT
        LODA,R3 T3
        LODA,R0 AUROLE,R3
        COMI,R0 ROLE_BUYER
        RETC,GT
        RETC,LT
        LODA,R1 AUSTEPV         ; lim = 30 steps
        LODI,R2 30
        ZBSR    *ZMUL8
        STRA,R2 ARN             ; (under 256)
        LODA,R0 AUBASEL         ; R1:R2 = base + lim
        ADDZ    R2
        STRZ    R2
        PPSL    PSLWC
        LODA,R0 AUBASEH
        ADDI,R0 0
        CPSL    PSLWC
        STRZ    R1
        LODA,R0 ARWH            ; p over it?
        COMZ    R1
        BCTR,GT AST6
        RETC,LT
        LODA,R0 ARWL
        COMZ    R2
        RETC,LT
        RETC,EQ
AST6:   LODA,R0 ARWL            ; base = p - lim
        SUBA,R0 ARN
        STRA,R0 ARN             ; (LPUT takes R1, R2)
        PPSL    PSLWC
        LODA,R0 ARWH
        SUBI,R0 0
        CPSL    PSLWC
        LPUT    AUBASEH
        LODA,R0 ARN
        LPUT    AUBASEL
        RETC,UN

; PRICEY: R0 = the line of price ARWH:ARWL (price_y): 2 lines a step over
; the base (41 at most) or under it (13).
PRICEY: LODA,R0 ARWH
        COMA,R0 AUBASEH
        BCTR,LT PY5
        BCFR,EQ PY1
        LODA,R0 ARWL
        COMA,R0 AUBASEL
        BCTR,LT PY5
PY1:    LODA,R0 ARWL            ; over
        SUBA,R0 AUBASEL
        STRZ    R2
        PPSL    PSLWC
        LODA,R0 ARWH
        SUBA,R0 AUBASEH
        CPSL    PSLWC
        STRZ    R1
        LODI,R3 AUSUP
        BSTR,UN PYSC
        RRL,R0
        STRZ    R1
        LODI,R0 AUYBASE
        SUBZ    R1
        RETC,UN
PY5:    LODA,R0 AUBASEL         ; under
        SUBA,R0 ARWL
        STRZ    R2
        PPSL    PSLWC
        LODA,R0 AUBASEH
        SUBA,R0 ARWH
        CPSL    PSLWC
        STRZ    R1
        LODI,R3 AUSDN
        BSTR,UN PYSC
        RRL,R0
        ADDI,R0 AUYBASE
        RETC,UN
; PYSC: R0 = steps in R1:R2 dollars (a quarter at 4 a step), R3 at most.
PYSC:   LODA,R0 AUSTEPV
        COMI,R0 4
        BCFR,EQ PYS2
        LODI,R0 2
        STRA,R0 ARN
PYS1:   CPSL    PSLC            ; R1:R2 >>= 1
        PPSL    PSLWC
        LODZ    R1
        RRR,R0
        STRZ    R1
        LODZ    R2
        RRR,R0
        STRZ    R2
        CPSL    PSLWC
        LODA,R0 ARN
        SUBI,R0 1
        STRA,R0 ARN
        BCFR,EQ PYS1
PYS2:   LODZ    R1
        BCFR,EQ PYS3
        LODZ    R2
        COMZ    R3
        RETC,LT
        RETC,EQ
PYS3:   LODZ    R3
        RETC,UN

; ARREC: R3 = seat R1's record. Keeps R1.
ARREC:  LODA,R0 ARREC4,R1
        STRZ    R3
        RETC,UN
ARREC4: DB      0,29,58,87
ARCOLX: DB      12,44,76,108    ; a player's column (its sprite)

; ---- the players (au_draw) ---------------------------------------------------------
; ARDRAW: each player at its price (before the lead-in, at its starting
; line); blinking while dealing (two steps on, two off); a walking frame
; every other step on the way.
ARDRAW: LODI,R0 4               ; (from seat AUPH & 3, in turn: four new
        STRA,R0 ARDN            ;  pictures a step and room for three must
        LODA,R0 AUPH            ;  not always leave the same seat out)
        ANDI,R0 3
        STRA,R0 T3
ARD1:   LODA,R3 T3
        LODA,R0 AUROLE,R3
        BCTA,EQ ARD8            ; none: off
        STRA,R0 T2
        LODA,R0 AUSTG           ; T1: its line
        COMI,R0 STAGE_LEAD
        BCFR,LT ARD2
        LODI,R0 AUYBOT
        LODA,R1 T2
        COMI,R1 ROLE_SELLER
        BCFR,EQ ARD3
        LODI,R0 AUYSELL
        BCTR,UN ARD3
ARD2:   LODA,R0 T3
        RRL,R0
        STRZ    R3
        LODA,R0 AUPOS,R3
        STRA,R0 ARWH
        LODA,R0 AUPOS+1,R3
        STRA,R0 ARWL
        BSTA,UN PRICEY
ARD3:   STRA,R0 T1
        LODA,R3 T3              ; blinking: off on steps 2 and 3 of 4
        LODA,R0 ARBIT,R3
        ANDA,R0 AUBLINK
        BCTR,EQ ARD4
        LODA,R0 AUPH
        ANDI,R0 2
        BCTA,GT ARD8
ARD4:   LODI,R0 4               ; standing, or walking on odd steps
        STRA,R0 T2
        LODA,R0 AUPH
        ANDI,R0 1
        BCTR,EQ ARD5
        LODA,R0 T3
        RRL,R0
        STRZ    R3
        LODA,R0 AUPOS,R3
        COMA,R0 AUTGT,R3
        BCFR,EQ ARD4A
        LODA,R0 AUPOS+1,R3
        COMA,R0 AUTGT+1,R3
        BCTR,EQ ARD5
ARD4A:  LODI,R0 5
        STRA,R0 T2
ARD5:   BSTA,UN ARFIGQ
        LODA,R1 T3              ; feet on the line
        LODA,R0 T1
        SUBI,R0 16
        STRZ    R2
        LODA,R0 ARCOLX,R1
        STRZ    R3
        ZBSR    *ZSPRSET
        BCTR,UN ARD9
ARD8:   LODA,R1 T3
        ZBSR    *ZSPRHID
ARD9:   LODA,R0 T3
        ADDI,R0 1
        ANDI,R0 3
        STRA,R0 T3
        LODA,R0 ARDN
        SUBI,R0 1
        STRA,R0 ARDN
        BCFA,EQ ARD1
        RETC,UN
ARBIT:  DB      1,2,4,8

; ARFIGQ: seat T3's figure, frame T2 of its species, into its glyph when
; not there already and the queue has room. Keeps T1, T3.
ARFIGQ: LODA,R1 T3
        BSTA,UN ARREC
        LODA,R0 H_PLAYERS+PL_SPECIES,R3
        ANDI,R0 7
        STRZ    R1              ; species * 10 + frame
        RRL,R0
        RRL,R0
        ADDZ    R1
        RRL,R0
        ADDA,R0 T2
        STRA,R0 T2
        LODA,R0 T3
        ADDI,R0 ARFIGO
        STRZ    R3
        LODA,R0 *PRAMH,R3
        COMA,R0 T2
        RETC,EQ
        LODA,R0 UDCQN
        COMI,R0 3
        RETC,EQ
        RETC,GT
        LODA,R0 T2
        LODI,R1 FRRAMP2
        BSTA,UN RAMPUT
        LODA,R0 T1              ; (MUL8 takes T0, UDCQUE T0 and T1)
        STRA,R0 ARN
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
        LODA,R0 ARN
        STRA,R0 T1
        RETC,UN

; ---- the price lines (au_lines) -------------------------------------------------------
; ARLINES: the sellers' line (the lowest ask, or the store's price while it
; has stock) and the buyers' (the highest bid, or what the store pays /
; the land scale's bottom), each a row of its glyph.
ARLINES: LODA,R0 AUKIND         ; land: none before the lead-in
        COMI,R0 1
        BCFR,EQ ARL1
        LODA,R0 AUSTG
        COMI,R0 STAGE_LEAD
        RETC,LT
ARL1:   LODA,R0 UDCQN           ; (two glyphs may change: room first)
        COMI,R0 1
        RETC,GT
        LODI,R0 $FF             ; sellers: from none, or the store's
        STRA,R0 ARWH
        STRA,R0 ARWL
        LODA,R0 AUKIND
        COMI,R0 2
        BCFR,EQ ARL2
        LODA,R3 AUGOOD
        LODA,R0 ST_STOCK,R3
        BCTR,EQ ARL2
        LODA,R0 A_STORESELL
        STRA,R0 ARWH
        LODA,R0 A_STORESELL+1
        STRA,R0 ARWL
ARL2:   LODI,R1 ROLE_SELLER
        BSTA,UN ARBEST
        LODI,R0 255
        STRA,R0 ARL1V           ; ARL1V: its line
        LODA,R0 ARWH
        COMI,R0 $FF
        BCFR,EQ ARL3
        LODA,R0 ARWL
        COMI,R0 $FF
        BCTR,EQ ARL4
ARL3:   BSTA,UN PRICEY
        STRA,R0 ARL1V
ARL4:   LODA,R0 A_STOREBUY      ; buyers: from the store's, or the base
        STRA,R0 ARWH
        LODA,R0 A_STOREBUY+1
        STRA,R0 ARWL
        LODA,R0 AUKIND
        COMI,R0 2
        BCTR,EQ ARL5
        LODA,R0 AUBASEH
        STRA,R0 ARWH
        LODA,R0 AUBASEL
        STRA,R0 ARWL
ARL5:   LODI,R1 ROLE_BUYER
        BSTA,UN ARBEST
        BSTA,UN PRICEY
        STRA,R0 ARL2V           ; ARL2V: its line
        LODA,R0 ARL1V              ; never over the labels
        COMI,R0 126
        BCFR,GT ARL6
        LODI,R0 255
        STRA,R0 ARL1V
ARL6:   LODA,R0 ARL2V
        COMI,R0 126
        BCFR,GT ARL7
        LODI,R0 255
        STRA,R0 ARL2V
ARL7:   LODA,R0 ARL1V
        COMA,R0 AULN1
        BCFR,EQ ARL8
        LODA,R0 ARL2V
        COMA,R0 AULN2
        RETC,EQ
ARL8:   LODA,R0 AULN1           ; the old rows off (unless a new one is there)
        BSTA,UN ARLOFF
        LODA,R0 AULN2
        BSTA,UN ARLOFF
        LODA,R0 ARL1V
        LPUT    AULN1
        LODA,R0 ARL2V
        LPUT    AULN2
        LODA,R0 ARL1V              ; the new ones: one row, or two
        COMI,R0 255
        BCTA,EQ ARL10
        LODA,R0 ARL2V
        COMI,R0 255
        BCTR,EQ ARL9
        LODA,R0 ARL1V
        EORA,R0 ARL2V
        ANDI,R0 $F8
        BCFR,EQ ARL9
        LODA,R0 ARL1V              ; both in one row: $3E with both
        BSTA,UN ARLBIT
        STRA,R0 ARN
        LODA,R0 ARL2V
        BSTA,UN ARLBIT
        IORA,R0 ARN
        LODI,R2 6
        BSTA,UN ARLPAT
        LODA,R0 ARL1V
        LODI,R1 GLN2
        BCTA,UN ARLFILL
ARL9:   LODA,R0 ARL1V              ; the sellers'
        BSTA,UN ARLBIT
        LODI,R2 4
        BSTA,UN ARLPAT
        LODA,R0 ARL1V
        LODI,R1 GLNS
        BSTA,UN ARLFILL
ARL10:  LODA,R0 ARL2V              ; the buyers'
        COMI,R0 255
        RETC,EQ
        BSTR,UN ARLBIT
        LODI,R2 5
        BSTA,UN ARLPAT
        LODA,R0 ARL2V
        LODI,R1 GLNB
        BCTA,UN ARLFILL

; ARLOFF: the line at R0 (255 none) off, unless a new one's row is its.
ARLOFF: COMI,R0 255
        RETC,EQ
        STRA,R0 ARN
        EORA,R0 ARL1V
        ANDI,R0 $F8
        RETC,EQ
        LODA,R0 ARN
        EORA,R0 ARL2V
        ANDI,R0 $F8
        RETC,EQ
        LODA,R0 ARN
        LODI,R1 GSPACE
; ARLFILL: cells 1-14 of line R0's row = glyph R1. Clobbers R0-R3.
ARLFILL: RRR,R0
        RRR,R0
        RRR,R0
        ANDI,R0 $1F
        STRA,R1 ARWH
        STRZ    R1
        LODI,R2 0
        ZBSR    *ZDPOS
        LODA,R0 ARWH
        LODI,R3 14
        ZBRR    *ZDFILL
; ARLBIT: R0 = the mask bit of line R0's pixel row (rows 0, 2, 4, 6).
ARLBIT: ANDI,R0 7
        RRR,R0
        STRZ    R3
        LODA,R0 ARLBITS,R3
        RETC,UN
ARLBITS: DB     1,2,4,8
; ARLPAT: glyph R2 (user glyph 4-6) = the dotted rows of mask R0.
ARLPAT: RRL,R0                  ; LNPAT + mask * 8
        RRL,R0
        RRL,R0
        ADDI,R0 LNPAT&$FF
        STRZ    R1
        PPSL    PSLWC
        LODI,R0 LNPAT>>8
        ADDI,R0 0
        CPSL    PSLWC
        LODI,R3 PG_ARENA
        ZBRR    *ZUDCQ

; ARBEST: ARWH:ARWL = the lowest ask (R1 ROLE_SELLER: under it) or the
; highest bid (ROLE_BUYER: over it) among the players' lines, from the
; lead-in on. Keeps R1.
ARBEST: LODA,R0 AUSTG
        COMI,R0 STAGE_LEAD
        RETC,LT
        LODI,R3 0
ARBS1:  LODA,R0 AUROLE,R3
        COMZ    R1
        BCFR,EQ ARBS3
        LODZ    R3              ; its price
        RRL,R0
        STRZ    R2
        LODA,R0 AUPOS,R2
        COMA,R0 ARWH
        BCTR,LT ARBS4
        BCTR,GT ARBS5
        LODA,R0 AUPOS+1,R2
        COMA,R0 ARWL
        BCTR,LT ARBS4
        BCTR,GT ARBS5
        BCTR,UN ARBS3
ARBS4:  COMI,R1 ROLE_SELLER     ; under: a seller's is better
        BCFR,EQ ARBS3
        BCTR,UN ARBS6
ARBS5:  COMI,R1 ROLE_BUYER      ; over: a buyer's is better
        BCFR,EQ ARBS3
ARBS6:  LODA,R0 AUPOS,R2
        STRA,R0 ARWH
        LODA,R0 AUPOS+1,R2
        STRA,R0 ARWL
ARBS3:  ADDI,R3 1
        COMI,R3 4
        BCFR,EQ ARBS1
        RETC,UN

; ---- a frame (auction_isr, auction_tick) ---------------------------------------------
; The state only notes what changed (ARFLAGS); the frame draws: a new
; screen or stage (p_arc), then money, the clock, the message, the land's
; price and who blinks (p_arb) and the players; the lines walk every 8
; frames; a label and the clock each frame (p_arb); our hands (p_arc).
ARTICK: LODA,R0 SCRNOW
        COMI,R0 SCR_AUC
        RETC,LT
        RETC,GT
        LODA,R0 ARFLAGS
        ANDI,R0 ARF_SCR|ARF_BEGIN
        BCTR,EQ ART0
        LODI,R1 PG_ARC
        LODI,R3 E_ENTER
        BSTA,UN PGFAR
ART0:   LODA,R0 ARFLAGS
        BCTR,EQ ART1
        EORZ    R0
        LPUT    ARFLAGS
        LODI,R1 PG_ARB
        LODI,R3 E_ENTER
        BSTA,UN PGFAR
        BSTA,UN ARDRAW
ART1:   LODA,R0 AUON            ; the lines step every 8 frames
        BCTR,EQ ART4
        LODA,R0 AUTKS
        ADDA,R0 DT
        LPUT    AUTKS
ART2:   LODA,R0 AUTKS
        COMI,R0 8
        BCTR,LT ART4
        SUBI,R0 8
        LPUT    AUTKS
        LODA,R0 AUPH
        ADDI,R0 1
        LPUT    AUPH
        LODI,R0 0
ART3:   STRA,R0 T3
        BSTA,UN AUSTEP
        LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFR,EQ ART3
        LODI,R1 PG_ARB          ; who blinks, the land's price
        LODI,R3 E_STATE
        BSTA,UN PGFAR
        BSTA,UN ARDRAW
        BCTR,UN ART2
ART4:   BSTA,UN ARLINES
        LODI,R1 PG_ARB          ; a label, the clock
        LODI,R3 E_TICK
        BSTA,UN PGFAR
        LODA,R0 H_MYSEAT        ; a player's hands
        COMI,R0 4
        RETC,GT
        RETC,EQ
        LODI,R1 PG_ARC
        LODI,R3 E_TICK
        BCTA,UN PGFAR

; the line glyphs: mask bit k a dotted line on pixel row 2k
LNPAT:
M       SET     0
        REPT    16
        DB      (M&1)*$AA,0,((M>>1)&1)*$AA,0,((M>>2)&1)*$AA,0,((M>>3)&1)*$AA,0
M       SET     M+1
        ENDM
