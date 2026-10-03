; status.inc -- the lobby, the status summary, the game over (status.c).
; Page 2. The summary's walk-ins are fx scripts in the grant page.

PVP     DEFL    FXEND           ; (the summary's walk-ins use the actors)
        PV      LBSPEC,1        ; our species
        PV      LBSENT,1        ; a left/right push already sent
        PV      LBLAST,4        ; per seat: species + flags*8 drawn
        PV      LBLCD,2         ; the countdown shown (seconds)
        PV      LBTMPJ,1        ; LBSEAT: species
        PV      LBTMPK,1        ;   species + flags*8
        PV      LBTMPN,1        ;   name[0]
        PV      LBTMPR,1        ;   its first row
        PV      LBTMPS,1        ;   the seat
        PV      LBNAME,10       ; a name; "OVERALL, "
        PV      SUORD,4         ; seats by rank
        PV      SUDRAWN,1
        PV      SUPRESS,1
        PV      SULAND,1
        PV      SUI,1           ; the row being drawn
        PV      SUV,2
        PV      GOSTAGE,1
        IF      PVP > RAMEND
RAMEND  DEFL    PVP
        ENDIF

LBROW0  EQU     3               ; seat k: rows 3+2k, 4+2k
SUROW0  EQU     2               ; summary: seat i's rows 2+2i, 3+2i

; ATY: the Atari line that SPRSET puts on screen line L (6 + (y-16)*3/8).
ATY     MACRO   lin
        DW      16+((lin)-6)*8/3+1
        ENDM

; the phase entries (loop.inc PGENT)
STENTER: LD     A,(PHS)
        CP      PH_LOBBY
        JP      Z,LBENTER
        CP      PH_SUMM
        JP      Z,SUENTER
        JP      GOENTER
STSTATE: LD     A,(PHS)
        CP      PH_LOBBY
        JP      Z,LBSTATE
        CP      PH_SUMM
        JP      Z,SUSTATE
        RET
; LVASK: the leave key (C) outside the lobby, from MAINLP. NC: not ours
; (the lobby's tick, test mode, no state yet). C: we asked; the game stood
; still meanwhile (no PE_ANIM, the keys swallowed), and on a second C we left.
LVASK:  LD      A,(STOK)
        OR      A
        RET     Z
        LD      A,(PHS)
        OR      A               ; PH_LOBBY
        RET     Z
        LD      A,(TESTMD)
        OR      A
        RET     NZ
        LD      B,M_LEAVEASK
        CALL    MSGSHOW
LVA1:   CALL    FRWAIT
        CALL    SNDTICK
        CALL    MSGTICK
        CALL    INPOLL
        CALL    INTAKE
        LD      A,(EVBTN)
        OR      A
        JR      NZ,LVA2
        LD      A,(EVKEY)
        OR      A
        JR      Z,LVA1
        CP      KEYLV
        JR      NZ,LVA2
        CALL    FXSTOP
; LVGO: leave the table (the server gives our seat to the computer), then
; the table list and a new join.
LVGO:   LD      A,A_LEAVE
        LD      (ACT),A
        CALL    NETSYNC
        CALL    FARCALL
        DB      PGBOOT
        DW      REROOM
        SCF
        RET
LVA2:   CALL    MSGCLR          ; no: carry on
        XOR     A
        LD      (EVBTN),A
        LD      (EVKEY),A
        LD      HL,(FRAMES)     ; and don't catch up the frames we asked in
        LD      (DONE),HL
        SCF
        RET

; SPTICK: every frame, from MAINLP. With a seat, the leave key asks
; (LVASK). Without one (the game was under way when we joined, or we left
; it), only the leave key counts, and it leaves at once: there is no seat to
; lose. SPECTATOR goes in a corner each screen leaves free -- row 0, but
; under the big titles of the auctions and the summaries -- every 32 frames,
; as a repaint takes it off. C: the loop starts again (we asked, or left).
SPTICK: LD      A,(STOK)
        OR      A
        JR      Z,SPT8
        LD      A,(MYSEAT)
        CP      4
        JR      NC,SPT1
        LD      HL,SPAT         ; a seat now: the tag off (the lobby's row 0)
        BIT     0,(HL)
        JR      Z,SPT8
        RES     0,(HL)
        LD      HL,SPTXT+9
        LD      DE,0
        LD      B,9
        LD      C,CW
        CALL    TXTN
SPT8:   LD      A,(EVKEY)
        CP      KEYLV
        JP      Z,LVASK
        OR      A
        RET
SPT1:   XOR     A
        LD      (EVBTN),A
        LD      (INDIR),A
        LD      A,(EVKEY)
        CP      KEYLV
        JR      Z,SPT5
SPT2:   XOR     A
        LD      (EVKEY),A
        LD      A,(FRAMES)
        AND     0E0H
        LD      HL,SPAT
        XOR     (HL)
        AND     0E0H
        RET     Z               ; (NC)
        XOR     (HL)
        OR      1
        LD      (HL),A
        LD      A,(SCRNOW)
        OR      A
        RET     Z
        LD      D,0
        CP      SCR_AUC
        JR      NZ,SPT3
        LD      D,2
SPT3:   CP      SCR_SUMM
        JR      NZ,SPT4
        INC     D
SPT4:   CP      SCR_FOUND       ; the founder, then the final table
        JR      NZ,SPT6
        LD      A,(GOSTAGE)
        OR      A
        JR      Z,SPT6
        INC     D
SPT6:   LD      E,0
        LD      HL,SPTXT
        LD      C,CW
        CALL    TXTAT
        OR      A
        RET
SPT5:   LD      A,(PHS)
        OR      A               ; PH_LOBBY: its tick leaves
        RET     Z
        LD      A,(TESTMD)
        OR      A
        JR      NZ,SPT2
        CALL    FXSTOP
        JP      LVGO
SPTXT:  DB      "SPECTATOR",0

STTICK: LD      A,(PHS)
        CP      PH_LOBBY
        JP      Z,LBTICK
        CP      PH_SUMM
        JP      Z,SUTICK
        JP      GOTICK
STANIM: CALL    FARCALL
        DB      PGGRANT
        DW      FXRUN
        RET

; STANDSP: slot A: seat B's species standing (frame f in C) at x E, on
; screen line D. From the window (a state that just arrived) or HOT.
STANDSP: PUSH   AF
        LD      A,E
        LD      (SPA_X),A
        XOR     A
        LD      (SPA_X+1),A
        LD      (SPA_FL),A
        LD      A,SH_SPEC
        LD      (SPA_SH),A
        LD      A,D             ; the line -> the Atari's
        SUB     6
        LD      L,A
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL           ; *8
        PUSH    BC
        LD      B,3             ; /3
        CALL    DIVB
        POP     BC
        LD      DE,17
        ADD     HL,DE
        LD      (SPA_Y),HL
        LD      A,B
        CALL    SPSPEC          ; species * 11 + f
        ADD     A,C
        LD      (SPA_FR),A
        LD      A,B
        CALL    SPCOL
        LD      (SPA_COL),A
        POP     AF
        JP      SPRSET

; DIVB: HL = HL / B (small).
DIVB:   LD      DE,0
        LD      C,B
        LD      B,0
DVB1:   OR      A
        SBC     HL,BC
        JR      C,DVB2
        INC     DE
        JR      DVB1
DVB2:   EX      DE,HL
        RET

; SPSPEC: A = seat A's species * 11 (window if MAPSRC, else HOT).
SPSPEC: PUSH    HL
        PUSH    DE
        LD      E,A
        LD      A,(MAPSRC)
        OR      A
        LD      A,E
        JR      Z,SPS1
        LD      E,PL_SPEC
        CALL    WPLAYER
        CALL    RXGETB
        JR      SPS2
SPS1:   CALL    HPLAYER         ; HP_SPEC
        LD      A,(HL)
SPS2:   AND     7
        LD      E,A
        ADD     A,A
        ADD     A,A
        ADD     A,E             ; *5
        ADD     A,A             ; *10
        ADD     A,E             ; *11
        POP     DE
        POP     HL
        RET

; SPCOL: A = seat A's colour (window if MAPSRC, else HOT).
SPCOL:  PUSH    HL
        PUSH    DE
        LD      E,A
        LD      A,(MAPSRC)
        OR      A
        LD      A,E
        JR      Z,SPC1
        CALL    WPLCOL
        JR      SPC2
SPC1:   CALL    PLCOLOR
SPC2:   POP     DE
        POP     HL
        RET

; WPLCOL: seat A's colour code, from the window.
WPLCOL: PUSH    HL
        PUSH    DE
        LD      E,PL_COL
        CALL    WPLAYER
        CALL    RXGETB
        AND     3
        LD      E,A
        LD      D,0
        LD      HL,PLCOLT
        ADD     HL,DE
        LD      A,(HL)
        POP     DE
        POP     HL
        RET

; ==============================================================================
; the lobby: seats, species, ready (the Atari's "you have selected a
; tournament level game" screen). Left/right change our species, the
; trigger toggles ready, C leaves the table.
; ==============================================================================
LBENTER: XOR    A
        LD      (ANIM),A
        CALL    SPROFF
        CALL    CLRSCR
        LD      A,SCR_TEXT
        LD      (SCRNOW),A
        LD      HL,LBLAST
        LD      B,4
LBE1:   LD      (HL),255
        INC     HL
        DJNZ    LBE1
        LD      A,3
        LD      (SDMODE),A
        LD      A,K_WORD
        LD      B,W_MULE
        LD      D,0
        LD      C,CO
        CALL    SDRAWM
        LD      A,3
        LD      (SDMODE),A
        LD      A,K_WORD
        LD      B,W_TOURN
        LD      D,1
        LD      C,CB
        CALL    SDRAWM
        LD      A,2
        LD      (SDMODE),A
        LD      A,K_MSG
        LD      B,M_LOBBYHELP
        LD      D,11
        LD      C,CW
        CALL    SDRAWM
        XOR     A
        LD      (LBSENT),A
        LD      HL,0FFFEH       ; neither a count nor waiting
        LD      (LBLCD),HL
        LD      A,12
        LD      (MSGROW),A
        LD      A,CW
        LD      (MSGCOL),A
        LD      A,(MYSEAT)
        CP      4
        RET     NC
        LD      E,PL_SPEC
        CALL    WPLAYER
        CALL    RXGETB
        AND     7
        LD      (LBSPEC),A
        RET

LBSTATE: LD     B,0
LBS1:   PUSH    BC
        CALL    LBSEAT
        POP     BC
        INC     B
        LD      A,B
        CP      4
        JR      NZ,LBS1
        LD      A,(MYSEAT)
        CP      4
        RET     C
        LD      B,M_SPECTATE
        LD      HL,0
        LD      D,H
        LD      E,L
        JP      MSGSHOW

; LBSEAT: seat B's rows, if they changed.
LBSEAT: LD      A,B
        LD      (LBTMPS),A
        LD      E,PL_FLAG
        CALL    WPLAYER
        CALL    RXGETB
        LD      C,A             ; flags
        DEC     HL
        DEC     HL
        CALL    RXGETB          ; species
        AND     7
        LD      (LBTMPJ),A
        LD      A,C
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      HL,LBTMPJ
        ADD     A,(HL)
        LD      (LBTMPK),A      ; species + flags*8
        LD      A,B
        LD      E,0
        CALL    WPLAYER
        CALL    RXGETB
        LD      (LBTMPN),A      ; name[0]
        LD      E,B
        LD      D,0
        LD      HL,LBLAST
        ADD     HL,DE
        LD      A,(LBTMPK)
        CP      (HL)
        JR      NZ,LBSE1
        LD      A,(LBTMPN)
        OR      A
        RET     NZ
LBSE1:  LD      A,(LBTMPK)
        LD      (HL),A
        LD      A,(LBTMPS)
        ADD     A,A
        ADD     A,LBROW0
        LD      (LBTMPR),A
        CALL    CLRROW
        LD      A,(LBTMPR)
        INC     A
        CALL    CLRROW
        LD      A,(LBTMPN)
        OR      A
        JR      NZ,LBSE2
        LD      A,(LBTMPS)      ; an open seat
        CALL    SPRHIDE
        LD      A,(LBTMPR)
        LD      D,A
        LD      E,8
        LD      A,K_WORD
        LD      B,W_OPEN
        LD      C,CW
        JP      SDRAW
LBSE2:  LD      A,(LBTMPS)      ; the name, big, in the seat's colour
        LD      E,0
        CALL    WPLAYER
        LD      DE,LBNAME
        LD      BC,8
        CALL    RXCOPY
        XOR     A
        LD      (DE),A
        LD      A,(LBTMPS)
        CALL    WPLCOL
        LD      C,A
        LD      A,(LBTMPR)
        LD      D,A
        LD      E,8
        LD      HL,LBNAME
        LD      B,8
        CALL    BIGN
        LD      A,(LBTMPR)      ; the species
        LD      D,A
        LD      E,28
        LD      A,(LBTMPJ)
        LD      B,A
        LD      A,K_SPEC
        LD      C,CW
        CALL    SDRAW
        LD      A,(LBTMPR)      ; the species, standing, at the left
        LD      B,6
        CALL    MULB
        ADD     A,2
        LD      D,A
        LD      E,12
        LD      A,(LBTMPS)
        LD      B,A
        LD      C,5
        CALL    STANDSP
        LD      A,(LBTMPK)      ; computer / ready
        RRCA
        RRCA
        RRCA
        LD      C,A
        LD      A,K_WORD
        LD      B,W_COMPUTER
        BIT     0,C             ; PF_AI
        JR      NZ,LBSE3
        LD      A,K_MSG
        LD      B,M_READY
        BIT     1,C             ; PF_READY
        RET     Z
LBSE3:  PUSH    AF
        LD      A,(LBTMPR)
        INC     A
        LD      D,A
        LD      E,28
        POP     AF
        LD      C,CW
        JP      SDRAW

; MULB: A = A * B (small).
MULB:   LD      C,A
        XOR     A
MLB1:   ADD     A,C
        DJNZ    MLB1
        RET

LBTICK: CALL    MSGTICK
        LD      A,(EVKEY)       ; C: leave this table and pick another
        CP      KEYLV
        JP      Z,LVGO
LBT1:   ; the countdown to the start, in seconds
        LD      HL,(HOTH+W_CDOWN)
        BIT     7,H
        JR      NZ,LBT3
        PUSH    HL
        CALL    SINCERX
        EX      DE,HL           ; DE = since
        POP     HL              ; HL = countdown
        OR      A
        SBC     HL,DE
        JR      NC,LBT2
        LD      HL,0
LBT2:   LD      B,60
        CALL    DIVB
        LD      DE,(LBLCD)
        OR      A
        SBC     HL,DE
        ADD     HL,DE
        JR      Z,LBT4
        INC     D               ; was it none or waiting (0FFxxH)?
        CALL    Z,SNDALRT       ; the countdown has begun
        LD      (LBLCD),HL
        LD      B,M_LEAVEHELP
        LD      DE,0
        CALL    MSGSHOW
        JR      LBT4
LBT3:   LD      HL,(LBLCD)
        LD      A,H
        AND     L
        INC     A
        JR      Z,LBT4
        LD      HL,0FFFFH
        LD      (LBLCD),HL
        LD      B,M_WAITING
        LD      HL,0
        LD      D,H
        LD      E,L
        CALL    MSGSHOW
LBT4:   LD      A,(MYSEAT)
        CP      4
        RET     NC
        ; species: left/right, one change a push
        LD      A,(INDIR)
        AND     DIRLT+DIRRT
        JR      NZ,LBT5
        LD      (LBSENT),A
        JR      LBT6
LBT5:   LD      B,A
        LD      A,(LBSENT)
        OR      A
        JR      NZ,LBT6
        INC     A
        LD      (LBSENT),A
        LD      A,(LBSPEC)
        INC     A
        BIT     1,B             ; DIRRT
        JR      NZ,LBT5A
        SUB     2
LBT5A:  AND     7
        LD      (LBSPEC),A
        LD      L,A
        LD      H,0
        LD      A,A_SPEC
        CALL    ACTSEND
        LD      A,SNSEL
        CALL    SNDPLAY
LBT6:   LD      A,(EVBTN)
        OR      A
        RET     Z
        LD      A,(MYSEAT)
        CALL    HPLAYER
        INC     HL
        INC     HL              ; HP_FLAG
        LD      A,(HL)
        AND     PF_READY
        LD      A,A_READY
        JR      Z,LBT7
        LD      A,A_UNRDY
LBT7:   LD      HL,0
        CALL    ACTSEND
        LD      A,SNSEL
        JP      SNDPLAY

; ==============================================================================
; the status summary (410 jiffies of drawing, then everyone presses their
; button): the players walk in from the right along the bottom, then climb
; to their rows in rank order; then the table prints.
; ==============================================================================

; SURANK: the seats in rank order (rank 1 first); anyone without a rank last.
SURANK: LD      HL,SUORD
        LD      C,0             ; C = found so far (bit per seat)
        LD      D,1             ; rank
SUR1:   LD      B,0
SUR2:   PUSH    HL
        LD      A,B
        CALL    SURANKOF
        POP     HL
        CP      D
        JR      NZ,SUR3
        LD      (HL),B
        INC     HL
        LD      A,B
        CALL    SUBIT
        OR      C
        LD      C,A
SUR3:   INC     B
        LD      A,B
        CP      4
        JR      NZ,SUR2
        INC     D
        LD      A,D
        CP      5
        JR      NZ,SUR1
        LD      B,0             ; the rest
SUR4:   LD      A,B
        CALL    SUBIT
        AND     C
        JR      NZ,SUR5
        LD      (HL),B
        INC     HL
SUR5:   INC     B
        LD      A,B
        CP      4
        JR      NZ,SUR4
        RET
SUBIT:  PUSH    BC
        LD      B,A
        INC     B
        LD      A,80H
SUB1:   RLCA
        DJNZ    SUB1
        POP     BC
        RET

; SURANKOF: A = seat A's rank (window if MAPSRC, else HOT). Keeps B, C, D.
SURANKOF: LD    E,A
        LD      A,(MAPSRC)
        OR      A
        LD      A,E
        JR      Z,SRO1
        LD      E,PL_RANK
        CALL    WPLAYER
        JP      RXGETB
SRO1:   CALL    HPLAYER
        LD      A,HP_RANK
        ADD     A,L
        LD      L,A
        JR      NC,SRO2
        INC     H
SRO2:   LD      A,(HL)
        RET

; SUENTER: round 0's summary follows the colonists' arrival: the Atari
; lands the transport ship first ("you're landing on the planet Irata.").
SUENTER: XOR    A
        LD      (ANIM),A
        LD      (SULAND),A
        LD      A,(RND)
        OR      A
        JR      NZ,SUBEGIN
        CALL    PENOW
        LD      DE,300
        OR      A
        SBC     HL,DE
        JR      NC,SUBEGIN
        LD      A,1
        LD      (SULAND),A
        CALL    MAPENTER
        LD      B,T_TRANSPORT
        LD      HL,0
        CALL    SHTITLE
        LD      A,11
        LD      (MSGROW),A
        LD      A,CW
        LD      (MSGCOL),A
        LD      B,M_LANDING
        LD      HL,0
        LD      D,H
        LD      E,L
        CALL    MSGSHOW
        CALL    FARCALL
        DB      PGGRANT
        DW      XFXLAND
        RET

SUBEGIN: XOR    A
        LD      (ANIM),A
        CALL    FXSTOP
        CALL    SPROFF
        CALL    CLRSCR
        LD      HL,(FRAMES)
        LD      (PHT0),HL
        LD      A,SCR_SUMM
        LD      (SCRNOW),A
        XOR     A
        LD      (SUDRAWN),A
        LD      (SUPRESS),A
        CALL    SURANK
        ; arriving late: straight to the table
        CALL    PENOW
        LD      DE,401
        OR      A
        SBC     HL,DE
        JR      C,SUB2
        LD      A,(RND)
        OR      A
        JP      NZ,SUTABLE
SUB2:   XOR     A
SUB3:   LD      (SUI),A
        CALL    FXACTOR         ; IX = actor SUI
        LD      A,(SUI)
        LD      E,A
        LD      D,0
        LD      HL,SUORD
        ADD     HL,DE
        LD      B,(HL)          ; B = seat
        LD      (IX+AC_KIND),FK_SINGLE
        LD      A,(SUI)
        LD      (IX+AC_SLOT),A
        LD      (IX+AC_SHAPE),SH_SPEC
        LD      A,B
        CALL    SPSPEC
        LD      (IX+AC_FBASE),A
        LD      (IX+AC_FRAME),2
        LD      (IX+AC_D2),1
        LD      A,B
        CALL    SPCOL
        LD      (IX+AC_COL),A
        LD      (IX+AC_FLAGS),0
        LD      (IX+AC_XL),143
        LD      (IX+AC_XH),0
        LD      (IX+AC_YL),158  ; (the Atari's 152, plus the rows' offset)
        LD      (IX+AC_YH),0
        LD      A,(SUI)
        CALL    FARCALL
        DB      PGGRANT
        DW      XFXSUW
        LD      A,(SUI)
        INC     A
        CP      4
        JR      C,SUB3
        RET

; FXACTOR: IX = actor A (fx.inc ACTOR, which is in the grant page).
FXACTOR: LD     E,A
        ADD     A,A
        ADD     A,E
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      E,A
        LD      D,0
        LD      IX,FXA
        ADD     IX,DE
        RET

; SUTABLE: the table (status.c su_table).
SUTABLE: LD     A,1
        LD      (SUDRAWN),A
        CALL    FXSTOP
        XOR     A               ; the players stand at the left of their rows
SUT1:   LD      (SUI),A
        LD      E,A
        LD      D,0
        LD      HL,SUORD
        ADD     HL,DE
        LD      B,(HL)
        ADD     A,A
        ADD     A,SUROW0
        LD      C,6
        PUSH    BC
        LD      B,C
        CALL    MULB
        POP     BC
        ADD     A,2
        LD      D,A
        LD      E,11
        LD      C,8
        LD      A,(SUI)
        CALL    STANDSP
        LD      A,(SUI)
        INC     A
        CP      4
        JR      C,SUT1
        CALL    SUTITLE         ; "STATUS SUMMARY #n"
        LD      A,K_WORD
        LD      B,W_MONEY
        LD      D,1
        LD      E,13
        LD      C,CW
        CALL    SDRAW
        LD      A,K_WORD
        LD      B,W_LAND
        LD      D,1
        LD      E,24
        LD      C,CW
        CALL    SDRAW
        LD      A,K_WORD
        LD      B,W_GOODS
        LD      D,1
        LD      E,35
        LD      C,CW
        CALL    SDRAW
        LD      A,1
        LD      (SDMODE),A
        LD      A,K_WORD
        LD      B,W_COLONY
        LD      D,10
        LD      E,6
        LD      C,CO
        CALL    SDRAWM
        CALL    SUVALUES
        LD      A,(PHS)
        CP      PH_SUMM
        RET     NZ
        CALL    SNDALRT
        LD      A,11
        LD      (MSGROW),A
        LD      A,CW
        LD      (MSGCOL),A
        LD      B,M_PRESSALL
        LD      HL,0
        LD      D,H
        LD      E,L
        JP      MSGSHOW

; SUTITLE: "STATUS SUMMARY #" and the round, big, centred on row D.
SUTITLE: LD     A,(RND)
        LD      L,A
        LD      H,0
        CALL    NUMSTR          ; the round in LINBUF, A its length
        LD      B,A
        LD      A,16            ; "STATUS SUMMARY #" is 16
        ADD     A,B
        LD      B,A
        LD      A,20
        SUB     B
        AND     0FEH
        LD      E,A
        PUSH    DE
        LD      A,0
        CALL    CLRROW
        POP     DE
        PUSH    DE
        LD      A,1
        LD      (SDMODE),A
        LD      A,K_WORD
        LD      B,W_SUMMARY
        LD      D,0
        LD      C,CO
        CALL    SDRAWM
        POP     DE
        LD      A,E
        ADD     A,32
        LD      E,A
        LD      D,0
        LD      HL,LINBUF
        LD      C,CO
        JP      BIGAT

; SUVALUES: the numbers (they change as players press: redrawn per state).
; From the window (a state has just arrived, or STVALID).
SUVALUES: XOR   A
SUV1:   LD      (SUI),A
        LD      E,A
        LD      D,0
        LD      HL,SUORD
        ADD     HL,DE
        LD      A,(HL)
        LD      (LBTMPS),A      ; the seat
        CALL    WPLCOL
        LD      (LBTMPJ),A      ; its colour
        LD      A,(SUI)
        ADD     A,A
        ADD     A,SUROW0
        LD      (LBTMPR),A      ; its row
        ; money
        LD      A,(LBTMPS)
        LD      E,PL_MONEY
        CALL    WPLAYER
        CALL    RXGETW
        EX      DE,HL
        LD      B,6
        LD      E,6
        CALL    SUBIG
        ; land: 500 a plot, plus the outfit cost + 35 for each M.U.L.E. on one
        LD      HL,0
        LD      (SUV),HL
        LD      C,0
SUV2:   LD      A,C
        ADD     A,W_OWNER-W_TERR
        LD      E,A
        LD      D,0
        LD      HL,W_TERR
        ADD     HL,DE
        CALL    RXGETB
        LD      B,A
        LD      A,(LBTMPS)
        CP      B
        JR      NZ,SUV3
        LD      HL,(SUV)
        LD      DE,500
        ADD     HL,DE
        LD      (SUV),HL
        LD      A,C
        ADD     A,W_MULES-W_TERR
        LD      E,A
        LD      D,0
        LD      HL,W_TERR
        ADD     HL,DE
        CALL    RXGETB
        CP      4
        JR      NC,SUV3
        INC     A               ; (good + 1) * 25 + 35
        LD      B,A
        LD      HL,(SUV)
        LD      DE,25
SUV2A:  ADD     HL,DE
        DJNZ    SUV2A
        LD      DE,35
        ADD     HL,DE
        LD      (SUV),HL
SUV3:   INC     C
        LD      A,C
        CP      NPLOTS
        JR      C,SUV2
        LD      HL,(SUV)
        LD      B,5
        LD      E,18
        CALL    SUBIG
        ; goods: each good's units at the store's price
        LD      HL,0
        LD      (SUV),HL
        LD      C,0
SUV4:   LD      A,(LBTMPS)
        LD      E,PL_GOODS
        CALL    WPLAYER
        LD      A,C
        ADD     A,L
        LD      L,A
        JR      NC,SUV4A
        INC     H
SUV4A:  CALL    RXGETB
        OR      A
        JR      Z,SUV5
        LD      B,A
        LD      A,C
        ADD     A,A
        ADD     A,W_STORE+4 AND 0FFH
        LD      L,A
        LD      H,(W_STORE+4) SHR 8
        JR      NC,SUV4B
        INC     H
SUV4B:  CALL    RXGETW          ; the price
        LD      HL,(SUV)
SUV4C:  ADD     HL,DE
        DJNZ    SUV4C
        LD      (SUV),HL
SUV5:   INC     C
        LD      A,C
        CP      4
        JR      C,SUV4
        LD      HL,(SUV)
        LD      B,6
        LD      E,28
        CALL    SUBIG
        ; "TOTAL" and the score
        LD      A,(LBTMPR)
        INC     A
        LD      D,A
        LD      E,8
        LD      A,K_WORD
        LD      B,W_TOTAL
        LD      C,CW
        CALL    SDRAW
        LD      A,(LBTMPS)
        LD      E,PL_SCORE
        CALL    WPLAYER
        CALL    RXGETW
        EX      DE,HL
        LD      A,(LBTMPR)
        INC     A
        LD      (LBTMPR),A
        LD      B,7
        LD      E,26
        CALL    SUBIG
        LD      A,(SUI)
        INC     A
        CP      4
        JP      C,SUV1
        ; the colony's total
        LD      HL,W_COL+6
        LD      DE,LBNAME
        LD      BC,4
        CALL    RXCOPY
        LD      HL,LBNAME
        LD      B,7
        CALL    NUM32R
        LD      HL,LINBUF
        LD      D,10
        LD      E,26
        LD      C,CO
        LD      B,7
        JP      BIGN

; SUBIG: HL right-aligned in B big characters at column E of row LBTMPR,
; in the colour LBTMPJ.
SUBIG:  PUSH    DE
        CALL    NUMR
        POP     DE
        LD      A,(LBTMPR)
        LD      D,A
        LD      A,(LBTMPJ)
        LD      C,A
        LD      HL,LINBUF
        JP      BIGAT

SUSTATE: LD     A,(SULAND)
        OR      A
        RET     NZ
        CALL    SU410
        CALL    NC,SUTABLE
        LD      A,(SUDRAWN)
        OR      A
        RET     Z
        JP      SUVALUES

; SU410: carry if not drawn yet and 410 frames have not passed; NC when
; the table is due (and not yet drawn). Z/NZ unused.
SU410:  LD      A,(SUDRAWN)
        OR      A
        SCF
        RET     NZ
        LD      HL,(FRAMES)
        LD      DE,(PHT0)
        OR      A
        SBC     HL,DE
        LD      DE,410
        OR      A
        SBC     HL,DE
        RET                     ; C: not yet

SUTICK: LD      A,(SULAND)
        OR      A
        JR      Z,SUT2
        CALL    MSGTICK
        LD      A,(FXFLAG)
        CP      1
        JR      Z,SUT1A
        XOR     A
        CALL    FXACTOR
        LD      A,(IX+AC_ST)
        OR      A
        RET     NZ
SUT1A:  XOR     A
        LD      (SULAND),A
        JP      SUBEGIN
SUT2:   LD      A,(STVALID)     ; the table reads the window
        OR      A
        JR      Z,SUT3
        CALL    SU410
        JR      C,SUT3
        LD      A,1
        LD      (MAPSRC),A
        CALL    SUTABLE
        XOR     A
        LD      (MAPSRC),A
SUT3:   LD      A,(SUDRAWN)
        OR      A
        RET     Z
        CALL    MSGTICK
        LD      A,(EVBTN)
        OR      A
        RET     Z
        LD      A,(SUPRESS)
        OR      A
        RET     NZ
        LD      A,(MYSEAT)
        CP      4
        RET     NC
        LD      A,1
        LD      (SUPRESS),A
        LD      A,A_READY
        LD      HL,0
        CALL    ACTSEND
        LD      A,SNSEL
        JP      SNDPLAY

; ==============================================================================
; game over: FIRST FOUNDER, then the final summary and the colony's rating
; ==============================================================================
GOENTER: XOR    A
        LD      (ANIM),A
        LD      (GOSTAGE),A
        CALL    FXSTOP
        CALL    SPROFF
        CALL    CLRSCR
        LD      A,SCR_FOUND
        LD      (SCRNOW),A
        CALL    SURANK
        LD      HL,SFIRST
        LD      D,1
        LD      E,4
        LD      C,CS
        CALL    BIGAT
        LD      HL,SFOUNDER
        LD      D,1
        LD      E,24
        LD      C,CS
        CALL    BIGAT
        LD      A,1             ; the others stand down the left
GOE1:   LD      (SUI),A
        LD      E,A
        LD      D,0
        LD      HL,SUORD
        ADD     HL,DE
        LD      B,(HL)
        LD      C,A             ; lines 18 + 16 i
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,2
        LD      D,A
        LD      E,4
        LD      C,5
        LD      A,(SUI)
        CALL    STANDSP
        LD      A,(SUI)
        INC     A
        CP      4
        JR      C,GOE1
        LD      A,SNFOUND
        JP      SNDPLAY

SFIRST: DB      "FIRST",0
SFOUNDER: DB    "FOUNDER",0

GOTICK: LD      A,(GOSTAGE)
        OR      A
        JP      NZ,MSGTICK
        ; the winner pulses, ~32 frames a step
        LD      A,(SUORD)
        LD      B,A
        LD      A,(FRAMES)
        AND     32
        LD      A,CW
        JR      NZ,GOT2
        LD      A,B
        CALL    SPCOL
GOT2:   PUSH    AF
        LD      A,B
        CALL    SPSPEC
        ADD     A,5
        LD      (SPA_FR),A
        LD      A,SH_SPEC
        LD      (SPA_SH),A
        LD      HL,76
        LD      (SPA_X),HL
        LD      HL,40           ; (screen line 14)
        LD      (SPA_Y),HL
        XOR     A
        LD      (SPA_FL),A
        POP     AF
        LD      (SPA_COL),A
        XOR     A
        CALL    SPRSET
        CALL    PENOW
        LD      DE,541
        OR      A
        SBC     HL,DE
        RET     C
        LD      A,(STVALID)     ; the table reads the window
        OR      A
        RET     Z
        LD      A,1
        LD      (GOSTAGE),A
        CALL    SPROFF
        CALL    CLRSCR
        LD      A,1
        LD      (MAPSRC),A
        CALL    SUTABLE
        XOR     A
        LD      (MAPSRC),A
        ; "OVERALL, " + the colony's rating
        LD      HL,W_COL+2
        CALL    RXGETB
        CP      7
        RET     NC
        PUSH    AF
        LD      A,K_WORD
        LD      C,W_OVERALL
        LD      DE,LBNAME
        LD      B,10
        CALL    STRGET
        LD      HL,LBNAME
        LD      (MSGPFX),HL
        POP     BC              ; B = the rating
        LD      C,B
        LD      A,K_RATE
        CALL    STRLOC
        LD      A,B
        LD      (MSGPG),A
        LD      (MSGADR),HL
        LD      A,11
        LD      (MSGROW),A
        LD      A,CW
        LD      (MSGCOL),A
        JP      MSGSTART
