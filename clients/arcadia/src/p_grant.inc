; p_grant.inc -- the land grant (mekkogx grant.c).
;
; The cursor sweeps the map a plot every 12 jiffies, skipping the store and
; owned plots; the button claims the plot under it. The server judges the
; claim as of the press (it takes a plot the cursor left up to 36 jiffies
; ago). Between states the cursor steps on its own, never past the plot
; the state says a computer will stop it on (h.grantStop), and a press is
; held to the plot it was made on. Round 1 opens with the transport ship
; leaving ("back in 12 months"); the server holds the cursor for it.
;
; The cursor is sprite 3: a plot's outline, white, 8 x 16.

        BCTA,UN GRENTER         ; E_ENTER
        BCTA,UN GRSTATE         ; E_STATE
        BCTA,UN GRTICK          ; E_TICK

        INCLUDE "src/grram.inc"

SLOTCUR EQU     3

GRENTER: BSTA,UN MAPSCR         ; the map, its title and message rows
        EORZ    R0
        LPUT    MAPDOTS
        LODI,R0 AN_GRANT
        LPUT    ANIM
        EORZ    R0
        LPUT    GSHOW
        EORZ    R0
        LPUT    GCLAIMED
        EORZ    R0
        LPUT    GDEPART
        EORZ    R0
        LPUT    GPARK
        LODI,R0 255
        LPUT    GCUR
        LODI,R0 255
        LPUT    GLIM
        LODI,R0 255
        LPUT    GPRESS
        LODI,R0 255
        LPUT    GWANT
        LODI,R0 255
        LPUT    GWAS
        BSTA,UN GRCURS
        LODA,R0 H_ROUND         ; round 1: the ship leaves the store first
        COMI,R0 1
        BCFR,EQ GRE1
        BSTA,UN PENOW           ; (while the phase is young: 700 jiffies)
        COMI,R1 700>>8
        BCTR,GT GRE1
        BCTR,LT GRE0
        COMI,R2 700&$FF
        BCFR,LT GRE1
GRE0:   LODI,R0 1
        LPUT    GDEPART
        LODI,R1 T_TRANSPORT
        BSTA,UN TITLE
        LODI,R1 M_DEPART
        ZBSR    *ZSAY
        LODI,R0 FX_DEPART
        BCTA,UN FXSTART         ; tail call
GRE1:   LODI,R1 T_LANDGRANT
        BSTA,UN TITLE
        LODI,R1 M_SELECT
        ZBSR    *ZSAY
        BCTA,UN ALERTME

; GRCURS: the cursor's picture (the ship's slices took the sprites' glyphs),
; white, 16 lines, off for now. Clobbers R0-R3.
GRCURS: LODI,R0 ART_CURS>>8
        LODI,R1 ART_CURS&$FF
        LODI,R2 SLOTCUR
        LODI,R3 PG_ART
        ZBSR    *ZUDCQ
        LODI,R1 SLOTCUR
        LODI,R2 SWHITE|$80
        ZBSR    *ZSPRCOL
        LODI,R1 SLOTCUR
        ZBRR    *ZSPRHID

; The ship has gone: the grant proper begins.
DEPDONE: BSTR,UN GRCURS
        EORZ    R0
        LPUT    GDEPART
        EORZ    R0
        LPUT    FXON
        LODI,R1 T_LANDGRANT
        BSTA,UN TITLE
        LODI,R1 M_SELECT
        ZBSR    *ZSAY
        LODI,R0 1
        LPUT    GSHOW
; ALERTME: the alert, for a player with a seat.
ALERTME: LODA,R0 H_MYSEAT
        COMI,R0 4
        RETC,GT
        RETC,EQ
        SND     SND_ALERT
        RETC,UN

; NEXTFREE: R1 = the free plot after R1 (45 past the last). Leaf. Clobbers R0.
NEXTFREE: ADDI,R1 1
        COMI,R1 NUM_PLOTS
        RETC,EQ
        RETC,GT
        COMI,R1 STORE_PLOT
        BCTR,EQ NEXTFREE
        LODA,R0 MP_OWNER,R1
        COMI,R0 4
        BCFR,LT NFR9            ; nobody's
        BCTR,UN NEXTFREE
NFR9:   RETC,UN

; ---- a state (grant_state) -------------------------------------------------------
GRSTATE:
        LODA,R0 G_STEP          ; the server's cursor: -1 before, 45 when done
        BCFR,EQ GS1
        LODI,R0 12
GS1:    LPUT    GSTEP
        LODA,R0 H_MYSEAT        ; ---- our claim ----
        COMI,R0 4
        BCFA,LT GS10
        STRZ    R3
        LODA,R0 G_CLAIMED,R3
        STRA,R0 T2              ; T2: the plot the server says we have
        COMI,R0 NUM_PLOTS
        BCFR,LT GS2
        LODI,R0 1               ; ours: no more waiting
        LPUT    GCLAIMED
        LODI,R0 255
        LPUT    GWANT
GS2:    LODA,R0 GWANT
        COMI,R0 NUM_PLOTS
        BCFA,LT GS10            ; nothing out
        LODA,R0 T2
        COMI,R0 NUM_PLOTS
        BCTA,LT GS10
; the state answering the claim judges it; those before were made before
; the server saw it (its cursor may have gone on, a computer's plot may
; still be ours)
        LODA,R0 ACTSENT
        COMI,R0 ACT_CLAIM
        BCFR,EQ GS4
        LODA,R0 H_ERR           ; turned down: the server's cursor wasn't
        BCTR,EQ GS3             ;  there yet (behind ours) -- or no
        LODA,R0 G_CURSOR
        BCTR,LT GS2A            ; (-1: behind)
        COMA,R0 GWANT
        BCFR,LT GS3
GS2A:   LODI,R0 1
        LPUT    GPARK
        BCTA,UN GS10
GS3:    BCTA,UN GSAGAIN
GS4:    LODA,R0 GPARK           ; parked: when the server's cursor gets there
        BCTA,EQ GS10
        LODA,R0 ACT
        BCFA,EQ GS10
        LODA,R0 G_CURSOR
        BCTA,LT GS10
        COMA,R0 GWANT
        BCTA,LT GS10
        EORZ    R0
        LPUT    GPARK
        LODA,R0 G_CURSOR
        COMI,R0 NUM_PLOTS
        BCTR,EQ GSAGAIN
        LODI,R0 ACT_CLAIM
        LODI,R1 0
        LODA,R2 GWANT
        ZBSR    *ZACT
        BCTA,UN GS10
GSAGAIN: LODI,R0 255            ; turned down: press again
        LPUT    GWANT
        EORZ    R0
        LPUT    GCLAIMED
        EORZ    R0
        LPUT    GPARK
        SND     SND_ERROR
; ---- the cursor (while our claim is out it waits on its plot) ----
GS10:   BSTA,UN DUE             ; T0:T1 = the frame the server's next step is due
        LODA,R0 GWANT
        COMI,R0 NUM_PLOTS
        BCTA,LT GS20
        LODA,R1 G_CURSOR
        COMA,R1 GCUR
        BCFR,EQ GS11
        BSTA,UN SETDUE          ; ours is the server's: in step with it
        BCTA,UN GS20
; behind us, with no claim since (made before our last steps, or late off
; a stalled line): wait here for the server's cursor to come and go rather
; than jump back
GS11:   LODA,R0 GCUR
        COMI,R0 NUM_PLOTS
        BCFA,LT GS13
        BSTA,UN CLAIMSAME
        BCFA,EQ GS13
        LODI,R3 0               ; R3: steps from the server's cursor to ours
        LODA,R1 G_CURSOR
GS12:   STRA,R3 T3
        BSTA,UN NEXTFREE
        LODA,R3 T3
        ADDI,R3 1
        COMI,R3 8
        BCTR,EQ GS12A
        COMA,R1 GCUR
        BCTR,LT GS12
GS12A:  COMA,R1 GCUR
        BCFR,EQ GS13
        STRA,R3 T3              ; due = server's due + steps * step
        LODA,R0 T0              ; (MUL8 takes T0)
        STRA,R0 T2
        LODA,R1 GSTEP
        LODA,R2 T3
        ZBSR    *ZMUL8
        LODZ    R2
        ADDA,R0 T1
        STRA,R0 T1
        PPSL    PSLWC
        LODZ    R1
        ADDA,R0 T2
        CPSL    PSLWC
        STRA,R0 T0
        BSTA,UN SETDUE
        BCTR,UN GS20
GS13:   LODA,R0 G_CURSOR        ; a claim (ours, or another's the cursor went
        LPUT    GCUR            ;  back for): the server's cursor
        BSTA,UN SETDUE
        LODI,R0 255
        LPUT    GWAS
GS20:   LODA,R0 H_GRANTSTOP     ; how far it may go: the computer's stop
        BCTR,EQ GS21
        SUBI,R0 1
        BCTR,UN GS22
GS21:   LODA,R1 G_CURSOR        ; (an older server: one step past its cursor)
        BSTA,UN NEXTFREE
        LODZ    R1
GS22:   LPUT    GLIM
        LODA,R0 GDEPART         ; the server's cursor has started: the ship
        BCTR,EQ GS23            ;  has gone too
        LODA,R0 G_CURSOR
        COMI,R0 NUM_PLOTS
        BCFR,LT GS23
        BSTA,UN DEPDONE
GS23:   LODA,R0 GDEPART
        RETC,GT
        LODI,R0 1
        LPUT    GSHOW
        RETC,UN

; CLAIMSAME: CC EQ when no seat's claim changed since we drew them (the
; map shows a claim as its owner's plot: the screen is the last state).
CLAIMSAME: LODI,R3 0
CLS1:   LODA,R0 G_CLAIMED,R3
        COMI,R0 NUM_PLOTS
        BCFR,LT CLS2
        STRZ    R2              ; a claimed plot must show its claimer
        LODA,R0 PLOFFT,R2
        STRZ    R2
        LODA,R0 SCR0+16,R2      ; (its B cell: anything owned is not a
        ANDI,R0 $3F             ;  space)
        BCTR,EQ CLS9            ; not drawn yet: changed
CLS2:   ADDI,R3 1
        COMI,R3 4
        BCFR,EQ CLS1
        TMI,R3  0               ; the same: EQ
        RETC,UN
CLS9:   LODI,R0 1               ; (not EQ)
        RETC,UN

; DUE: T0:T1 = rx_frame + the server's next (the frame its step is due).
DUE:    LODA,R0 RXFRL
        ADDA,R0 G_NEXT+1
        STRA,R0 T1
        PPSL    PSLWC
        LODA,R0 RXFRH
        ADDA,R0 G_NEXT
        CPSL    PSLWC
        STRA,R0 T0
        RETC,UN
SETDUE: LODA,R0 T0
        LPUT    GDUEH
        LODA,R0 T1
        LPUT    GDUEL
        RETC,UN

; ---- a frame (grant_tick, grant_isr) -----------------------------------------------
GRTICK: LODA,R0 GDEPART
        BCTR,EQ GRT1
        LODA,R0 FXFLAG          ; the ship has gone
        COMI,R0 1
        BCFA,EQ GRT9
        BSTA,UN DEPDONE
GRT1:   LODA,R0 EVBTN           ; the button: the plot the cursor was on
        BCTA,EQ GRT5            ;  as it went down
        LODA,R0 GPRESS
        COMI,R0 255
        BCFR,EQ GRT3
        LODA,R0 GSHOW
        BCTR,EQ GRT3
        LODA,R0 GCUR
        COMI,R0 NUM_PLOTS
        BCFR,LT GRT3
        STRZ    R2              ; just stepped (a frame ago): the one before
        LODA,R0 FRML
        SUBA,R0 GSTEPF
        COMI,R0 1
        BCTR,GT GRT2
        LODA,R0 GWAS
        COMI,R0 NUM_PLOTS
        BCFR,LT GRT2
        STRZ    R2
GRT2:   LODZ    R2
        LPUT    GPRESS
GRT3:   LODA,R0 GPRESS          ; claim it, the cursor held there until the
        COMI,R0 NUM_PLOTS       ;  server answers
        BCTR,LT GRT4
        LODA,R0 GCUR
GRT4:   STRA,R0 T2
        LODI,R0 255
        LPUT    GPRESS
        LODA,R0 GCLAIMED
        BCFR,EQ GRT5
        LODA,R0 H_MYSEAT
        COMI,R0 4
        BCFR,LT GRT5
        LODA,R0 T2
        COMI,R0 NUM_PLOTS
        BCFR,LT GRT5
        LODI,R0 ACT_CLAIM
        LODI,R1 0
        LODA,R2 T2
        ZBSR    *ZACT
        LODI,R0 1
        LPUT    GCLAIMED
        LODA,R0 T2
        LPUT    GWANT
        LODA,R0 T2
        LPUT    GCUR
        LODI,R0 255
        LPUT    GWAS
        SND     SND_BUY
GRT5:   LODA,R0 DT              ; the cursor: once a frame gone by
        STRA,R0 UNIONB
GRT6:   BSTR,UN GRISR
        LODA,R0 UNIONB
        SUBI,R0 1
        STRA,R0 UNIONB
        BCFR,EQ GRT6
GRT9:   RETC,UN

; GRISR: the cursor steps to the next free plot when its step is due,
; blanking that frame, as the Atari's does.
GRISR:  LODA,R0 GCUR
        COMI,R0 NUM_PLOTS
        BCFA,LT GRI3
        LODA,R0 GWANT
        COMI,R0 NUM_PLOTS
        BCTA,LT GRI3
        LODA,R1 GDUEH           ; due yet? (frame - due >= 0)
        LODA,R2 GDUEL
        ZBSR    *ZSINCE
        TMI,R1  $80
        BCTR,EQ GRI3
        LODA,R1 GCUR
        BSTA,UN NEXTFREE
        COMA,R1 GLIM
        BCTR,GT GRI3
        STRA,R1 T2              ; a step (LPUT takes R1)
        LODA,R0 GCUR
        LPUT    GWAS
        LODA,R0 T2
        LPUT    GCUR
        LODA,R0 GDUEL
        ADDA,R0 GSTEP
        STRA,R0 T1
        PPSL    PSLWC
        LODA,R0 GDUEH
        ADDI,R0 0
        CPSL    PSLWC
        STRA,R0 T0
        BSTA,UN SETDUE
        LODA,R0 FRML
        LPUT    GSTEPF
        LODI,R1 SLOTCUR
        ZBRR    *ZSPRHID
GRI3:   LODA,R0 GCUR            ; drawn on its plot, or off
        COMI,R0 NUM_PLOTS
        BCFR,LT GRI4
        LODA,R0 GSHOW
        BCTR,EQ GRI4
        LODA,R2 GCUR
        LODA,R0 PLROW,R2        ; line 16 + 16 * row
        STRA,R0 T0
        LODA,R0 PLCOL,R2        ; column 24 + 8 * column
        STRZ    R3
        LODA,R2 T0
        LODI,R1 SLOTCUR
        ZBRR    *ZSPRSET
GRI4:   LODI,R1 SLOTCUR
        ZBRR    *ZSPRHID

        INCLUDE "src/mapscr.inc"
        INCLUDE "src/plots.inc"
