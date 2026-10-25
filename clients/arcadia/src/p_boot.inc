; p_boot.inc -- the boot page: the title, who we are, which table (mekkogx
; boot.c, main.c's title and leave_ask); and, from a game, another table,
; "JOINING..." and the leave question.
;
; The FujiNet lobby hands a game its details in appkeys (creator 1, app 1):
;   key 0    the player's name (every FujiNet game's)
;   key 10   the table's URL, "http://host:port/?table=<id>"
; Without them it asks for a name and lists the build's server's tables.
; The table chosen goes back to key 10, so starting again rejoins it.

        BCTA,UN BOOT            ; E_ENTER: the title, then the name and table
        RETC,UN                 ; E_STATE
        NOP
        NOP
        RETC,UN                 ; E_TICK
        NOP
        NOP
        BCTA,UN JOINING         ; E_JOIN
        RETC,UN                 ; +3
        NOP
        NOP
        BCTA,UN RECHOOSE        ; E_RECHO: the server turned us away
        BCTA,UN LEAVEQ          ; E_LEAVE: Option in a game

NAMEMAX EQU     8
BOOTROW EQU     12              ; the boot's message row

; ---- the start ---------------------------------------------------------------
; (the title page has been shown: MAININIT) The offline test mode needs no
; network; else the name and the table.
BOOT:   LODA,R0 TESTM
        RETC,GT
        BCTA,UN BTNET

; back to plain text rows
TEXTSCR: BSTA,UN SCRG26
        RETC,UN

; ---- the name and the table --------------------------------------------------
BTNET:  BSTA,UN TEXTSCR
        LODI,R1 BOOTROW
        LODI,R2 STCONN
        BSTA,UN CENTRE
        BSTA,UN AKLOAD          ; the lobby's appkeys
        LODI,R3 SCNAME-$2900    ; (RAM-H reads go through PRAMH)
        LODA,R0 *PRAMH,R3
        BCFR,EQ BTN1
        BSTA,UN NAMEIN          ; no name: ask
BTN1:   LODI,R3 SCTABLE-$2900
        LODA,R0 *PRAMH,R3
        BCFR,EQ BTN2
        BCTA,UN TABLESEL        ; no table: choose (and keep it)
BTN2:   BCTA,UN AKSAVER         ; tail call: the room kept for next time

; ---- JOINING... (main.c game_start) ---------------------------------------------
JOINING: BSTA,UN TEXTSCR
        LODI,R1 BOOTROW
        LODI,R2 SJOIN
        BCTA,UN CENTRE

; ---- another table (main.c choose_table) -------------------------------------------
RECHOOSE: BSTA,UN TEXTSCR
        LODI,R0 0               ; the room we had is forgotten
        LODI,R3 SCTABLE-$2900
        BSTA,UN RAMHPUT
        BSTA,UN AKCLRR
        BCTA,UN TABLESEL        ; (which keeps it)

; ---- the leave question (main.c leave_ask) ---------------------------------------------
; The game stands still while we ask: Option again leaves (the server
; gives the computer our seat), anything else carries on. A spectator,
; or anyone in the lobby, just goes.
LEAVEQ: LODA,R0 H_MYSEAT
        COMI,R0 4
        BCFR,LT LEAVE
        LODA,R0 H_PHASE         ; (the lobby: no game to lose)
        BCTR,EQ LEAVE
        LODI,R1 M_LEAVEASK
        ZBSR    *ZSAY
LVQ1:   BSTA,UN VBSYNC
        BSTA,UN TICKRD
        BSTA,UN KEYDEC
        LODA,R0 EVBTN
        BCFR,EQ LVQ2
        LODA,R0 EVKEY
        COMI,R0 K_NONE
        BCTR,EQ LVQ1
        COMI,R0 K_OPT
        BCTR,EQ LEAVE
LVQ2:   LODA,R1 MSGROW          ; carry on: the question off
        LODA,R3 MSGN
        BCTA,UN DCLR
LEAVE:  EORZ    R0
        CPUT    STOK
        LODI,R0 ACT_LEAVE
        BSTA,UN NETSYNC
        BSTA,UN RECHOOSE
        BCTA,UN GAMESTART

; NETSYNC: send action R0 and wait for its answer (a poll on its way is
; seen through first). R0 = NHAVE. Calls four down. Clobbers R0-R3, T*.
NETSYNC: STRA,R0 T2
NS1:    LODA,R0 NSTATE          ; a request under way: through first
        BCTR,EQ NS2
        BSTA,UN VBSYNC
        BSTA,UN NSTEP
        BCTR,UN NS1
NS2:    LODA,R0 T2
        STRA,R0 ACT
        BSTA,UN NBEGIN
NS3:    BSTA,UN VBSYNC
        BSTA,UN TICKRD
        BSTA,UN NSTEP
        BCTR,EQ NS3
        LODA,R0 NHAVE
        RETC,UN

; ---- the appkeys (net.c) -----------------------------------------------------------
AKNAME  EQU     0
AKROOM  EQU     10

; AKOPEN: open key R1 (R2 = 0 read, 1 write). CC EQ when it opened.
; OPEN_APPKEY's payload is six raw bytes: creator lo, hi, app, key, mode,
; and a reserved 0. Calls two down. Clobbers R0-R3.
AKOPEN: STRA,R2 T0
        STRA,R1 T1
        LODI,R3 FCOKEY
        LODI,R2 0
        BSTA,UN MSTARTF
        LODI,R1 1
        LODA,R0 *PTXD,R1
        LODI,R1 0
        LODA,R0 *PTXD,R1
        LODI,R1 1
        LODA,R0 *PTXD,R1
        LODA,R1 T1
        LODA,R0 *PTXD,R1
        LODA,R1 T0
        LODA,R0 *PTXD,R1
        LODI,R1 0
        LODA,R0 *PTXD,R1
; AKGO: commit, wait; CC EQ on an ACK.
AKGO:   BSTA,UN MWAIT
        BCFR,EQ AKG9
        LODA,R0 *PREPLY
        COMI,R0 FCACK
AKG9:   RETC,UN
AKCLOSE: LODI,R3 FCCKEY
        LODI,R2 0
        BSTA,UN MSTARTF
        BCTR,UN AKGO

; AKREAD: open key R1 and read it: the reply's first two bytes are its
; length (LE), the data follows. R0 = the length (0: none or failed; a
; key is at most 64). Calls three down. Clobbers R0-R3, T0, T1.
AKREAD: LODI,R2 0
        BSTA,UN AKOPEN
        BCFR,EQ AKR8
        LODI,R3 FCRKEY
        LODI,R2 0
        BSTA,UN MSTARTF
        BSTR,UN AKGO
        BCFR,EQ AKR8
        LODI,R3 1               ; the length's high byte: 0, or it isn't a key
        LODA,R0 *PRDATA,R3
        BCFR,EQ AKR8
        LODA,R0 *PRDATA         ; (the window holds still until the next
        STRA,R0 T2              ;  commit: the close comes after the copy)
        RETC,UN
AKR8:   BSTR,UN AKCLOSE
        EORZ    R0
        RETC,UN

; AKLOAD: the name (A-Z and 0-9 only, upper-cased, at most 8: it goes in
; the URL unescaped) and the room ("http..." split at "?table="; the
; table letters and digits only). Calls four down.
AKLOAD: LODI,R0 0               ; nothing yet
        LODI,R3 SCNAME-$2900
        BSTA,UN RAMHPUT
        LODI,R0 0
        LODI,R3 SCTABLE-$2900
        BSTA,UN RAMHPUT
        LODI,R0 0
        LODI,R3 SCENDPT-$2900
        BSTA,UN RAMHPUT
        LODI,R1 AKNAME
        BSTR,UN AKREAD
        BCTA,EQ AKL3
        COMI,R0 NAMEMAX+1
        BCTR,LT AKL1
        LODI,R0 NAMEMAX
AKL1:   STRA,R0 T2              ; T2: characters left
        LODI,R1 FRRAMP2         ; the name into SCNAME, as it goes
        LODI,R3 SCNAME-$2900
        BSTA,UN RAMSET
        LODI,R3 1               ; reply byte 2 onwards
AKL2:   LODA,R0 *PRDATA,R3,+
        BSTA,UN UPPER
        COMI,R0 $30             ; 0-9, A-Z only
        BCTR,LT AKLBAD
        COMI,R0 $5A
        BCTR,GT AKLBAD
        COMI,R0 $3A
        BCTR,LT AKL2A
        COMI,R0 $41
        BCTR,LT AKLBAD
AKL2A:  BSTA,UN RAMPUT1
        LODA,R0 T2
        SUBI,R0 1
        STRA,R0 T2
        BCFR,EQ AKL2
        EORZ    R0              ; its end
        BSTA,UN RAMPUT1
        BCTR,UN AKL2B
AKLBAD: BSTA,UN RAMPEND         ; a character we can't send: no name
        EORZ    R0
        LODI,R3 SCNAME-$2900
        BSTA,UN RAMHPUT
AKL2B:  BSTA,UN RAMPEND
        BSTA,UN AKCLOSE
AKL3:   LODI,R1 AKROOM          ; the room
        BSTA,UN AKREAD
        BCTA,EQ AKL9
        COMI,R0 5
        BCTA,LT AKL8
        STRA,R0 T2              ; T2: characters
        LODI,R3 2
        LODA,R0 *PRDATA,R3
        COMI,R0 $68             ; 'h': a URL
        BCTA,EQ AKL4
AKL8:   BSTA,UN AKCLOSE
        BCTA,UN AKL9
AKL4:   LODI,R1 FRRAMP2         ; the endpoint: up to '?' (64 at most)
        LODI,R3 SCENDPT-$2900
        BSTA,UN RAMSET
        LODI,R3 1
AKL5:   LODA,R0 *PRDATA,R3,+
        COMI,R0 $3F
        BCTR,EQ AKL6
        STRA,R0 T3              ; (the last one: a '/' comes off)
        BSTA,UN RAMPUT1
        LODA,R0 T2
        SUBI,R0 1
        STRA,R0 T2
        BCFR,EQ AKL5
AKL6:   STRA,R3 T1              ; where the '?' is
        EORZ    R0
        BSTA,UN RAMPUT1
        BSTA,UN RAMPEND
        BSTA,UN ENDSLASH        ; no trailing '/', so "<endpoint>/state" has one
        LODA,R0 T2              ; "table=" next?
        BCTA,EQ AKL8
        LODA,R3 T1
        LODI,R2 0
AKL7:   LODA,R0 *PRDATA,R3,+
        COMA,R0 STABLEQ,R2
        BCTA,LT AKL8
        BCTA,GT AKL8
        ADDI,R2 1
        COMI,R2 6
        BCFR,EQ AKL7
        STRA,R3 T1
        LODI,R1 FRRAMP2         ; the table: letters and digits, 8 at most
        LODI,R3 SCTABLE-$2900
        BSTA,UN RAMSET
        LODI,R2 NAMEMAX
        LODA,R3 T1
AKL7A:  LODA,R0 *PRDATA,R3,+
        STRA,R3 T1
        COMI,R0 $30
        BCTR,LT AKL7B
        COMI,R0 $7A
        BCTR,GT AKL7B
        STRA,R2 T0
        BSTA,UN RAMPUT1
        LODA,R2 T0
        LODA,R3 T1
        BDRR,R2 AKL7A
AKL7B:  EORZ    R0
        BSTA,UN RAMPUT1
        BSTA,UN RAMPEND
        BCTA,UN AKL8
AKL9:   RETC,UN
STABLEQ: DB     $74,$61,$62,$6C,$65,$3D        ; "table=" (ASCII)

; ENDSLASH: the endpoint's last character off if it is a '/'. Calls one
; down. Clobbers R0, R1, R3.
ENDSLASH: LODI,R3 SCENDPT-$2900-1
ES1:    LODA,R0 *PRAMH,R3,+
        BCFR,EQ ES1
        SUBI,R3 1
        LODA,R0 *PRAMH,R3
        COMI,R0 $2F
        RETC,LT
        RETC,GT
        EORZ    R0
; RAMHPUT: RAM-H byte $2900 + R3 = R0. Calls one down. Clobbers R0-R3.
RAMHPUT: LODI,R1 FRRAMP2
        BCTA,UN RAMPUT

; AKSAVER: key 10 = <endpoint>/?table=<t> (mekkogx appkey_save_room).
; AKCLRR: key 10 emptied. Calls three down.
AKSAVER: LODI,R1 AKROOM
        LODI,R2 1
        BSTA,UN AKOPEN
        BCFA,EQ AKCLOSE
        LODI,R3 FCWKEY
        LODI,R2 0
        BSTA,UN MSTARTF
        LODI,R3 SCENDPT-$2900   ; the endpoint, ours or the build's
        LODA,R0 *PRAMH,R3
        BCTR,EQ AKS1
        SUBI,R3 1
        BSTR,UN TXH
        BCTR,UN AKS2
AKS1:   LODI,R3 $FF
AKS1A:  LODA,R0 UENDPT,R3,+
        BCTR,EQ AKS2
        STRZ    R1
        LODA,R0 *PTXD,R1
        BCTR,UN AKS1A
AKS2:   LODI,R3 $FF             ; "/?table="
AKS3:   LODA,R0 SQTABLE,R3,+
        BCTR,EQ AKS4
        STRZ    R1
        LODA,R0 *PTXD,R1
        BCTR,UN AKS3
AKS4:   LODI,R3 SCTABLE-$2900-1
        BSTR,UN TXH
        BSTA,UN AKGO
        BCTA,UN AKCLOSE
AKCLRR: LODI,R1 AKROOM
        LODI,R2 1
        BSTA,UN AKOPEN
        BCFA,EQ AKCLOSE
        LODI,R3 FCWKEY
        LODI,R2 0
        BSTA,UN MSTARTF
        BSTA,UN AKGO
        BCTA,UN AKCLOSE
; TXH: the RAM-H string after $2900 + R3 into the TX stream. Leaf.
TXH:    LODA,R0 *PRAMH,R3,+
        RETC,EQ
        STRZ    R1
        LODA,R0 *PTXD,R1
        BCTR,UN TXH
SQTABLE: DB     $2F,$3F,$74,$61,$62,$6C,$65,$3D,0     ; "/?table=" (ASCII)

; UPPER: R0 a-z to A-Z. Leaf.
UPPER:  COMI,R0 $61
        RETC,LT
        COMI,R0 $7A
        RETC,GT
        SUBI,R0 $20
        RETC,UN

; ---- the name (boot.c name_entry, no keyboard) ------------------------------------
; Up and down turn the letter at the cursor through A-Z and 0-9 (held, it
; runs on); FIRE or right takes it, left or Clear rubs out, Enter or Start
; ends. Saved to key 0 for next time.
NAMEIN: BSTA,UN TEXTSCR
        DCOL    CWHITE
        LODI,R1 6
        LODI,R2 SYNAME
        BSTA,UN CENTRE
        LODI,R1 16
        LODI,R2 SNHELP1
        BSTA,UN CENTRE
        LODI,R1 18
        LODI,R2 SNHELP2
        BSTA,UN CENTRE
        LODI,R1 20
        LODI,R2 SNHELP3
        BSTA,UN CENTRE
        EORZ    R0
        STRA,R0 T0              ; T0: letters so far
        STRA,R0 T1              ; T1: the letter at the cursor (0-35)
        STRA,R0 T3              ; T3: the repeat count
NM1:    BSTA,UN VBSYNC
        BSTA,UN TICKRD
        BSTA,UN KEYDEC
        LODA,R0 T3
        BCTR,EQ NM2
        SUBI,R0 1
        STRA,R0 T3
NM2:    LODA,R0 EVKEY           ; ends
        COMI,R0 K_ENT
        BCTA,EQ NMEND
        COMI,R0 K_START
        BCTA,EQ NMEND
        COMI,R0 K_CLR           ; rubs out
        BCTR,EQ NMRUB
        LODA,R0 T3
        BCFR,EQ NM3
        LODA,R0 INDIR
        TMI,R0  D_LEFT
        BCTR,EQ NMRUB
NM3:    LODA,R0 EVBTN           ; takes it
        BCFR,EQ NMTAKE
        LODA,R0 T3
        BCFA,EQ NMSHOW
        LODA,R0 INDIR
        TMI,R0  D_RIGHT
        BCTR,EQ NMTAKE
        TMI,R0  D_UP
        BCTA,EQ NMUP
        TMI,R0  D_DOWN
        BCTA,EQ NMDN
        BCTA,UN NMSHOW
NMRUB:  LODA,R0 T0
        BCTA,EQ NMSHOW
        SUBI,R0 1
        STRA,R0 T0
        LODI,R0 12
        STRA,R0 T3
        SND     SND_SELECT
        BCTA,UN NMSHOW
NMTAKE: LODA,R0 T0
        COMI,R0 NAMEMAX
        BCTA,EQ NMSHOW
        ADDI,R0 SCNAME-$2900
        STRZ    R3
        LODA,R2 T1
        LODA,R0 PICKSET,R2
        BSTA,UN RAMHPUT
        LODA,R0 T0
        ADDI,R0 1
        STRA,R0 T0
        LODI,R0 12
        STRA,R0 T3
        SND     SND_SELECT
        BCTR,UN NMSHOW
NMUP:   LODA,R0 T1
        SUBI,R0 1
        BCFR,LT NMUD
        LODI,R0 35
        BCTR,UN NMUD
NMDN:   LODA,R0 T1
        ADDI,R0 1
        COMI,R0 36
        BCFR,EQ NMUD
        EORZ    R0
NMUD:   STRA,R0 T1
        LODI,R0 8
        STRA,R0 T3
NMSHOW: DAT     10,3            ; the name, then the letter blinking
        LODI,R3 SCNAME-$2900
        LODA,R1 T0
        BCTR,EQ NMS2
NMS1:   LODA,R0 *PRAMH,R3
        ADDI,R3 1
        STRA,R3 T2
        BSTA,UN A2G
        IORI,R0 CMAGEN
        BSTA,UN DGLYPH
        LODA,R3 T2
        BDRR,R1 NMS1
NMS2:   LODA,R0 FRML            ; the cursor: the letter, or a bar
        ANDI,R0 $10
        BCTR,EQ NMS3
        LODA,R2 T1
        LODA,R0 PICKSET,R2
        BSTA,UN A2G
        BCTR,UN NMS4
NMS3:   LODI,R0 GBOTBAR
NMS4:   STRA,R0 T2
        DAT     10,3
        LODA,R0 T0              ; (R2: the cell before the cursor)
        ADDZ    R2
        STRZ    R2
        LODA,R0 T2
        BSTA,UN DGLYPH
        LODI,R0 0               ; and nothing after it
        BSTA,UN DGLYPH
        BCTA,UN NM1
NMEND:  LODA,R0 T0              ; a name of at least one letter
        BCFR,EQ NME1
        LODI,R3 SCNAME-$2900
        LODA,R2 T1
        LODA,R0 PICKSET,R2
        BSTA,UN RAMHPUT
        LODI,R0 1
        STRA,R0 T0
NME1:   LODA,R0 T0              ; its end
        ADDI,R0 SCNAME-$2900
        STRZ    R3
        EORZ    R0
        BSTA,UN RAMHPUT
        SND     SND_SELECT
        LODI,R1 AKNAME          ; kept for next time
        LODI,R2 1
        BSTA,UN AKOPEN
        BCFA,EQ AKCLOSE
        LODI,R3 FCWKEY
        LODI,R2 0
        BSTA,UN MSTARTF
        LODI,R3 SCNAME-$2900-1
        BSTA,UN TXH
        BSTA,UN AKGO
        BCTA,UN AKCLOSE
PICKSET: DB     $41,$42,$43,$44,$45,$46,$47,$48,$49,$4A,$4B,$4C,$4D
        DB      $4E,$4F,$50,$51,$52,$53,$54,$55,$56,$57,$58,$59,$5A
        DB      $30,$31,$32,$33,$34,$35,$36,$37,$38,$39

; A2G: R0 ASCII to its glyph (A-Z a-z 0-9 . , + $ /; anything else a
; space). Leaf.
A2G:    BSTA,UN UPPER
        COMI,R0 $41
        BCTR,LT A2G1
        COMI,R0 $5A
        BCTR,GT A2G9
        SUBI,R0 $41-GALPHA
        RETC,UN
A2G1:   COMI,R0 $30
        BCTR,LT A2G2
        COMI,R0 $39
        BCTR,GT A2G9
        SUBI,R0 $30-GDIGIT
        RETC,UN
A2G2:   COMI,R0 $2E
        BCFR,EQ A2G3
        LODI,R0 GDOT
        RETC,UN
A2G3:   COMI,R0 $2C
        BCFR,EQ A2G4
        LODI,R0 GCOMMA
        RETC,UN
A2G4:   COMI,R0 $2F
        BCFR,EQ A2G5
        LODI,R0 GSLASH
        RETC,UN
A2G5:   COMI,R0 $24
        BCFR,EQ A2G9
        LODI,R0 GDOLLAR
        RETC,UN
A2G9:   EORZ    R0
        RETC,UN

; ---- the table (boot.c table_select) -----------------------------------------------
; /tables?bin=1: a count, then 33-byte MuleTables (id 8, name 20, players
; 5). Up and down choose, FIRE or Enter joins; the room is then kept
; (AKSAVER, a tail call, so a caller is one return shallower).
TBMAX   EQU     7               ; seven fit the reply's first slice
TABLESEL:
        BSTA,UN TEXTSCR
        DCOL    CMAGEN
        LODI,R1 0
        LODI,R2 SCHOOSE
        BSTA,UN CENTRE
        DCOL    CWHITE
        LODI,R1 BOOTROW
        LODI,R2 SLOAD
        BSTA,UN CENTRE
        LODI,R0 ACT_TABLES
        BSTA,UN NETSYNC
        COMI,R0 NH_TABLES
        BCFA,EQ TBNONE
        LODA,R0 *PRDATA         ; the count
        BCTA,EQ TBNONE
        COMI,R0 TBMAX
        BCTR,LT TB1
        LODI,R0 TBMAX
TB1:    STRA,R0 T0              ; T0: tables
        LODI,R1 BOOTROW
        LODI,R3 1
        BSTA,UN DCLR
        LODI,R1 3               ; each: its name (14 cells), its players
        LODI,R3 0               ;  under it (R3: the record's first byte,
TB2:    STRA,R1 T2              ;  less one)
        STRA,R3 T3
        BSTA,UN DPOS
        LODI,R2 1
        ADDI,R3 8               ; past the id
        LODI,R0 14
        STRA,R0 T1
TB3:    LODA,R0 *PRDATA,R3,+
        BSTA,UN A2G
        BSTA,UN DPUTC
        LODA,R0 T1
        SUBI,R0 1
        STRA,R0 T1
        BCFR,EQ TB3
        LODA,R1 T2              ; the players, "n / 4" as "n/4", under it
        ADDI,R1 1
        BSTA,UN DPOS
        LODA,R3 T3
        ADDI,R3 28
        LODI,R2 1
        LODI,R0 0
        STRA,R0 T1
TB4:    LODA,R0 *PRDATA,R3,+
        COMI,R0 $20
        BCTR,EQ TB5
        BSTA,UN A2G
        BSTA,UN DPUTC
TB5:    LODA,R0 T1
        ADDI,R0 1
        STRA,R0 T1
        COMI,R0 5
        BCFR,EQ TB4
        LODA,R3 T3
        ADDI,R3 33
        LODA,R1 T2
        ADDI,R1 2
        LODZ    R1
        SUBI,R0 3
        RRR,R0
        COMA,R0 T0
        BCFA,EQ TB2
        LODI,R1 22
        LODI,R2 STBHELP
        BSTA,UN CENTRE
        LODI,R0 0
        STRA,R0 T1              ; T1: the choice
        STRA,R0 T3              ; T3: the repeat count
TB6:    BSTA,UN VBSYNC
        BSTA,UN TICKRD
        BSTA,UN KEYDEC
        LODI,R1 0               ; the marks
TB7:    STRA,R1 T2
        LODZ    R1
        RRL,R0
        ADDI,R0 3
        STRZ    R1
        LODI,R2 $FF
        BSTA,UN DPOS
        LODA,R1 T2
        LODI,R0 0
        COMA,R1 T1
        BCFR,EQ TB8
        LODI,R0 GTRIUL|CBLUE
TB8:    BSTA,UN DGLYPH
        LODA,R1 T2
        ADDI,R1 1
        COMA,R1 T0
        BCFR,EQ TB7
        LODA,R0 T3
        BCTR,EQ TB9
        SUBI,R0 1
        STRA,R0 T3
        BCTR,UN TB11
TB9:    LODA,R0 INDIR
        TMI,R0  D_UP
        BCFR,EQ TB10
        LODA,R0 T1
        BCTR,EQ TB11
        SUBI,R0 1
        BCTR,UN TB10A
TB10:   TMI,R0  D_DOWN
        BCFR,EQ TB11
        LODA,R0 T1
        ADDI,R0 1
        COMA,R0 T0
        BCTR,EQ TB11
TB10A:  STRA,R0 T1
        LODI,R0 12
        STRA,R0 T3
        SND     SND_SELECT
TB11:   LODA,R0 EVBTN
        BCFR,EQ TB12
        LODA,R0 EVKEY
        COMI,R0 K_ENT
        BCTA,EQ TB12
        BCTA,UN TB6
TB12:   LODA,R2 T1              ; its id (8 bytes, NUL-padded) is the table
        LODI,R1 33
        BSTA,UN MUL8
        LODZ    R2
        STRA,R0 T3
        LODI,R1 FRRAMP2
        LODI,R3 SCTABLE-$2900
        BSTA,UN RAMSET
        LODI,R0 8
        STRA,R0 T2
        LODA,R3 T3
TB13:   LODA,R0 *PRDATA,R3,+
        STRA,R3 T3
        BSTA,UN RAMPUT1
        LODA,R3 T3
        LODA,R0 T2
        SUBI,R0 1
        STRA,R0 T2
        BCFR,EQ TB13
        EORZ    R0
        BSTA,UN RAMPUT1
        BSTA,UN RAMPEND
        SND     SND_SELECT
        BCTA,UN AKSAVER
TBNONE: LODI,R1 BOOTROW         ; no tables: say where, and try again
        LODI,R2 SNOTAB
        BSTA,UN CENTRE
TBN1:   BSTA,UN VBSYNC
        BSTA,UN TICKRD
        BSTA,UN KEYDEC
        LODA,R0 EVBTN
        BCFR,EQ TBN2
        LODA,R0 EVKEY
        COMI,R0 K_ENT
        BCFR,EQ TBN1
TBN2:   BCTA,UN TABLESEL

; CENTRE: the glyph string at SBOOT + R2 centred on row R1. Calls one
; down. Clobbers R0-R3.
CENTRE: LODI,R3 $FF             ; its width
CEN1:   LODA,R0 SBOOT,R2
        COMI,R0 $FF
        BCTR,EQ CEN2
        ADDI,R2 1
        ADDI,R3 1
        BCTR,UN CEN1
CEN2:   STRA,R2 T1              ; (where it ends)
        ADDI,R3 1
        LODI,R0 16
        SUBZ    R3
        ANDI,R0 $FE
        RRR,R0
        SUBI,R0 1
        STRZ    R2
        LODA,R0 T1
        SUBZ    R3
        STRA,R0 T1              ; its start
        BSTA,UN DPOS
        LODA,R3 T1
CEN3:   LODA,R0 SBOOT,R3
        COMI,R0 $FF
        RETC,EQ
        BSTA,UN DPUTC
        ADDI,R3 1
        BCTR,UN CEN3

SBOOT:
SNOTAB  EQU     $-SBOOT
        DB      "NO TABLES THERE",$FF
STCONN  EQU     $-SBOOT
        DB      "CONNECTING...",$FF
SJOIN   EQU     $-SBOOT
        DB      "JOINING...",$FF
SCHOOSE EQU     $-SBOOT
        DB      "CHOOSE A TABLE",$FF
SLOAD   EQU     $-SBOOT
        DB      "LOADING...",$FF
STBHELP EQU     $-SBOOT
        DB      "DISC, THEN FIRE",$FF
SYNAME  EQU     $-SBOOT
        DB      "YOUR NAME",$FF
SNHELP1 EQU     $-SBOOT
        DB      "DISC UP/DOWN",$FF
SNHELP2 EQU     $-SBOOT
        DB      "FIRE TAKES IT",$FF
SNHELP3 EQU     $-SBOOT
        DB      "CLEAR RUBS, ENTER",$FF

        ASCII
        INCLUDE "build/endpoint.inc"
        GLYPHS
