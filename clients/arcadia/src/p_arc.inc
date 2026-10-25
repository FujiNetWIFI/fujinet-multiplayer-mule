; p_arc.inc -- the arena's third page (see p_arena.inc): a new screen, a
; stage's title and fresh start, and our hands (declaring, the stick,
; colluding). The arena far-calls it (PGFAR).

        BCTA,UN ARCSET          ; E_ENTER: the screen / the stage, as flagged
        RETC,UN                 ; E_STATE
        NOP
        NOP
        BCTA,UN ARCIN           ; E_TICK: a player's input

        INCLUDE "src/auram.inc"
        INCLUDE "src/arena.inc"

ARCSET: LODA,R0 ARFLAGS
        ANDI,R0 ARF_SCR
        BCTR,EQ ARC1
        BSTA,UN ARSCREEN
ARC1:   BSTA,UN ARTITLE
        BCTA,UN ARBEGIN

; ARCIN: declaring; from the lead-in, the stick; trading goods, FIRE
; colludes.
ARCIN:  LODA,R0 AUSTG
        COMI,R0 STAGE_DECLARE
        BCTA,EQ ARDECL
        COMI,R0 STAGE_LEAD
        RETC,LT
        COMI,R0 STAGE_DONE
        RETC,EQ
        BSTA,UN ARSTICK
        LODA,R0 EVBTN
        RETC,EQ
        LODA,R0 AUKIND
        COMI,R0 2
        RETC,LT
        LODA,R0 AUSTG
        COMI,R0 STAGE_ACTIVE
        RETC,LT
        LODI,R0 ACT_COLL
        LODI,R1 0
        LODI,R2 0
        ZBRR    *ZACT

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

; ARREC: R3 = seat R1's record. Keeps R1.
ARREC:  LODA,R0 ARREC4,R1
        STRZ    R3
        RETC,UN
ARREC4: DB      0,29,58,87
ARCOLX: DB      12,44,76,108    ; a player's column (its sprite)

; ---- the screen -----------------------------------------------------------------
; ARSCREEN: a new auction screen (auction_enter).
ARSCREEN: BSTA,UN SCRG26
        LODI,R0 SCR_AUC
        LPUT    SCRNOW
        LODI,R0 255
        LPUT    AUSTG
        EORZ    R0
        LPUT    AUON
        LODI,R0 0               ; each in its colour, 16 lines
ARSC1:  STRA,R0 T3
        LODA,R1 T3
        BSTA,UN ARREC
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATSPR,R3
        STRZ    R2
        LODA,R1 T3
        ZBSR    *ZSPRCOL
        LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFR,EQ ARSC1
        LODI,R0 ARMSG
        LPUT    MSGROW
        LODI,R0 ARMSGN
        LPUT    MSGN
        RETC,UN

; ARTITLE: the title, the good and the store's prices.
ARTITLE: LODI,R1 0
        LODI,R3 2
        ZBSR    *ZDCLR
        DCOL    CWHITE
        LODI,R1 T_LANDAUC
        LODA,R0 AUKIND
        COMI,R0 1
        BCTA,EQ TITLE           ; (land: the title only)
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
        DCOL    CWHITE          ; what the store pays, left; charges, right
        LODI,R1 1
        LODI,R2 $FF
        ZBSR    *ZDPOS
        LODA,R0 A_STOREBUY
        LODA,R1 A_STOREBUY+1
        LODI,R3 4
        ZBSR    *ZDNUM16
        LODI,R1 1
        LODI,R2 11
        ZBSR    *ZDPOS
        LODA,R0 A_STORESELL
        LODA,R1 A_STORESELL+1
        LODI,R3 4
        ZBSR    *ZDNUM16
        LODI,R1 T_STATUS        ; STATUS while declaring, then AUCTION
        LODA,R0 AUSTG
        COMI,R0 STAGE_DECLARE
        BCFA,GT TITLE
        BCTA,EQ TITLE
        LODI,R1 T_AUCTION
        BCTA,UN TITLE

; ARBEGIN: a stage begins (au_stage_begin): the arena cleared, everything
; drawn afresh; the alert to declare and as the lines are about to go.
ARBEGIN: LODI,R1 2               ; (row 24 too: the status page's message ran there)
        LODI,R3 23
        ZBSR    *ZDCLR
        LODI,R0 ARMSG           ; our message box (the status page had its own)
        LPUT    MSGROW
        LODI,R0 ARMSGN
        LPUT    MSGN
        LODI,R1 FRRAMP2         ; no figures loaded (the status page's are
        LODI,R3 ARFIGO          ;  its own)
        BSTA,UN RAMSET
        LODI,R0 255
        BSTA,UN RAMPUT1
        LODI,R0 255
        BSTA,UN RAMPUT1
        LODI,R0 255
        BSTA,UN RAMPUT1
        LODI,R0 255
        BSTA,UN RAMPUT1
        BSTA,UN RAMPEND
        LODI,R0 255
        LPUT    AULN1
        LODI,R0 255
        LPUT    AULN2
        LODI,R0 255
        LPUT    AUMSG
        LODI,R0 255
        CPUT    AUTBU
        LODI,R0 255
        LPUT    AUHBH
        LODI,R0 255
        LPUT    AUSBH
        LODI,R0 1
        LPUT    AUMALL
        LODI,R1 FRRAMP2         ; every label again
        LODI,R3 AULBLO
        BSTA,UN RAMSET
        LODI,R0 8
        STRA,R0 ARN
ARB1:   LODI,R0 $FF
        BSTA,UN RAMPUT1
        LODA,R0 ARN
        SUBI,R0 1
        STRA,R0 ARN
        BCFR,EQ ARB1
        BSTA,UN RAMPEND
        LODI,R1 ARBROW          ; the clock's row: block graphics
        LODI,R2 $FF
        ZBSR    *ZDPOS
        LODI,R0 GMOSAIC
        LODI,R3 1
        ZBSR    *ZDFILL
        EORZ    R0
        LODI,R3 15
        ZBSR    *ZDFILL
        LODA,R0 AUSTG
        COMI,R0 STAGE_DONE
        BCFR,EQ ARB2
        SND     SND_AUCOVR
        RETC,UN
ARB2:   COMI,R0 STAGE_ACTIVE
        RETC,EQ
        LODA,R0 H_MYSEAT
        COMI,R0 4
        RETC,GT
        RETC,EQ
        SND     SND_ALERT
        RETC,UN

; ARDECL: up, a seller; down, a buyer; 0, sitting it out.
ARDECL: LODA,R3 H_MYSEAT
        LODA,R0 AUROLE,R3
        STRA,R0 T2
        LODI,R1 ROLE_SELLER
        LODI,R2 2
        LODA,R0 INDIR
        TMI,R0  D_UP
        BCTR,EQ ARDC2
        LODI,R1 ROLE_BUYER
        LODI,R2 1
        TMI,R0  D_DOWN
        BCTR,EQ ARDC2
        LODA,R0 EVKEY
        COMI,R0 0
        RETC,GT
        LODI,R1 ROLE_NONE       ; (sitting out: once declared, again)
        LODI,R2 0
ARDC2:  LODZ    R1
        COMA,R0 T2
        BCFR,EQ ARDC3
        COMI,R1 ROLE_NONE
        RETC,GT
        RETC,LT
ARDC3:  STRA,R2 T1
        LODA,R0 H_MYSEAT
        ADDI,R0 AUROLE&$FF
        STRZ    R3
        LODZ    R1
        ZBSR    *ZLPUT
        LODI,R0 ACT_DECL
        LODI,R1 0
        LODA,R2 T1
        ZBSR    *ZACT
        SND     SND_SELECT
        LODA,R0 ARFLAGS         ; drawn again next frame
        IORI,R0 ARF_DIRTY
        LPUT    ARFLAGS
        LODI,R1 FRRAMP2         ; its label again
        LODA,R0 H_MYSEAT
        RRL,R0
        ADDI,R0 AULBLO
        STRZ    R3
        LODI,R0 $FF
        BCTA,UN RAMPUT

; ARSTICK: our stick to the server when it changes (au_stick): up, down,
; let go (where our line stopped: HOLD). Our own line moves at once.
ARSTICK: LODI,R2 0
        LODA,R0 INDIR
        TMI,R0  D_UP
        BCFR,EQ ARK1
        LODI,R2 1
ARK1:   TMI,R0  D_DOWN
        BCFR,EQ ARK2
        LODI,R2 2
ARK2:   LODZ    R2
        COMA,R0 AUPUSH
        RETC,EQ
        LODA,R0 ACT
        RETC,GT
        LODZ    R2
        STRA,R0 T3
        LPUT    AUPUSH
        LODA,R0 H_MYSEAT        ; T2: our pair's index
        RRL,R0
        STRA,R0 T2
        LODA,R0 T3
        BCFR,EQ ARK3
        LODA,R0 AUSTG
        COMI,R0 STAGE_ACTIVE
        BCFR,EQ ARK3
        LODA,R3 T2              ; let go, running: HOLD at our line
        LODA,R0 AUPOS+1,R3
        STRZ    R2
        LODA,R0 AUPOS,R3
        STRZ    R1
        LODI,R0 ACT_HOLD
        ZBSR    *ZACT
        LODA,R0 FRML
        LPUT    AUHELD
        BCTR,UN ARK4
ARK3:   LODI,R0 ACT_PUSH
        LODI,R1 0
        LODA,R2 T3
        ZBSR    *ZACT
ARK4:   LODA,R0 AUSTG           ; running: our line moves now
        COMI,R0 STAGE_ACTIVE
        RETC,LT
        RETC,GT
        LODA,R3 T2              ; let go: where it is
        LODA,R0 AUPOS,R3
        STRA,R0 ARWH
        LODA,R0 AUPOS+1,R3
        STRA,R0 ARWL
        LODA,R0 T3
        BCTR,EQ ARK8
        COMI,R0 2
        BCFR,EQ ARK5
        LODA,R0 A_LO            ; down: the scale's bottom
        STRA,R0 ARWH
        LODA,R0 A_LO+1
        STRA,R0 ARWL
        BCTR,UN ARK8
ARK5:   LODA,R0 AUKIND          ; up: goods, the top; land, our money
        COMI,R0 1
        BCTR,EQ ARK6
        LODA,R0 A_HI
        STRA,R0 ARWH
        LODA,R0 A_HI+1
        STRA,R0 ARWL
        BCTR,UN ARK8
ARK6:   LODA,R1 H_MYSEAT
        BSTA,UN ARREC
        LODA,R0 H_PLAYERS+PL_MONEY,R3
        STRA,R0 ARWH
        LODA,R0 H_PLAYERS+PL_MONEY+1,R3
        ANDI,R0 $FC
        STRA,R0 ARWL
ARK8:   LODA,R0 H_MYSEAT
        STRA,R0 T3
        LODI,R0 AUTGT&$FF
        BCTA,UN PUTW

        INCLUDE "src/mapscr.inc"

SEATSPR: DB     SBLUE|$80,SMAGEN|$80,SCYAN|$80,SWHITE|$80
GOODCOL: DB     5,4,3,3
GOODNM: DB      SFOOD&$FF,SENERGY&$FF,SSMITH&$FF,SCRYST&$FF
SFOOD:  DB      "FOOD",$FF
SENERGY: DB     "ENERGY",$FF
SSMITH: DB      "SMITHORE",$FF
SCRYST: DB      "CRYSTITE",$FF
        IF      (SCRYST>>8) <> (SFOOD>>8)
        ERROR   "the goods' names cross a 256-byte page"
        ENDIF
