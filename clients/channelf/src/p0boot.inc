; ---------------------------------------------------------------------------
; p0boot.inc -- page 0: start-up (boot.c, net.c's appkeys and table list,
; teststart.c, autoplay.c).
;
; The FujiNet lobby hands a game its details in appkeys (creator 1, app 1):
;   key 0    the player's name (shared by every FujiNet game)
;   key 10   the table's URL, "https://host/?table=<id>"
; Without them we ask for a name and list the default server's tables. The
; chosen table is written back to key 10, so restarting rejoins it.
;
; Everything here runs before the loop, or from it through FAR with no
; request in flight, so it may wait on the cart.
; ---------------------------------------------------------------------------

NELEN   EQU PV+0                ; name entry: letters so far
NEK     EQU PV+1                ;   the letter offered
NEREP   EQU PV+2                ;   frames before a held stick repeats
TBN     EQU PV+3                ; tables listed
TBSEL   EQU PV+4
TBREP   EQU PV+5
TBTOP   EQU PV+6                ; the first one on the screen
NEX0    EQU PV+7                ; the name's first cell
TBROW   EQU PV+8                ; table list: the row being drawn
TBI     EQU PV+9                ;   and its table

NAMEY   EQU 22
TBY     EQU MAPY                ; the table list's first row
TBROWS  EQU 6                   ; rows of it on the screen
TBMAX   EQU 9
TBRSZ   EQU 33                  ; a MuleTable: iD[8], name[20], players[5]

; TCAT -- a constant string centred on pixel row y, colour col.
TCAT    MACRO str, y, col
        LI y
        LR 2,A
        LIS col
        LR 6,A
        DCI str
        PI TCENTER
        ENDM

; ---------------------------------------------------------------------------
; BOOT -- who we are and which table (boot.c boot).  START has seen the
; cart's magic already.
; ---------------------------------------------------------------------------
BOOT:   ENTER
        PI DCLS
        PI AKLOAD
        LDB SCNAME
        NI 0FFH
        BNZ BT1
        PI NAMEENT
BT1:    LDB SCTABL
        NI 0FFH
        BNZ BT2
        PI TBSELECT
BT2:    PI AKSROOM
        LEAVE

; TBRECH -- another table (main.c choose_table, through FAR): the room we
;   had forgotten, the list, the new one kept.
TBRECH: ENTER
        PI AKCROOM
        PI TBSELECT
        PI AKSROOM
        LEAVE

; ---------------------------------------------------------------------------
; Appkeys.  r3 = the key.
; ---------------------------------------------------------------------------

; AKOPEN -- OPEN_APPKEY: no parameters, a 6-byte payload (fujiDevice's
;   struct appkey): creator 1 (LE), app 1, key r3, mode r4, reserved 0.
;   A = 0 if it worked.  Keeps r3-r6.
AKOPEN: ENTER
        LI FD_FUJI
        LR 0,A
        LI FC_AKOPN
        LR 1,A
        CLR
        LR 2,A
        PI FNBEG                ; leaves DC0 on the TX page
        LIS 1
        ST                      ; creator, LE
        CLR
        ST
        LIS 1
        ST                      ; app
        LR A,3
        ST                      ; key
        LR A,4
        ST                      ; mode
        CLR
        ST                      ; reserved: must be sent
        PI FNGO
        PI FNACK
        LEAVE

; AKREAD -- key r3 into AKBUF, NUL-terminated, at most 64.  A = its length
;   (0: none).
AKREAD: ENTER
        CLR
        LR 4,A
        PI AKOPEN
        NI 0FFH
        BNZ AKR0
        LI FD_FUJI
        LR 0,A
        LI FC_AKRD
        LR 1,A
        CLR
        LR 2,A
        PI FNBEG
        PI FNGO
        PI FNACK
        NI 0FFH
        BNZ AKR0
        LDB FNREPLY             ; the length, at most 64
        LR 5,A
        LDB FNREPLY+1
        NI 0FFH
        BNZ AKR64
        LR A,5
        CI 64
        BC AKR1
AKR64:  LI 64
        LR 5,A
AKR1:   DCI AKBUF
        XDC
        DCI FNREPLY+2
        LR A,5
        NI 0FFH
        BZ AKR2
        LR 0,A
        CLR
        LR 1,A
        PI MEMCPY
AKR2:   XDC
        CLR
        ST
        LR A,5
        LEAVE
AKR0:   CLR
        STB AKBUF
        LEAVE

; AKWRITE -- key r3 = the string in AKBUF: the payload is the key, all of
;   it (none clears it).  Best effort.
AKWRITE: ENTER
        LIS 1
        LR 4,A
        PI AKOPEN
        LI FD_FUJI
        LR 0,A
        LI FC_AKWR
        LR 1,A
        CLR
        LR 2,A
        PI FNBEG
        DCI AKBUF
        XDC
        PI FNTXS
        PI FNGO
        LEAVE

; ISALNUM -- C set if r4 is a letter or a digit.  Leaf.  Clobbers A.
ISALNUM: LR A,4
        CI 'z'
        BNC ISALN
        CI 'a'-1
        BNC ISALY
        CI 'Z'
        BNC ISALN
        CI 'A'-1
        BNC ISALY
        CI '9'
        BNC ISALN
        CI '0'-1
        BNC ISALY
ISALN:  NI 0FFH                 ; C clear
        POP
ISALY:  CI 0FFH                 ; C set
        POP

; AKLOAD -- the name and the table from the lobby's appkeys (appkey_load).
AKLOAD: ENTER
        CLR
        STB SCNAME
        CLR
        LR 3,A
        PI AKREAD               ; key 0: the name; A-Z and 0-9 only go
        NI 0FFH                 ; into the URL unescaped
        BZ AKL3
        DCI SCNAME
        XDC
        DCI AKBUF
        LIS 8
        LR 6,A                  ; at most 8
        LIS 1
        LR 5,A                  ; all letters and digits so far
AKL1:   LM
        NI 0FFH
        BZ AKL2
        CI 'a'-1
        BC AKLUP
        CI 'z'
        BNC AKLUP
        AI 0E0H                 ; lower case up
AKLUP:  LR 4,A
        PI ISALNUM
        BC AKLOK
        CLR
        LR 5,A
AKLOK:  LR A,4
        XDC
        ST
        XDC
        DS 6
        BNZ AKL1
AKL2:   XDC
        CLR
        ST
        XDC
        LR A,5
        NI 0FFH
        BNZ AKL3
        CLR
        STB SCNAME              ; not a name we can send: none
AKL3:   DCI SCENDPT             ; the build's endpoint, unless key 10 says
        XDC
        DCI UENDPT
        PI STRCPY
        CLR
        STB SCTABL
        LI 10
        LR 3,A
        PI AKREAD               ; only a URL counts (an unset key can read
        CI 4                    ; back as zeros)
        BC AKL5
        LDB AKBUF
        CI 'h'
        BNZ AKL5
        DCI SCENDPT
        XDC
        DCI AKBUF
        PI STRCPY
        PI SPLITURL
AKL5:   DCI SCENDPT             ; no trailing '/', so "<endpoint>/state"
        PI STRLEN               ; never has two
        NI 0FFH
        BZ AKLX
        AI 0FFH
        DCI SCENDPT
        ADC
        LM
        CI '/'
        BNZ AKLX
        LI 0FFH
        ADC
        CLR
        ST
AKLX:   LEAVE

; SPLITURL -- "https://host/?table=xyz": SCENDPT keeps the front, SCTABL
;   gets xyz (letters and digits only, else no table).
SPLITURL: ENTER
        DCI SCENDPT
SU1:    LM
        NI 0FFH
        BZ SUX
        CI '?'
        BNZ SU1
        LI 0FFH                 ; the endpoint ends at the '?'
        ADC
        CLR
        ST
        XDC                     ; "table=" next?
        DCI S_TBLEQ
        XDC
        LIS 6
        LR 4,A
SU2:    LM
        LR 5,A
        XDC
        LM
        XDC
        XS 5
        BNZ SUX
        DS 4
        BNZ SU2
        XDC
        DCI SCTABL
        XDC
        LIS 8
        LR 4,A
SU3:    LM
        NI 0FFH
        BZ SU4
        CI '&'
        BZ SU4
        LR 4,A
        PI ISALNUM
        BNC SUBAD
        LR A,4
        XDC
        ST
        XDC
        DS 4
        BNZ SU3
SU4:    XDC
        CLR
        ST
SUX:    LEAVE
SUBAD:  CLR
        STB SCTABL
        LEAVE
S_TBLEQ: DB "table=",0

; AKSROOM -- key 10 = "<endpoint>/?table=<table>" (appkey_save_room).
AKSROOM: ENTER
        DCI AKBUF
        XDC
        DCI SCENDPT
        PI STRCPY
        DCI S_QTBL
        PI STRCPY
        DCI SCTABL
        PI STRCPY
        DCI AKBUF
        PI STRLEN
        LR 5,A
        LI 10
        LR 3,A
        PI AKWRITE
        LEAVE
S_QTBL: DB "/?table=",0

; AKCROOM -- key 10 emptied (zeros read back as no table), and no table.
AKCROOM: ENTER
        CLR
        STB AKBUF
        CLR
        LR 5,A
        LI 10
        LR 3,A
        PI AKWRITE
        CLR
        STB SCTABL
        LEAVE

; AKSNAME -- key 0 = the name picked at boot, for next time.
AKSNAME: ENTER
        DCI AKBUF
        XDC
        DCI SCNAME
        PI STRCPY
        DCI AKBUF
        PI STRLEN
        LR 5,A
        CLR
        LR 3,A
        PI AKWRITE
        LEAVE

; ---------------------------------------------------------------------------
; NAMEENT -- no keyboard: pick the letters (boot.c name_entry). Up and down
; turn the one at the cursor through A-Z and 0-9 (held, it runs on); the
; plunger or right takes it, pull-up or left rubs out, START ends.
; ---------------------------------------------------------------------------
NAMEENT: ENTER
        PI DCLS
        TCAT S_YNAME, TITLEY, C_WHITE
        TCAT S_NHELP1, 34, C_WHITE
        TCAT S_NHELP2, MSGY, C_WHITE
        DCI SCNAME
        CLR
        LIS 9
        LR 0,A
NEZ:    CLR
        ST
        DS 0
        BNZ NEZ
        CLR
        STB NELEN
        STB NEK
        STB NEREP
NE1:    PI FRWAIT
        PI SNDTICK
        PI INPOLL
        PI INTAKE
        LDB NEREP
        NI 0FFH
        BZ NE2
        AI 0FFH
        STB NEREP
NE2:    LDB NELEN               ; rub out: the key, or left
        NI 0FFH
        BZ NE4
        LDB EVKEY
        CI KEY_BACK
        BZ NE3
        LDB NEREP
        NI 0FFH
        BNZ NE4
        LDB INPDIR
        NI DIR_LEFT
        BZ NE4
NE3:    DECB NELEN
        DCI SCNAME
        ADC
        CLR
        ST
        LI 12
        STB NEREP
        JMP NESEL
NE4:    LDB EVKEY               ; the end: a name of one letter at least
        CI KEY_END
        BNZ NE5
        LDB NELEN
        NI 0FFH
        BNZ NEDONE
        PI NETAKE
NEDONE: LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
        PI AKSNAME
        LEAVE
NE5:    LDB NELEN               ; take it: the plunger, or right
        CI 7
        BNC NE6
        LDB EVBTN
        NI 0FFH
        BNZ NE5T
        LDB NEREP
        NI 0FFH
        BNZ NE6
        LDB INPDIR
        NI DIR_RIGHT
        BZ NE6
NE5T:   PI NETAKE
        LI 12
        STB NEREP
        JMP NESEL
NE6:    LDB NEREP               ; up and down: the letter
        NI 0FFH
        BNZ NEDRW
        LDB INPDIR
        NI DIR_UP
        BZ NE7
        LDB NEK
        AI 0FFH
        CI 35
        BC NE8
        LI 35
        BR NE8
NE7:    LDB INPDIR
        NI DIR_DOWN
        BZ NEDRW
        LDB NEK
        INC
        CI 35
        BC NE8
        CLR
NE8:    STB NEK
        LIS 8
        STB NEREP
        BR NEDRW
NESEL:  LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
NEDRW:  PI NEDRAW
        JMP NE1

; NETAKE -- the letter offered becomes the next one of the name.  Leaf.
;   Clobbers A, r0, DC0.
NETAKE: LDB NEK
        DCI PICK
        ADC
        LM
        LR 0,A
        LDB NELEN
        DCI SCNAME
        ADC
        LR A,0
        ST
        INCB NELEN
        POP

; NEDRAW -- the name centred, the cursor after it blinking with the letter
;   offered; each cell drawn once (no clearing first, no flicker).
NEDRAW: ENTER
        LDB NELEN               ; first cell: (TCOLS - n - 1) / 2
        COM
        INC
        AI TCOLS-1
        SR 1
        STB NEX0
        LR 0,A
        DCI LINBUF
        PI SPACES
        LI TX0
        LR 1,A
        LI NAMEY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI LINBUF
        PI DTEXT
        LI NAMEY
        LR 2,A
        LIS C_ORANGE
        LR 6,A
        DCI SCNAME
        PI DTEXT
        LDB NELEN               ; the cursor, while there is room
        CI 7
        BNC NED2
        LDB FRAMES
        NI 16
        BZ NED1
        LDB NEK
        DCI PICK
        ADC
        LM
        BR NEDC
NED1:   LI '_'
NEDC:   LR 0,A
        LI NAMEY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        PI DGLYPH
        LR A,1
        AI 4
        LR 1,A
NED2:   LR A,1                  ; spaces to the end of the row
        COM
        INC
        AI TX0+4*TCOLS
        SR 1
        SR 1
        LR 0,A
        DCI LINBUF
        PI SPACES
        LI NAMEY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI LINBUF
        PI DTEXT
        LEAVE

PICK:   DB "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
S_YNAME: DB "YOUR NAME?",0
S_NHELP1: DB "UP/DOWN A LETTER, PUSH TAKES IT",0
S_NHELP2: DB "PULL RUBS OUT, START ENDS",0

; ---------------------------------------------------------------------------
; TBSELECT -- choose a table from /tables: up/down, then the plunger (or
; START) (boot.c table_select).
; ---------------------------------------------------------------------------
TBSELECT: ENTER
TSL0:   PI DCLS
        TCAT S_CHOOSE, TITLEY, C_WHITE
        TCAT S_LOAD, MSGY, C_WHITE
        PI NTABLES
        STB TBN
        LI MSGY
        STB MSGROW
        PI MSGCLR
        LDB TBN
        NI 0FFH
        BNZ TSL2
        TCAT S_NOTAB, 22, C_WHITE
        LI 28
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI SCENDPT
        PI TCENTER
        TCAT S_AGAIN, MSGY, C_WHITE
TSLW:   PI FRWAIT
        PI SNDTICK
        PI INPOLL
        PI INTAKE
        LDB EVBTN
        NI 0FFH
        JNZ TSL0
        LDB EVKEY
        CI KEY_END
        JZ TSL0
        BR TSLW
TSL2:   CLR
        STB TBSEL
        STB TBREP
        PI TBDRAW
        TCAT S_TBHELP, MSGY, C_WHITE
TSLL:   PI FRWAIT
        PI SNDTICK
        PI INPOLL
        PI INTAKE
        LDB TBREP
        NI 0FFH
        BZ TSL3
        AI 0FFH
        STB TBREP
        BR TSL5
TSL3:   LDB INPDIR
        NI DIR_UP
        BZ TSL4
        LDB TBSEL
        NI 0FFH
        BZ TSL5
        AI 0FFH
        BR TSLM
TSL4:   LDB INPDIR
        NI DIR_DOWN
        BZ TSL5
        LDB TBN                 ; sel + 1 < n
        LR 5,A
        LDB TBSEL
        INC
        LR 4,A
        COM
        AS 5                    ; carries when n > sel + 1
        BNC TSL5
        LR A,4
TSLM:   STB TBSEL
        LI 12
        STB TBREP
        LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
        PI TBDRAW
TSL5:   LDB EVBTN
        NI 0FFH
        BNZ TSL6
        LDB EVKEY
        CI KEY_END
        JNZ TSLL
TSL6:   LDB TBSEL               ; its id, at most 8
        LR 0,A
        PI TBREC
        XDC
        DCI SCTABL
        XDC
        LIS 8
        LR 0,A
        CLR
        LR 1,A
        PI MEMCPY
        CLR
        STB SCTABL+8
        LDB SCTABL
        NI 0FFH
        JZ TSL0
        LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
        LEAVE

S_CHOOSE: DB "CHOOSE A TABLE",0
S_LOAD: DB "LOADING...",0
S_NOTAB: DB "NO TABLES FOUND AT",0
S_AGAIN: DB "PUSH: TRY AGAIN",0
S_TBHELP: DB "UP/DOWN, THEN PUSH",0

; TBREC -- DC0 = table r0's record (TBL + 1 + 33 * r0).  Leaf.
;   Clobbers A, r0.
TBREC:  DCI TBL+1
        LR A,0
        NI 0FFH
        BZ TBRX
TBR1:   LI TBRSZ
        ADC
        DS 0
        BNZ TBR1
TBRX:   POP

; TBDRAW -- the list, a window of TBROWS rows round the choice: each row
;   '>' or ' ', the name (17), ' ', the players ("n / 4").
TBDRAW: ENTER
        LDB TBSEL               ; the first row: sel - 5, or 0
        CI TBROWS-1
        LI 0
        BC TBD0
        LDB TBSEL
        AI 256-(TBROWS-1)
TBD0:   STB TBTOP
        CLR
        STB TBROW
TBDL:   LDB TBTOP
        LR 4,A
        LDB TBROW
        AS 4
        STB TBI
        LR 4,A
        DCI LINBUF              ; 24 spaces, a NUL
        LI TCOLS
        LR 0,A
        PI SPACES
        LDB TBN                 ; a table there?
        LR 5,A
        LR A,4
        COM
        AS 5                    ; carries when n > i
        BNC TBDDR
        LDB TBSEL
        LR 5,A
        LR A,4
        XS 5
        BNZ TBD1
        LI '>'
        STB LINBUF
TBD1:   LDB TBI                 ; the name: up to 17, stopping at a NUL
        LR 0,A
        PI TBREC
        LI 8
        ADC
        XDC
        DCI LINBUF+1
        XDC
        LI 17
        LR 0,A
TBD2:   LM
        NI 0FFH
        BZ TBD3
        XDC
        ST
        XDC
        DS 0
        BNZ TBD2
TBD3:   LDB TBI                 ; the players, at cell 19
        LR 0,A
        PI TBREC
        LI 28
        ADC
        XDC
        DCI LINBUF+19
        XDC
        LIS 5
        LR 0,A
TBD4:   LM
        NI 0FFH
        BZ TBD5
        XDC
        ST
        XDC
        DS 0
        BNZ TBD4
TBD5:
TBDDR:  LDB TBROW               ; y = TBY + 6 * row
        LR 4,A
        SL 1
        AS 4
        SL 1
        AI TBY
        LR 2,A
        LI TX0
        LR 1,A
        LIS C_WHITE
        LR 6,A
        DCI LINBUF
        PI DTEXT
        INCB TBROW
        CI TBROWS
        JNZ TBDL
        LEAVE

; NTABLES -- /tables?bin=1 into TBL through the request engine, waited on.
;   A = the tables there (at most TBMAX, and no more than came).
NTABLES: ENTER
        LI ACT_TABLS
        STB ACT
        PI NSYNC
        CI 1
        BZ NT1
        CLR
        LEAVE
NT1:    LDB TBL
        CI TBMAX
        BC NT2
        LI TBMAX
NT2:    STB TBN
        LDW 0, 1, NGOT          ; what came: (got - 1) / 33 whole ones
        LIS 1
        LR 2,A
        CLR
        LR 3,A
        PI SUB16
        LI TBRSZ
        LR 2,A
        PI DIV8
        LR A,1
        NI 0FFH
        BNZ NT3
        LDB TBN                 ; n > that: that
        LR 5,A
        LR A,0
        COM
        AS 5
        BNC NT3
        LR A,0
        STB TBN
NT3:    LDB TBN
        LEAVE

; ---------------------------------------------------------------------------
; TESTST -- the offline test mode's fake state (teststart.c test_start):
; round 1 on a generated map. TESTHK+1 is the phase + 1 (0: develop), +2 the
; colony event / auction stage / the seat whose turn it is, +3 the player
; event. Actions are answered locally (TESTPOLL).
; ---------------------------------------------------------------------------
TSTPH   EQU PV+16               ; the phase asked for (+1)
TSTARG  EQU PV+17
TSTPEV  EQU PV+18
TSTMP   EQU PV+19               ; the map loop: plot, column, row
TSTCOL  EQU PV+20
TSTROW  EQU PV+21

TESTST: ENTER
        LIS 1
        STB TESTMD
        LDB TESTHK+1
        STB TSTPH
        LDB TESTHK+2
        STB TSTARG
        LDB TESTHK+3
        STB TSTPEV
        DCI ST                  ; all zeros
        LI STSIZE&0FFH
        LR 0,A
        LI STSIZE>>8
        LR 1,A
        PI MEMZERO
        SETB ST+H_MAGIC, 'M'
        SETB ST+H_VER, 2
        SETB ST+H_TYPE, 1
        LDB TSTPH
        AI 0FFH
        CI PH_OVER
        BC TS1
        LIS PH_DEV              ; 0: develop
TS1:    STB ST+H_PHASE
        SETB ST+H_ROUND, 1
        SETB ST+H_CDOWN, 0FFH
        ST
        ; the players: "PL0".."PL3", species 1 + 2i (+ arg in the lobby and
        ; the summary: 1 shows the other four), colour i, $1000, rank i + 1,
        ; food 1/3/5/7 (short, even, over), score 5000 - 700i
        CLR
        LR 6,A
TSP1:   LR A,6
        LR 0,A
        PI TSTPL                ; DC0 = player r6
        LI 'P'
        ST
        LI 'L'
        ST
        LR A,6
        AI '0'
        ST
        LR A,6
        LR 0,A
        PI TSTPL
        LIS P_SPEC
        ADC
        LR A,6
        SL 1
        INC
        LR 5,A
        LDB TSTPH
        CI PH_LOBBY+1
        BZ TSP2
        CI PH_SUM+1
        BNZ TSP3
TSP2:   LDB TSTARG
        AS 5
        LR 5,A
TSP3:   LR A,6
        LR 0,A
        PI TSTPL
        LIS P_SPEC
        ADC
        LR A,5
        NI 7
        ST                      ; species
        LR A,6
        ST                      ; colour
        CLR
        ST                      ; flags
        LI 1000&0FFH
        ST
        LI 1000>>8
        ST                      ; money
        LR A,6
        SL 1
        INC
        ST                      ; food
        LR A,6
        LR 0,A
        PI TSTPL
        LI P_SCORE
        ADC
        LR A,6                  ; 5000 - 700 i
        LR 2,A
        LI 700&0FFH
        LR 0,A
        LI 700>>8
        LR 1,A
        PI MUL8
        LR A,0
        LR 2,A
        LR A,1
        LR 3,A
        LI 5000&0FFH
        LR 0,A
        LI 5000>>8
        LR 1,A
        PI SUB16
        LR A,0
        LR 4,A
        LR A,1
        LR 5,A
        LR A,6
        LR 0,A
        PI TSTPL
        LI P_SCORE
        ADC
        LR A,4
        ST
        LR A,5
        ST
        LR A,6                  ; rank, production, spoilage
        INC
        ST
        LR A,6
        AI 3
        ST
        LR A,6
        LR 0,A
        PI TSTPL
        LI P_SPOIL
        ADC
        LR A,6
        ST
        LR A,6
        INC
        LR 6,A
        CI 3
        JC TSP1
        SETB ST+PLAYERS+3*PLSIZE+P_FLAGS, PF_AI
        ; the map: the river down column 4, the store in the middle,
        ; mountains either side
        CLR
        STB TSTMP
        STB TSTCOL
        STB TSTROW
TSM1:   CLR
        LR 5,A                  ; plain
        LDB TSTCOL
        CI 4
        BNZ TSM2
        LIS T_RIVER
        LR 5,A
TSM2:   LDB TSTMP
        CI STOREPL
        BNZ TSM3
        LIS T_STORE
        LR 5,A
TSM3:   LDB TSTROW              ; col == row % 4: mountains (row % 3 + 1)
        NI 3
        LR 4,A
        LDB TSTCOL
        XS 4
        BNZ TSM4
        LDB TSTROW
        LR 0,A
        PI MOD3
        INC
        SL 1
        SL 1
        AI T_MOUNT
        LR 5,A
TSM4:   LDB TSTROW              ; col == 5 + row % 4: ((row + 1) % 3 + 1)
        NI 3
        AI 5
        LR 4,A
        LDB TSTCOL
        XS 4
        BNZ TSM5
        LDB TSTROW
        INC
        LR 0,A
        PI MOD3
        INC
        SL 1
        SL 1
        AI T_MOUNT
        LR 5,A
TSM5:   LDB TSTMP
        DCI ST+M_TERR
        ADC
        LR A,5
        ST
        LDB TSTMP
        DCI ST+M_OWNER
        ADC
        LI 0FFH
        ST
        LDB TSTMP
        DCI ST+M_MULE
        ADC
        LI 0FFH
        ST
        LDB TSTMP
        DCI ST+M_CRYS
        ADC
        LI 0FFH
        ST
        INCB TSTCOL
        CI 9
        BNZ TSM6
        CLR
        STB TSTCOL
        INCB TSTROW
TSM6:   INCB TSTMP
        CI NPLOTS
        JNZ TSM1
        ; some land: plot, owner, M.U.L.E., production
        DCI TSTLAND
TSL1:   LM
        CI 0FFH
        BZ TSL9
        LR 4,A                  ; plot
        LM
        LR 5,A                  ; owner
        LM
        LR 6,A                  ; mule
        LM
        LR 3,A                  ; production
        LR Q,DC
        DCI ST+M_OWNER
        LR A,4
        ADC
        LR A,5
        ST
        DCI ST+M_MULE
        LR A,4
        ADC
        LR A,6
        ST
        DCI ST+M_PROD
        LR A,4
        ADC
        LR A,3
        ST
        LR DC,Q
        BR TSL1
TSL9:   SETB ST+S_MULES, 14
        SETB ST+S_MPRIC, 100
        CLR
        ST
        CLR                     ; prices 20/40/60/80, stock 8
        LR 6,A
TSS1:   DCI ST+S_STOCK
        LR A,6
        ADC
        LIS 8
        ST
        DCI ST+S_PRICE
        LR A,6
        SL 1
        ADC
        LR A,6
        INC
        SL 1
        SL 1
        LR 5,A
        SL 1
        SL 1
        AS 5                    ; 20 (i + 1)
        ST
        CLR
        ST
        LR A,6
        INC
        LR 6,A
        CI 3
        BC TSS1
        ; a turn: ours (or seat arg's), started, 2828 jiffies, a wampus early on
        LDB TSTPEV
        LR 4,A
        LDB TSTARG
        NI 3
        STB ST+T_SEAT
        LIS TF_START
        ST
        LI 0FFH
        ST                      ; carrying nothing
        LR A,4
        ST                      ; the player event
        LI 2828&0FFH
        ST
        LI 2828>>8
        ST                      ; length
        CLR
        ST
        ST                      ; elapsed
        LI 2828&0FFH
        ST
        LI 2828>>8
        ST                      ; left
        CLR
        ST
        ST                      ; endIn
        LI 28
        ST                      ; pTU
        LIS 1
        ST                      ; nWampus
        CLR
        ST                      ; wampus 0: plot 0, mount 0, from 0, to 2815
        ST
        ST
        ST
        LI 2815&0FFH
        ST
        LI 2815>>8
        ST
        ; production / colony event: event arg on plot 10, total 123456
        LDB TSTARG
        STB ST+C_EVENT
        LI 10
        ST
        LIS 4
        ST                      ; rating
        DCI ST+C_TOTAL
        LI 40H
        ST
        LI 0E2H
        ST
        LIS 1
        ST
        ; auctions
        SETB ST+A_KIND, 2
        SETB ST+A_STEP, 1
        LDB ST+H_PHASE
        CI PH_LAUC
        BNZ TSA1
        SETB ST+A_KIND, 1
        SETB ST+A_STEP, 4
        SETB ST+A_START, 240
        SETB ST+A_PLOT, 13
TSA1:   SETB ST+A_SBUY, 30
        SETB ST+A_SSELL, 65
        CLR
        LR 6,A
TSA2:   DCI ST+A_ROLE
        LR A,6
        ADC
        LIS ROLE_BUY
        ST
        DCI ST+A_POS
        LR A,6
        SL 1
        ADC
        LR A,6
        SL 1
        SL 1
        AS 6
        AI 20
        ST                      ; 20 + 5i
        DCI ST+A_TGT
        LR A,6
        SL 1
        ADC
        LR A,6
        SL 1
        SL 1
        AS 6
        AI 40
        ST                      ; 40 + 5i
        LR A,6
        INC
        LR 6,A
        CI 3
        BC TSA2
        SETB ST+A_ROLE+1, ROLE_SELL
        SETB ST+A_TICKS, 70
        LDB ST+H_PHASE          ; the stage: arg, in declare, trading and land
        CI PH_DECL
        BZ TSA3
        CI PH_TRADE
        BZ TSA3
        CI PH_LAUC
        BNZ TSA4
TSA3:   LDB TSTARG
        STB ST+A_STAGE
TSA4:   SETB ST+A_HI, 66
        SETB ST+A_LO, 20
        ; the grant: the cursor waits for the ship (-1), then steps every 12
        SETB ST+GR_CUR, 0FFH
        LI 0FFH
        ST
        ST
        ST
        ST
        LI 12
        ST                      ; step
        LI 912&0FFH
        ST
        LI 912>>8
        ST                      ; next: 186 + 726
        DCI SCNAME
        XDC
        DCI S_PL0
        PI STRCPY
        DCI SCTABL
        XDC
        DCI S_TEST
        PI STRCPY
        LEAVE
S_PL0:  DB "PL0",0
S_TEST: DB "test",0
; {plot, owner, mule, production}
TSTLAND: DB 10,0,G_FOOD,5, 12,1,G_ENER,3, 30,3,G_ENER,0, 20,0,G_SMIT,8
        DB 14,2,0FFH,0, 34,2,G_CRYS,2, 0FFH

; TSTPL -- DC0 = player r0's 29 bytes.  Leaf.  Clobbers A, r0.
TSTPL:  DCI ST+PLAYERS
        LR A,0
        NI 0FFH
        BZ TSTPX
TSTP1:  LI PLSIZE
        ADC
        DS 0
        BNZ TSTP1
TSTPX:  POP

; MOD3 -- A = r0 mod 3 (r0 < 64).  Leaf.
MOD3:   LR A,0
MOD3L:  CI 2
        BC MOD3X
        AI 0FDH
        BR MOD3L
MOD3X:  POP

; ---------------------------------------------------------------------------
; AUTOPL -- plays by itself, for soak tests (autoplay.c; AUTOPLAY=1 builds
; only). It presses only what a player would: ready, claims, turns, go on.
; ---------------------------------------------------------------------------
        IF AUTOPLAY
AUTOPL: ENTER
        LDW 0, 1, FRAMES
        LDW 2, 3, APLAST
        PI SUB16
        LR A,1
        NI 0FFH
        BNZ AP1
        LR A,0
        CI 44
        BC APX
AP1:    LDB STOK
        NI 0FFH
        BZ APX
        LDB MYSEAT
        CI 3
        BNC APX
        LDW 0, 1, FRAMES
        STW 0, 1, APLAST
        LDB PHASE
        CI PH_LOBBY
        BNZ AP2
        LDB MYSEAT
        LR 0,A
        PI TSTPL
        LI P_FLAGS
        ADC
        LM
        NI PF_READY
        BNZ APX
        BR APBTN
AP2:    CI PH_SUM
        BZ APBTN
        CI PH_GRANT
        BNZ AP2D
        LDB ST+GR_CUR           ; late, so a soak sees the computers' stops
        CI 19
        BC APX
        CI NPLOTS-1
        BNC APX
        BR APBTN
AP2D:   CI PH_DEV
        BNZ AP3
        LDB DVSTATE
        CI DV_PROMPT
        BZ APBTN
        CI DV_WALK
        BNZ APX
        LIS KEY_END
        STB EVKEY
        LEAVE
AP3:    CI PH_DECL
        BNZ APX
        LI KEY_SITOUT
        STB EVKEY
        LEAVE
APBTN:  LIS 1
        STB EVBTN
APX:    LEAVE
        ENDIF
