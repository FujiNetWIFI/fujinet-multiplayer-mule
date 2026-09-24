; test.inc -- the offline test mode (test.c, teststart.c) and autoplay
; (autoplay.c). Page 9.
;
; When the harness pokes 5AH into TESTHK during the title, the game skips
; FujiNet and fakes a state: round 1 on a generated map. TESTHK+1 is the
; phase + 1 (0: develop), +2 the colony event / auction stage / the seat
; whose turn it is, +3 the player event. Actions are answered here, so
; walking, the town, events and animations can be checked in seconds.
;
; There is no room for a 550-byte state in RAM: in test mode RXGETB asks
; TSTGETB, which makes each byte up from the offset (teststart.c's state),
; with the few things actions change kept in RAM (TPH .. TPAT).

TF_STARTED EQU  1
TF_ENDED EQU    2
TF_HASMULE EQU  4

; ---- autoplay: plays by itself, for soak tests against a real server
; (make AUTOPLAY=1). It only presses what a player would: ready, claims,
; turns, continue.
AUTOPL: LD      HL,(FRAMES)
        LD      DE,(APLAST)
        OR      A
        SBC     HL,DE
        LD      DE,45
        OR      A
        SBC     HL,DE
        RET     C
        LD      A,(STOK)
        OR      A
        RET     Z
        LD      A,(MYSEAT)
        CP      4
        RET     NC
        LD      HL,(FRAMES)
        LD      (APLAST),HL
        LD      A,(PHS)
        CP      PH_LOBBY
        JR      NZ,AP1
        LD      A,(MYSEAT)
        CALL    HPLAYER
        INC     HL
        INC     HL
        LD      A,(HL)
        AND     PF_READY
        RET     NZ
APBTN:  LD      A,1
        LD      (EVBTN),A
        RET
AP1:    CP      PH_SUMM
        JR      Z,APBTN
        CP      PH_GRANT
        JR      Z,APBTN
        CP      PH_DEVEL
        JR      NZ,AP2
        LD      A,(DVSTATE)
        CP      DV_PROMPT
        JR      Z,APBTN
        CP      DV_WALK
        RET     NZ
        LD      A,KEYEND
        LD      (EVKEY),A
        RET
AP2:    CP      PH_DECL
        RET     NZ
        LD      A,'0'
        LD      (EVKEY),A
        RET

; ---- starting ------------------------------------------------------------------
TESTSTART: LD   A,1
        LD      (TESTMD),A
        LD      (STVALID),A
        LD      A,(TESTHK+1)
        DEC     A
        CP      PH_OVER+1
        JR      C,TQS1
        LD      A,PH_DEVEL
TQS1:   LD      (TPH),A
        LD      A,(TESTHK+2)
        LD      (TARG),A
        LD      A,(TESTHK+3)
        LD      (TPEV),A
        LD      HL,0
        LD      (TELAP),HL
        LD      (TRES),HL
        LD      (TTEL),HL
        XOR     A
        LD      (TERR),A
        LD      (TCUR),A
        LD      (TNP),A
        LD      HL,12
        LD      (TNEXT),HL
        LD      HL,2828
        LD      (TLEFT),HL
        LD      A,255
        LD      (TCLM),A
        LD      (TCARRY),A
        LD      A,TF_STARTED
        LD      (TFL),A
        LD      HL,TSNAME
        LD      DE,SCNAME
        LD      BC,4
        LDIR
        LD      HL,TSTABLE
        LD      DE,SCTABL
        LD      BC,5
        LDIR
        RET
TSNAME: DB      "PL0",0
TSTABLE: DB     "test",0

; ---- answering an action, as the server would (roughly); the clocks age -----------
TESTPOLL: CALL  SINCERX
        LD      (TSW),HL
        LD      A,1
        LD      (NETHAVE),A
        LD      A,(ACT)
        LD      (ACTSNT),A
        XOR     A
        LD      (TERR),A
        LD      A,(ACT)
        LD      HL,(ACTARG)
        CP      A_BUYM
        JR      NZ,TP1
        LD      A,TF_STARTED+TF_HASMULE
        LD      (TFL),A
        JR      TP9
TP1:    CP      A_RETURN
        JR      NZ,TP2
        LD      A,TF_STARTED
        LD      (TFL),A
        JR      TP9
TP2:    CP      A_OUTFIT
        JR      NZ,TP3
        LD      A,L
        LD      (TCARRY),A
        JR      TP9
TP3:    CP      A_INSTAL
        JR      NZ,TP4
        LD      A,TF_STARTED
        LD      (TFL),A
        LD      A,(TCARRY)
        LD      B,A
        CALL    TPATCH
        JR      TP9
TP4:    CP      A_ASSAY
        JR      NZ,TP5
        LD      HL,3
        LD      (TRES),HL
        JR      TP9
TP5:    CP      A_PUB
        JR      NZ,TP6
        LD      A,TF_STARTED+TF_ENDED
        LD      (TFL),A
        LD      HL,75
        LD      (TRES),HL
        JR      TP9
TP6:    CP      A_CLAIM
        JR      NZ,TP9
        LD      A,L
        LD      (TCLM),A
        LD      B,255
        CALL    TPATCH
TP9:    ; the turn's time
        LD      HL,(TLEFT)
        LD      DE,(TSW)
        LD      A,(TPH)
        CP      PH_DEVEL
        JR      NZ,TP10
        OR      A
        SBC     HL,DE
        JR      Z,TP10
        JR      NC,TP11
TP10:   LD      HL,0
TP11:   LD      (TLEFT),HL
        LD      A,(TPH)         ; the phase's clock stops with the turn's
        CP      PH_DEVEL
        JR      NZ,TP12
        LD      A,H
        OR      L
        JR      Z,TP13
TP12:   LD      HL,(TELAP)
        ADD     HL,DE
        LD      (TELAP),HL
TP13:   LD      HL,(TTEL)
        ADD     HL,DE
        LD      (TTEL),HL
        LD      A,(TPH)
        CP      PH_GRANT
        JR      NZ,TP15
        ; the cursor steps every 12: v = next + cursor * 12 + w
        LD      A,(TCUR)
        LD      L,A
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL
        LD      B,H
        LD      C,L
        ADD     HL,HL
        ADD     HL,BC           ; *12
        LD      BC,(TNEXT)
        ADD     HL,BC
        ADD     HL,DE
        LD      C,0             ; /12 (at most 45)
TP14:   LD      DE,12
        OR      A
        SBC     HL,DE
        JR      C,TP14A
        INC     C
        LD      A,C
        CP      45
        JR      C,TP14
        LD      HL,-12+12       ; (done: the cursor at the end)
TP14A:  ADD     HL,DE
        EX      DE,HL           ; DE = v % 12
        LD      HL,12
        OR      A
        SBC     HL,DE
        LD      (TNEXT),HL
        LD      A,C
        CP      45
        JR      C,TP14B
        LD      A,45
TP14B:  LD      (TCUR),A
TP15:   XOR     A
        LD      (ACT),A
        RET

; TPATCH: plot L now ours (seat 0), M.U.L.E. B.
TPATCH: LD      A,(TNP)
        CP      4
        RET     NC
        LD      E,A
        ADD     A,A
        ADD     A,E
        LD      E,A
        LD      D,0
        PUSH    HL
        LD      HL,TPAT
        ADD     HL,DE
        POP     DE
        LD      (HL),E          ; the plot
        INC     HL
        LD      (HL),0          ; the owner
        INC     HL
        LD      (HL),B          ; the M.U.L.E.
        LD      HL,TNP
        INC     (HL)
        RET

; ---- the made-up state: TSTGETB, A = byte HL ---------------------------------------
TSTGETB: PUSH   BC
        PUSH    DE
        PUSH    HL
        CALL    TSG
        POP     HL
        POP     DE
        POP     BC
        RET

TSG:    LD      DE,16
        OR      A
        SBC     HL,DE
        JP      C,TSGH          ; the header
        LD      DE,132-16
        SBC     HL,DE
        JR      C,TSGP          ; the players
        LD      DE,357-132
        OR      A
        SBC     HL,DE
        JP      C,TSGM          ; the map
        LD      DE,388-357
        OR      A
        SBC     HL,DE
        JP      C,TSGS          ; the store
        LD      DE,396-388
        OR      A
        SBC     HL,DE
        JP      C,TSGG          ; the grant
        LD      DE,454-396
        OR      A
        SBC     HL,DE
        JP      C,TSGA          ; the auction
        LD      DE,540-454
        OR      A
        SBC     HL,DE
        JP      C,TSGT          ; the turn
        LD      A,L             ; the colony: 540..549
        LD      E,A
        LD      D,0
        LD      HL,TCOLONY
        ADD     HL,DE
        LD      A,E
        OR      A
        LD      A,(TARG)        ; the event
        RET     Z
        LD      A,(HL)
        RET
TCOLONY: DB     0,10,4,0,0,0,40H,0E2H,01H,00H   ; plot 10, rating 4, 123456

; the players: 29 bytes each ("PL0".., species (1 + 2k) & 7, colour k,
; the last a computer, $1000, food 1 + 2k, score 5000 - 700k, rank k + 1,
; food produced 3 + k, spoiled k)
TSGP:   ADD     HL,DE           ; (back to 0..115)
        LD      C,0             ; C = seat, L = the byte
TSP1:   LD      A,L
        SUB     29
        JR      C,TSP2
        LD      L,A
        INC     C
        JR      TSP1
TSP2:   LD      A,L
        OR      A
        LD      A,'P'
        RET     Z
        LD      A,L
        DEC     A
        LD      A,'L'
        RET     Z
        LD      A,L
        CP      2
        JR      NZ,TSP3
        LD      A,'0'
        ADD     A,C
        RET
TSP3:   CP      9
        JR      NZ,TSP4
        LD      A,C
        ADD     A,A
        INC     A
        AND     7
        RET
TSP4:   CP      10
        LD      A,C
        RET     Z
        LD      A,L
        CP      11
        JR      NZ,TSP5
        LD      A,C
        CP      3
        LD      A,PF_AI
        RET     Z
        XOR     A
        RET
TSP5:   CP      12
        LD      A,0E8H
        RET     Z
        LD      A,L
        CP      13
        LD      A,03H
        RET     Z
        LD      A,L
        CP      14
        JR      NZ,TSP6
        LD      A,C
        ADD     A,A
        INC     A
        RET
TSP6:   CP      18              ; the score
        JR      C,TSP9
        CP      20
        JR      NC,TSP7
        PUSH    AF
        LD      HL,5000
        LD      DE,-700
        LD      A,C
        OR      A
        JR      Z,TSP6B
TSP6A:  ADD     HL,DE
        DEC     A
        JR      NZ,TSP6A
TSP6B:  POP     AF
        CP      18
        LD      A,L
        RET     Z
        LD      A,H
        RET
TSP7:   JR      NZ,TSP8
        LD      A,C             ; 20: the rank
        INC     A
        RET
TSP8:   CP      21
        JR      NZ,TSP8A
        LD      A,C
        ADD     A,3
        RET
TSP8A:  CP      25
        LD      A,C
        RET     Z
TSP9:   XOR     A
        RET

; the header
TSGH:   ADD     HL,DE           ; (back to 0..15)
        LD      A,L
        LD      E,A
        LD      D,0
        CP      3
        LD      A,(TPH)
        RET     Z
        LD      A,E
        CP      6
        LD      A,(TERR)
        RET     Z
        LD      A,E
        CP      8
        LD      A,(FRAMES)
        RET     Z
        LD      A,E
        CP      9
        LD      A,(FRAMES+1)
        RET     Z
        LD      A,E
        CP      10
        LD      A,(TELAP)
        RET     Z
        LD      A,E
        CP      11
        LD      A,(TELAP+1)
        RET     Z
        LD      A,E
        CP      14
        LD      A,(TRES)
        RET     Z
        LD      A,E
        CP      15
        LD      A,(TRES+1)
        RET     Z
        LD      HL,THEADER
        ADD     HL,DE
        LD      A,(HL)
        RET
THEADER: DB     'M',2,1,0,1,0,0,0,0,0,0,0,0FFH,0FFH,0,0

; the map: terrain, owner, mule, crystite, production (45 each)
TSGM:   ADD     HL,DE           ; (back to 0..224)
        LD      C,0             ; C = which, L = the plot
TQM1:   LD      A,L
        SUB     45
        JR      C,TSM2
        LD      L,A
        INC     C
        JR      TQM1
TSM2:   LD      B,L             ; B = plot
        LD      A,C
        OR      A
        JR      Z,TSMTER
        DEC     A
        JR      Z,TSMOWN
        DEC     A
        JR      Z,TSMMULE
        DEC     A
        LD      A,0FFH          ; crystite: unknown
        RET     Z
        LD      A,B             ; production
        CP      10
        LD      A,5
        RET     Z
        LD      A,B
        CP      12
        LD      A,3
        RET     Z
        LD      A,B
        CP      31
        LD      A,8
        RET     Z
        XOR     A
        RET
TSMOWN: CALL    TSMPAT
        LD      A,(HL)
        RET     Z
        LD      HL,TOWNDEF
        JR      TSMDEF
TSMMULE: CALL   TSMPAT
        INC     HL
        LD      A,(HL)
        RET     Z
        LD      HL,TMULEDEF
TSMDEF: LD      A,(HL)          ; {plot, value} ..., 255
        INC     HL
        CP      255
        RET     Z               ; (255: none)
        CP      B
        LD      A,(HL)
        INC     HL
        RET     Z
        JR      TSMDEF
TOWNDEF: DB     10,0,12,1,30,3,31,0,14,2,255
TMULEDEF: DB    10,0,12,1,30,1,31,2,255

; TSMPAT: Z and HL at the {owner, mule} of plot B if an action changed it.
TSMPAT: LD      A,(TNP)
        OR      A
        JR      Z,TSMP9
        LD      C,A
        LD      HL,TPAT
TSMP1:  LD      A,(HL)
        INC     HL
        CP      B
        RET     Z
        INC     HL
        INC     HL
        DEC     C
        JR      NZ,TSMP1
TSMP9:  OR      1               ; NZ
        RET

; the terrain: the river down column 4, the store in the middle, mountains
; either side
TSMTER: LD      A,B
        LD      C,0             ; C = the row, A = the column
TST1:   CP      9
        JR      C,TST2
        SUB     9
        INC     C
        JR      TST1
TST2:   LD      D,A             ; D = column
        LD      E,0             ; plain
        CP      4
        JR      NZ,TST3
        LD      E,1             ; river
TST3:   LD      A,B
        CP      STOREP
        JR      NZ,TST4
        LD      E,3
TST4:   LD      A,C             ; column == row % 4: mountains (row % 3 + 1)
        AND     3
        CP      D
        JR      NZ,TST5
        LD      A,C
        CALL    MOD3
        INC     A
        ADD     A,A
        ADD     A,A
        ADD     A,2
        LD      E,A
TST5:   LD      A,C             ; column == 5 + row % 4: ((row + 1) % 3 + 1)
        AND     3
        ADD     A,5
        CP      D
        JR      NZ,TST6
        LD      A,C
        INC     A
        CALL    MOD3
        INC     A
        ADD     A,A
        ADD     A,A
        ADD     A,2
        LD      E,A
TST6:   LD      A,E
        RET
MOD3:   SUB     3
        JR      NC,MOD3
        ADD     A,3
        RET

; the store: 8 of each, prices 20/40/60/80, 14 M.U.L.E.s at $100
TSGS:   ADD     HL,DE           ; (back to 0..30)
        LD      E,L
        LD      D,0
        LD      HL,TSTORE
        ADD     HL,DE
        LD      A,(HL)
        RET
TSTORE: DB      8,8,8,8
        DW      20,40,60,80
        DW      0,0,0,0
        DW      0,0,0,0
        DB      14
        DW      100

; the grant: the cursor, our claim, a step of 12
TSGG:   ADD     HL,DE           ; (back to 0..7)
        LD      A,L
        OR      A
        LD      A,(TCUR)
        RET     Z
        LD      A,L
        DEC     A
        LD      A,(TCLM)
        RET     Z
        LD      A,L
        CP      5
        LD      A,12
        RET     Z
        JR      C,TSGG1
        LD      A,L
        CP      6
        LD      A,(TNEXT)
        RET     Z
        LD      A,(TNEXT+1)
        RET
TSGG1:  LD      A,0FFH          ; the others' claims: none
        RET

; the auction (a goods auction; land in the land auction's phase)
TSGA:   ADD     HL,DE           ; (back to 0..57)
        LD      A,L
        LD      E,A
        LD      D,0
        LD      A,(TPH)
        CP      PH_LAUC
        JR      NZ,TSA1
        LD      A,E             ; land: kind 1, step 4, start 240, plot 13
        OR      A
        LD      A,1
        RET     Z
        LD      A,E
        CP      2
        LD      A,13
        RET     Z
        LD      A,E
        CP      8
        LD      A,4
        RET     Z
        LD      A,E
        CP      10
        LD      A,240
        RET     Z
        LD      A,E
        CP      11
        LD      A,0
        RET     Z
TSA1:   LD      A,E             ; the stage: the argument, in declare,
        CP      7               ; trading and the land auction
        JR      NZ,TSA2
        LD      A,(TPH)
        CP      PH_DECL
        JR      Z,TSA1A
        CP      PH_TRADE
        JR      Z,TSA1A
        CP      PH_LAUC
        LD      A,0
        RET     NZ
TSA1A:  LD      A,(TARG)
        RET
TSA2:   LD      HL,TAUC
        ADD     HL,DE
        LD      A,(HL)
        RET
TAUC:   DB      2,0,0,0FFH,0,0FFH,0,0,1,0       ; kind .. traded
        DW      0,0,20,66,30,65,0,0,0           ; start .. stageLeft
        DB      70,0                            ; ticks, tickLeft
        DB      1,2,1,1                         ; roles
        DW      20,25,30,35                     ; pos
        DW      40,45,50,55                     ; target
        DB      0,0,0,0,0,0,0,0                 ; units, push

; the turn: ours (or seat TARG's), 2828 jiffies, a wampus early on
TSGT:   ADD     HL,DE           ; (back to 0..85)
        LD      A,L
        LD      E,A
        OR      A
        JR      NZ,TST_1
        LD      A,(TARG)
        AND     3
        RET
TST_1:  CP      1
        LD      A,(TFL)
        RET     Z
        LD      A,E
        CP      2
        LD      A,(TCARRY)
        RET     Z
        LD      A,E
        CP      3
        LD      A,(TPEV)
        RET     Z
        LD      A,E
        CP      6
        LD      A,(TTEL)
        RET     Z
        LD      A,E
        CP      7
        LD      A,(TTEL+1)
        RET     Z
        LD      A,E
        CP      8
        LD      A,(TLEFT)
        RET     Z
        LD      A,E
        CP      9
        LD      A,(TLEFT+1)
        RET     Z
        LD      A,E
        CP      20
        JR      NC,TST_9
        LD      D,0
        LD      HL,TTURN
        ADD     HL,DE
        LD      A,(HL)
        RET
TST_9:  XOR     A
        RET
TTURN:  DB      0,0,0,0
        DW      2828            ; length
        DW      0,0,0           ; elapsed, left, endIn (above)
        DB      28,1            ; pTU, one wampus:
        DB      0,0             ;   plot 0, mountain 0
        DW      0,2815          ;   from, to
