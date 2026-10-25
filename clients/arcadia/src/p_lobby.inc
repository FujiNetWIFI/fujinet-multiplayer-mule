; p_lobby.inc -- the lobby (mekkogx status.c lobby_*): the four seats, each
; a band with its colonist standing (a sprite), its name, species and
; ready; the countdown once everyone is in. The disc changes our species,
; FIRE toggles ready; Option leaves (fixb.inc, no question here).
;
;   row 1  M.U.L.E.          row 2  TOURNAMENT
;   rows 4-6, 8-10, 12-14, 16-18: seat k at row 4 + 4k: the figure at
;          column 1, the name at column 3, the species under it, READY or
;          COMPUTER under that; our seat in corners
;   rows 20-23 the help, rows 24-25 the countdown or "waiting"

        BCTA,UN LBENTER         ; E_ENTER
        BCTA,UN LBSTATE         ; E_STATE
        BCTA,UN LBTICK          ; E_TICK

        ZONE    UNIONL,UNIONLE
        VAR     LBLAST,4        ; each seat as drawn: species + flags * 8 (255: again)
        VAR     LBFIG,4         ; the species in each seat's glyph (255: none)
        VAR     LBSENT,1        ; 1: the disc is held after a species change
        VAR     LBCD,1          ; the seconds shown ($FE nothing yet, $FF waiting)
        VAR     LBMINE,1        ; our species, as we last asked

LBMSG   EQU     24              ; the countdown's rows

LBENTER: BSTA,UN SCRG26
        LODI,R0 SCR_TEXT
        LPUT    SCRNOW
        LODI,R0 LBMSG
        LPUT    MSGROW
        LODI,R0 2
        LPUT    MSGN
        LODI,R3 0               ; every seat drawn afresh, no glyphs loaded
LBE1:   STRA,R3 T3
        LODI,R0 255
        ADDI,R3 LBLAST&$FF
        ZBSR    *ZLPUT
        LODA,R3 T3
        LODI,R0 255
        ADDI,R3 LBFIG&$FF
        ZBSR    *ZLPUT
        LODA,R3 T3
        ADDI,R3 1
        COMI,R3 4
        BCFR,EQ LBE1
        EORZ    R0
        LPUT    LBSENT
        LODI,R0 $FE
        LPUT    LBCD
        DAT     1,4             ; the title
        DCOL    CMAGEN
        TXTAT   SMULE
        DAT     2,3
        DCOL    CCYAN
        TXTAT   STOURN
        DCOL    CWHITE
        LODI,R1 M_LOBBYHELP     ; the help
        LODI,R2 20
        LODI,R3 4
        ZBSR    *ZSAYBX
        LODA,R0 H_MYSEAT        ; our species, to step from
        COMI,R0 4
        BCTR,LT LBE2
        RETC,UN
LBE2:   BSTR,UN LBREC
        LODA,R0 H_PLAYERS+PL_SPECIES,R3
        ANDI,R0 7
        LPUT    LBMINE
        RETC,UN

; LBREC: R3 = seat R0's record. Leaf.
LBREC:  STRZ    R3
        LODA,R0 LBRECS,R3
        STRZ    R3
        RETC,UN
LBRECS: DB      0,29,58,87

; ---- a state: the seats that changed (lobby_state) -----------------------------
LBSTATE: LODI,R0 0
LBS1:   STRA,R0 T3              ; T3: the seat
        BSTR,UN LBREC
        LODA,R0 H_PLAYERS+PL_FLAGS,R3
        RRL,R0
        RRL,R0
        RRL,R0
        ANDI,R0 $F8
        STRA,R0 T2
        LODA,R0 H_PLAYERS+PL_SPECIES,R3
        ANDI,R0 7
        IORA,R0 T2
        STRA,R0 T2              ; T2: what the band shows
        LODA,R0 H_PLAYERS+PL_NAME,R3
        BCTR,EQ LBS2            ; (an open seat: drawn every time)
        LODA,R3 T3
        LODA,R0 LBLAST,R3
        COMA,R0 T2
        BCTR,EQ LBS3
LBS2:   LODA,R3 T3
        LODA,R0 T2
        ADDI,R3 LBLAST&$FF
        ZBSR    *ZLPUT
        BSTR,UN LBBAND
LBS3:   LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFR,EQ LBS1
        RETC,UN

; LBBAND: seat T3's band. Clobbers R0-R3, T0-T2.
LBBAND: LODA,R0 T3              ; T1: its first row (4 + 4k)
        RRL,R0
        RRL,R0
        ADDI,R0 4
        STRA,R0 T1
        LODA,R1 T1              ; cleared
        LODI,R3 3
        ZBSR    *ZDCLR
        LODA,R0 T3
        BSTA,UN LBREC
        STRA,R3 T0              ; T0: its record
        LODA,R0 H_PLAYERS+PL_NAME,R3
        BCFA,EQ LBB2
        LODA,R1 T1              ; open
        LODI,R2 2
        ZBSR    *ZDPOS
        DCOL    CWHITE
        TXTAT   SOPEN
        LODA,R1 T3
        ZBRR    *ZSPRHID
LBB2:   LODA,R0 H_PLAYERS+PL_COLOR,R3   ; its ink
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATINK,R3
        STRA,R0 DCOLOR
        LODA,R1 T1              ; the name
        LODI,R2 2
        ZBSR    *ZDPOS
        LODA,R1 T3
        ZBSR    *ZDNAME
        LODA,R1 T1              ; the species
        ADDI,R1 1
        LODI,R2 2
        ZBSR    *ZDPOS
        DCOL    CWHITE
        LODA,R3 T0
        LODA,R0 H_PLAYERS+PL_SPECIES,R3
        ANDI,R0 7
        STRZ    R3
        LODA,R0 SPECTAB,R3
        STRZ    R1
        LODI,R0 SPECNM>>8
        ZBSR    *ZDTXT
        LODA,R1 T1              ; computer, or ready
        ADDI,R1 2
        LODI,R2 2
        ZBSR    *ZDPOS
        LODA,R3 T0
        LODA,R0 H_PLAYERS+PL_FLAGS,R3
        TMI,R0  PF_AI
        BCFR,EQ LBB3
        TXTAT   SCOMP
        BCTR,UN LBB4
LBB3:   TMI,R0  PF_READY
        BCFR,EQ LBB4
        DCOL    CCYAN
        TXTAT   SREADY
LBB4:   LODA,R0 H_MYSEAT        ; ours: in corners
        COMA,R0 T3
        BCFR,EQ LBB5
        DCOL    CWHITE
        LODA,R1 T1
        LODI,R2 $FF
        ZBSR    *ZDPOS
        LODI,R0 GCNRTL
        ZBSR    *ZDPUTC
        LODI,R2 14
        LODI,R0 GCNRTR
        ZBSR    *ZDPUTC
        LODA,R1 T1
        ADDI,R1 2
        LODI,R2 $FF
        ZBSR    *ZDPOS
        LODI,R0 GCNRBL
        ZBSR    *ZDPUTC
        LODI,R2 14
        LODI,R0 GCNRBR
        ZBSR    *ZDPUTC
LBB5:   LODA,R3 T0              ; its colonist: standing at column 1, 16 lines
        LODA,R0 H_PLAYERS+PL_COLOR,R3
        ANDI,R0 3
        STRZ    R3
        LODA,R0 SEATSPR,R3
        STRZ    R2
        LODA,R1 T3
        ZBSR    *ZSPRCOL
        LODA,R0 T1              ; line: row * 8
        RRL,R0
        RRL,R0
        RRL,R0
        STRZ    R2
        LODA,R1 T3
        LODI,R3 8
        ZBRR    *ZSPRSET

; ---- a frame (lobby_tick) ------------------------------------------------------------
LBTICK: BSTA,UN LBFIGS
        BSTA,UN LBCOUNT
        LODA,R0 H_MYSEAT        ; the rest is a player's
        COMI,R0 4
        RETC,GT
        RETC,EQ
        LODA,R0 INDIR           ; species: left or right, once a push
        ANDI,R0 D_LEFT|D_RIGHT
        BCFR,EQ LBT1
        LPUT    LBSENT
        BCTR,UN LBT3
LBT1:   STRA,R0 T2
        LODA,R0 LBSENT
        BCFR,EQ LBT3
        LODI,R0 1
        LPUT    LBSENT
        LODA,R0 LBMINE          ; right: +1
        ADDI,R0 1
        LODA,R1 T2
        COMI,R1 D_RIGHT
        BCTR,EQ LBT2
        LODA,R0 LBMINE          ; left: -1
        ADDI,R0 7
LBT2:   ANDI,R0 7
        LPUT    LBMINE
        LODI,R0 ACT_SPEC
        LODI,R1 0
        LODA,R2 LBMINE
        ZBSR    *ZACT
        SND     SND_SELECT
LBT3:   LODA,R0 EVBTN           ; FIRE: ready, or not
        RETC,EQ
        LODA,R0 H_MYSEAT
        BSTA,UN LBREC
        LODA,R0 H_PLAYERS+PL_FLAGS,R3
        LODI,R1 ACT_READY
        TMI,R0  PF_READY
        BCFR,EQ LBT4
        LODI,R1 ACT_UNRDY
LBT4:   LODZ    R1
        LODI,R1 0
        LODI,R2 0
        ZBSR    *ZACT
        SND     SND_SELECT
        RETC,UN

; LBFIGS: a seat whose colonist changed gets its picture (the queue takes
; three a frame; the rest wait). Clobbers R0-R3, T0-T3.
LBFIGS: LODI,R0 0
LBF1:   STRA,R0 T3
        LODA,R0 UDCQN
        COMI,R0 3
        RETC,EQ
        RETC,GT
        LODA,R0 T3
        BSTA,UN LBREC
        LODA,R0 H_PLAYERS+PL_NAME,R3
        BCTR,EQ LBF2
        LODA,R0 H_PLAYERS+PL_SPECIES,R3
        ANDI,R0 7
        STRA,R0 T2
        LODA,R3 T3
        LODA,R0 LBFIG,R3
        COMA,R0 T2
        BCTR,EQ LBF2
        LODA,R0 T2              ; noted, then queued
        ADDI,R3 LBFIG&$FF
        ZBSR    *ZLPUT
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
        STRZ    R3              ; R0:R1 = it
        LODZ    R2
        STRZ    R1
        LODZ    R3
        LODA,R2 T3
        LODI,R3 PG_ART
        ZBSR    *ZUDCQ
LBF2:   LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFA,EQ LBF1
        RETC,UN

; LBCOUNT: the countdown in seconds, said when it changes; "waiting" (or
; "watching") while there is none. The alert as it begins.
LBCOUNT: LODA,R0 H_COUNTDOWN
        TMI,R0  $80
        BCTA,EQ LBC5            ; (-1: waiting)
        LODA,R1 RXFRH           ; (countdown - frames since the state) / 60
        LODA,R2 RXFRL
        ZBSR    *ZSINCE
        LODA,R0 H_COUNTDOWN+1
        SUBZ    R2
        STRZ    R2
        PPSL    PSLWC
        LODA,R0 H_COUNTDOWN
        SUBZ    R1
        CPSL    PSLWC
        STRZ    R1
        TMI,R1  $80
        BCFR,EQ LBC1
        LODI,R1 0               ; (gone by: 0)
        LODI,R2 0
LBC1:   LODI,R3 60
        ZBSR    *ZDIV8
        LODZ    R2
        COMA,R0 LBCD
        RETC,EQ
        STRA,R0 T3
        LODA,R0 LBCD            ; just begun: the alert
        COMI,R0 $FE
        BCTR,LT LBC2
        LODA,R0 H_MYSEAT
        COMI,R0 4
        BCFR,LT LBC2
        SND     SND_ALERT
LBC2:   LODA,R0 T3
        LPUT    LBCD
        LODI,R1 M_LEAVEHELP
        LODI,R2 0
        LODA,R3 T3
        ZBRR    *ZSAYV
LBC5:   LODA,R0 LBCD
        COMI,R0 $FF
        RETC,EQ
        LODI,R0 $FF
        LPUT    LBCD
        LODI,R1 M_WAITING
        LODA,R0 H_MYSEAT
        COMI,R0 4
        BCTR,LT LBC6
        LODI,R1 M_SPECTATE
LBC6:   ZBRR    *ZSAY

SEATINK: DB     CBLUE,CMAGEN,CCYAN,CWHITE       ; by a player's colour
SEATSPR: DB     SBLUE|$80,SMAGEN|$80,SCYAN|$80,SWHITE|$80

SMULE:  DB      "M.U.L.E.",$FF
STOURN: DB      "TOURNAMENT",$FF
SOPEN:  DB      "OPEN",$FF
SCOMP:  DB      "COMPUTER",$FF
SREADY: DB      "READY",$FF
SPECTAB: DB     SPM&$FF,SPG&$FF,SPP&$FF,SPB&$FF,SPS&$FF,SPF&$FF,SPL&$FF,SPH&$FF
SPECNM:
SPM:    DB      "MECHTRON",$FF
SPG:    DB      "GOLLUMER",$FF
SPP:    DB      "PACKER",$FF
SPB:    DB      "BONZOID",$FF
SPS:    DB      "SPHEROID",$FF
SPF:    DB      "FLAPPER",$FF
SPL:    DB      "LEGGITE",$FF
SPH:    DB      "HUMANOID",$FF
        IF      (SPH>>8) <> (SPM>>8)
        ERROR   "the species' names cross a 256-byte page"
        ENDIF
