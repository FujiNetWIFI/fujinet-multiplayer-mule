; dev2.inc -- more of the development phase (develop.c): what the server
; said about a map action, the button on the map, the wampus, the player
; event, whose turn it is, and the computers' walks. Page 6; the variables
; are dev1.inc's.

; the amount multiplier of each player event (text.c pevent_x): the amount
; is x * 25 * (round / 4 + 1); 100 per food M.U.L.E. (x2), 101 per mining
; M.U.L.E. (x2), 102 per energy M.U.L.E. (x1)
PEVX:   DB      0,0,2,4,100,4,8,2,3,6,4,2,0,0,3,101,102,6,4,4,4,0
SSTARS: DB      "*** ",0

; DVMAPRES: what the server said about a map action we sent.
DVMAPRES: LD    A,(ACTSNT)
        CP      A_INSTAL
        RET     C
        CP      A_WAMPUS+1
        RET     NC
        LD      A,CW
        LD      (MSGCOL),A
        LD      HL,W_ERR
        CALL    RXGETB
        OR      A
        JR      Z,DMR1
        LD      A,SNERR
        CALL    SNDPLAY
        XOR     A
        LD      (ACTSNT),A
        RET
DMR1:   LD      HL,W_RESLT
        CALL    RXGETW
        LD      (DMRV),DE
        LD      A,(ACTSNT)
        LD      B,0
        CP      A_INSTAL
        JR      NZ,DMR3
        LD      A,E
        DEC     A
        OR      D
        JR      NZ,DMR2
        LD      B,M_RANAWAY
        LD      A,SNERR
        CALL    SNDPLAY
        JR      DMR9
DMR2:   LD      A,SNBUY
        CALL    SNDPLAY
        JR      DMR9
DMR3:   CP      A_WAMPUS
        JR      NZ,DMR4
        LD      B,M_CAUGHT
        LD      A,SNFOUND
        CALL    SNDPLAY
        JR      DMR9
DMR4:   CP      A_SELL
        JR      NZ,DMR5
        LD      B,M_FORSALE
        JR      DMR9
DMR5:   CP      A_UNSELL
        JR      NZ,DMR9
        LD      B,M_NOTFORSALE
DMR9:   LD      A,B
        OR      A
        JR      Z,DMR10
        LD      HL,(DMRV)
        LD      DE,0
        CALL    MSGSHOW
DMR10:  XOR     A
        LD      (ACTSNT),A
        RET

; DVBUTTON: the button on the map: catch the wampus, install the M.U.L.E.
; we lead, list land after the land office, or take an assay sample.
DVBUTTON: LD    C,255           ; the plot under the figure's middle
        LD      A,(WX)
        CP      12
        JR      C,DBU2
        LD      A,(WY)
        CP      14
        JR      C,DBU2
        LD      A,(WX)          ; (x - 12) / 16
        SUB     12
        RRCA
        RRCA
        RRCA
        RRCA
        AND     0FH
        CP      9
        JR      NC,DBU2
        LD      B,A
        LD      A,(WY)          ; (y + 2 - 16) / 16
        SUB     14
        RRCA
        RRCA
        RRCA
        RRCA
        AND     0FH
        CP      5
        JR      NC,DBU2
        LD      E,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,E
        ADD     A,B
        LD      C,A
DBU2:   LD      A,C
        LD      (DBUMP),A
        LD      A,(DVWP)        ; within 8 of a showing wampus catches it
        CP      12
        JR      NC,DBU3
        LD      A,(WMULE)
        OR      A
        JR      NZ,DBU3
        LD      A,(WPX)
        LD      B,A
        LD      A,(WX)
        ADD     A,8
        CP      B
        JR      C,DBU3
        LD      A,B
        ADD     A,8
        LD      B,A
        LD      A,(WX)
        CP      B
        JR      Z,DBU2A
        JR      NC,DBU3
DBU2A:  LD      A,(WPY)
        LD      B,A
        LD      A,(WY)
        ADD     A,4
        CP      B
        JR      C,DBU3
        LD      A,B
        ADD     A,8
        LD      B,A
        LD      A,(WY)
        CP      B
        JR      Z,DBU2B
        JR      NC,DBU3
DBU2B:  LD      A,A_WAMPUS
        LD      HL,0
        JP      ACTSEND
DBU3:   LD      A,(DBUMP)
        CP      NPLOTS
        RET     NC
        LD      L,A
        LD      H,0
        LD      A,(WMULE)
        OR      A
        JR      Z,DBU4
        LD      A,A_INSTAL
        CALL    ACTSEND
        LD      A,SNSEL
        JP      SNDPLAY
DBU4:   LD      A,(DVOFFICE)
        OR      A
        JR      Z,DBU6
        LD      DE,HOWNER       ; ours?
        ADD     HL,DE
        LD      A,(MYSEAT)
        CP      (HL)
        RET     NZ
        LD      A,(STVALID)     ; listed already? (the window says)
        OR      A
        RET     Z
        LD      A,(DBUMP)
        LD      E,A
        LD      D,0
        LD      HL,W_PROD
        ADD     HL,DE
        CALL    RXGETB
        LD      B,A_SELL
        BIT     6,A
        JR      Z,DBU5
        LD      B,A_UNSELL
DBU5:   XOR     A
        LD      (DVOFFICE),A
        LD      A,(DBUMP)
        LD      L,A
        LD      H,0
        LD      A,B
        JP      ACTSEND
DBU6:   LD      A,(DBUMP)
        LD      (DVSAMP),A
        LD      A,SNSAMP
        JP      SNDPLAY

; DVWAMPUS: turn.wampus lists when (turn jiffies) and where it shows. Read
; from the window; between states (STVALID 0) the last answer stands.
DVWAMPUS: LD    A,(STVALID)
        OR      A
        JP      Z,DW5
        LD      A,255
        LD      (DVWP),A
        LD      HL,W_TURN+TO_NWAMP
        CALL    RXGETB
        CP      13
        JR      C,DW1
        LD      A,12
DW1:    OR      A
        JR      Z,DW4
        LD      B,A
        CALL    SINCERX         ; w = elapsed + since
        LD      DE,(DVEL)
        ADD     HL,DE
        LD      (DBUW),HL
        LD      C,0
        LD      HL,W_TURN+TO_WAMP+2
DW2:    PUSH    HL
        CALL    RXGETW          ; from
        EX      DE,HL
        LD      DE,(DBUW)
        EX      DE,HL           ; HL = w, DE = from
        OR      A
        SBC     HL,DE
        POP     HL
        JR      C,DW3
        PUSH    HL
        INC     HL
        INC     HL
        CALL    RXGETW          ; to
        LD      HL,(DBUW)
        OR      A
        SBC     HL,DE
        POP     HL
        JR      NC,DW3
        LD      A,C
        LD      (DVWP),A
DW3:    LD      DE,6
        ADD     HL,DE
        INC     C
        DJNZ    DW2
DW4:    LD      A,(DVWP)
        CP      12
        JR      NC,DWHIDE
        LD      E,A             ; on the plot's mountains
        ADD     A,A
        ADD     A,E
        ADD     A,A
        LD      E,A
        LD      D,0
        LD      HL,W_TURN+TO_WAMP
        ADD     HL,DE
        CALL    RXGETB
        LD      B,A             ; plot
        INC     HL
        CALL    RXGETB
        ADD     A,A
        LD      C,A             ; mount * 2
        LD      A,B
        LD      E,0
DW4A:   CP      9
        JR      C,DW4B
        SUB     9
        INC     E
        JR      DW4A
DW4B:   ADD     A,A             ; (mp % 9) * 16 + 16 + 1 + c * 2
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,17
        ADD     A,C
        LD      (WPX),A
        LD      A,E             ; (mp / 9) * 16 + ...
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,17
        ADD     A,C
        LD      (WPY),A
DW5:    LD      A,(DVWP)
        CP      12
        JR      NC,DWHIDE
        ; it flashes: 8 frames on, 8 off
        LD      A,(FRAMES)
        AND     8
        JR      Z,DWHIDE
        LD      A,(WPX)
        SUB     8
        LD      L,A
        LD      H,0
        LD      (SPA_X),HL
        LD      A,(WPY)
        SUB     8
        LD      L,A
        LD      H,0
        ADD     HL,HL
        LD      (SPA_Y),HL
        LD      A,SH_WAMP
        LD      (SPA_SH),A
        XOR     A
        LD      (SPA_FR),A
        LD      (SPA_FL),A
        LD      A,CW
        LD      (SPA_COL),A
        LD      A,SLOTWAMP
        JP      SPRSET
DWHIDE: LD      A,SLOTWAMP
        JP      SPRHIDE

; DVEVMSG: the player event, shown at the start of the turn as on the Atari.
DVEVMSG: LD     A,(DVEVSH)
        OR      A
        RET     NZ
        LD      A,(STVALID)
        OR      A
        RET     Z
        INC     A
        LD      (DVEVSH),A
        LD      HL,W_TURN+TO_EVENT
        CALL    RXGETB
        OR      A
        RET     Z
        CP      23
        RET     NC
        LD      (DEVK),A
        ; v = 25 * (round / 4 + 1)
        LD      A,(RND)
        RRCA
        RRCA
        AND     3FH
        INC     A
        LD      B,A
        LD      HL,0
        LD      DE,25
DE1:    ADD     HL,DE
        DJNZ    DE1
        LD      (DEVV),HL
        LD      HL,0
        LD      (DEVV2),HL
        LD      A,(DEVK)
        DEC     A
        LD      E,A
        LD      D,0
        LD      HL,PEVX
        ADD     HL,DE
        LD      A,(HL)
        CP      100
        JR      C,DE6
        ; per M.U.L.E. of ours of a kind
        LD      (DEVJ),A
        LD      C,0
        LD      HL,HOWNER
        LD      DE,HMULE
DE2:    LD      A,(MYSEAT)
        CP      (HL)
        JR      NZ,DE4
        LD      A,(DE)
        LD      B,A
        LD      A,(DEVJ)
        CP      100
        JR      NZ,DE3
        LD      A,B
        OR      A               ; food
        JR      NZ,DE4
        INC     C
        JR      DE4
DE3:    CP      101
        JR      NZ,DE3A
        LD      A,B
        CP      2               ; smithore, crystite
        JR      C,DE4
        CP      4
        JR      NC,DE4
        INC     C
        JR      DE4
DE3A:   LD      A,B
        CP      1               ; energy
        JR      NZ,DE4
        INC     C
DE4:    INC     HL
        INC     DE
        LD      A,L
        CP      (HOWNER+NPLOTS) AND 0FFH
        JR      NZ,DE2
        LD      A,(DEVJ)
        CP      102
        JR      NZ,DE5
        LD      A,C             ; v *= n
        CALL    DEVMUL
        JR      DE7
DE5:    LD      HL,(DEVV)       ; v2 = v * 2 * n; v *= 2
        ADD     HL,HL
        LD      (DEVV),HL
        LD      A,C
        CALL    DEVMUL
        LD      HL,(DEVV)
        LD      (DEVV2),HL
        LD      HL,(DEVV)       ; (DEVMUL left v*2*n in DEVV: undo)
        LD      A,(DEVJ)
        CP      101
        JR      Z,DE7           ; 101: v = v2
        LD      A,(RND)         ; 100: v = 2 * 25 * (round / 4 + 1)
        RRCA
        RRCA
        AND     3FH
        INC     A
        LD      B,A
        LD      HL,0
        LD      DE,50
DE5A:   ADD     HL,DE
        DJNZ    DE5A
        LD      (DEVV),HL
        JR      DE7
DE6:    CALL    DEVMUL          ; v *= x
DE7:    LD      HL,SSTARS
        LD      DE,DVNAME
        LD      BC,5
        LDIR
        LD      HL,DVNAME
        LD      (MSGPFX),HL
        LD      HL,(DEVV)
        LD      (MSGV1),HL
        LD      HL,(DEVV2)
        LD      (MSGV2),HL
        LD      A,(DEVK)
        DEC     A
        LD      C,A
        LD      A,K_PEV
        CALL    STRLOC
        LD      A,B
        LD      (MSGPG),A
        LD      (MSGADR),HL
        LD      A,CW
        LD      (MSGCOL),A
        JP      MSGSTART

; DEVMUL: DEVV *= A.
DEVMUL: LD      HL,0
        OR      A
        JR      Z,DM2
        LD      B,A
        LD      DE,(DEVV)
DM1:    ADD     HL,DE
        DJNZ    DM1
DM2:    LD      (DEVV),HL
        RET

; DVSAYTURN: someone's turn (DVMSGK: 0 playing, 1 a computer, 2 over).
DVSAYTURN: LD   A,(DVMSGK)
        CP      1
        JR      NZ,DST2
        LD      A,DV_WAIT
        LD      (DVSTATE),A
        LD      B,M_COMP
        CALL    D2MSG0
        JP      AIWALK
DST2:   CP      2
        JR      NZ,DST5
        LD      A,DV_OVER
        LD      (DVSTATE),A
        LD      A,(ACTSNT)
        CP      A_PUB
        JR      NZ,DST3
        LD      A,(DVMINE)
        OR      A
        JR      Z,DST3
        LD      HL,W_RESLT
        CALL    RXGETW
        EX      DE,HL
        LD      B,M_GAMBLE
        LD      DE,0
        JP      MSGSHOW
DST3:   LD      HL,(DVLEFT)
        LD      A,H
        OR      L
        JP      NZ,MSGCLR
        LD      B,M_TIMEOUT
        JP      D2MSG0
DST5:   LD      A,DV_WAIT       ; "<name>'S TURN"
        LD      (DVSTATE),A
        LD      A,(DVSEAT)
        LD      E,0
        CALL    WPLAYER
        LD      DE,DVNAME
        LD      BC,8
        CALL    RXCOPY
        XOR     A
        LD      (DE),A
        LD      HL,DVNAME
        LD      (MSGPFX),HL
        LD      C,M_TURN
        LD      A,K_MSG
        CALL    STRLOC
        LD      A,B
        LD      (MSGPG),A
        LD      (MSGADR),HL
        JP      MSGSTART

; ---- a computer's turn: the server plays it at once, so the map already
; shows what it did. Here its figure walks out of the store to the plot that
; gained a M.U.L.E. (or one of its plots) with the M.U.L.E. behind, which
; stays there, then walks back into the store and waits (dv_ai_walk).
; Positions are the Atari's (x colour clocks, y lines): the store 75, 88.
AICX    EQU     75
AICY    EQU     88

AIWALK: LD      C,255           ; j: the plot (a state is arriving: MAPSRC)
        XOR     A
        LD      (AIMULE),A
        LD      B,0
AW1:    LD      A,B
        CP      STOREP
        JR      Z,AW3
        LD      E,B
        LD      D,0
        LD      HL,W_OWNER
        ADD     HL,DE
        CALL    RXGETB
        LD      HL,DVSEAT
        CP      (HL)
        JR      NZ,AW3
        LD      A,C
        CP      255
        JR      NZ,AW2
        LD      C,B
AW2:    LD      HL,W_MULES      ; a M.U.L.E. new there: this plot
        ADD     HL,DE
        CALL    RXGETB
        CP      4
        JR      NC,AW3
        LD      HL,HMULE
        ADD     HL,DE
        CP      (HL)
        JR      Z,AW3
        LD      C,B
        LD      A,1
        LD      (AIMULE),A
AW3:    INC     B
        LD      A,B
        CP      NPLOTS
        JR      C,AW1
        LD      A,C
        CP      NPLOTS
        JR      C,AW4
        LD      A,13            ; no land: out and back
AW4:    LD      (DVAIP),A
        CALL    PLOTXYM         ; the plot's middle (Atari)
        LD      (AITX),A        ; (x)
        LD      A,E
        LD      (AITY),A        ; (y)
        LD      A,(DVSEAT)
        LD      E,HP_SPEC
        CALL    D2PLB
        AND     7
        LD      (AISP),A
        LD      A,(DVSEAT)
        CALL    D2PLC
        LD      (AICOL),A
        LD      A,(DVAI)
        CP      2
        CALL    Z,MAPALL        ; the last one's land
        LD      A,1
        LD      (DVAI),A
        ; steps: across 2 every 6 frames, then 2 lines every 6
        LD      A,(AITX)
        SUB     AICX
        JR      NC,AW5
        NEG
AW5:    SRL     A
        INC     A
        LD      (AIHC),A
        LD      A,(AITY)
        SUB     AICY
        JR      NC,AW6
        NEG
AW6:    SRL     A
        INC     A
        LD      (AIHI),A
        LD      A,1             ; the figure: out along x
        LD      (AIST),A
        LD      A,7             ; its M.U.L.E.: a wait first
        LD      (AIST+1),A
        LD      A,(AIMULE)
        OR      A
        JR      NZ,AW7
        LD      (AIST+1),A
AW7:    LD      B,2
        LD      HL,AIX
AW8:    LD      (HL),AICX
        INC     HL
        DJNZ    AW8
        LD      B,2
        LD      HL,AIY
AW9:    LD      (HL),AICY
        INC     HL
        DJNZ    AW9
        LD      A,(AIHC)
        LD      (AICNT),A
        LD      A,6
        LD      (AIT),A
        LD      A,24
        LD      (AIT+1),A
        XOR     A
        LD      (AIALT),A
        LD      (AIALT+1),A
        LD      (FXFLAG),A
        CALL    AIDRAW0
        LD      A,(AIMULE)
        OR      A
        RET     Z
        JP      AIDRAW1

; PLOTXYM: plot A -> A = x + 4, E = y + 12 (Atari: its middle-ish).
PLOTXYM: LD     E,16+12
PXM1:   CP      9
        JR      C,PXM2
        SUB     9
        PUSH    AF
        LD      A,E
        ADD     A,32
        LD      E,A
        POP     AF
        JR      PXM1
PXM2:   ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,8+4
        RET

; AISTEP: every frame of a computer's turn: the walks.
AISTEP: LD      A,(AIST)
        OR      A
        CALL    NZ,AIST0
        LD      A,(AIST+1)
        OR      A
        RET     Z
        JP      AIST1

; the figure: 1 out x, 2 out y, 3 on the plot, 4 back y, 5 back x, 6 home
AIST0:  LD      HL,AIT
        DEC     (HL)
        RET     NZ
        LD      (HL),6
        LD      A,(AIST)
        CP      3
        JR      NZ,AS01
        XOR     A               ; off the plot: back the way it came
        LD      (FXFLAG),A
        LD      A,4
        LD      (FXFLAG),A
        LD      (AIST),A
        LD      A,(AIHI)
        LD      (AICNT),A
        RET
AS01:   CP      6
        RET     Z
        LD      C,0             ; C = 0 figure
        CALL    AIMOVE
        LD      HL,AICNT
        DEC     (HL)
        RET     NZ
        LD      A,(AIST)        ; the move is done: the next stage
        INC     A
        LD      (AIST),A
        CP      3
        JR      NZ,AS02
        LD      A,3             ; on the plot: outlined, 90 frames
        LD      (FXFLAG),A
        LD      A,90
        LD      (AIT),A
        RET
AS02:   LD      B,A
        LD      A,(AIHC)
        CP      B               ; (just a count: x stages 1, 5; y 2, 4)
        LD      A,B
        CP      5
        LD      A,(AIHC)
        JR      Z,AS03
        LD      A,(AIHI)
AS03:   LD      (AICNT),A
        RET

; the M.U.L.E.: 7 waiting, 8 out x, 9 out y, 10 installed (20 frames)
AIST1:  LD      HL,AIT+1
        DEC     (HL)
        RET     NZ
        LD      (HL),6
        LD      A,(AIST+1)
        CP      7
        JR      NZ,AQ11
        LD      A,8
        LD      (AIST+1),A
        LD      A,(AIHC)
        LD      (AICNT+1),A
        RET
AQ11:   CP      10
        JR      NZ,AQ12
        XOR     A               ; it goes no further
        LD      (AIST+1),A
        LD      A,SLOTMULE
        JP      SPRHIDE
AQ12:   LD      C,1
        CALL    AIMOVE
        LD      HL,AICNT+1
        DEC     (HL)
        RET     NZ
        LD      A,(AIST+1)
        INC     A
        LD      (AIST+1),A
        CP      10
        JR      NZ,AQ13
        LD      A,20
        LD      (AIT+1),A
        RET
AQ13:   LD      A,(AIHI)
        LD      (AICNT+1),A
        RET

; AIMOVE: actor C's step for its stage (x or y, out or back), and redraw.
AIMOVE: LD      B,0
        LD      HL,AIST
        ADD     HL,BC
        LD      A,(HL)          ; the stage
        LD      HL,AIX
        ADD     HL,BC
        LD      E,2             ; out along x: towards the plot
        CP      1
        JR      Z,AIM1
        CP      8
        JR      Z,AIM1
        CP      5
        JR      Z,AIM2
        LD      HL,AIY
        ADD     HL,BC
        CP      4
        JR      Z,AIM4
        LD      A,(AITY)        ; out along y
        CP      AICY
        JR      AIM3
AIM1:   LD      A,(AITX)
        CP      AICX
        JR      AIM3
AIM2:   LD      A,(AITX)        ; back along x
        CP      AICX
        CCF
AIM3:   LD      A,(HL)          ; C: towards smaller
        JR      NC,AIM3A
        SUB     E
        JR      AIM3B
AIM3A:  ADD     A,E
AIM3B:  LD      (HL),A
        LD      HL,AIALT
        ADD     HL,BC
        LD      A,(HL)
        XOR     1
        LD      (HL),A
        LD      A,C
        OR      A
        JR      Z,AIDRAW0
        JR      AIDRAW1
AIM4:   LD      A,(AITY)        ; back along y
        CP      AICY
        CCF
        JR      AIM3

; AIDRAW0 / AIDRAW1: the figure (facing its way) / its M.U.L.E.
AIDRAW0: LD     A,(AISP)
        LD      B,A
        ADD     A,A
        ADD     A,A
        ADD     A,B
        ADD     A,A             ; *10
        LD      B,A
        LD      A,(AIALT)
        ADD     A,B
        LD      (SPA_FR),A
        LD      A,SH_FIG
        LD      (SPA_SH),A
        LD      A,(AICOL)
        LD      (SPA_COL),A
        LD      A,(AIST)        ; facing: out, towards the plot; back, home
        CP      4
        LD      A,(AITX)
        JR      NC,AID1
        CP      AICX            ; out: flipped if the plot is left
        LD      A,0
        JR      NC,AID2
        INC     A
        JR      AID2
AID1:   CP      AICX            ; back: flipped if home is left
        LD      A,1
        JR      NC,AID2
        XOR     A
AID2:   LD      (SPA_FL),A
        LD      A,(AIX)
        LD      L,A
        LD      H,0
        LD      (SPA_X),HL
        LD      A,(AIY)
        LD      L,A
        LD      (SPA_Y),HL
        LD      A,SLOTPL
        JP      SPRSET
AIDRAW1: LD     A,2
        LD      B,A
        LD      A,(AIALT+1)
        ADD     A,B
        LD      (SPA_FR),A
        LD      A,SH_MULE
        LD      (SPA_SH),A
        LD      A,CW
        LD      (SPA_COL),A
        LD      A,(AITX)
        CP      AICX
        LD      A,0
        JR      NC,AID3
        INC     A
AID3:   LD      (SPA_FL),A
        LD      A,(AIX+1)
        LD      L,A
        LD      H,0
        LD      (SPA_X),HL
        LD      A,(AIY+1)
        LD      L,A
        LD      (SPA_Y),HL
        LD      A,SLOTMULE
        JP      SPRSET

; DVAIMARKS: per frame in a computer's turn: its land outlined in white for
; 40 frames as the turn starts, and the plot it works while it is there.
DVAIMARKS: LD   A,(DVAI)
        CP      1
        JR      NZ,DAM1
        LD      A,1
        CALL    AILAND
        LD      A,2
        LD      (DVAI),A
        LD      A,(FRAMES)
        LD      (DVAIT),A
        JR      DAM2
DAM1:   CP      2
        JR      NZ,DAM2
        LD      A,(DVAIT)
        LD      B,A
        LD      A,(FRAMES)
        SUB     B
        CP      40
        JR      C,DAM2
        XOR     A
        CALL    AILAND
        XOR     A
        LD      (DVAI),A
DAM2:   LD      A,(FXFLAG)
        CP      3
        JR      Z,DAM3
        CP      4
        RET     NZ
DAM3:   CP      3
        JR      NZ,DAM4
        LD      A,(DVAIP)
        LD      (MAPBLK),A
        LD      A,1
        LD      (MAPBLON),A
DAM4:   LD      A,(DVAIP)
        CP      NPLOTS
        CALL    C,MAPPLOT
        LD      A,NOBODY
        LD      (MAPBLK),A
        XOR     A
        LD      (FXFLAG),A
        RET

; AILAND: the computer's plots outlined (A 1) or as they are (A 0).
AILAND: LD      (DBUMP),A
        LD      B,0
AL1:    LD      E,B
        LD      D,0
        LD      HL,HOWNER
        ADD     HL,DE
        LD      A,(DVSEAT)
        CP      (HL)
        JR      NZ,AL3
        LD      A,(DBUMP)
        OR      A
        JR      Z,AL2
        LD      A,B
        LD      (MAPBLK),A
        LD      A,1
        LD      (MAPBLON),A
AL2:    PUSH    BC
        LD      A,B
        CALL    MAPPLOT
        POP     BC
        LD      A,NOBODY
        LD      (MAPBLK),A
AL3:    INC     B
        LD      A,B
        CP      NPLOTS
        JR      C,AL1
        RET
