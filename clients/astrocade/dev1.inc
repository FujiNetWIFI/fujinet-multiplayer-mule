; dev1.inc -- the development phase: turns, walking, the town (develop.c).
; Page 5; dev2.inc (page 6) has the map's button, the wampus, the player
; event and the computers' walks.
;
; The server keeps no player position. We walk locally and send actions:
; buy/return a M.U.L.E. at the corral, outfit it at a shop, install it on a
; plot, assay, list land, catch the wampus, gamble at the pub. A turn starts
; in the middle of town, as on the Atari.
;
; Walking positions are the Intellivision client's MOB units, so every
; constant is the one measured there (clients/intv/docs/ANIMATION.md):
; x = colour clock + 8, y = scanline / 2 + 8. SPRSET gets (x - 8, (y - 8) * 2).

PVP     DEFL    PHRAM
        PV      DVSEAT,1
        PV      DVFLAGS,1
        PV      DVMINE,1
        PV      DVPTU,1
        PV      DVEVSH,1        ; the player event shown
        PV      DVSAMP,1        ; plot we took an assay sample from (255 none)
        PV      DVOFFICE,1      ; 1: the land office waits for a plot
        PV      DVMSGS,1
        PV      DVMSGK,1
        PV      DVLEFT,2        ; turn time left / elapsed at the last state
        PV      DVEL,2
        PV      DVWP,1          ; wampus showing (255 none)
        PV      WPX,1
        PV      WPY,1
        PV      DVBWAIT,1       ; in a doorway waiting for the server
        PV      DVBT0,2
        PV      DVAI,1          ; a computer's plot marks
        PV      DVAIT,1
        PV      DVAIP,1
        PV      WX,1            ; walking
        PV      WY,1
        PV      WTOWN,1
        PV      WHOLD,1
        PV      WEVENT,1
        PV      WBLDG,1
        PV      WCNT,1
        PV      WSTEP,1
        PV      WFRAME,1
        PV      WFACE,1
        PV      WSP,1
        PV      WCOL,1
        PV      WIN,1
        PV      WMULE,1
        PV      WTX,4
        PV      WTY,4
        PV      WTH,1
        PV      WIX,1           ; the stick this step: -1 / 0 / 1
        PV      WIY,1
        PV      DVNAME,10       ; a name, for "<name>'S TURN"
        PV      AIX,2           ; the computer's walk: figure, M.U.L.E.
        PV      AIY,2           ;   (MOB units)
        PV      AIST,2          ;   stage
        PV      AICNT,2         ;   steps left in it
        PV      AIT,2           ;   frames to the next step
        PV      AIALT,2
        PV      AITX,1          ;   the plot's MOB x, y
        PV      AITY,1
        PV      AIHC,1          ;   steps across, down
        PV      AIHI,1
        PV      AIMULE,1        ;   a M.U.L.E. goes along
        PV      AISP,1
        PV      AICOL,1
        PV      WTP,1           ; WALKTER's plot
        PV      DMRV,2          ; dev2.inc's scratch
        PV      DBUMP,1
        PV      DBUW,2
        PV      DEVK,1
        PV      DEVJ,1
        PV      DEVV,2
        PV      DEVV2,2
        IF      PVP > RAMEND
RAMEND  DEFL    PVP
        ENDIF

SLOTPL  EQU     0
SLOTMULE EQU    1
SLOTWAMP EQU    2

WE_TOWNL EQU    1
WE_TOWNR EQU    2
WE_STOREL EQU   3
WE_STORER EQU   4
WE_BLDG EQU     5

B_ASSAY EQU     4
B_LAND  EQU     5
B_PUB   EQU     6
B_CORRAL EQU    7

TOWNYTOP EQU    37              ; street: player top 37..41
TOWNYBOT EQU    41
TOWNYDN EQU     57              ; walked into a lower building
TOWNYUP EQU     21              ; walked into an outfitter
TOWNXL  EQU     36              ; one more step left leaves town
TOWNXR  EQU     132
TOWNXS  EQU     84

TO_SEAT EQU     0               ; the turn (mule_proto.h MuleTurn)
TO_FLAGS EQU    1
TO_EVENT EQU    3
TO_ELAP EQU     6
TO_LEFT EQU     8
TO_PTU  EQU     12
TO_NWAMP EQU    13
TO_WAMP EQU     14

ERR_MONEY EQU   8
ERR_NOSTOCK EQU 7

; the doors: the four outfitters (by good: food, energy, smithore,
; crystite), then assay, land, pub, corral
DOORLO: DB      120,96,72,48,48,68,88,108
DOORHI: DB      120,96,72,48,48,68,88,124

; ---- the phase entries -------------------------------------------------------------
D5ENTER: LD     A,255
        LD      (DVSTATE),A
        LD      (DVMSGS),A
        XOR     A
        LD      (WMULE),A
        LD      (MAPDOTS),A
        LD      (DVAI),A
        LD      (DVBWAIT),A
        LD      (SCRNOW),A
        LD      (AIST),A
        LD      (AIST+1),A
        RET
D5ANIM: LD      A,(ANIM)
        CP      AN_WALK
        JP      Z,WALKISR
        LD      A,(DVSTATE)
        CP      DV_WAIT
        RET     NZ
        CALL    FARCALL
        DB      PGDEV2
        DW      AISTEP
        RET

; ---- walking, every frame (isr_walk): a step every 6 frames (the map: 14 on
; the river, 21 in the mountains); a step is 4 x / 2 y in town, 2 / 1 on the
; map. On the map the player is the Atari's small figure, in town the species.
WALKDRAW: LD    A,(WX)          ; the player
        SUB     8
        LD      L,A
        LD      H,0
        LD      (SPA_X),HL
        LD      A,(WY)
        SUB     8
        LD      L,A
        LD      H,0
        ADD     HL,HL
        LD      (SPA_Y),HL
        LD      A,(WCOL)
        LD      (SPA_COL),A
        XOR     A
        LD      (SPA_FL),A
        LD      A,(WSP)
        LD      B,A
        ADD     A,A
        ADD     A,A
        ADD     A,B
        ADD     A,A             ; *10
        LD      C,A
        LD      A,(WTOWN)
        OR      A
        LD      A,SH_FIG
        JR      Z,WD1
        LD      A,C             ; the town: species * 11
        ADD     A,B
        LD      C,A
        LD      A,SH_SPEC
WD1:    LD      (SPA_SH),A
        LD      A,(WFACE)
        ADD     A,C
        LD      (SPA_FR),A
        LD      A,SLOTPL
        CALL    SPRSET
        LD      A,(WMULE)       ; the M.U.L.E., on the oldest step we recall
        OR      A
        LD      A,SLOTMULE
        JP      Z,SPRHIDE
        LD      A,(WTH)
        LD      E,A
        LD      D,0
        LD      HL,WTX
        ADD     HL,DE
        LD      A,(HL)
        OR      A
        JR      NZ,WD2
        LD      E,0
        LD      HL,WTX
        LD      A,(HL)
        OR      A
        RET     Z
WD2:    SUB     8
        LD      L,A
        LD      H,0
        LD      (SPA_X),HL
        LD      HL,WTY
        ADD     HL,DE
        LD      A,(HL)
        LD      C,A
        LD      A,(WTOWN)
        OR      A
        LD      A,C
        JR      Z,WD3
        ADD     A,3             ; in town it walks a little lower
WD3:    SUB     8
        LD      L,A
        LD      H,0
        ADD     HL,HL
        LD      (SPA_Y),HL
        LD      A,SH_MULE
        LD      (SPA_SH),A
        LD      A,(WTOWN)
        OR      A
        LD      A,(WFRAME)
        JR      NZ,WD4
        ADD     A,2
WD4:    LD      (SPA_FR),A
        LD      A,(WFACE)       ; facing left: mirrored
        CP      2
        JR      Z,WD5
        CP      3
WD5:    LD      A,0
        JR      NZ,WD6
        INC     A
WD6:    LD      (SPA_FL),A
        LD      A,CW
        LD      (SPA_COL),A
        LD      A,SLOTMULE
        JP      SPRSET

; WALKTER: how long a map step takes: a mountain makes it 21 frames, the
; river 14 (the figure's middle against the plot's peaks and the river band).
WALKTER: LD     A,(WX)
        SUB     6               ; cx = x - 8 + 2
        LD      C,A
        LD      A,(WY)
        SUB     8
        ADD     A,A
        ADD     A,2
        LD      B,A             ; cy = (y - 8) * 2 + 2
        LD      A,C
        CP      8
        RET     C
        LD      A,B
        CP      16
        RET     C
        LD      A,C             ; px = (cx - 8) / 16
        SUB     8
        RRCA
        RRCA
        RRCA
        RRCA
        AND     0FH
        CP      9
        RET     NC
        LD      D,A
        LD      A,B             ; py = (cy - 16) / 32
        SUB     16
        RLCA
        RLCA
        RLCA
        AND     7
        CP      5
        RET     NC
        LD      E,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,E
        ADD     A,D             ; p = py * 9 + px
        LD      (WTP),A
        LD      L,A
        LD      H,0
        PUSH    DE
        LD      DE,HTERR
        ADD     HL,DE
        POP     DE
        LD      A,(HL)
        LD      L,A
        AND     3
        CP      1
        JR      NZ,WT2
        LD      A,C             ; the river: cx + 4 >= 76 && cx <= 83
        ADD     A,4
        CP      76
        RET     C
        LD      A,C
        CP      84
        RET     NC
        LD      A,(WCNT)
        ADD     A,8
        LD      (WCNT),A
        RET
WT2:    LD      A,L
        AND     0CH
        RET     Z
        ; the peaks: x 1..8, lines 2..20 of the plot
        LD      A,D             ; PLOT_X = 8 + 16 px
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,8
        LD      H,A
        LD      A,C
        ADD     A,4
        DEC     A
        SUB     H               ; cx + 4 > PX + 1
        JR      C,WT9
        JR      Z,WT9
        LD      A,H
        ADD     A,9
        LD      L,A
        LD      A,C
        CP      L               ; cx < PX + 9
        RET     NC
        LD      A,E             ; PLOT_Y = 16 + 32 py
        RRCA
        RRCA
        RRCA
        ADD     A,16
        LD      H,A
        LD      A,B
        ADD     A,6
        SUB     H
        SUB     3               ; cy + 6 > PY + 2
        RET     C
        LD      A,H
        ADD     A,20
        LD      L,A
        LD      A,B
        CP      L               ; cy < PY + 20
        RET     NC
        LD      A,(WCNT)
        ADD     A,15
        LD      (WCNT),A
WT9:    RET

; WALKTOWN: a step in town (WIX, WIY).
WALKTOWN: LD    A,5
        LD      (WCNT),A
        ; along the street: 4 a step; out of either end: back to the map
        LD      A,(WIX)
        OR      A
        JR      Z,WTN3
        JP      P,WTN2
        LD      A,(WX)
        CP      TOWNXL+1
        JR      NC,WTN1
        LD      A,WE_TOWNL
WTNEV:  LD      (WEVENT),A
        LD      A,1
        LD      (WHOLD),A
        RET
WTN1:   SUB     4
        LD      (WX),A
        JR      WTN3
WTN2:   LD      A,(WX)
        CP      TOWNXR
        LD      A,WE_TOWNR
        JR      NC,WTNEV
        LD      A,(WX)
        ADD     A,4
        LD      (WX),A
WTN3:   ; the street is two steps deep; pushing on at a door walks you in; the
        ; outfitters let you in only with a M.U.L.E.
        LD      A,(WIY)
        OR      A
        RET     Z
        JP      M,WTN6
        LD      A,(WY)
        CP      TOWNYBOT
        JR      NC,WTN4
        ADD     A,2
        LD      (WY),A
        RET
WTN4:   LD      B,4             ; the lower buildings
WTN5:   CALL    WDOOR
        JR      NC,WTN5A
        LD      A,1
        LD      (WIN),A
        LD      A,B
        LD      (WBLDG),A
WTN5A:  INC     B
        LD      A,B
        CP      8
        JR      C,WTN5
        RET
WTN6:   LD      A,(WY)
        CP      TOWNYTOP+1
        JR      C,WTN7
        SUB     2
        LD      (WY),A
        RET
WTN7:   LD      A,(WMULE)
        OR      A
        RET     Z
        LD      B,0             ; the outfitters: right at the door
WTN8:   LD      E,B
        LD      D,0
        LD      HL,DOORLO
        ADD     HL,DE
        LD      A,(WX)
        CP      (HL)
        JR      NZ,WTN9
        LD      A,2
        LD      (WIN),A
        LD      A,B
        LD      (WBLDG),A
WTN9:   INC     B
        LD      A,B
        CP      4
        JR      C,WTN8
        RET

; WDOOR: carry if WX is within door B.
WDOOR:  LD      E,B
        LD      D,0
        LD      HL,DOORLO
        ADD     HL,DE
        LD      A,(WX)
        CP      (HL)
        CCF
        RET     NC              ; x < lo
        LD      HL,DOORHI
        ADD     HL,DE
        LD      A,(HL)
        LD      C,A
        LD      A,(WX)
        CP      C
        JR      Z,WDR1
        RET                     ; C: x < hi
WDR1:   SCF
        RET

; WALKBLDG: through a door: 2 a step until inside, then DVTICK runs the
; building's action and puts us back on the street.
WALKBLDG: LD    A,5
        LD      (WCNT),A
        LD      A,(WFRAME)
        XOR     1
        LD      (WFRAME),A
        LD      C,A
        LD      A,(WIN)
        CP      1
        JR      NZ,WB2
        LD      A,(WY)
        ADD     A,2
        LD      (WY),A
        LD      A,6
        ADD     A,C
        LD      (WFACE),A
        LD      A,(WY)
        CP      TOWNYDN
        JR      C,WB9
WB1:    XOR     A
        LD      (WIN),A
        LD      A,WE_BLDG
        LD      (WEVENT),A
        LD      A,1
        LD      (WHOLD),A
        JR      WB9
WB2:    LD      A,(WY)
        SUB     2
        LD      (WY),A
        LD      A,8
        ADD     A,C
        LD      (WFACE),A
        LD      A,(WY)
        CP      TOWNYUP+1
        JR      C,WB1
WB9:    JP      WALKDRAW

; WALKMAP: a step on the map: WX, WY is the small figure's top left, kept to
; x 15..157, y 16..92.
WALKMAP: LD     A,5
        LD      (WCNT),A
        LD      A,(WIX)
        OR      A
        JR      Z,WM3
        JP      P,WM2
        LD      A,(WX)
        CP      16
        JR      C,WM3
        SUB     2
        LD      (WX),A
        JR      WM3
WM2:    LD      A,(WX)
        CP      157
        JR      NC,WM3
        ADD     A,2
        LD      (WX),A
WM3:    LD      A,(WIY)
        OR      A
        JR      Z,WM5
        JP      P,WM4
        LD      A,(WY)
        CP      17
        JR      C,WM5
        DEC     A
        LD      (WY),A
        JR      WM5
WM4:    LD      A,(WY)
        CP      92
        JR      NC,WM5
        INC     A
        LD      (WY),A
WM5:    CALL    WALKTER
        ; into the store (plot 22: x 80..95, y 48..63)
        LD      A,(WX)
        ADD     A,5
        CP      80
        RET     C
        LD      A,(WX)
        ADD     A,2
        CP      96
        RET     NC
        LD      A,(WY)
        ADD     A,3
        CP      48
        RET     C
        LD      A,(WY)
        CP      64
        RET     NC
        LD      A,(WX)
        CP      88
        LD      A,WE_STOREL
        JR      C,WM6
        LD      A,WE_STORER
WM6:    LD      (WEVENT),A
        LD      A,1
        LD      (WHOLD),A
        RET

WALKISR: LD     A,(WHOLD)
        OR      A
        RET     NZ
        LD      HL,WCNT
        LD      A,(HL)
        OR      A
        JR      Z,WI1
        DEC     (HL)
        RET
WI1:    LD      A,(WIN)
        OR      A
        JP      NZ,WALKBLDG
        LD      A,(INDIR)
        LD      C,0
        BIT     3,A
        JR      Z,WI2
        LD      C,-1 AND 0FFH
WI2:    BIT     1,A
        JR      Z,WI3
        LD      C,1
WI3:    LD      B,0
        BIT     2,A
        JR      Z,WI4
        LD      B,-1 AND 0FFH
WI4:    BIT     0,A
        JR      Z,WI5
        LD      B,1
WI5:    LD      A,C
        LD      (WIX),A
        LD      A,B
        LD      (WIY),A
        OR      C
        JR      NZ,WI6
        LD      A,(WSTEP)       ; standing: facing us
        OR      A
        RET     Z
        XOR     A
        LD      (WSTEP),A
        LD      A,5
        LD      (WFACE),A
        JP      WALKDRAW
WI6:    LD      A,(WTOWN)
        OR      A
        JR      Z,WI7
        CALL    WALKTOWN
        JR      WI8
WI7:    CALL    WALKMAP
WI8:    LD      A,(WHOLD)
        OR      A
        RET     NZ
        LD      A,1
        LD      (WSTEP),A
        LD      A,(WFRAME)
        XOR     1
        LD      (WFRAME),A
        LD      C,A
        LD      A,(WIX)
        OR      A
        JR      Z,WI10
        JP      M,WI9
        LD      A,C             ; right: 0/1
        JR      WI12
WI9:    LD      A,C             ; left: 2/3
        ADD     A,2
        JR      WI12
WI10:   LD      A,(WIY)
        OR      A
        LD      A,C
        JP      M,WI11
        ADD     A,6             ; down: 6/7
        JR      WI12
WI11:   ADD     A,8             ; up: 8/9
WI12:   LD      (WFACE),A
        LD      A,SNSTEP
        CALL    SNDPLAY
        ; remember where we were, for the M.U.L.E. to follow
        LD      A,(WTH)
        LD      E,A
        LD      D,0
        LD      HL,WTX
        ADD     HL,DE
        LD      A,(WX)
        LD      (HL),A
        LD      HL,WTY
        ADD     HL,DE
        LD      A,(WY)
        LD      (HL),A
        LD      A,E
        INC     A
        AND     3
        LD      (WTH),A
        JP      WALKDRAW

; ---- the town screen -------------------------------------------------------------
TOWNMONEY: LD   A,(SCRNOW)
        CP      SCR_TOWN
        RET     NZ
        LD      A,(MYSEAT)
        CP      4
        RET     NC
        LD      A,10
        LD      (MSGROW),A
        LD      A,(MYSEAT)      ; (the window, while a state arrives)
        LD      E,HP_MONEY+1
        CALL    DVPLB
        LD      H,A
        LD      A,(MYSEAT)
        LD      E,HP_MONEY
        CALL    DVPLB
        LD      L,A
        LD      B,M_MONEY
        LD      DE,0
        CALL    MSGSHOW
        LD      A,11
        LD      (MSGROW),A
        ; the corral: its price, and how many are left
        LD      A,(STVALID)
        OR      A
        RET     Z
        LD      HL,W_STORE+29
        CALL    RXGETW
        EX      DE,HL
        LD      D,9
        LD      E,26
        LD      B,3
        CALL    DVNUM
        LD      HL,W_STORE+28
        CALL    RXGETB
        LD      L,A
        LD      H,0
        LD      D,6
        LD      E,27
        LD      B,2
; DVNUM: HL right-aligned in B cells at row D, column E, white.
DVNUM:  PUSH    DE
        CALL    NUMR
        POP     DE
        LD      HL,LINBUF
        LD      C,CW
        JP      TXTAT

DVRESUME: XOR   A
        LD      (WHOLD),A
        LD      A,5
        LD      (WFACE),A
        CALL    WALKDRAW
        LD      A,AN_WALK
        LD      (ANIM),A
        RET

TOWNENTER: XOR  A
        LD      (ANIM),A
        CALL    FXSTOP
        CALL    SPROFF
        CALL    CLRSCR
        LD      A,SCR_TOWN
        LD      (SCRNOW),A
        CALL    TOWN
        LD      B,T_DEVEL
        LD      A,(RND)
        LD      L,A
        LD      H,0
        CALL    SHTITLE
        CALL    TOWNMONEY
        LD      A,136
        LD      (BARX),A
        LD      A,16
        LD      (BARTOP),A
        LD      A,144
        LD      (BARH),A
        LD      A,255
        LD      (BARLAST),A
        LD      A,1
        LD      (WTOWN),A
        JP      DVRESUME

DVMAPENTER: CALL MAPENTER
        LD      B,T_DEVEL
        LD      A,(RND)
        LD      L,A
        LD      H,0
        JP      SHTITLE

; DVBLANK: the screen blanks for about 22 frames between the town and the map.
DVBLANK: XOR    A
        LD      (ANIM),A
        CALL    SPROFF
        CALL    CLRSCR
        LD      B,22
DVK1:   PUSH    BC
        CALL    FRWAIT
        CALL    SNDTICK
        POP     BC
        DJNZ    DVK1
        XOR     A
        LD      HL,WTX
        LD      B,4
DVK2:   LD      (HL),A
        INC     HL
        DJNZ    DVK2
        RET

DVSTARTW: LD    A,DV_WALK
        LD      (DVSTATE),A
        LD      A,255
        LD      (DVSAMP),A
        XOR     A
        LD      (DVOFFICE),A
        LD      (DVEVSH),A
        LD      (DVBWAIT),A
        LD      A,(MYSEAT)
        LD      E,HP_SPEC
        CALL    DVPLB
        AND     7
        LD      (WSP),A
        LD      A,(MYSEAT)
        CALL    DVPLC
        LD      (WCOL),A
        LD      (BARCOL),A
        XOR     A
        LD      HL,WTX
        LD      B,8
DVS1:   LD      (HL),A
        INC     HL
        DJNZ    DVS1
        LD      A,TOWNXS
        LD      (WX),A
        LD      A,TOWNYTOP+2
        LD      (WY),A
        XOR     A
        LD      (WIN),A
        LD      (WEVENT),A
        LD      (WCNT),A
        CALL    TOWNENTER
        LD      A,SNSEL
        JP      SNDPLAY

DVSTOPW: XOR    A
        LD      (ANIM),A
        INC     A
        LD      (WHOLD),A
        LD      A,DV_OVER
        LD      (DVSTATE),A
        LD      A,SLOTPL
        CALL    SPRHIDE
        LD      A,SLOTMULE
        CALL    SPRHIDE
        LD      A,SLOTWAMP
        CALL    SPRHIDE
        LD      A,(SCRNOW)
        CP      SCR_MAP
        RET     Z
        JP      DVMAPENTER

; DVPLB: seat A's player byte E (HP_*), window while a state arrives.
DVPLB:  PUSH    HL
        PUSH    DE
        LD      D,A
        LD      A,(MAPSRC)
        OR      A
        LD      A,D
        JR      Z,DPB1
        PUSH    AF
        LD      A,E
        ADD     A,9
        LD      E,A
        POP     AF
        CALL    WPLAYER
        CALL    RXGETB
        JR      DPB2
DPB1:   CALL    HPLAYER
        LD      A,E
        ADD     A,L
        LD      L,A
        JR      NC,DPB1A
        INC     H
DPB1A:  LD      A,(HL)
DPB2:   POP     DE
        POP     HL
        RET
; DVPLC: seat A's colour.
DVPLC:  PUSH    HL
        PUSH    DE
        LD      E,HP_COL
        CALL    DVPLB
        AND     3
        LD      E,A
        LD      D,0
        LD      HL,PLCOLT
        ADD     HL,DE
        LD      A,(HL)
        POP     DE
        POP     HL
        RET

; ---- buildings: the action goes to the server; its answer puts us back out ----
BACKSTREET: XOR A
        LD      (DVBWAIT),A
        LD      A,(DVSTATE)
        CP      DV_WALK
        RET     NZ
        LD      A,TOWNYTOP+2
        LD      (WY),A
        LD      A,SNSEL
        CALL    SNDPLAY
        JP      DVRESUME

DVBLDG: LD      A,(WBLDG)
        CP      B_CORRAL
        JR      NZ,DB1
        LD      A,(WMULE)
        OR      A
        LD      A,A_RETURN
        JR      NZ,DBSEND
        LD      A,A_BUYM
        JR      DBSEND
DB1:    CP      4
        JR      NC,DB2
        LD      L,A             ; an outfitter: the good
        LD      H,0
        LD      A,A_OUTFIT
        JR      DBSEND0
DB2:    CP      B_ASSAY
        JR      NZ,DB3
        LD      A,(DVSAMP)
        CP      NPLOTS
        JR      NC,DB2A
        LD      L,A
        LD      H,0
        LD      A,A_ASSAY
        JR      DBSEND0
DB2A:   LD      B,M_NOSAMPLE
        CALL    DVMSG0
        JP      BACKSTREET
DB3:    CP      B_LAND
        JR      NZ,DB4
        LD      A,1
        LD      (DVOFFICE),A
        LD      B,M_LANDOFFICE
        CALL    DVMSG0
        JP      BACKSTREET
DB4:    CP      B_PUB
        JP      NZ,BACKSTREET
        LD      A,A_PUB
DBSEND: LD      HL,0
DBSEND0: CALL   ACTSEND         ; stand in the door until the server answers
        LD      A,1
        LD      (DVBWAIT),A
        LD      HL,(FRAMES)
        LD      (DVBT0),HL
        RET

DVMSG0: LD      HL,0
        LD      D,H
        LD      E,L
        JP      MSGSHOW

DVBRESULT: LD   HL,W_ERR
        CALL    RXGETB
        CP      ERR_MONEY
        JR      NZ,DBR1
        LD      B,M_NOMONEY
        CALL    DVMSG0
        JR      DBR9
DBR1:   CP      ERR_NOSTOCK
        JR      NZ,DBR2
        LD      B,M_NOMULES
        CALL    DVMSG0
        JR      DBR9
DBR2:   OR      A
        JR      Z,DBR3
        LD      A,SNERR
        CALL    SNDPLAY
        JR      DBR9
DBR3:   LD      A,(ACTSNT)
        CP      A_ASSAY
        JR      NZ,DBR4
        LD      HL,W_RESLT
        CALL    RXGETB
        AND     7
        ADD     A,M_ASSAY
        LD      B,A
        CALL    DVMSG0
        LD      A,255
        LD      (DVSAMP),A
        JR      DBR9
DBR4:   CP      A_BUYM
        JR      Z,DBR5
        CP      A_OUTFIT
        JR      NZ,DBR9
DBR5:   LD      A,SNBUY
        CALL    SNDPLAY
DBR9:   JP      TOWNMONEY

; ---- a new state ---------------------------------------------------------------------
DVSTATEF: LD    HL,W_TURN
        CALL    RXGETB
        LD      (DVSEAT),A
        INC     HL
        CALL    RXGETB
        LD      (DVFLAGS),A
        LD      HL,W_TURN+TO_LEFT
        CALL    RXGETW
        LD      (DVLEFT),DE
        LD      HL,W_TURN+TO_ELAP
        CALL    RXGETW
        LD      (DVEL),DE
        LD      HL,W_TURN+TO_PTU
        CALL    RXGETB
        OR      A
        JR      NZ,DSF1
        LD      A,28
DSF1:   LD      (DVPTU),A
        LD      A,(MYSEAT)
        LD      B,A
        LD      A,(DVSEAT)
        CP      B
        LD      A,0
        JR      NZ,DSF2
        INC     A
DSF2:   LD      (DVMINE),A
        OR      A
        JR      Z,DSF5
        LD      A,(DVFLAGS)
        AND     TF_STARTED+TF_ENDED
        CP      TF_STARTED
        JR      NZ,DSF5
        ; our own turn
        LD      A,(DVSTATE)
        CP      DV_WALK
        CALL    NZ,DVSTARTW
        ; the server's view of what we lead wins over ours
        LD      A,(DVFLAGS)
        AND     TF_HASMULE
        JR      Z,DSF3
        LD      A,1
DSF3:   LD      (WMULE),A
        LD      A,(DVBWAIT)
        OR      A
        JR      Z,DSF4
        LD      A,(ACTSNT)
        CP      A_BUYM
        JR      C,DSF4
        CP      A_PUB+1
        JR      NC,DSF4
        CP      A_INSTAL
        JR      Z,DSF4
        CP      A_SELL
        JR      Z,DSF4
        CP      A_UNSELL
        JR      Z,DSF4
        CP      A_WAMPUS
        JR      Z,DSF4
        CALL    DVBRESULT
        CALL    BACKSTREET
DSF4:   LD      A,(SCRNOW)
        CP      SCR_MAP
        RET     NZ
        CALL    MAPUPD
        CALL    FARCALL
        DB      PGDEV2
        DW      DVMAPRES
        RET
DSF5:   ; anything else happens on the map
        LD      A,(DVSTATE)
        CP      DV_WALK
        CALL    Z,DVSTOPW
        LD      A,(SCRNOW)
        CP      SCR_MAP
        CALL    NZ,DVMAPENTER
        LD      A,11
        LD      (MSGROW),A
        LD      A,CW
        LD      (MSGCOL),A
        LD      A,(DVMINE)
        OR      A
        JR      Z,DSF6
        LD      A,(DVFLAGS)
        AND     TF_STARTED
        JR      NZ,DSF6
        LD      A,(DVSTATE)
        CP      DV_PROMPT
        JR      Z,DSF9
        LD      A,DV_PROMPT
        LD      (DVSTATE),A
        LD      B,M_START
        CALL    DVMSG0
        LD      A,SNTURN
        CALL    SNDPLAY
        JR      DSF9
DSF6:   LD      A,(DVSEAT)
        CP      4
        JR      NC,DSF9
        ; someone's turn: say so once per turn, then once when it ends. A
        ; computer's turn is over on the server at once but plays out on the
        ; screen, so it stays "taking his turn" until the next seat
        LD      C,1
        LD      E,HP_FLAG
        CALL    DVPLB
        AND     PF_AI
        JR      NZ,DSF7
        LD      C,2
        LD      A,(DVFLAGS)
        AND     TF_ENDED
        JR      NZ,DSF7
        LD      C,0
DSF7:   LD      A,(DVMSGS)
        LD      B,A
        LD      A,(DVSEAT)
        CP      B
        JR      NZ,DSF8
        LD      A,(DVMSGK)
        CP      C
        JR      Z,DSF9
DSF8:   LD      A,(DVSEAT)
        LD      (DVMSGS),A
        LD      A,C
        LD      (DVMSGK),A
        CALL    FARCALL
        DB      PGDEV2
        DW      DVSAYTURN
DSF9:   ; the time bar for whoever is playing, in their colour
        LD      A,(DVSEAT)
        CP      4
        JR      NC,DSF10
        CALL    DVPLC
        LD      (BARCOL),A
DSF10:  LD      A,(DVFLAGS)
        AND     TF_STARTED+TF_ENDED
        CP      TF_STARTED
        LD      HL,0
        JR      NZ,DSF11
        LD      HL,(DVLEFT)
DSF11:  CALL    DVBAR
        JP      MAPUPD

; DVBAR: the time bar for HL jiffies left (a line a PTU).
DVBAR:  LD      A,(DVPTU)
        LD      C,A
        LD      B,0
        LD      DE,0
DVBR1:  OR      A
        SBC     HL,BC
        JR      C,DVBR2
        INC     DE
        JR      DVBR1
DVBR2:  LD      A,D
        OR      A
        LD      A,E
        JR      Z,DVBR3
        LD      A,255
DVBR3:  JP      BARDRAW

; DVLEFTNOW: HL = the turn's jiffies left now.
DVLEFTNOW: CALL SINCERX
        EX      DE,HL
        LD      HL,(DVLEFT)
        OR      A
        SBC     HL,DE
        RET     NC
        LD      HL,0
        RET

DVTICK: CALL    MSGTICK
        LD      A,(DVSTATE)
        CP      DV_PROMPT
        JR      NZ,DT1
        LD      A,(EVBTN)
        OR      A
        RET     Z
        LD      A,A_BEGIN
        LD      HL,0
        CALL    ACTSEND
        LD      A,SNSEL
        JP      SNDPLAY
DT1:    CP      DV_WALK
        JR      Z,DT3
        CP      DV_WAIT
        JR      NZ,DT2
        CALL    FARCALL
        DB      PGDEV2
        DW      DVAIMARKS
DT2:    ; watching: run the bar down between polls
        LD      A,(DVFLAGS)
        AND     TF_STARTED+TF_ENDED
        CP      TF_STARTED
        RET     NZ
        CALL    DVLEFTNOW
        JP      DVBAR
DT3:    ; our own turn
        CALL    DVLEFTNOW
        CALL    DVBAR
        CALL    FARCALL
        DB      PGDEV2
        DW      DVEVMSG
        LD      A,(DVBWAIT)
        OR      A
        JR      Z,DT4
        LD      HL,(FRAMES)     ; no answer in 600: give up waiting
        LD      DE,(DVBT0)
        OR      A
        SBC     HL,DE
        LD      DE,601
        OR      A
        SBC     HL,DE
        RET     C
        JP      BACKSTREET
DT4:    LD      A,(WEVENT)
        OR      A
        JR      Z,DT9
        CP      WE_BLDG
        JR      NZ,DT5
        XOR     A
        LD      (WEVENT),A
        JP      DVBLDG
DT5:    CP      WE_STOREL
        JR      NC,DT7
        ; out of town: onto the map
        LD      B,68
        CP      WE_TOWNL
        JR      Z,DT6
        LD      B,99
DT6:    LD      A,B
        LD      (WX),A
        LD      A,52
        LD      (WY),A
        XOR     A
        LD      (WEVENT),A
        LD      (WTOWN),A
        CALL    DVBLANK
        CALL    DVMAPENTER
        LD      A,255
        LD      (BARLAST),A
        CALL    DVRESUME
        LD      A,20            ; the first map step waits 20 frames
        LD      (WCNT),A
        RET
DT7:    LD      B,TOWNXL        ; into the store: the town
        CP      WE_STOREL
        JR      Z,DT8
        LD      B,TOWNXR
DT8:    LD      A,B
        LD      (WX),A
        LD      A,TOWNYTOP+2
        LD      (WY),A
        XOR     A
        LD      (WEVENT),A
        CALL    DVBLANK
        JP      TOWNENTER
DT9:    LD      A,(WTOWN)
        OR      A
        JR      NZ,DT10
        CALL    FARCALL
        DB      PGDEV2
        DW      DVWAMPUS
        LD      A,(EVBTN)
        OR      A
        JR      Z,DT10
        CALL    FARCALL
        DB      PGDEV2
        DW      DVBUTTON
DT10:   LD      A,(EVKEY)
        CP      KEYEND
        RET     NZ
        LD      A,A_END
        LD      HL,0
        JP      ACTSEND
