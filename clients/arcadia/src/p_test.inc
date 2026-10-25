; p_test.inc -- the offline test mode (mekkogx teststart.c, test.c), and
; AUTOPLAY (autoplay.c) in builds that have it.
;
; tools/arcsim.py's `test PH ARG PEV` pokes $5A, the phase + 1 (0: the
; development), the argument (the colony event, the auction's stage, the
; seat whose turn it is) and the player event into TESTHK at the title. The
; game then skips the FujiNet and fakes a state: round 1 on a generated map
; (build/teststate.inc). Actions are answered here as the server would,
; roughly, so walking, the town, the events and the animations can be
; checked in seconds.

        BCTA,UN TSTART          ; E_ENTER: the fake state
        RETC,UN                 ; E_STATE
        NOP
        NOP
        RETC,UN                 ; E_TICK
        NOP
        NOP
        BCTA,UN TPOLL           ; E_EXTRA: answer the action, age the clocks

TESTHK  EQU     UNIONB          ; (the boot page's)

; ---- teststart.c -------------------------------------------------------------
TSTART: LODI,R1 FRRAMP0         ; the state into RAM-L, from $0600
        LODI,R3 0
        BSTA,UN RAMSET
        LODI,R3 $FF
TS1:    LODA,R0 TSTL,R3,+
        BSTA,UN RAMPUT1
        COMI,R3 255
        BCFR,EQ TS1
        LODI,R3 $FF
TS2:    LODA,R0 TSTL+256,R3,+
        BSTA,UN RAMPUT1
        COMI,R3 TSTLN-256-1
        BCFR,EQ TS2
        BSTA,UN RAMPEND
        LODI,R1 FRRAMP2         ; the wampus list into RAM-H
        LODI,R3 0
        BSTA,UN RAMSET
        LODI,R3 $FF
TS3:    LODA,R0 TSTW,R3,+
        BSTA,UN RAMPUT1
        COMI,R3 TSTWN-1
        BCFR,EQ TS3
        BSTA,UN RAMPEND
; what the test asked for
        LODA,R0 TESTHK+1        ; the phase: + 1, or 0 for the development
        SUBI,R0 1
        BCFR,LT TS4
        LODI,R0 PHASE_DEVELOP
TS4:    STRA,R0 T3              ; T3: the phase
        CPUT    H_PHASE
        LODA,R0 TESTHK+2        ; the turn's seat, the colony's event
        ANDI,R0 3
        CPUT    TN_SEAT
        LODA,R0 TESTHK+2
        CPUT    C_EVENT
        LODA,R0 TESTHK+3
        CPUT    TN_EVENT
        LODA,R0 T3              ; the lobby, the summary: the argument shifts
        COMI,R0 PHASE_LOBBY     ;  the species (1 shows the other four)
        BCTR,EQ TS5
        COMI,R0 PHASE_SUMMARY
        BCFR,EQ TS6
TS5:    LODI,R1 0
TS5A:   STRA,R1 T2
        LODZ    R1              ; (1 + 2i + arg) & 7
        ADDZ    R1
        ADDI,R0 1
        ADDA,R0 TESTHK+2
        ANDI,R0 7
        STRA,R0 T1
        LODA,R1 T2
        LODA,R0 PL29T,R1
        ADDI,R0 (H_PLAYERS+PL_SPECIES-$0600)&$FF
        STRZ    R3
        LODI,R1 FRRAMP0
        LODA,R0 T1
        BSTA,UN RAMPUT
        LODA,R1 T2
        ADDI,R1 1
        COMI,R1 4
        BCFR,EQ TS5A
TS6:    LODA,R0 T3              ; the auctions: the stage; the land's own
        COMI,R0 PHASE_DECLARE
        BCTR,EQ TS7
        COMI,R0 PHASE_TRADING
        BCTR,EQ TS7
        COMI,R0 PHASE_LANDAUCTION
        BCFR,EQ TS8
        LODI,R0 1
        CPUT    A_KIND
        LODI,R0 4
        CPUT    A_STEP
        LODI,R0 240
        CPUT    A_START+1
        LODI,R0 13
        CPUT    A_PLOT
TS7:    LODA,R0 TESTHK+2
        CPUT    A_STAGE
TS8:    LODI,R1 FRRAMP2         ; who we are
        LODI,R3 SCNAME-$2900
        BSTA,UN RAMSET
        LODI,R0 $50
        BSTA,UN RAMPUT1
        LODI,R0 $4C
        BSTA,UN RAMPUT1
        LODI,R0 $30
        BSTA,UN RAMPUT1
        EORZ    R0
        BSTA,UN RAMPUT1
        BSTA,UN RAMPEND
        RETC,UN
PL29T:  DB      0,29,58,87

; ---- test.c: an action answered as the server would, roughly ------------------
TPOLL:  LODA,R1 RXFRH           ; T0:T1 = frames since the last state
        LODA,R2 RXFRL
        ZBSR    *ZSINCE
        STRA,R1 T0
        STRA,R2 T1
        LODA,R0 ACT
        STRA,R0 ACTSENT
        EORZ    R0
        CPUT    H_ERR
        LODA,R0 ACTSENT
        COMI,R0 ACT_BUYMU
        BCFR,EQ TP1
        LODI,R0 TF_STARTED|TF_HASMULE
        CPUT    TN_FLAGS
        BCTA,UN TPAGE
TP1:    COMI,R0 ACT_RETURN
        BCFR,EQ TP2
        LODI,R0 TF_STARTED
        CPUT    TN_FLAGS
        BCTA,UN TPAGE
TP2:    COMI,R0 ACT_OUTFIT
        BCFR,EQ TP3
        LODA,R0 ACTARGL
        CPUT    TN_CARRYING
        BCTA,UN TPAGE
TP3:    COMI,R0 ACT_INST
        BCFR,EQ TP4
        LODI,R0 TF_STARTED
        CPUT    TN_FLAGS
        LODA,R0 ACTARGL         ; owner[arg] = 0, mule[arg] = carrying
        ADDI,R0 (MP_OWNER-$0600)&$FF
        STRZ    R3
        LODI,R1 FRRAMP0
        EORZ    R0
        BSTA,UN RAMPUT
        LODA,R0 ACTARGL         ; (the M.U.L.E.s run into RAM-L's next 256)
        ADDI,R0 (MP_MULE-$0600)&$FF
        STRZ    R3
        LODI,R1 FRRAMP0
        TPSL    PSLC
        BCFR,EQ TP3A
        LODI,R1 FRRAMP1
TP3A:   LODA,R0 TN_CARRYING
        BSTA,UN RAMPUT
        BCTA,UN TPAGE
TP4:    COMI,R0 ACT_ASSAY
        BCFR,EQ TP5
        LODI,R0 3
        CPUT    H_RESULT+1
        BCTA,UN TPAGE
TP5:    COMI,R0 ACT_PUB
        BCFR,EQ TP6
        LODI,R0 TF_STARTED|TF_ENDED
        CPUT    TN_FLAGS
        LODI,R0 75
        CPUT    H_RESULT+1
        BCTA,UN TPAGE
TP6:    COMI,R0 ACT_CLAIM
        BCFR,EQ TPAGE
        LODA,R0 ACTARGL
        CPUT    G_CLAIMED
        LODA,R0 ACTARGL
        ADDI,R0 (MP_OWNER-$0600)&$FF
        STRZ    R3
        LODI,R1 FRRAMP0
        EORZ    R0
        BSTA,UN RAMPUT
; the clocks: the turn's time runs down, the phase's, the server's, on
TPAGE:  LODA,R0 H_PHASE
        COMI,R0 PHASE_DEVELOP
        BCFA,EQ TPA2
        LODA,R0 TN_LEFT+1       ; left = left > w ? left - w : 0
        SUBA,R0 T1
        STRA,R0 T3
        PPSL    PSLWC
        LODA,R0 TN_LEFT
        SUBA,R0 T0
        CPSL    PSLWC
        STRA,R0 T2
        TPSL    PSLC            ; (borrowed past 0: none left)
        BCFR,EQ TPA1
        IORA,R0 T3
        BCTR,EQ TPA1
        LODA,R0 T2
        CPUT    TN_LEFT
        LODA,R0 T3
        CPUT    TN_LEFT+1
        BCTR,UN TPA2
TPA1:   EORZ    R0
        CPUT    TN_LEFT
        EORZ    R0
        CPUT    TN_LEFT+1
        BCTA,UN TPA3            ; (the time is up: the phase's clock stands)
TPA2:   LODA,R0 H_ELAPSED+1     ; elapsed += w
        ADDA,R0 T1
        STRA,R0 T3
        PPSL    PSLWC
        LODA,R0 H_ELAPSED
        ADDA,R0 T0
        CPSL    PSLWC
        CPUT    H_ELAPSED
        LODA,R0 T3
        CPUT    H_ELAPSED+1
TPA3:   LODA,R0 H_T+1           ; t += w (the auction lines step on it)
        ADDA,R0 T1
        STRA,R0 T3
        PPSL    PSLWC
        LODA,R0 H_T
        ADDA,R0 T0
        CPSL    PSLWC
        CPUT    H_T
        LODA,R0 T3
        CPUT    H_T+1
        LODA,R0 TN_ELAPSED+1    ; the turn's elapsed += w
        ADDA,R0 T1
        STRA,R0 T3
        PPSL    PSLWC
        LODA,R0 TN_ELAPSED
        ADDA,R0 T0
        CPSL    PSLWC
        CPUT    TN_ELAPSED
        LODA,R0 T3
        CPUT    TN_ELAPSED+1
        LODA,R0 H_PHASE         ; the grant: the server's cursor
        COMI,R0 PHASE_GRANT
        BCFA,EQ TPDONE
; as the server's: it waits (-1) for the ship, then steps every 12; next
; is the jiffies to its next step
        LODA,R0 G_CURSOR
        BCFA,LT TPG2            ; started
        LODA,R0 G_NEXT          ; still waiting: next > w?
        COMA,R0 T0
        BCTR,GT TPG1
        BCTR,LT TPG0
        LODA,R0 G_NEXT+1
        COMA,R0 T1
        BCTR,GT TPG1
TPG0:   LODA,R0 T1              ; no: w -= next, the cursor at 0
        SUBA,R0 G_NEXT+1
        STRA,R0 T1
        PPSL    PSLWC
        LODA,R0 T0
        SUBA,R0 G_NEXT
        CPSL    PSLWC
        STRA,R0 T0
        EORZ    R0
        CPUT    G_CURSOR
        EORZ    R0
        CPUT    G_NEXT
        LODI,R0 12
        CPUT    G_NEXT+1
        BCTR,UN TPG2
TPG1:   LODA,R0 G_NEXT+1        ; yes: next -= w
        SUBA,R0 T1
        STRA,R0 T3
        PPSL    PSLWC
        LODA,R0 G_NEXT
        SUBA,R0 T0
        CPSL    PSLWC
        CPUT    G_NEXT
        LODA,R0 T3
        CPUT    G_NEXT+1
        BCTA,UN TPDONE
TPG2:   LODA,R0 T1              ; v = w + 12 - next + cursor * 12
        ADDI,R0 12              ;  (T2:T3 first: MUL8 takes T0)
        STRA,R0 T3
        PPSL    PSLWC
        LODA,R0 T0
        ADDI,R0 0
        CPSL    PSLWC
        STRA,R0 T2
        LODA,R0 T3
        SUBA,R0 G_NEXT+1
        STRA,R0 T3
        PPSL    PSLWC
        LODA,R0 T2
        SUBA,R0 G_NEXT
        CPSL    PSLWC
        STRA,R0 T2
        LODA,R2 G_CURSOR
        LODI,R1 12
        ZBSR    *ZMUL8
        LODZ    R2
        ADDA,R0 T3
        STRZ    R2
        PPSL    PSLWC
        LODZ    R1
        ADDA,R0 T2
        CPSL    PSLWC
        STRZ    R1
        LODI,R3 12              ; cursor = v / 12 (45 at most), next = 12 - v % 12
        ZBSR    *ZDIV8
        LODZ    R1
        STRA,R0 T3
        LODZ    R2
        COMI,R0 45
        BCTR,LT TPG3
        LODI,R0 45
TPG3:   CPUT    G_CURSOR
        LODI,R0 12
        SUBA,R0 T3
        CPUT    G_NEXT+1
        EORZ    R0
        CPUT    G_NEXT
TPDONE: EORZ    R0
        STRA,R0 ACT
        LODI,R0 NH_STATE
        STRA,R0 NHAVE
        RETC,UN

        INCLUDE "build/teststate.inc"
