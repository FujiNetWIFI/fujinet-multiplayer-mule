; grant.inc -- the land grant (grant.c) and production with its colony
; events (events.c). The grant page, with the fx engine (fx.inc).

PVP     DEFL    FXEND
        PV      GCUR,1          ; the cursor's plot (255 before, 45 done)
        PV      GNEXT,1         ; frames to its next step
        PV      GSTEP,1
        PV      GSHOW,1
        PV      GRDEP,1         ; the ship is leaving first
        PV      GRCLM,1         ; we have claimed (or asked to)
        PV      GRSRV,1         ; the cursor in the last state
        PV      GRPRESS,1       ; the cursor's plot when the button went down
        PV      GRWAS,1         ; the plot before the cursor's last step
        PV      GRWANT,1        ; the plot we claimed and wait on
        PV      PREVT,1         ; production: the colony event
        PV      PRPLOT,1
        PV      PRSTART,1
        PV      PRDONE,1
        PV      PRLAST,2        ; the fire / quake's last flicker
        PV      PRK,1
        IF      PVP > RAMEND
RAMEND  DEFL    PVP
        ENDIF

SLOTCUR EQU     2
CE_PEST EQU     1
CE_PIRATES EQU  2
CE_ACID EQU     3
CE_QUAKE EQU    4
CE_SUNSPOT EQU  5
CE_METEOR EQU   6
CE_RADIATION EQU 7
CE_FIRE EQU     8
CE_SHIP EQU     9

; the phase entries
G3ENTER: LD     A,11            ; the message rows under the map
        LD      (MSGROW),A
        LD      A,CW
        LD      (MSGCOL),A
        LD      A,(PHS)
        CP      PH_GRANT
        JP      Z,GRENTER
        JP      PRENTER
G3STATE: LD     A,(PHS)
        CP      PH_GRANT
        JP      Z,GRSTATE
        RET
G3TICK: LD      A,(PHS)
        CP      PH_GRANT
        JP      Z,GRTICK
        JP      PRTICK
G3ANIM: LD      A,(ANIM)
        CP      AN_GRANT
        CALL    Z,GRISR
        JP      FXRUN

; PLOTXY: plot A -> HL = its x (8 + 16 * (p % 9)), DE = its Atari y
; (16 + 32 * (p / 9)).
PLOTXY: LD      E,16
PXY1:   CP      9
        JR      C,PXY2
        SUB     9
        PUSH    AF
        LD      A,E
        ADD     A,32
        LD      E,A
        POP     AF
        JR      PXY1
PXY2:   ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,8
        LD      L,A
        LD      H,0
        LD      D,H
        RET

; ---- the land grant (grant.c) ----------------------------------------------
; The cursor sweeps the map a plot every 12 jiffies, skipping the store
; and owned plots; the button claims the plot under it. The server judges
; the claim as of the press (it takes a plot the cursor left up to 36
; jiffies ago), so the press is latched with the plot on the screen then.
GRENTER: XOR    A
        LD      (MAPDOTS),A
        CALL    MAPENTER
        LD      A,AN_GRANT
        LD      (ANIM),A
        XOR     A
        LD      (GSHOW),A
        LD      (GNEXT),A
        LD      (GRCLM),A
        LD      (GRDEP),A
        LD      A,255
        LD      (GCUR),A
        LD      (GRPRESS),A
        LD      (GRWANT),A
        LD      (GRWAS),A
        LD      A,(RND)
        CP      1
        JR      NZ,GRE2
        CALL    PENOW
        LD      DE,700
        OR      A
        SBC     HL,DE
        JR      NC,GRE2
        LD      A,1             ; the ship leaves the store first
        LD      (GRDEP),A
        LD      B,T_TRANSPORT
        LD      HL,0
        CALL    SHTITLE
        LD      B,M_DEPART
        CALL    MSG0
        XOR     A
        LD      HL,FXDEPART
        JP      FXSTART
GRE2:   LD      B,T_LANDGRANT
        LD      HL,0
        CALL    SHTITLE
        CALL    SNDALRT
        LD      B,M_SELECT
        JP      MSG0

; MSG0: message B with no numbers.
MSG0:   LD      HL,0
        LD      D,H
        LD      E,L
        JP      MSGSHOW

GRSTATE: CALL   MAPUPD
        ; sync the cursor: -1 (255) before it starts, 45 when it's done
        LD      HL,W_GRANT
        CALL    RXGETB
        LD      (GRSRV),A
        LD      C,A             ; C = k
        LD      HL,W_GRANT+5
        CALL    RXGETB
        OR      A
        JR      NZ,GRS1
        LD      A,12
GRS1:   LD      (GSTEP),A
        LD      HL,W_GRANT+6
        CALL    RXGETW          ; DE = next
        LD      A,D
        OR      A
        LD      A,E
        JR      Z,GRS2
        LD      A,255
GRS2:   LD      B,A             ; B = w
        LD      A,(GCUR)
        CP      C
        JR      NZ,GRS3
        LD      A,(GNEXT)
        CP      B
        JR      C,GRS5
        JR      Z,GRS5
        JR      GRS4
GRS3:   LD      A,255
        LD      (GRWAS),A
GRS4:   LD      A,C
        LD      (GCUR),A
        LD      A,B
        OR      A
        JR      NZ,GRS4A
        INC     A
GRS4A:  LD      (GNEXT),A
GRS5:   LD      A,(GRDEP)
        OR      A
        JR      NZ,GRS6
        INC     A
        LD      (GSHOW),A
GRS6:   LD      A,(MYSEAT)
        CP      4
        RET     NC
        LD      E,A
        LD      D,0
        LD      HL,W_GRANT+1
        ADD     HL,DE
        CALL    RXGETB
        LD      B,A             ; B = mine
        CP      NPLOTS
        JR      NC,GRS7
        LD      A,1
        LD      (GRCLM),A
        LD      A,255
        LD      (GRWANT),A
GRS7:   LD      A,(GRWANT)
        CP      NPLOTS
        RET     NC
        LD      D,A             ; D = j
        LD      A,B
        CP      NPLOTS
        RET     C
        ; every claim is answered by the next state, so a plot a computer
        ; owns is not lost yet: the server may still give it
        LD      HL,W_ERR
        CALL    RXGETB
        LD      E,A             ; E = err
        LD      A,(ACTSNT)
        CP      A_CLAIM
        JR      NZ,GRS8
        LD      A,E
        OR      A
        JR      Z,GRS8
        LD      A,C             ; turned down as the cursor wasn't there
        CP      D               ; yet: keep the claim, send it again
        JR      NC,GRS9
        XOR     A
        LD      (ACTSNT),A
        RET
GRS8:   LD      A,C             ; turned down, or the cursor is past it
        CP      NPLOTS
        JR      NC,GRS9
        LD      A,D
        CP      C
        JR      C,GRS9
        LD      A,(ACTSNT)
        CP      A_CLAIM
        JR      NZ,GRS10
        LD      A,E
        OR      A
        JR      Z,GRS10
GRS9:   LD      A,255
        LD      (GRWANT),A
        XOR     A
        LD      (GRCLM),A
        LD      A,SNERR
        JP      SNDPLAY
GRS10:  LD      A,C             ; the cursor is there: send it again
        CP      D
        RET     NZ
        LD      A,(ACT)
        OR      A
        RET     NZ
        LD      A,(ACTSNT)
        CP      A_CLAIM
        RET     Z
        LD      L,D
        LD      H,0
        LD      A,A_CLAIM
        JP      ACTSEND

; GRISR: every frame: the cursor moves to the next free plot every GSTEP
; frames, blanking for one frame at each step, like the Atari.
GRISR:  LD      A,(GCUR)
        CP      NPLOTS
        JR      NC,GRI0
        LD      A,(GSHOW)
        OR      A
        JR      NZ,GRI1
GRI0:   LD      A,SLOTCUR
        JP      SPRHIDE
GRI1:   LD      A,(EVBTN)
        OR      A
        JR      Z,GRI2
        LD      A,(GRPRESS)
        CP      255
        JR      NZ,GRI2
        LD      A,(GCUR)
        LD      (GRPRESS),A
        LD      A,(GSTEP)
        LD      B,A
        LD      A,(GNEXT)
        CP      B
        JR      NZ,GRI2
        LD      A,(GRWAS)
        CP      NPLOTS
        JR      NC,GRI2
        LD      (GRPRESS),A
GRI2:   LD      A,(GNEXT)
        OR      A
        JR      Z,GRI6
        DEC     A
        LD      (GNEXT),A
        JR      NZ,GRI6
        ; at most one step past where the server last had it
        LD      A,(GRSRV)
        CP      NPLOTS
        JR      NC,GRI3
        LD      B,A
        LD      A,(GCUR)
        CP      B
        JR      Z,GRI3
        JR      C,GRI3
        LD      A,1
        LD      (GNEXT),A
        JR      GRI6
GRI3:   LD      A,(GCUR)
        LD      (GRWAS),A
GRI4:   INC     A
        CP      NPLOTS
        JR      NC,GRI5
        CP      STOREP
        JR      Z,GRI4
        LD      E,A
        LD      D,0
        LD      HL,HOWNER
        ADD     HL,DE
        LD      C,A
        LD      A,(HL)
        CP      NOBODY
        LD      A,C
        JR      NZ,GRI4
GRI5:   LD      (GCUR),A
        LD      A,(GSTEP)
        LD      (GNEXT),A
        LD      A,SLOTCUR
        JP      SPRHIDE
GRI6:   LD      A,(GCUR)
        CALL    PLOTXY
        LD      (SPA_X),HL
        LD      (SPA_Y),DE
        LD      A,SH_CURS
        LD      (SPA_SH),A
        XOR     A
        LD      (SPA_FR),A
        LD      (SPA_FL),A
        LD      A,CW
        LD      (SPA_COL),A
        LD      A,SLOTCUR
        JP      SPRSET

GRTICK: CALL    MSGTICK
        LD      A,(GRDEP)
        OR      A
        JR      Z,GRT2
        LD      A,(FXFLAG)
        CP      1
        JR      Z,GRT1
        XOR     A
        CALL    ACTOR
        LD      A,(IX+AC_ST)
        OR      A
        RET     NZ
GRT1:   XOR     A               ; the ship has gone: the grant proper begins
        LD      (GRDEP),A
        CALL    XFXSTOP
        LD      B,T_LANDGRANT
        LD      HL,0
        CALL    SHTITLE
        CALL    SNDALRT
        LD      B,M_SELECT
        CALL    MSG0
        LD      A,1
        LD      (GSHOW),A
        RET
GRT2:   LD      A,(EVBTN)
        OR      A
        RET     Z
        ; claim the plot the cursor was on when the button went down, and
        ; hold the cursor there for the server's pause after a claim
        LD      A,(GRPRESS)
        CP      NPLOTS
        JR      C,GRT3
        LD      A,(GCUR)
GRT3:   LD      C,A
        LD      A,255
        LD      (GRPRESS),A
        LD      A,(GRCLM)
        OR      A
        RET     NZ
        LD      A,(MYSEAT)
        CP      4
        RET     NC
        LD      A,C
        CP      NPLOTS
        RET     NC
        LD      (GRWANT),A
        LD      (GCUR),A
        LD      L,A
        LD      H,0
        LD      A,A_CLAIM
        CALL    ACTSEND
        LD      A,1
        LD      (GRCLM),A
        LD      A,(GSTEP)
        ADD     A,A
        ADD     A,A
        LD      (GNEXT),A
        LD      A,255
        LD      (GRWAS),A
        LD      A,SNBUY
        JP      SNDPLAY

; ---- production and the colony events (events.c) -------------------------------
PRENTER: XOR    A
        LD      (MAPDOTS),A
        CALL    MAPENTER
        LD      HL,W_COL
        CALL    RXGETB
        LD      (PREVT),A
        INC     HL
        CALL    RXGETB
        CP      NPLOTS
        JR      C,PRE1
        XOR     A
PRE1:   LD      (PRPLOT),A
        XOR     A
        LD      (PRSTART),A
        LD      (PRDONE),A
        LD      (FXFLAG),A
        LD      A,(PREVT)
        OR      A
        JR      Z,PRE2
        CP      10
        JR      NC,PRE2
        ADD     A,T_EVENT-1
        LD      B,A
        LD      HL,0
        CALL    SHTITLE
        LD      A,(PREVT)
        ADD     A,M_EVENT-1
        LD      B,A
        CALL    MSG0
        JP      PRSTARTEV
PRE2:   LD      B,T_PROD
        LD      A,(RND)
        LD      L,A
        LD      H,0
        CALL    SHTITLE
        LD      A,1
        LD      (PRDONE),A
        RET

PRSTARTEV:
        LD      A,(PREVT)
        CP      CE_PIRATES
        JR      NZ,PS1
        XOR     A
        LD      HL,FXPIRATE
        CALL    FXSTART
        JP      PS9
PS1:    CP      CE_SHIP
        JR      NZ,PS2
        XOR     A
        LD      HL,FXLAND
        CALL    FXSTART
        JP      PS9
PS2:    CP      CE_PEST
        JR      NZ,PS3
        LD      A,(PRPLOT)
        CALL    PLOTXY
        LD      BC,0
        CALL    ACSETXY
        XOR     A
        LD      HL,FXPEST
        CALL    FXSTART
        JP      PS9
PS3:    CP      CE_ACID
        JR      NZ,PS4
        LD      A,(PRPLOT)
        CALL    PLOTXY
        LD      HL,0
        LD      BC,0
        CALL    ACSETXY
        XOR     A
        LD      HL,FXCLOUD
        CALL    FXSTART
        LD      A,(PRPLOT)
        CALL    PLOTXY
        LD      HL,24
        ADD     HL,DE
        EX      DE,HL
        LD      HL,0
        LD      BC,1
        CALL    ACSETXY
        LD      A,1
        LD      HL,FXBOLT
        CALL    FXSTART
        JP      PS9
PS4:    CP      CE_SUNSPOT
        JR      NZ,PS5
        XOR     A
        LD      HL,FXSPARKS
        CALL    FXSTART
        LD      A,1
        LD      HL,FXSPARK2
        CALL    FXSTART
        JP      PS9
PS5:    CP      CE_RADIATION
        JR      NZ,PS6
        LD      A,(PRPLOT)
        CALL    PLOTXY
        LD      BC,8
        ADD     HL,BC
        EX      DE,HL
        ADD     HL,BC
        EX      DE,HL
        LD      BC,0
        CALL    ACSETXY
        XOR     A
        LD      HL,FXRADIAT
        CALL    FXSTART
        JP      PS9
PS6:    CP      CE_METEOR
        JR      NZ,PS7
        ; from 120 right and 120 lines up of the plot
        LD      A,(PRPLOT)
        CALL    PLOTXY
        LD      BC,120
        ADD     HL,BC
        LD      A,L
        CP      183
        JR      C,PS6A
        LD      L,182
PS6A:   EX      DE,HL
        OR      A
        SBC     HL,BC
        EX      DE,HL
        LD      BC,0
        CALL    ACSETXY
        XOR     A
        LD      HL,FXMETEOR
        CALL    FXSTART
        JP      PS9
PS7:    LD      HL,(FRAMES)     ; fire, quake: timed here
        LD      (PHT0),HL
PS9:    LD      A,1
        LD      (PRSTART),A
        LD      A,SNEVT
        JP      SNDPLAY

; ACSETXY: actor C at x HL, y DE.
ACSETXY: LD     A,C
        CALL    ACTOR
        LD      (IX+AC_XL),L
        LD      (IX+AC_XH),H
        LD      (IX+AC_YL),E
        LD      (IX+AC_YH),D
        RET

PRTICK: CALL    MSGTICK
        LD      A,(PRDONE)
        OR      A
        JR      Z,PRT1
        LD      A,(MAPDOTS)     ; the production dots (from the window:
        OR      A               ; when it holds a state)
        RET     NZ
        LD      A,(STVALID)
        OR      A
        RET     Z
        LD      (MAPDOTS),A
        JP      MAPALL
PRT1:   LD      A,(PREVT)
        CP      CE_FIRE
        JR      NZ,PRT3
        ; the store burns for 640 frames: fire flickers over it
        LD      HL,(FRAMES)
        LD      DE,(PHT0)
        OR      A
        SBC     HL,DE
        LD      DE,640
        OR      A
        SBC     HL,DE
        JP      NC,PRT9
        CALL    PRFLICK
        RET     C
        CALL    RAND
        LD      (PRK),A
        ; x 8 + 16 * 4 (+8), line 6 + 12 * 2 (+3): PLOT 22
        AND     1
        ADD     A,A
        ADD     A,PX0+16
        LD      B,A
        LD      A,(PRK)
        AND     2
        LD      C,6+24
        JR      Z,PRT2
        LD      C,6+24+6
PRT2:   LD      A,(PRK)
        LD      E,CO
        BIT     3,A
        JR      NZ,PRT2A
        LD      E,CW
PRT2A:  RRCA
        RRCA
        AND     1
        ADD     A,2             ; PIC_FIRE0 / 1
        CALL    PIC
        LD      A,(PRK)
        AND     30H
        RET     NZ
        LD      A,STOREP
        JP      MAPPLOT
PRT3:   CP      CE_QUAKE
        JR      NZ,PRT6
        ; the ground shakes: plots jolt (flash outlined) for 300 frames
        LD      HL,(FRAMES)
        LD      DE,(PHT0)
        OR      A
        SBC     HL,DE
        LD      DE,300
        OR      A
        SBC     HL,DE
        JR      NC,PRT5
        CALL    PRFLICK
        RET     C
        CALL    PRUNBL
PRT4:   CALL    RAND
        AND     3FH
        CP      NPLOTS
        JR      NC,PRT4
        LD      (MAPBLK),A
        LD      A,1
        LD      (MAPBLON),A
        LD      A,(MAPBLK)
        JP      MAPPLOT
PRT5:   CALL    PRUNBL
        JR      PRT9
PRT6:   LD      A,(FXFLAG)
        CP      1
        JR      Z,PRT9
        LD      A,(PRSTART)
        OR      A
        RET     Z
        XOR     A
        CALL    ACTOR
        LD      A,(IX+AC_ST)
        OR      A
        RET     NZ
        LD      A,1
        CALL    ACTOR
        LD      A,(IX+AC_ST)
        OR      A
        RET     NZ
PRT9:   LD      A,1
        LD      (PRDONE),A
        CALL    XFXSTOP
        LD      B,T_PROD
        LD      A,(RND)
        LD      L,A
        LD      H,0
        CALL    SHTITLE
        XOR     A               ; PRTICK draws the dots
        LD      (MAPDOTS),A
        RET

; PRFLICK: carry unless 4 frames have passed since the last flicker.
PRFLICK: LD     HL,(FRAMES)
        LD      DE,(PRLAST)
        OR      A
        SBC     HL,DE
        LD      A,H
        OR      A
        JR      NZ,PRF1
        LD      A,L
        CP      4
        RET     C
PRF1:   LD      HL,(FRAMES)
        LD      (PRLAST),HL
        OR      A
        RET

; PRUNBL: the plot jolted last, back as it was.
PRUNBL: LD      A,(MAPBLK)
        CP      NPLOTS
        RET     NC
        LD      C,A
        LD      A,NOBODY
        LD      (MAPBLK),A
        LD      A,C
        JP      MAPPLOT

; ---- the events' scripts (events.c) --------------------------------------------
PIRY    EQU     66
FXPIRATE: DB    FX_LOOK,FK_SHIP,5,SH_NOSE,1,0,CW,CO
        DB      FX_POS
        XB      182
        XB      PIRY
        DB      FX_WAIT,36
        DB      FX_MOVE,-1 AND 0FFH,0,1,230
        DB      FX_HIDE,FX_WAIT,18
        DB      FX_POS
        XB      -40
        XB      PIRY
        DB      FX_MOVE,1,0,4,78
        DB      FX_MOVE,5,0,6,9
        DB      FX_SIG,2                        ; landed
        DB      FX_WAIT,150
        DB      FX_MOVE,-5 AND 0FFH,0,8,9
        DB      FX_MOVE,-1 AND 0FFH,0,4,90
        DB      FX_HIDE
        DB      FX_SIG,1
        DB      FX_END

; the pest crawls round its plot, 2 every 5 frames, pausing at the corners
FXPEST: DB      FX_LOOK,FK_SINGLE,3,SH_PEST,0,1,CW,0
        DB      FX_FRAME,0
        DB      FX_MOVE,2,0,5,3,FX_WAIT,11
        DB      FX_MOVE,0,2,5,3,FX_WAIT,11
        DB      FX_MOVE,-2 AND 0FFH,0,5,3,FX_WAIT,11
        DB      FX_MOVE,0,-2 AND 0FFH,5,3,FX_WAIT,11
        DB      FX_LOOP,6,28
        DB      FX_HIDE
        DB      FX_SIG,1
        DB      FX_END

; acid rain: the cloud drifts along its row, its colour cycling, with its
; lightning 24 lines under it, flickering
FXCLOUD: DB     FX_LOOK,FK_SINGLE,3,SH_CLOUD,0,0,CW,0
        DB      FX_FRAME,0,FX_COL,CW,FX_MOVE,1,0,4,5
        DB      FX_FRAME,1,FX_COL,CB,FX_MOVE,1,0,4,5
        DB      FX_FRAME,2,FX_COL,CW,FX_MOVE,1,0,4,5
        DB      FX_FRAME,3,FX_COL,CB,FX_MOVE,1,0,4,5
        DB      FX_LOOP,7,36
        DB      FX_HIDE
        DB      FX_SIG,1
        DB      FX_END
FXBOLT: DB      FX_LOOK,FK_SINGLE,4,SH_BOLT,0,0,CO,0
        DB      FX_FRAME,0,FX_MOVE,1,0,4,5
        DB      FX_HIDE,FX_MOVE,1,0,4,5
        DB      FX_FRAME,0,FX_MOVE,1,0,4,5
        DB      FX_HIDE,FX_MOVE,1,0,4,5
        DB      FX_LOOP,7,26
        DB      FX_HIDE
        DB      FX_END

; sunspots: single pixels flickering round the map
SPK     MACRO   spx, spy
        DB      FX_POS
        XB      spx
        XB      spy
        DB      FX_WAIT,6
        ENDM
FXSPARKS: DB    FX_LOOK,FK_SINGLE,3,SH_SPARK,0,0,CW,0
        SPK     32,44
        SPK     112,124
        SPK     64,28
        SPK     142,84
        SPK     12,144
        SPK     92,50
        SPK     52,112
        SPK     128,36
        DB      FX_LOOP,7,40
        DB      FX_HIDE
        DB      FX_SIG,1
        DB      FX_END
FXSPARK2: DB    FX_LOOK,FK_SINGLE,4,SH_SPARK,0,0,CO,0
        DB      FX_WAIT,3
        SPK     80,136
        SPK     22,74
        SPK     132,152
        SPK     102,24
        SPK     44,164
        SPK     152,60
        SPK     8,104
        SPK     88,94
        DB      FX_LOOP,7,40
        DB      FX_HIDE
        DB      FX_END

; radiation: the plot's M.U.L.E. flashes, then bolts off to the right
FXRADIAT: DB    FX_LOOK,FK_SINGLE,3,SH_MULE,2,0,CW,0
        DB      FX_FRAME,2,FX_WAIT,6,FX_HIDE,FX_WAIT,6
        DB      FX_LOOP,5,7
        DB      FX_FRAME,2
        DB      FX_MOVE,2,0,1,90
        DB      FX_HIDE
        DB      FX_SIG,1
        DB      FX_END

; the meteorite falls from the top right onto its plot
FXMETEOR: DB    FX_LOOK,FK_SINGLE,3,SH_METEOR,0,0,CO,0
        DB      FX_FRAME,0
        DB      FX_MOVE,-2 AND 0FFH,2,2,60
        DB      FX_HIDE
        DB      FX_SIG,1
        DB      FX_END

; ---- the summary's walk-ins (status.c), run from page 2 -----------------------
; each player enters 30 frames after the one before, walks left to x 11
; (33 steps of 4), then climbs to its row, 4 lines every 6 frames
SUW     MACRO   pwait, psteps
        DB      FX_WAIT,pwait
        DB      FX_MOVE,-4 AND 0FFH,0,6,33
        DB      FX_FRAME,8
        DB      FX_MOVE,0,-4 AND 0FFH,6,psteps
        DB      FX_END
        ENDM
FXSUW0:
        SUW     1,30
FXSUW1:
        SUW     30,22
FXSUW2:
        SUW     60,14
FXSUW3:
        SUW     90,6
FXSUWT: DW      FXSUW0,FXSUW1,FXSUW2,FXSUW3

; XFXSUW: actor A (its look set by the caller) walks in.
XFXSUW: PUSH    AF
        ADD     A,A
        LD      E,A
        LD      D,0
        LD      HL,FXSUWT
        ADD     HL,DE
        LD      A,(HL)
        INC     HL
        LD      H,(HL)
        LD      L,A
        POP     AF
        JP      FXSTART

; XFXLAND: actor 0 lands the transport ship (the summary of round 0).
XFXLAND: XOR    A
        LD      (FXFLAG),A
        LD      HL,FXLAND
        JP      FXSTART
