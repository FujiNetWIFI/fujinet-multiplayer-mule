; ---------------------------------------------------------------------------
; p1stat.inc -- page 1: the lobby, the status summary and the game over
; (mekkogx status.c), laid out for 24 columns:
;
;   the lobby                       the summary (and the game over's table)
;   y 4  M.U.L.E. TOURNAMENT        y 4  STATUS SUMMARY #n
;   y10  <> SPECIES  PUSH: READY    y10  MONEY  LAND GOODS TOTAL
;   y16  MODE: LEAVE                y16-47  a seat a row of 8, by rank,
;   y22-53  a seat a row of 8: its          its species standing at the left
;        species, name, species,    y50  COLONY 123456
;        CPU or READY               y56  the message: one row (MSGONE)
;   y56  waiting / the countdown
;
; The summary's walk-ins are fx scripts (fxdata.inc); the game over shows
; FIRST FOUNDER, then the summary's table and the colony's rating.
; ---------------------------------------------------------------------------

LB_SPEC EQU PV+0                ; our species
LB_SENT EQU PV+1                ; a left/right push already sent
LB_LAST EQU PV+2                ; 4: per seat, species + flags * 8 drawn
LB_LCD  EQU PV+6                ; 2: the countdown shown (seconds; FFFE none
                                ; yet, FFFF waiting)
LB_SPC  EQU PV+8                ; watching
LB_K    EQU PV+9                ; LOBBYS: the seat, its text row,
LB_R    EQU PV+10
LB_J    EQU PV+11               ;   species, flags, name[0]
LB_C    EQU PV+12
LB_N    EQU PV+13
LB_ALR  EQU PV+14               ; the countdown has begun: the alert
SU_ORD  EQU PV+16               ; 4: seats by rank
SU_DRWN EQU PV+20               ; the table is up
SU_LND  EQU PV+21               ; round 0: the ship lands first
GO_STG  EQU PV+22               ; the game over: 0 FIRST FOUNDER, 1 the table
SU_I    EQU PV+23               ; scratch: the row,
SU_V    EQU PV+24               ;   2: a value,
SU_K    EQU PV+26               ;   the seat,
SU_G    EQU PV+27               ;   a good,
SU_Y    EQU PV+28               ;   the pixel row,
SU_C    EQU PV+29               ;   the colour
SU_ALL  EQU PV+30

LBROW0  EQU 22                  ; the lobby's first seat (sprite top)
SUROW0  EQU 16                  ; the summary's
SUHEADY EQU 10

MULEV:  DB 60, 85, 110, 135     ; a M.U.L.E. of good g: (g + 1) * 25 + 35

; ---------------------------------------------------------------------------
; The lobby
; ---------------------------------------------------------------------------

; LOBBYE -- lobby_enter.
LOBBYE: ENTER
        PI DCLS
        LI LBROW0
        LR 4,A
        LI LBROW0+31
        LR 5,A
        PI BANDS3
        LIS SCR_TEXT
        STB SCRNOW
        CLR
        STB SPCOL
        LI 0FFH
        STB LB_LAST
        ST
        ST
        ST
        LI WD_MULE              ; "M.U.L.E. TOURNAMENT"
        LR 0,A
        DCI STW
        PI STRP
        XDC
        DCI LINBUF
        XDC
        PI STRCPY
        XDC
        LI ' '
        ST
        XDC
        LI WD_TOURNAMENT
        LR 0,A
        DCI STW
        PI STRP
        PI STRCPY
        LI TITLEY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI LINBUF
        PI TCENTER
        LI MS_LOBBYHELP         ; the help: two rows from y10
        LR 0,A
        DCI STM
        PI STRP
        LI 10
        LR 2,A
        LIS C_WHITE
        LR 6,A
        PI TCENTER
        LI MSGY                 ; the message: one row, y56
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        LIS 1
        STB MSGONE
        CLR
        STB LB_SENT
        STB LB_SPC
        LI 0FEH                 ; neither a count nor waiting
        STB LB_LCD
        LI 0FFH
        ST
        LDB MYSEAT
        CI 3
        BNC LE1
        LR 0,A
        PI PLDC
        LI P_SPEC
        ADC
        LM
        NI 7
        STB LB_SPEC
LE1:    LEAVE

; LOBBYS -- lobby_state: each seat redrawn when it changed.
LOBBYS: ENTER
        CLR
        STB LB_K
LS1:    LDB LB_K
        LR 0,A
        PI PLDC
        LI P_FLAGS
        ADC
        LM
        STB LB_C
        LDB LB_K
        LR 0,A
        PI PLDC
        LI P_SPEC
        ADC
        LM
        NI 7
        STB LB_J
        LDB LB_K
        LR 0,A
        PI PLDC
        LM                      ; (P_NAME is 0)
        STB LB_N
        LDB LB_C                ; key = species + flags * 8
        SL 1
        SL 1
        SL 1
        LR 6,A
        LDB LB_J
        AS 6
        LR 6,A
        LDB LB_K
        DCI LB_LAST
        ADC
        LM
        XS 6
        BNZ LS2
        LDB LB_N                ; the same, and someone's: as it is
        NI 0FFH
        JNZ LSNX
LS2:    LDB LB_K
        DCI LB_LAST
        ADC
        LR A,6
        ST
        LDB LB_K                ; its text row, cleared
        SL 1
        SL 1
        SL 1
        AI LBROW0+1
        STB LB_R
        LR 2,A
        LI TX0
        LR 1,A
        LI TCOLS
        LR 3,A
        PI DCLRT
        LDB LB_N
        NI 0FFH
        JZ LSOPEN
        LDB LB_K                ; the name, 8 at most, in the seat's colour
        LR 0,A
        PI PLCOLR
        LR 6,A
        LI 14
        LR 1,A
        LDB LB_R
        LR 2,A
        LIS 8
        LR 4,A
        LDB LB_K
        LR 0,A
        PI PLDC
        PI DTEXTN
        LDB LB_R                ; the species' name
        LR 2,A
        LIS C_WHITE
        LR 6,A
        LI 48
        LR 1,A
        LDB LB_J
        LR 0,A
        DCI STS
        PI STRP
        PI DTEXT
        LDB LB_C                ; a computer, or ready
        NI PF_AI
        BZ LS3
        LI WD_COMPUTER
        LR 0,A
        DCI STW
        BR LS4
LS3:    LDB LB_C
        NI PF_READY
        BZ LS5
        LI MS_READY
        LR 0,A
        DCI STM
LS4:    PI STRP
        LR H,DC
        LDB LB_R
        LR 2,A
        LI 82                   ; (5 letters end on cell 23)
        LR 1,A
        LIS C_WHITE
        LR 6,A
        LR DC,H
        PI DTEXT
LS5:    LDB LB_K                ; the species, standing, at its left
        LR 0,A
        LR 5,A
        LIS 5
        LR 6,A
        LIS 8
        LR 1,A
        LDB LB_K
        SL 1
        SL 1
        SL 1
        AI LBROW0
        SL 1
        SL 1
        AI 0F0H                 ; the Atari's y for that top: 4 top - 16
        LR 2,A
        PI SPSPEC
        BR LSNX
LSOPEN: LI WD_OPEN
        LR 0,A
        DCI STW
        PI STRP
        LR H,DC
        LI 14
        LR 1,A
        LDB LB_R
        LR 2,A
        LIS C_WHITE
        LR 6,A
        LR DC,H
        PI DTEXT
        LDB LB_K
        LR 0,A
        PI SPRHIDE
LSNX:   INCB LB_K
        CI 3
        JC LS1
        LEAVE

; LOBBYT -- lobby_tick: leave, the countdown, species and ready.
LOBBYT: ENTER
        PI MSGTICK
        LDB EVKEY               ; leave this table and pick another
        CI KEY_LEAVE
        BNZ LT0
        TAIL LEAVETB            ; (this page's code off the K-stack: the
                                ; table list maps page 0)
LT0:    LDB MYSEAT              ; watching or not: the waiting line again
        CI 3
        LIS 0
        BC LT1
        LIS 1
LT1:    LR 4,A
        LDB LB_SPC
        XS 4
        BZ LT2
        LR A,4
        STB LB_SPC
        LI 0FEH
        STB LB_LCD
        LI 0FFH
        ST
LT2:    LDW 0, 1, ST+H_CDOWN    ; the countdown to the start, in seconds
        LR A,1
        NI 080H
        JNZ LT5
        LR A,0
        LR 6,A
        LR A,1
        LR 7,A                  ; w
        LDW 0, 1, FRAMES        ; since the state
        LDW 2, 3, RXFRAME
        PI SUB16
        LR A,6
        LR 2,A
        LR A,7
        LR 3,A
        PI GE16
        BC LT3Z
        LR A,0                  ; (w - since) / 60
        LR 4,A
        LR A,1
        LR 5,A
        LR A,6
        LR 0,A
        LR A,7
        LR 1,A
        LR A,4
        LR 2,A
        LR A,5
        LR 3,A
        PI SUB16
        LI 60
        LR 2,A
        PI DIV8
        BR LT4
LT3Z:   CLR
        LR 0,A
        LR 1,A
LT4:    LDW 2, 3, LB_LCD        ; redrawn only when it changes (a scrolling
        LR A,0                  ; line would start again)
        XS 2
        BNZ LT4A
        LR A,1
        XS 3
        JZ LT6
LT4A:   CLR
        STB LB_ALR
        LR A,3                  ; none shown before: the count has begun
        CI 0FFH
        BNZ LT4B
        LR A,2
        CI 0FDH
        BC LT4B
        LIS 1
        STB LB_ALR
LT4B:   STW 0, 1, LB_LCD
        STW 0, 1, EXPV1
        CLR
        STB EXPV2
        ST
        LI MS_LEAVEHELP
        LR 0,A
        PI MSGSHOW
        LDB LB_ALR
        NI 0FFH
        BZ LT6
        LDB MYSEAT
        CI 3
        BNC LT6
        LIS SND_ALERT
        LR 0,A
        PI SNDPLAY
        BR LT6
LT5:    LDW 2, 3, LB_LCD        ; no countdown: waiting (or watching), once
        LR A,2
        CI 0FFH
        BNZ LT5A
        LR A,3
        CI 0FFH
        BZ LT6
LT5A:   LI 0FFH
        STB LB_LCD
        ST
        LDB LB_SPC
        NI 0FFH
        LI MS_WAITING
        BZ LT5B
        LI MS_SPECTATE
LT5B:   LR 0,A
        CLR
        STB EXPV1
        ST
        STB EXPV2
        ST
        PI MSGSHOW
LT6:    LDB MYSEAT
        CI 3
        JNC LTX
        LDB INPDIR              ; species: left/right, one change a push
        NI DIR_LEFT|DIR_RIGHT
        BZ LT8
        LDB LB_SENT
        NI 0FFH
        BNZ LT9
        LIS 1
        STB LB_SENT
        LDB INPDIR
        NI DIR_RIGHT
        LDB LB_SPEC
        BZ LT7L
        INC
        BR LT7
LT7L:   AI 7
LT7:    NI 7
        STB LB_SPEC
        LR 1,A
        LIS ACT_SPEC
        LR 0,A
        CLR
        LR 2,A
        PI ACTSEND
        LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
        BR LT9
LT8:    CLR
        STB LB_SENT
LT9:    LDB EVBTN               ; the button: ready, or not
        NI 0FFH
        BZ LTX
        LDB MYSEAT
        LR 0,A
        PI PLDC
        LI P_FLAGS
        ADC
        LM
        NI PF_READY
        LIS ACT_READY
        BZ LT10
        LIS ACT_UNRDY
LT10:   LR 0,A
        CLR
        LR 1,A
        LR 2,A
        PI ACTSEND
        LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
LTX:    LEAVE

; ---------------------------------------------------------------------------
; The status summary
; ---------------------------------------------------------------------------

; SURANK -- SU_ORD: the seats in rank order (rank 1 first), anyone without
;   a rank last (su_rank).
SURANK: ENTER
        CLR
        LR 8,A                  ; n
        LIS 1
        LR 6,A                  ; k: the rank
SR1:    CLR
        LR 5,A                  ; i: the seat
SR2:    LR A,5
        LR 0,A
        PI PLDC
        LI P_RANK
        ADC
        LM
        XS 6
        BNZ SR3
        LR A,8
        CI 3
        BNC SR3
        DCI SU_ORD
        ADC
        LR A,5
        ST
        LR A,8
        INC
        LR 8,A
SR3:    LR A,5
        INC
        LR 5,A
        CI 3
        BC SR2
        LR A,6
        INC
        LR 6,A
        CI 4
        BC SR1
        CLR                     ; the rest, in seat order
        LR 5,A
SR4:    CLR
        LR 4,A
SR5:    LR A,4                  ; seat i among the first n?
        XS 8
        BZ SR6
        DCI SU_ORD
        LR A,4
        ADC
        LM
        XS 5
        BZ SR7
        LR A,4
        INC
        LR 4,A
        BR SR5
SR6:    LR A,8
        CI 3
        BNC SR7
        DCI SU_ORD
        ADC
        LR A,5
        ST
        LR A,8
        INC
        LR 8,A
SR7:    LR A,5
        INC
        LR 5,A
        CI 3
        BC SR4
        LEAVE

; SULAND -- r2:r3 = seat r0's land: 500 a plot, plus the outfit cost + 35
;   for each M.U.L.E. on one (the server's ScoreOf).  Leaf.  Clobbers A, r4,
;   DC0.
SULAND: CLR
        LR 2,A
        LR 3,A
        LR 4,A
SL1:    DCI ST+M_OWNER
        LR A,4
        ADC
        LM
        XS 0
        BNZ SL3
        LR A,2                  ; + 500
        AI 0F4H
        LR 2,A
        LR A,3
        LNK
        AI 1
        LR 3,A
        DCI ST+M_MULE
        LR A,4
        ADC
        LM
        CI 3
        BNC SL3
        DCI MULEV
        ADC
        LM
        AS 2
        LR 2,A
        LR A,3
        LNK
        LR 3,A
SL3:    LR A,4
        INC
        LR 4,A
        CI NPLOTS-1
        BC SL1
        POP

; SUGOODS -- r0:r1 = seat r0's goods at the store's prices.
SUGOODS: ENTER
        LR A,0
        STB SU_K
        CLR
        STB SU_V
        ST
        STB SU_G
SG1:    LDB SU_G                ; goods[g]
        AI P_GOODS
        LR 5,A
        LDB SU_K
        LR 0,A
        PI PLDC
        LR A,5
        ADC
        LM
        LR 2,A
        LDB SU_G                ; times price[g]
        SL 1
        DCI ST+S_PRICE
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        PI MUL8
        LDW 2, 3, SU_V
        PI ADD16
        STW 0, 1, SU_V
        INCB SU_G
        CI 3
        BC SG1
        LDW 0, 1, SU_V
        LEAVE

; SUNUM -- r0:r1 right-aligned in 5 cells from cell r3, on SU_Y, in SU_C.
SUNUM:  ENTER
        LR A,3
        STB DTMP+0
        LIS 5
        LR 4,A
        LDB SU_Y
        LR 2,A
        LDB SU_C
        LR 6,A
        PI PUTNUM
        LEAVE

; SUVALS -- the numbers (they change as players press: redrawn per state).
SUVALS: ENTER
        CLR
        STB SU_I
SV1:    LDB SU_I
        DCI SU_ORD
        ADC
        LM
        STB SU_K
        LR 0,A
        PI PLCOLR
        STB SU_C
        LDB SU_I
        SL 1
        SL 1
        SL 1
        AI SUROW0+1
        STB SU_Y
        LDB SU_K                ; money
        LR 0,A
        PI PLDC
        LI P_MONEY
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LIS 1
        LR 3,A
        PI SUNUM
        LDB SU_K                ; land
        LR 0,A
        PI SULAND
        LR A,2
        LR 0,A
        LR A,3
        LR 1,A
        LIS 7
        LR 3,A
        PI SUNUM
        LDB SU_K                ; goods
        LR 0,A
        PI SUGOODS
        LI 13
        LR 3,A
        PI SUNUM
        LDB SU_K                ; the total
        LR 0,A
        PI PLDC
        LI P_SCORE
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LI 19
        LR 3,A
        PI SUNUM
        INCB SU_I
        CI 3
        JC SV1
        LI WD_COLONY            ; "COLONY 123456" on y50
        LR 0,A
        DCI STW
        PI STRP
        XDC
        DCI LINBUF
        XDC
        PI STRCPY
        XDC
        LI ' '
        ST
        XDC
        PI FMT32
        LI MSGY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI LINBUF
        PI TCENTER
        LEAVE

; FMT32 -- the colony's total (ST+C_TOTAL, 32 bits) in decimal at DC1
;   (status.c big_num32): q * 10000 + r, 65536 being 6 * 10000 + 5536.
FMT32:  ENTER
        XDC
        LR Q,DC                 ; Q: where it goes
        LDW 0, 1, ST+C_TOTAL    ; the low 16 bits: q = lo / 10000, r = lo % 10000
        CLR
        LR 6,A
        LR 7,A
F32A:   LI 10000&0FFH
        LR 2,A
        LI 10000>>8
        LR 3,A
        PI GE16
        BNC F32B
        PI SUB16
        LR A,6
        INC
        LR 6,A
        BR F32A
F32B:   DCI ST+C_TOTAL+2        ; h, the high 16: 6 and 5536 each
        LM
        LR 8,A
        LM
        LR 9,A
F32C:   LR A,8
        NI 0FFH
        BNZ F32D
        LR A,9
        NI 0FFH
        BZ F32E
        DS 9
F32D:   DS 8
        LR A,6                  ; q += 6
        AI 6
        LR 6,A
        LR A,7
        LNK
        LR 7,A
        LI 0A0H                 ; r += 5536
        LR 2,A
        LI 015H
        LR 3,A
        PI ADD16
        LI 10000&0FFH           ; r >= 10000: r -= 10000, q++
        LR 2,A
        LI 10000>>8
        LR 3,A
        PI GE16
        BNC F32C
        PI SUB16
        LR A,6
        INC
        LR 6,A
        LR A,7
        LNK
        LR 7,A
        BR F32C
F32E:   LR A,6                  ; q: then r as four digits
        NI 0FFH
        BNZ F32F
        LR A,7
        NI 0FFH
        BZ F32G
F32F:   STW 0, 1, SU_V
        LR A,6
        LR 0,A
        LR A,7
        LR 1,A
        LR DC,Q
        PI FMTNUM
        LR DC,Q
        ADC
        LR Q,DC
        LDW 0, 1, SU_V          ; r + 10000 is "1dddd": its last four
        LI 10000&0FFH
        LR 2,A
        LI 10000>>8
        LR 3,A
        PI ADD16
        DCI LINBUF+40
        PI FMTNUM
        DCI LINBUF+41
        XDC
        LR DC,Q
        XDC
        LIS 5
        LR 0,A
        CLR
        LR 1,A
        PI MEMCPY
        LEAVE
F32G:   LR DC,Q
        PI FMTNUM
        LEAVE

; SUTITLE -- "STATUS SUMMARY #n" on the top row.
SUTITLE: ENTER
        LDB RND
        LR 7,A
        LI WD_SUMMARY
        LR 0,A
        DCI STW
        PI STRP
        XDC
        DCI LINBUF
        XDC
        PI STRCPY
        XDC
        LR A,7
        LR 0,A
        CLR
        LR 1,A
        PI FMTNUM
        LI TITLEY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI LINBUF
        PI TCENTER
        LEAVE

; HEADW -- word r0 on the headings' row from pixel x r1.
HEADW:  ENTER
        LI SUHEADY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI STW
        PI STRP
        PI DTEXT
        LEAVE

; SUBANDS -- the summary's: the seats' rows light, the rest black.
SUBANDS: ENTER
        LI SUROW0
        LR 4,A
        LI SUROW0+31
        LR 5,A
        PI BANDS3
        LEAVE

; SUTABLE -- su_table: the players stand at their rows, the headings, the
;   numbers, and (the summary) the prompt.
SUTABLE: ENTER
        LIS 1
        STB SU_DRWN
        PI FXSTOP
        CLR
        STB SPCOL
        CLR
        STB SU_I
ST1:    LDB SU_I
        DCI SU_ORD
        ADC
        LM
        LR 5,A                  ; the seat
        LDB SU_I
        LR 0,A                  ; slot i
        LIS 8
        LR 6,A                  ; frame: standing, facing us
        LIS 7
        LR 1,A
        LDB SU_I
        SL 1
        SL 1
        SL 1
        AI SUROW0
        SL 1
        SL 1
        AI 0F0H
        LR 2,A
        PI SPSPEC
        INCB SU_I
        CI 3
        BC ST1
        PI SUTITLE
        LI WD_MONEY             ; the headings, right-aligned over their
        LR 0,A                  ; columns (cells 1, 7, 13, 19: 5 wide)
        LI 10
        LR 1,A
        PI HEADW
        LI WD_LAND
        LR 0,A
        LI 38
        LR 1,A
        PI HEADW
        LI WD_GOODS
        LR 0,A
        LI 58
        LR 1,A
        PI HEADW
        LI WD_TOTAL
        LR 0,A
        LI 82
        LR 1,A
        PI HEADW
        PI SUVALS
        LDB PHASE
        CI PH_SUM
        BNZ STX
        LI MSGY
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        LIS 1
        STB MSGONE
        LI MS_PRESSALL
        LR 0,A
        CLR
        STB EXPV1
        ST
        STB EXPV2
        ST
        PI MSGSHOW
        LDB MYSEAT
        CI 3
        BNC STX
        LIS SND_ALERT
        LR 0,A
        PI SNDPLAY
STX:    LEAVE

; SUBEGIN -- su_begin: the screen cleared, the players walk in (or, coming
;   in late, the table at once).
SUBEGIN: ENTER
        CLR
        STB ANIM
        PI FXSTOP
        PI DCLS
        PI SUBANDS
        LDW 0, 1, FRAMES
        STW 0, 1, PHT0
        LIS SCR_SUM
        STB SCRNOW
        CLR
        STB SU_DRWN
        CLR
        STB SPCOL
        PI SURANK
        LDB RND
        NI 0FFH
        BZ SB1
        PI PENOW
        LI 400&0FFH
        LR 2,A
        LI 400>>8
        LR 3,A
        PI GE16
        BNC SB1
        PI SUTABLE
        LEAVE
SB1:    CLR                     ; each from (143, 168), walking left
        STB SU_I
SB2:    LDB SU_I
        DCI SU_ORD
        ADC
        LM
        STB SU_K
        LR 0,A
        PI PLCOLR
        STB SU_C
        LDB SU_K
        LR 0,A
        PI PLDC
        LI P_SPEC
        ADC
        LM
        NI 7
        LR 4,A
        SL 1
        SL 1
        SL 1
        AS 4
        AS 4
        AS 4
        STB SU_G                ; fbase: species * 11
        LDB SU_C
        LR 5,A
        LDB SU_G
        LR 6,A
        LDB SU_I
        LR 0,A
        PI FXAT
        LIS AC_XL
        ADC
        LI 143
        ST
        CLR
        ST
        LI 168                  ; (status.c's 152 + 16: the rows' place)
        ST
        CLR
        ST
        ST                      ; wait, n, per, dx, dy
        ST
        ST
        ST
        ST
        LIS FK_SINGLE
        ST
        LR A,0                  ; slot i
        ST
        LIS SH_SPECIES
        ST
        LIS 2                   ; walking left: frames 2, 3
        ST
        LIS 1
        ST
        LR A,5                  ; colour
        ST
        CLR
        ST                      ; col2
        ST                      ; flags
        LR A,6
        ST                      ; fbase
        LDB SU_I                ; its script
        SL 1
        DCI FXSUWT
        ADC
        LM
        LR 10,A
        LM
        LR 11,A
        LDB SU_I
        LR 0,A
        PI FXSTART
        INCB SU_I
        CI 3
        JC SB2
        LEAVE

; SUME -- summary_enter: round 0's follows the colonists' arrival, the
;   transport ship landing first.
SUME:   ENTER
        CLR
        STB ANIM
        PI FXSTOP
        CLR
        STB SU_LND
        LDB RND
        NI 0FFH
        BNZ SE1
        PI PENOW
        LI 300&0FFH
        LR 2,A
        LI 300>>8
        LR 3,A
        PI GE16
        BC SE1
        LIS 1
        STB SU_LND
        PI MAPENTER
        LI TI_TRANSPORT
        LR 0,A
        CLR
        LR 1,A
        PI TITLE
        LI MSGY
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        LIS MS_LANDING
        LR 0,A
        CLR
        STB EXPV1
        ST
        STB EXPV2
        ST
        PI MSGSHOW
        CLR
        STB FXFLAG
        CLR
        LR 0,A
        DCI FXLAND
        LR H,DC
        PI FXSTART
        LEAVE
SE1:    PI SUBEGIN
        LEAVE

; SUMS -- summary_state: the table once its time has come; the numbers
;   again when one under them changed.
SUMS:   ENTER
        LDB SU_LND
        NI 0FFH
        JNZ SSX
        CLR
        STB SU_ALL
        LDB SU_DRWN
        NI 0FFH
        BNZ SS1
        LI 410&0FFH
        LR 2,A
        LI 410>>8
        LR 3,A
        PI PHAGE
        NI 0FFH
        BZ SS1
        PI SUTABLE
        LIS 1
        STB SU_ALL
SS1:    LDB SU_DRWN
        NI 0FFH
        JZ SSX
        LDB SU_ALL
        NI 0FFH
        BNZ SSV
        DCI ST+PLAYERS          ; the players (money, goods, score)
        XDC
        DCI PREV+PLAYERS
        LI 4*PLSIZE
        LR 0,A
        PI MEMCMP
        NI 0FFH
        BNZ SSV
        DCI ST+M_OWNER          ; the plots and their M.U.L.E.s
        XDC
        DCI PREV+M_OWNER
        LI 2*NPLOTS
        LR 0,A
        PI MEMCMP
        NI 0FFH
        BNZ SSV
        DCI ST+S_PRICE          ; the prices
        XDC
        DCI PREV+S_PRICE
        LIS 8
        LR 0,A
        PI MEMCMP
        NI 0FFH
        BNZ SSV
        DCI ST+C_TOTAL          ; the colony's total
        XDC
        DCI PREV+C_TOTAL
        LIS 4
        LR 0,A
        PI MEMCMP
        NI 0FFH
        BZ SSX
SSV:    PI SUVALS
SSX:    LEAVE

; SUMT -- summary_tick.
SUMT:   ENTER
        LDB SU_LND
        NI 0FFH
        BZ SMT1
        PI MSGTICK              ; the ship landing
        LDB FXFLAG
        CI 1
        BZ SMT0
        LDB FXA+AC_ST
        NI 0FFH
        BNZ SMTX
SMT0:    CLR
        STB SU_LND
        PI SUBEGIN
SMTX:    LEAVE
SMT1:    LDB SU_DRWN
        NI 0FFH
        BNZ SMT2
        LI 410&0FFH
        LR 2,A
        LI 410>>8
        LR 3,A
        PI PHAGE
        NI 0FFH
        BZ SMTX
        PI SUTABLE
SMT2:    PI MSGTICK
        LDB EVBTN               ; again while the server doesn't show us
        NI 0FFH                 ; ready: a press it never got mustn't hold
        BZ SMTX                  ; the whole table
        LDB MYSEAT
        CI 3
        BNC SMTX
        LR 0,A
        PI PLDC
        LI P_FLAGS
        ADC
        LM
        NI PF_READY
        BNZ SMTX
        LIS ACT_READY
        LR 0,A
        CLR
        LR 1,A
        LR 2,A
        PI ACTSEND
        LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
        LEAVE

; ---------------------------------------------------------------------------
; Game over: FIRST FOUNDER, then the final summary and the colony's rating.
; ---------------------------------------------------------------------------

; OVERE -- gameover_enter.
OVERE:  ENTER
        CLR
        STB ANIM
        PI FXSTOP
        PI DCLS
        LIS VY0
        LR 4,A
        LI 47
        LR 5,A
        PI BANDS3
        LIS SCR_FOUN
        STB SCRNOW
        CLR
        STB GO_STG
        CLR
        STB SPCOL
        PI SURANK
        LI WD_FOUNDER           ; "FIRST", the winner, "FOUNDER"
        LR 0,A
        DCI STW
        PI STRP
        LR H,DC
        LI 18
        LR 1,A
        LIS 9
        LR 2,A
        LIS C_GREEN
        LR 6,A
        LIS 5
        LR 4,A
        LR DC,H
        PI DTEXTN
        LI WD_FOUNDER
        LR 0,A
        DCI STW
        PI STRP
        LIS 6
        ADC
        LR H,DC
        LI 52
        LR 1,A
        LIS 9
        LR 2,A
        LIS C_GREEN
        LR 6,A
        LR DC,H
        PI DTEXT
        LIS 1                   ; the others stand down the left
        STB SU_I
OE1:    LDB SU_I
        DCI SU_ORD
        ADC
        LM
        LR 5,A
        LDB SU_I
        LR 0,A
        LIS 5
        LR 6,A
        LIS 8
        LR 1,A
        LDB SU_I                ; y = 28 + 36 i
        SL 4
        SL 1
        LR 3,A
        LDB SU_I
        SL 1
        SL 1
        AS 3
        AI 28
        LR 2,A
        PI SPSPEC
        INCB SU_I
        CI 3
        BC OE1
        LIS SND_FOUND
        LR 0,A
        PI SNDPLAY
        LEAVE

; OVERT -- gameover_tick.
OVERT:  ENTER
        LDB GO_STG
        NI 0FFH
        JNZ GT9
        LDB SU_ORD              ; the winner pulses, ~32 frames a step
        LR 5,A
        LDB FRAMES
        NI 32
        BZ GOT1
        LIS C_WHITE
        BR GOT2
GOT1:   LR A,5
        LR 0,A
        PI PLCOLR
GOT2:   STB SPCOL
        CLR
        LR 0,A
        LIS 5
        LR 6,A
        LI 68
        LR 1,A
        LI 12
        LR 2,A
        PI SPSPEC
        CLR
        STB SPCOL
        PI PENOW
        LI 540&0FFH
        LR 2,A
        LI 540>>8
        LR 3,A
        PI GE16
        JNC GOTX
        LIS 1
        STB GO_STG
        PI SPRHALL
        PI DCLS
        PI SUBANDS
        PI SUTABLE
        LDB ST+C_RATE           ; "OVERALL, " and the colony's rating
        CI 6
        JNC GOTX
        LR 4,A
        LI WD_OVERALL
        LR 0,A
        DCI STW
        PI STRP
        XDC
        DCI TXT
        XDC
        PI STRCPY               ; DC1 on its NUL: TXT + 9
        CLR
        STB EXPV1
        ST
        STB EXPV2
        ST
        LR A,4
        LR 0,A
        DCI STR
        PI STRP
        PI TXTEXP
        AI 9
        LR 0,A
        LI MSGY
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        LIS 1
        STB MSGONE
        PI MSGSTRT
        BR GOTX
GT9:    PI MSGTICK
GOTX:   LEAVE
