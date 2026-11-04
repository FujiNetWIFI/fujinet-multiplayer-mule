; boot.inc -- start-up: the title, who we are, which table (main.c, boot.c,
; net.c's appkeys and table list). Page 0.
;
; The FujiNet lobby hands a game its details in appkeys (creator 1, app 1):
;   key 0    the player's name (shared by every FujiNet game)
;   key 10   the table's URL, "https://host/?table=<id>"
; Without them we ask for a name and list the default server's tables. The
; chosen table is written back to key 10, so restarting rejoins it; a name
; picked here is written to key 0 (boot.c, no keyboard).

PVP     DEFL    PHRAM
        PV      TBN,1           ; tables listed
        PV      TBSEL,1
        PV      TBREP,1
        PV      NMLEN,1         ; name entry: letters so far
        PV      NMCH,1          ;   the letter under the cursor
        PV      NMREP,1
        PV      BTBUF,21        ; a table's name, NUL-terminated
        PV      AKURL,81        ; APKSAVE builds the room's URL here
        IF      PVP > RAMEND
RAMEND  DEFL    PVP
        ENDIF

BOOT:   CALL    CLRSCR
        LD      A,K_WORD        ; the title, as on the Atari
        LD      B,W_MULE
        LD      D,8
        LD      C,C_ORANGE
        LD      HL,SDMODE
        LD      (HL),3
        CALL    XSDRAWM
        LD      A,K_WORD
        LD      B,W_TOURN
        LD      D,10
        LD      C,C_BLUE
        LD      HL,SDMODE
        LD      (HL),3
        CALL    XSDRAWM
        LD      A,3CH           ; the harness may poke now
        LD      (TESTHK),A
        LD      A,0C3H
        LD      (TESTHK+1),A
        LD      B,45
BT1:    PUSH    BC
        CALL    FRWAIT
        POP     BC
        LD      A,(TESTHK)
        CP      5AH
        JR      Z,BT2
        DJNZ    BT1
        CALL    BOOTNET
        JR      BT3
BT2:    LD      A,0A5H          ; tell the harness
        LD      (TESTHK),A
        CALL    TESTSTART
BT3:    JP      GAMESTART

; BOOTNET: the cart, the appkeys, the name, the table (boot.c boot).
BOOTNET: LD     HL,SCONN
        LD      D,16
        LD      C,C_WHITE
        CALL    TXTCTR
BN0:    CALL    FNCHECK
        JR      Z,BN1
        LD      HL,SNOCART
        LD      D,16
        LD      C,C_WHITE
        CALL    TXTCTR
        LD      B,60
BN0A:   PUSH    BC
        CALL    FRWAIT
        POP     BC
        DJNZ    BN0A
        JR      BN0
BN1:    CALL    APKLOAD
        LD      A,(SCNAME)
        OR      A
        CALL    Z,NAMEENT
        LD      A,(SCTABL)
        OR      A
        CALL    Z,TABLESEL
        JP      APKSAVE

SCONN:  DB      "CONNECTING...",0
SNOCART: DB     "NO FUJINET FOUND",0

; REROOM: the server turned us away (or we left): forget the table,
; choose another, join it (main.c choose_table).
REROOM: CALL    APKCLR
        CALL    TABLESEL
        CALL    APKSAVE
        JP      GAMESTART

; ---- the name ------------------------------------------------------------------
; NAMEENT: A-Z and 0-9, up to 8 (boot.c name_entry, no keyboard). Up and
; down turn the letter at the cursor (held, it runs on); fire or right takes
; it, * or left rubs out, # ends; the keypad's digits go straight in.
NAMEENT: CALL   CLRSCR
        LD      HL,SNAME1
        LD      D,6
        LD      C,C_WHITE
        CALL    TXTCTR
        LD      HL,SNAME2
        LD      D,16
        LD      C,C_WHITE
        CALL    TXTCTR
        LD      HL,SNAME3
        LD      D,18
        LD      C,C_WHITE
        CALL    TXTCTR
        LD      A,1
        LD      (INNAME),A
        XOR     A
        LD      (NMLEN),A
        LD      (NMREP),A
        LD      HL,SCNAME
        LD      B,9
NE0:    LD      (HL),A
        INC     HL
        DJNZ    NE0
        LD      A,'A'
        LD      (NMCH),A
NELOOP: CALL    FRWAIT
        CALL    INPOLL
        CALL    INTAKE
        LD      HL,NMREP
        LD      A,(HL)
        OR      A
        JR      Z,NE0A
        DEC     (HL)
NE0A:   LD      A,(EVKEY)
        CP      KEYEND
        JP      Z,NEDONE
        CP      KEYBK
        JR      Z,NEBACK
        CALL    ALNUM           ; the keypad's digits (and, from the
        JR      C,NE2           ; harness, letters)
        CALL    NEADD
        JR      NEDRAW
NE2:    LD      A,(EVBTN)
        OR      A
        JR      NZ,NETAKE
        LD      A,(NMREP)
        OR      A
        JR      NZ,NEDRAW
        LD      A,(INDIR)
        BIT     2,A             ; up: the letter before
        JR      NZ,NEUP
        BIT     0,A
        JR      NZ,NEDN
        BIT     1,A
        JR      NZ,NETAKE
        BIT     3,A
        JR      NZ,NEBACK
        JR      NEDRAW
NEUP:   LD      A,(NMCH)
        CALL    NMPREV
        JR      NESET
NEDN:   LD      A,(NMCH)
        CALL    NMNEXT
NESET:  LD      (NMCH),A
        LD      A,8
        LD      (NMREP),A
        JR      NEDRAW
NETAKE: LD      A,(NMCH)
        CALL    NEADD
        LD      A,12
        LD      (NMREP),A
        JR      NEDRAW
NEBACK: LD      A,(NMLEN)
        OR      A
        JR      Z,NEDRAW
        DEC     A
        LD      (NMLEN),A
        LD      E,A
        LD      D,0
        LD      HL,SCNAME
        ADD     HL,DE
        LD      (HL),0
        LD      A,12
        LD      (NMREP),A
        LD      A,SNSEL
        CALL    SNDPLAY
NEDRAW: LD      D,10            ; the name so far, then the letter under
        LD      E,6             ; the cursor, blinking
        LD      B,TCOLS-12
        CALL    CLRTXT
        LD      A,(NMLEN)       ; at (31 - n) / 2
        LD      B,A
        LD      A,TCOLS-1
        SUB     B
        SRL     A
        LD      E,A
        PUSH    DE
        LD      HL,SCNAME
        LD      D,10
        LD      C,C_ORANGE
        CALL    BIGAT
        POP     DE
        LD      A,(NMLEN)
        CP      8
        JP      NC,NELOOP
        ADD     A,E
        LD      E,A
        LD      D,10
        LD      A,(FRAMES)
        AND     16
        LD      A,(NMCH)
        JR      NZ,NED1
        LD      A,'_'
NED1:   LD      C,C_WHITE
        CALL    CHRAT
        JP      NELOOP
NEDONE: LD      A,(NMLEN)       ; nothing taken: a name of the one letter
        OR      A
        JR      NZ,NED2
        LD      A,(NMCH)
        CALL    NEADD
NED2:   XOR     A
        LD      (INNAME),A
        LD      A,SNSEL
        CALL    SNDPLAY
        LD      HL,SCNAME       ; for next time (boot.c appkey_save_name)
        CALL    STRLEN
        LD      B,A
        LD      C,AKNAME
        JP      AKWRITE

; NEADD: letter A onto the name (if there is room).
NEADD:  LD      C,A
        LD      A,(NMLEN)
        CP      8
        RET     NC
        LD      E,A
        INC     A
        LD      (NMLEN),A
        LD      D,0
        LD      HL,SCNAME
        ADD     HL,DE
        LD      (HL),C
        LD      A,SNSEL
        JP      SNDPLAY

; NMNEXT / NMPREV: A-Z then 0-9, round.
NMNEXT: CP      'Z'
        JR      NZ,NMN1
        LD      A,'0'
        RET
NMN1:   CP      '9'
        JR      NZ,NMN2
        LD      A,'A'
        RET
NMN2:   INC     A
        RET
NMPREV: CP      'A'
        JR      NZ,NMP1
        LD      A,'9'
        RET
NMP1:   CP      '0'
        JR      NZ,NMP2
        LD      A,'Z'
        RET
NMP2:   DEC     A
        RET

SNAME1: DB      "YOUR NAME?",0
SNAME2: DB      "UP/DOWN A LETTER, FIRE TAKES IT",0
SNAME3: DB      "* RUBS OUT, # ENDS",0

; ---- the table list --------------------------------------------------------------
; TABLESEL: choose a table from /tables (boot.c table_select): up/down,
; then fire or #.
TBMAX   EQU     9
TBROW0  EQU     4

TABLESEL:
        CALL    CLRSCR
        LD      HL,STCHOOSE
        LD      D,0
        LD      C,C_ORANGE
        CALL    BIGCTR
        LD      HL,SLOAD
        LD      D,23
        LD      C,C_WHITE
        CALL    TXTCTR
        CALL    TBFETCH
        LD      A,23
        CALL    CLRROW
        LD      A,(TBN)
        OR      A
        JR      NZ,TS2
        LD      HL,SNOTAB
        LD      D,22
        LD      C,C_WHITE
        CALL    TXTCTR
        LD      HL,ENDPT
        LD      D,23
        LD      C,C_WHITE
        CALL    TXTCTR
TS1:    CALL    FRWAIT
        CALL    INPOLL
        CALL    INTAKE
        LD      A,(EVBTN)
        OR      A
        JR      NZ,TABLESEL
        LD      A,(EVKEY)
        CP      KEYEND
        JR      Z,TABLESEL
        JR      TS1
TS2:    LD      B,0             ; the names and the seats taken
TS3:    PUSH    BC
        LD      A,B
        CALL    TBOFF           ; HL = the entry's offset
        LD      DE,8            ; name[20]
        ADD     HL,DE
        LD      DE,BTBUF
        LD      BC,20
        CALL    RXCOPY
        XOR     A
        LD      (DE),A
        POP     BC
        PUSH    BC
        CALL    TBROW
        LD      E,4
        LD      HL,BTBUF
        LD      C,C_WHITE
        CALL    TXTAT
        POP     BC
        PUSH    BC
        LD      A,B
        CALL    TBOFF
        LD      DE,28           ; players[5]
        ADD     HL,DE
        LD      DE,BTBUF
        LD      BC,5
        CALL    RXCOPY
        XOR     A
        LD      (DE),A
        POP     BC
        PUSH    BC
        CALL    TBROW
        LD      E,TCOLS-6
        LD      HL,BTBUF
        LD      C,C_WHITE
        CALL    TXTAT
        POP     BC
        INC     B
        LD      A,(TBN)
        CP      B
        JR      NZ,TS3
        LD      HL,STHELP
        LD      D,23
        LD      C,C_WHITE
        CALL    TXTCTR
        XOR     A
        LD      (TBSEL),A
        LD      (TBREP),A
TSLOOP: CALL    FRWAIT
        CALL    INPOLL
        CALL    INTAKE
        LD      B,0             ; the marks
TSM:    PUSH    BC
        CALL    TBROW
        LD      E,2
        LD      A,(TBSEL)
        CP      B
        LD      A,'>'
        JR      Z,TSM1
        LD      A,' '
TSM1:   LD      C,C_WHITE
        CALL    CHRAT
        POP     BC
        INC     B
        LD      A,(TBN)
        CP      B
        JR      NZ,TSM
        LD      HL,TBREP
        LD      A,(HL)
        OR      A
        JR      Z,TS5
        DEC     (HL)
        JR      TS7
TS5:    LD      A,(INDIR)
        BIT     2,A
        JR      Z,TS6
        LD      A,(TBSEL)
        OR      A
        JR      Z,TS7
        DEC     A
        JR      TS6B
TS6:    BIT     0,A
        JR      Z,TS7
        LD      A,(TBN)
        DEC     A
        LD      B,A
        LD      A,(TBSEL)
        CP      B
        JR      NC,TS7
        INC     A
TS6B:   LD      (TBSEL),A
        LD      A,12
        LD      (TBREP),A
        LD      A,SNSEL
        CALL    SNDPLAY
TS7:    LD      A,(EVBTN)
        OR      A
        JR      NZ,TS8
        LD      A,(EVKEY)
        CP      KEYEND
        JR      NZ,TSLOOP
TS8:    LD      A,(TBSEL)       ; the id: 8 bytes, NUL padded
        CALL    TBOFF
        LD      DE,SCTABL
        LD      BC,8
        CALL    RXCOPY
        XOR     A
        LD      (DE),A
        LD      A,(SCTABL)
        OR      A
        JP      Z,TABLESEL
        LD      A,SNSEL
        JP      SNDPLAY

; TBROW: D = table B's row (4 + 2B).
TBROW:  LD      A,B
        ADD     A,A
        ADD     A,TBROW0
        LD      D,A
        RET

; TBOFF: HL = the window offset of table A: 1 + 33 * A.
TBOFF:  LD      L,A
        LD      H,0
        LD      D,H
        LD      E,L
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL           ; *32
        ADD     HL,DE           ; *33
        INC     HL
        RET

STCHOOSE: DB    "CHOOSE A TABLE",0
SLOAD:  DB      "LOADING...",0
SNOTAB: DB      "NO TABLES FOUND AT",0
STHELP: DB      "UP/DOWN, FIRE",0

; GAMESTART: joining a table (main.c game_start).
GAMESTART:
        LD      A,255
        LD      (LASTPH),A
        LD      (LASTRND),A
        XOR     A
        LD      (SCRNOW),A
        LD      (STOK),A
        LD      (POLLW),A
        LD      (ACT),A
        LD      (ANIM),A
        CALL    CLRSCR
        LD      HL,SJOIN
        LD      D,11
        LD      C,C_WHITE
        CALL    TXTCTR
        LD      HL,(FRAMES)
        LD      (DONE),HL
        RET

SJOIN:  DB      "JOINING...",0
