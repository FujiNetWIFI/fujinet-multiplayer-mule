; loop.inc -- the frame loop and a state's arrival (main.c). Resident.
;
; Every phase page opens with its entry table at PGENT (after the 55H
; header): ENTER, STATE, TICK, ANIM -- the C client's enters[], states[],
; ticks[] and the per-frame animation (walk_isr / grant_isr / auction_isr,
; then fx_run). The phase's page is mapped while the loop runs, so the loop
; calls them directly; ARRIVE maps the next one.

PGENT   EQU     2007H
PE_ENTER EQU    PGENT
PE_STATE EQU    PGENT+3
PE_TICK EQU     PGENT+6
PE_ANIM EQU     PGENT+9

; the page each phase's code is in
PGOF:   DB      PGSTAT,PGGRANT,PGAUC,PGDEV,PGGRANT,PGAUC,PGAUC,PGSTAT,PGSTAT

; MAINLP: the frame loop. Never returns.
MAINLP: LD      A,1
        LD      (DBGAT),A
        CALL    FRWAIT
        LD      A,2
        LD      (DBGAT),A
        CALL    SNDTICK
        CALL    INPOLL
        CALL    INTAKE
        LD      A,(EVKEY)       ; C in a game: ask, then maybe leave
        CP      KEYLV
        JR      NZ,ML0
        CALL    FARCALL
        DB      PGSTAT
        DW      LVASK
        JP      C,MAINLP
ML0:
        IF      AUTOPLAY
        CALL    FARCALL
        DB      PGTEST
        DW      AUTOPL
        ENDIF
        ; what moves by itself, once for every frame that has passed (a slow
        ; frame, or a network call, is caught up with), at most 8
        LD      HL,(FRAMES)
        LD      DE,(DONE)
        LD      (DONE),HL
        OR      A
        SBC     HL,DE
        LD      A,H
        OR      A
        JR      NZ,ML1
        LD      A,L
        CP      9
        JR      C,ML2
ML1:    LD      A,8
ML2:    OR      A
        JR      Z,ML4
        LD      B,A
ML3:    PUSH    BC
        CALL    PE_ANIM
        POP     BC
        DJNZ    ML3
ML4:    LD      A,3
        LD      (DBGAT),A
        ; the network: one step a frame
        LD      A,(NSTATE)
        OR      A
        JR      Z,ML5
        CALL    FARCALL
        DB      PGNET
        DW      NETSTEP
        CALL    C,REPLY
        JR      ML7
ML5:    LD      HL,POLLW
        LD      A,(HL)
        OR      A
        JR      Z,ML6
        DEC     (HL)
        JR      Z,ML6
        LD      A,(ACT)
        OR      A
        JR      Z,ML7
ML6:    LD      A,(TESTMD)
        OR      A
        JR      Z,ML6N
        CALL    FARCALL
        DB      PGTEST
        DW      TESTPOLL
        CALL    REPLY           ; (a made-up state, as if it had arrived)
        JR      ML7
ML6N:   CALL    FARCALL
        DB      PGNET
        DW      NETBEGIN
        LD      A,(NSTATE)
        OR      A
        CALL    Z,REPLY
ML7:    LD      A,4
        LD      (DBGAT),A
        LD      A,(STOK)
        OR      A
        CALL    NZ,PE_TICK
        JP      MAINLP

; REPLY: a request is over (main.c reply). The net page reads the reply;
; the phase's page is mapped here, and its entries run.
REPLY:  CALL    FARCALL
        DB      PGNET
        DW      REPLY1
        CP      2
        JR      NZ,RPY1
        CALL    FARCALL         ; refused outright: another table
        DB      PGBOOT
        DW      REROOM
        JR      RPY9
RPY1:   OR      A
        JR      Z,RPY9
        LD      A,(PHS)         ; a state: its page, its entries
        LD      E,A
        LD      D,0
        LD      HL,PGOF
        ADD     HL,DE
        LD      A,(HL)
        CALL    SELPG
        LD      A,1             ; the map is drawn from the window meanwhile
        LD      (MAPSRC),A
        LD      A,(NEWPH)
        OR      A
        JR      Z,RPY2
        XOR     A
        LD      (MSGSCR),A
        LD      (ANIM),A
        CALL    FXSTOP
        CALL    PE_ENTER
RPY2:   CALL    PE_STATE
        CALL    FARCALL
        DB      PGNET
        DW      HOTCPY
        XOR     A
        LD      (MAPSRC),A
RPY9:   CALL    FARCALL
        DB      PGNET
        DW      POLLRATE
        RET

DV_WAIT EQU     0
DV_PROMPT EQU   1
DV_WALK EQU     2
DV_OVER EQU     3

; NETSYNC: the queued action now, and its answer (net.c net_sync).
NETSYNC: CALL   FARCALL
        DB      PGNET
        DW      XNETSYNC
        RET

; ---- helpers the phases share ----------------------------------------------
; PENOW: HL = the phase's elapsed jiffies now (rx_pe + frames since).
PENOW:  LD      HL,(FRAMES)
        LD      DE,(RXFRM)
        OR      A
        SBC     HL,DE
        LD      DE,(RXPE)
        ADD     HL,DE
        RET

; SINCERX: HL = frames since the last state.
SINCERX: LD     HL,(FRAMES)
        LD      DE,(RXFRM)
        OR      A
        SBC     HL,DE
        RET

; PLCOLOR: A = seat -> A = its colour code (pl_col[color & 3]), from HOT.
PLCOLOR: PUSH   HL
        PUSH    DE
        CALL    HPLAYER
        INC     HL              ; HP_COL
        LD      A,(HL)
        AND     3
        LD      E,A
        LD      D,0
        LD      HL,PLCOLT
        ADD     HL,DE
        LD      A,(HL)
        POP     DE
        POP     HL
        RET
PLCOLT: DB      CB,CO,CW,CS

; HPLAYER: HL = HOTP + seat A * 12. Preserves DE, BC.
HPLAYER: PUSH   DE
        AND     3
        LD      L,A
        ADD     A,A             ; *2
        ADD     A,L             ; *3
        ADD     A,A             ; *6
        ADD     A,A             ; *12
        LD      E,A
        LD      D,0
        LD      HL,HOTP
        ADD     HL,DE
        POP     DE
        RET

; WPLAYER: HL = the window offset of seat A's player + E.
WPLAYER: PUSH   DE
        AND     3
        LD      L,A
        LD      H,0
        LD      D,H
        LD      E,L
        ADD     HL,HL           ; *2
        ADD     HL,HL           ; *4
        ADD     HL,HL           ; *8
        ADD     HL,HL           ; *16
        ADD     HL,HL           ; *32
        OR      A
        SBC     HL,DE           ; *31
        SBC     HL,DE           ; *30
        SBC     HL,DE           ; *29
        LD      DE,W_PL
        ADD     HL,DE
        POP     DE
        LD      A,E
        ADD     A,L
        LD      L,A
        RET     NC
        INC     H
        RET
