; p_arb.inc -- the arena's second page (see p_arena.inc): its labels, the
; money and units, the clock, the message, the land's price line and who
; blinks. The arena far-calls it (PGFAR).

        BCTA,UN ARUPD           ; E_ENTER: a state's worth
        BCTA,UN ARSTEPD         ; E_STATE: after a step of the lines
        BCTA,UN ARBTICK         ; E_TICK: each frame

        INCLUDE "src/auram.inc"
        INCLUDE "src/arena.inc"

ARUPD:  BSTA,UN ARMONEY
        BSTA,UN ARTBAR
        BSTA,UN ARMSGS
ARSTEPD: BSTA,UN ARBLINK
        BCTA,UN ARLAND
ARBTICK: BSTA,UN ARLBL
        BCTA,UN ARTBAR

; ARREC: R3 = seat R1's record. Keeps R1.
ARREC:  LODA,R0 ARREC4,R1
        STRZ    R3
        RETC,UN
ARREC4: DB      0,29,58,87
ARCOLX: DB      12,44,76,108    ; a player's column (its sprite)

; ---- the labels, money, the clock -----------------------------------------------
; ARLBL: one label a frame, the next that changed (au_lbl_tick): BUY or
; SELL before the lead-in, then the player's price (at the scale's end, the
; word again).
ARLBL:  LODI,R0 4
        STRA,R0 ARN
ARLB1:  LODA,R0 AULBLI
        ADDI,R0 1
        ANDI,R0 3
        LPUT    AULBLI
        LODA,R0 AULBLI
        STRA,R0 T3
        STRZ    R3
        LODI,R0 0               ; w
        STRA,R0 ARWH
        LODI,R0 1
        STRA,R0 ARWL
        LODA,R0 AUROLE,R3
        BCTA,EQ ARLB4           ; none: 1
        STRA,R0 T2
        EORZ    R0
        STRA,R0 ARWL
        LODA,R0 AUSTG
        COMI,R0 STAGE_LEAD
        BCTA,LT ARLB4           ; before the lead-in: 0
        LODZ    R3
        RRL,R0
        STRZ    R3
        LODA,R0 AUPOS,R3
        STRA,R0 ARWH
        LODA,R0 AUPOS+1,R3
        STRA,R0 ARWL
        LODA,R0 T2              ; a buyer at the bottom, a seller at the top
        COMI,R0 ROLE_BUYER
        BCFR,EQ ARLB2
        LODA,R0 ARWH
        COMA,R0 A_LO
        BCTR,LT ARLB3
        BCFR,EQ ARLB4A
        LODA,R0 ARWL
        COMA,R0 A_LO+1
        BCTR,GT ARLB4A
        BCTR,UN ARLB3
ARLB2:  LODA,R0 ARWH
        COMA,R0 A_HI
        BCTR,GT ARLB3
        BCFR,EQ ARLB4A
        LODA,R0 ARWL
        COMA,R0 A_HI+1
        BCTR,LT ARLB4A
ARLB3:  EORZ    R0
        STRA,R0 ARWH
        STRA,R0 ARWL
        BCTR,UN ARLB4
ARLB4A: LODA,R0 ARWH            ; (a price of 1 shows as 2: 1 means none)
        BCFR,EQ ARLB4
        LODA,R0 ARWL
        COMI,R0 1
        BCFR,EQ ARLB4
        LODI,R0 2
        STRA,R0 ARWL
ARLB4:  LODA,R0 T3              ; changed?
        RRL,R0
        ADDI,R0 AULBLO
        STRZ    R3
        STRA,R3 T1
        LODA,R0 *PRAMH,R3
        COMA,R0 ARWH
        BCFR,EQ ARLB5
        LODA,R3 T1
        ADDI,R3 1
        LODA,R0 *PRAMH,R3
        COMA,R0 ARWL
        BCFR,EQ ARLB5
        LODA,R0 ARN             ; no: the next
        SUBI,R0 1
        STRA,R0 ARN
        BCFA,EQ ARLB1
        RETC,UN
ARLB5:  LODI,R1 FRRAMP2         ; noted
        LODA,R3 T1
        BSTA,UN RAMSET
        LODA,R0 ARWH
        BSTA,UN RAMPUT1
        LODA,R0 ARWL
        BSTA,UN RAMPUT1
        BSTA,UN RAMPEND
        LODI,R1 ARLROW          ; the field: cleared, then the word or price
        BSTA,UN ARFIELD
        LODA,R0 ARWH
        BCFR,EQ ARLB7
        LODA,R0 ARWL
        COMI,R0 1
        RETC,EQ                 ; 1: nothing
        BCFR,LT ARLB7
        LODA,R3 T3              ; 0: the word
        LODA,R0 AUROLE,R3
        LODI,R1 SSELL&$FF
        COMI,R0 ROLE_SELLER
        BCTR,EQ ARLB6
        LODI,R1 SBUY&$FF
ARLB6:  LODI,R0 SSELL>>8
        ZBRR    *ZDTXT
ARLB7:  LODA,R0 ARWH            ; a price: 4 digits
        LODA,R1 ARWL
        LODI,R3 4
        ZBRR    *ZDNUM16

; ARFIELD: seat T3's 4 cells of row R1 cleared, the display at its first,
; in its ink. Clobbers R0-R3.
ARFIELD: LODA,R0 T3
        RRL,R0
        RRL,R0
        SUBI,R0 1
        STRZ    R2
        STRA,R2 ARN
        ZBSR    *ZDPOS
        EORZ    R0
        LODI,R3 4
        ZBSR    *ZDFILL
        LODA,R2 ARN
        LODA,R1 T3
        BSTA,UN ARREC
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATINK,R3
        STRA,R0 DCOLOR
        RETC,UN

; ARMONEY: money, and units of the good, when a record changed (au_money).
ARMONEY: LODI,R1 0              ; the records' signature
        LODI,R3 0
ARMS1:  LODA,R0 H_PLAYERS,R3
        ADDZ    R1
        RRL,R0
        STRZ    R1
        ADDI,R3 1
        COMI,R3 4*29
        BCFR,EQ ARMS1
        LODA,R0 AUMALL
        BCFR,EQ ARMS2
        LODI,R3 AUMSIGO
        LODA,R0 *PRAMH,R3
        COMZ    R1
        RETC,EQ
ARMS2:  LODZ    R1
        CPUT    AUMSIG
        EORZ    R0
        LPUT    AUMALL
        LODI,R0 0
; a line each under the arena: the name in the seat's ink (6), its money
; (5 digits, white, at 7-11), its units of the good (goods, from the
; lead-in; cyan, at 13-15): a gap between each
ARMS3:  STRA,R0 T3
        ADDI,R0 ARMROW
        STRZ    R1
        LODI,R2 $FF
        ZBSR    *ZDPOS
        LODA,R1 T3
        BSTA,UN ARREC
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATINK,R3
        STRA,R0 DCOLOR
        LODA,R1 T3              ; name 0-5, money 7-11, units 13-15
        ZBSR    *ZDNAME6
        DCOL    CWHITE
        LODA,R1 T3
        BSTA,UN ARREC
        LODA,R0 H_PLAYERS+PL_MONEY+1,R3
        STRZ    R1
        LODA,R0 H_PLAYERS+PL_MONEY,R3
        LODI,R3 6
        ZBSR    *ZDNUM16
        LODA,R0 AUKIND          ; units: goods, from the lead-in
        COMI,R0 2
        BCFR,EQ ARMS4
        LODA,R0 AUSTG
        COMI,R0 STAGE_LEAD
        BCTR,LT ARMS4
        DCOL    CCYAN
        LODA,R1 T3
        BSTA,UN ARREC
        LODZ    R3
        ADDA,R0 AUGOOD
        STRZ    R3
        LODA,R0 H_PLAYERS+PL_GOODS,R3
        STRZ    R1
        EORZ    R0
        LODI,R3 4
        ZBSR    *ZDNUM16
        BCTR,UN ARMS5
ARMS4:  EORZ    R0              ; (none: blank)
        LODI,R3 4
        ZBSR    *ZDFILL
ARMS5:  LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFA,EQ ARMS3
        RETC,UN

; ARTBAR: the auction clock (au_timebar): its ticks left, a block column
; each on row 25, ticking as it runs down.
ARTBAR: EORZ    R0
        STRA,R0 ARN
        LODA,R0 AUSTG
        COMI,R0 STAGE_ACTIVE
        BCTR,EQ ARTB1
        COMI,R0 STAGE_DECLARE
        BCFA,EQ ARTB4
ARTB1:  LODA,R1 RXFRH           ; jiffies since the state, + per
        LODA,R2 RXFRL
        ZBSR    *ZSINCE
        LODI,R3 25              ; per: 25, or 6 idle and declaring
        LODA,R0 AUSTG
        COMI,R0 STAGE_DECLARE
        BCTR,EQ ARTB2
        LODA,R0 A_FLAGS
        TMI,R0  AF_IDLE
        BCFR,EQ ARTB3
ARTB2:  LODI,R3 6
ARTB3:  STRA,R3 ARWH
        LODZ    R2
        ADDZ    R3
        STRZ    R2
        PPSL    PSLWC
        LODZ    R1
        ADDI,R0 0
        CPSL    PSLWC
        STRZ    R1
        LODZ    R2              ; past the tick under way?
        SUBA,R0 A_TICKLEFT
        STRZ    R2
        PPSL    PSLWC
        LODZ    R1
        SUBI,R0 0
        CPSL    PSLWC
        STRZ    R1
        TMI,R1  $80
        BCTR,EQ ARTB4           ; no: all of them
        LODA,R3 ARWH            ; ticks gone = that / per
        ZBSR    *ZDIV8
        LODA,R0 A_TICKS
        SUBZ    R2
        BCTR,LT ARTB4A
        STRA,R0 ARN
        BCTR,UN ARTB5
ARTB4:  LODA,R0 AUSTG           ; (not running: none)
        COMI,R0 STAGE_ACTIVE
        BCTR,EQ ARTB4B
        COMI,R0 STAGE_DECLARE
        BCFR,EQ ARTB5
ARTB4B: LODA,R0 A_TICKS
        STRA,R0 ARN
        BCTR,UN ARTB5
ARTB4A: EORZ    R0
        STRA,R0 ARN
ARTB5:  LODA,R0 ARN
        BSTA,UN BARTICK
        LODI,R3 AUTBUO
        LODA,R0 *PRAMH,R3
        COMA,R0 ARN
        RETC,EQ
        LODA,R0 ARN
        CPUT    AUTBU
        LODI,R1 ARBROW          ; drawn: 15 cells of 3 columns
        LODI,R2 0
        ZBSR    *ZDPOS
        LODI,R0 15
        STRA,R0 ARWL
ARTB6:  LODA,R0 ARN
        COMI,R0 3
        BCTR,LT ARTB7
        LODI,R0 3
ARTB7:  STRZ    R3
        LODA,R0 ARN
        SUBZ    R3
        STRA,R0 ARN
        LODA,R0 ARTCELL,R3
        LODI,R3 1
        ZBSR    *ZDFILL
        LODA,R0 ARWL
        SUBI,R0 1
        STRA,R0 ARWL
        BCFR,EQ ARTB6
        RETC,UN
ARTCELL: DB     $00,$24,$36,$3F ; (white)

; ---- messages (au_message), the land's price line, who blinks ----------------------
ARMSGS: LODI,R1 255
        LODA,R0 AUSTG
        COMI,R0 STAGE_DECLARE
        BCFR,EQ ARM1
        LODA,R0 H_MYSEAT
        COMI,R0 4
        BCFR,LT ARM1
        LODI,R1 M_DECLARE
ARM1:   LODA,R0 AUSTG
        COMI,R0 STAGE_DONE
        BCFR,EQ ARM3
        LODI,R1 M_AUCOVER
        LODA,R0 AUKIND
        COMI,R0 1
        BCFR,EQ ARM3
        LODI,R1 M_SOLD
        LODA,R0 A_WINNER
        COMI,R0 4
        BCTR,LT ARM3
        LODI,R1 M_NOBIDS
ARM3:   LODA,R0 AUSTG
        COMI,R0 STAGE_ACTIVE
        BCFR,EQ ARM6
        LODA,R0 AUKIND
        COMI,R0 1
        BCTR,EQ ARM7
        LODA,R0 A_FLAGS
        TMI,R0  AF_COLLUDING
        BCFR,EQ ARM4
        LODI,R1 M_COLLUDE
ARM4:   TMI,R0  AF_CRITICAL
        BCFR,EQ ARM6
        LODI,R1 M_CRITICAL
ARM6:   LODZ    R1
        COMA,R0 AUMSG
        RETC,EQ
        STRA,R1 T3
        LPUT    AUMSG
        LODA,R1 T3
        COMI,R1 255
        BCFR,EQ ARM6A
        LODI,R1 ARMSG           ; none: the rows cleared
        LODI,R3 ARMSGN
        ZBRR    *ZDCLR
ARM6A:  DCOL    CWHITE
        ZBRR    *ZSAY
ARM7:   LODA,R0 AUBASEH         ; land, running: "bids start at" the
        COMA,R0 AUSBH           ;  scale's bottom, as it scrolls
        BCFR,EQ ARM8
        LODA,R0 AUBASEL
        COMA,R0 AUSBL
        RETC,EQ
ARM8:   LODA,R0 AUBASEH
        LPUT    AUSBH
        LODA,R0 AUBASEL
        LPUT    AUSBL
        LODI,R0 M_BIDSTART
        LPUT    AUMSG
        DCOL    CWHITE
        LODI,R1 M_BIDSTART
        LODA,R2 AUBASEH
        LODA,R3 AUBASEL
        ZBRR    *ZSAYV

; ARLAND: the land auction's price line on row 2: the opening price, the
; high bid, what it sold for.
ARLAND: LODA,R0 AUKIND
        COMI,R0 1
        RETC,GT
        LODA,R0 A_START
        STRA,R0 ARWH
        LODA,R0 A_START+1
        STRA,R0 ARWL
        LODI,R1 ROLE_BUYER      ; a bid at or over it: the high bid
        BSTA,UN ARWHO
        LODI,R0 M_LSTART
        STRA,R0 T2
        COMI,R3 4
        BCTR,EQ ARLD2
        LODI,R0 M_LHIGH
        STRA,R0 T2
ARLD2:  LODA,R0 AUSTG
        COMI,R0 STAGE_DONE
        BCFR,EQ ARLD3
        LODA,R0 A_PRICE
        IORA,R0 A_PRICE+1
        BCTR,EQ ARLD3
        LODA,R0 A_PRICE
        STRA,R0 ARWH
        LODA,R0 A_PRICE+1
        STRA,R0 ARWL
        LODI,R0 M_LSOLD
        STRA,R0 T2
ARLD3:  LODA,R0 ARWH
        COMA,R0 AUHBH
        BCFR,EQ ARLD4
        LODA,R0 ARWL
        COMA,R0 AUHBL
        RETC,EQ
ARLD4:  LODA,R0 ARWH
        LPUT    AUHBH
        LODA,R0 ARWL
        LPUT    AUHBL
        LODA,R0 AUHBH
        LPUT    SAYV1H
        LODA,R0 AUHBL
        LPUT    SAYV1L
        DCOL    CWHITE
        LODA,R1 T2
        LODI,R2 2
        LODI,R3 1
        ZBRR    *ZSAYBX

; ARBLINK: who is dealing blinks (au_blinkers): land, the high bidder (the
; winner once sold); goods, the buyer and seller whose lines meet, or
; whoever meets the store's.
ARBLINK: EORZ   R0
        STRA,R0 ARN
        LODA,R0 AUSTG
        COMI,R0 STAGE_ACTIVE
        BCTA,LT ARBL9
        LODA,R0 AUKIND
        COMI,R0 1
        BCFR,EQ ARBL3
        LODA,R0 AUSTG
        COMI,R0 STAGE_DONE
        BCFR,EQ ARBL1
        LODA,R3 A_WINNER
        COMI,R3 4
        BCTA,GT ARBL9
        BCTA,EQ ARBL9
        BCTR,UN ARBL2
ARBL1:  LODA,R0 A_START         ; the high bidder
        STRA,R0 ARWH
        LODA,R0 A_START+1
        STRA,R0 ARWL
        LODI,R1 ROLE_BUYER
        BSTA,UN ARWHO
        COMI,R3 4
        BCTA,EQ ARBL9
ARBL2:  LODA,R0 ARBIT,R3
        STRA,R0 ARN
        BCTA,UN ARBL9
ARBL3:  LODA,R0 AUSTG           ; goods, trading
        COMI,R0 STAGE_ACTIVE
        BCFA,EQ ARBL9
        EORZ    R0              ; the highest bid
        STRA,R0 ARWH
        STRA,R0 ARWL
        LODI,R1 ROLE_BUYER
        BSTA,UN ARWHO
        STRA,R3 T1
        LODA,R0 ARWH
        STRA,R0 T2
        LODA,R0 ARWL
        STRA,R0 T0              ; T2:T0, it
        LODI,R0 $FF             ; the lowest ask
        STRA,R0 ARWH
        STRA,R0 ARWL
        LODI,R1 ROLE_SELLER
        BSTA,UN ARWHO
        STRA,R3 T3
        LODA,R0 T1              ; both, and they meet: both
        COMI,R0 4
        BCTR,EQ ARBL5
        LODA,R0 T3
        COMI,R0 4
        BCTR,EQ ARBL5
        LODA,R0 T2
        COMA,R0 ARWH
        BCTR,LT ARBL5
        BCFR,EQ ARBL4
        LODA,R0 T0
        COMA,R0 ARWL
        BCTR,LT ARBL5
ARBL4:  LODA,R3 T1
        LODA,R0 ARBIT,R3
        LODA,R3 T3
        IORA,R0 ARBIT,R3
        STRA,R0 ARN
        BCTA,UN ARBL9
ARBL5:  LODA,R3 T1              ; a buyer at the store's price
        COMI,R3 4
        BCTR,EQ ARBL6
        LODA,R0 T2
        COMA,R0 A_STORESELL
        BCTR,LT ARBL6
        BCFR,EQ ARBL5A
        LODA,R0 T0
        COMA,R0 A_STORESELL+1
        BCTR,LT ARBL6
ARBL5A: LODA,R0 ARBIT,R3
        STRA,R0 ARN
ARBL6:  LODA,R3 T3              ; a seller at what the store pays
        COMI,R3 4
        BCTR,EQ ARBL9
        LODA,R0 ARWH
        COMA,R0 A_STOREBUY
        BCTR,GT ARBL9
        BCFR,EQ ARBL7
        LODA,R0 ARWL
        COMA,R0 A_STOREBUY+1
        BCTR,GT ARBL9
ARBL7:  LODA,R0 ARBIT,R3
        IORA,R0 ARN
        STRA,R0 ARN
ARBL9:  LODA,R0 ARN
        LPUT    AUBLINK
        RETC,UN

; ARWHO: as ARBEST, from the lead-in on, the seat with that price in R3 (4
; none). Keeps R1.
ARWHO:  LODI,R0 4
        STRA,R0 UNIONB+5
        LODI,R3 0
ARW1:   LODA,R0 AUROLE,R3
        COMZ    R1
        BCFR,EQ ARW3
        LODZ    R3
        RRL,R0
        STRZ    R2
        LODA,R0 AUPOS,R2
        COMA,R0 ARWH
        BCTR,LT ARW4
        BCTR,GT ARW5
        LODA,R0 AUPOS+1,R2
        COMA,R0 ARWL
        BCTR,LT ARW4
        BCTR,GT ARW5
        BCTR,UN ARW6            ; (equal: as good; the later seat)
ARW4:   COMI,R1 ROLE_SELLER
        BCFR,EQ ARW3
        BCTR,UN ARW6
ARW5:   COMI,R1 ROLE_BUYER
        BCFR,EQ ARW3
ARW6:   LODA,R0 AUPOS,R2
        STRA,R0 ARWH
        LODA,R0 AUPOS+1,R2
        STRA,R0 ARWL
        STRA,R3 UNIONB+5
ARW3:   ADDI,R3 1
        COMI,R3 4
        BCFR,EQ ARW1
        LODA,R3 UNIONB+5
        RETC,UN

ARBIT:  DB      1,2,4,8
SEATINK: DB     CBLUE,CMAGEN,CCYAN,CWHITE
SSELL:  DB      "SELL",$FF
SBUY:   DB      " BUY",$FF
        IF      (SBUY>>8) <> (SSELL>>8)
        ERROR   "the arena's words cross a 256-byte page"
        ENDIF
