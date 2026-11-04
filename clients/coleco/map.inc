; map.inc -- the planet (map.c): plots from the state, the time bar. Fixed.
;
;   text row 1       title
;   lines 16..175    the map (plot p at x 8 + 16 (p % 9), y 16 + 32 (p / 9))
;   text rows 22-23  message
;
; Where the state comes from: MAPSRC 1, the window (a state that just
; arrived: the phase's ENTER and STATE); 0, HOT (between states).

SCR_NONE EQU    0
SCR_TEXT EQU    1
SCR_MAP EQU     2
SCR_TOWN EQU    3
SCR_AUC EQU     4
SCR_SUMM EQU    5
SCR_FOUND EQU   6

AN_NONE EQU     0
AN_WALK EQU     1
AN_GRANT EQU    2
AN_AUC  EQU     3

MAP_BAR_X EQU   152

; MGET: A = map byte E (0..224: terrain, owner, mule, crystite, prod, 45
; each) of the state. Preserves BC, DE, HL.
MGET:   PUSH    HL
        PUSH    DE
        LD      D,0
        LD      A,(MAPSRC)
        OR      A
        JR      NZ,MG0
        LD      A,E             ; crystite, production: only in the window
        CP      135
        JR      C,MG1
        LD      A,(STVALID)
        OR      A
        LD      A,80H           ; (not there: "lost", no dots)
        JR      Z,MG2
MG0:    LD      HL,W_TERR
        ADD     HL,DE
        CALL    RXGETB
        JR      MG2
MG1:    LD      HL,HMAP
        ADD     HL,DE
        LD      A,(HL)
MG2:    POP     DE
        POP     HL
        RET

; SEATCOL: A = seat -> its colour, from the state.
SEATCOL: LD     E,A
        LD      A,(MAPSRC)
        OR      A
        LD      A,E
        JP      Z,PLCOLOR
        PUSH    HL
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

; MAPPLOT: plot A from the state (map.c map_draw_plot). Clobbers all.
MAPPLOT: LD     (MPP),A
        LD      E,A
        CALL    MGET            ; terrain
        LD      B,A
        AND     3
        CP      3
        LD      A,1
        JR      Z,MPA
        XOR     A
MPA:    LD      (PLK_ST),A
        LD      A,B
        AND     3
        CP      1
        LD      A,1
        JR      Z,MPB
        XOR     A
MPB:    LD      (PLK_RIV),A
        LD      A,B
        RRCA
        RRCA
        AND     3
        LD      (PLK_MNT),A
        XOR     A
        LD      (PLK_OWN),A
        LD      (PLK_ICON),A
        LD      (PLK_DOTS),A
        LD      A,C_WHITE
        LD      (PLK_COL),A
        LD      A,(MPP)
        ADD     A,45
        LD      E,A
        CALL    MGET            ; owner
        LD      (MPO),A
        LD      A,(MPP)
        ADD     A,90
        LD      E,A
        CALL    MGET            ; mule
        LD      (MPG),A
        LD      A,(PLK_ST)
        OR      A
        JP      NZ,MP9
        LD      A,(MPP)         ; blinking: up for auction, a computer's
        LD      B,A
        LD      A,(MAPBLK)
        CP      B
        JR      NZ,MP1
        LD      A,(MAPBLON)
        OR      A
        JR      NZ,MP2          ; (outlined in white)
MP1:    LD      A,(MPO)
        CP      4
        JR      NC,MP9          ; no owner: bare
        XOR     A               ; (not blinking)
        LD      (MPBL),A
        JR      MP3
MP2:    LD      A,1
        LD      (MPBL),A
MP3:    LD      A,1
        LD      (PLK_OWN),A
        LD      (PLK_ICON),A    ; a house
        LD      A,(MPO)
        CP      4
        JR      NC,MP4
        LD      A,(MPG)
        CP      4
        JR      NC,MP4
        ADD     A,ICON_FOOD     ; ICON_FOOD + good
        LD      (PLK_ICON),A
MP4:    LD      A,(MPBL)
        OR      A
        JR      Z,MP5
        LD      A,(MPO)         ; outlined in white (C_WHITE already)
        CP      4
        JR      C,MP9
        XOR     A
        LD      (PLK_ICON),A
        JR      MP9
MP5:    LD      A,(MPO)
        CALL    SEATCOL
        LD      (PLK_COL),A
        LD      A,(MAPDOTS)
        OR      A
        JR      Z,MP9
        LD      A,(MPG)
        CP      4
        JR      NC,MP9
        LD      A,(MPP)
        ADD     A,180
        LD      E,A
        CALL    MGET            ; prod
        BIT     7,A
        JR      NZ,MP9
        AND     15
        CP      9
        JR      C,MP6
        LD      A,8
MP6:    LD      B,A             ; at most MAPDSTEP (the production's count)
        LD      A,(MAPDSTEP)
        OR      A
        JR      Z,MP7
        CP      B
        JR      C,MP8
MP7:    LD      A,B
MP8:    LD      (PLK_DOTS),A
MP9:    LD      A,(MPP)
        JP      PLOT

; MAPALL: every plot.
MAPALL: XOR     A
MA1:    PUSH    AF
        CALL    MAPPLOT
        POP     AF
        INC     A
        CP      NPLOTS
        JR      C,MA1
        RET

; MAPUPD: the plots whose terrain, owner, M.U.L.E. or production changed
; (window against HOT: production isn't in HOT, so only while it shows).
MAPUPD: XOR     A
MU1:    LD      (MPQ),A
        LD      C,3             ; terrain, owner, mule
        LD      E,A
MU2:    LD      D,0
        LD      HL,W_TERR
        ADD     HL,DE
        CALL    RXGETB
        LD      HL,HMAP
        ADD     HL,DE
        CP      (HL)
        JR      NZ,MU3
        LD      A,E
        ADD     A,45
        LD      E,A
        DEC     C
        JR      NZ,MU2
        JR      MU4
MU3:    LD      A,(MPQ)
        CALL    MAPPLOT
MU4:    LD      A,(MPQ)
        INC     A
        CP      NPLOTS
        JR      C,MU1
        RET

; MAPENTER: the map screen (map.c map_enter).
MAPENTER: XOR   A
        LD      (ANIM),A
        CALL    FXSTOP
        CALL    MAPFRAME
        LD      A,SCR_MAP
        LD      (SCRNOW),A
        LD      A,NOBODY
        LD      (MAPBLK),A
        XOR     A
        LD      (MAPBLON),A
        LD      A,MAP_BAR_X
        LD      (BARX),A
        LD      A,16
        LD      (BARTOP),A
        LD      A,160
        LD      (BARH),A
        LD      A,255
        LD      (BARLAST),A
        JP      MAPALL

; BARTICK: the time bar's tick, the original's (its VBI, $5E98): with A
; units of the bar left (PTU on a turn, ticks in an auction) and A under 30,
; a tick every A frames, high and low by turns, so it hurries as the bar
; runs out (map.c bar_tick). Run every frame the bar runs; it counts the
; frames gone by. A tick cuts only a tick (a soft sound).
BARTICK: LD     C,A
        LD      A,(BTF)
        LD      E,A
        LD      A,(FRAMES)
        LD      (BTF),A
        SUB     E               ; frames gone by
        RET     Z
        LD      B,A
        LD      A,C
        OR      A
        JR      Z,BTK0
        CP      30
        JR      C,BTK1
BTK0:   LD      A,1             ; the first tick comes at 29, high
        LD      (BTK),A
        XOR     A
        LD      (BTG),A
        RET
BTK1:   LD      A,(BTK)
        SUB     B
        JR      C,BTK2
        JR      Z,BTK2
        LD      (BTK),A
        RET
BTK2:   LD      A,C
        LD      (BTK),A
        LD      A,(BTG)
        LD      E,A
        XOR     1
        LD      (BTG),A
        LD      A,E
        ADD     A,SNTICK
        JP      SNDPLAY

; BARDRAW: the time bar at A lines of BARH, in BARCOL (map.c bar_draw).
BARDRAW: LD     B,A
        LD      A,(BARH)
        CP      B
        JR      NC,BD1
        LD      B,A
BD1:    LD      A,(BARLAST)
        CP      B
        RET     Z
        CP      255
        JR      Z,BD2
        CP      B
        JR      C,BD2
        LD      C,A             ; shorter: the top of it to black
        SUB     B
        LD      E,A             ; last - px lines
        LD      A,B
        LD      (BARLAST),A
        LD      A,(BARTOP)
        LD      D,A
        LD      A,(BARH)
        ADD     A,D
        SUB     C
        LD      C,A             ; top + h - last
        JR      BD4
BD2:    LD      A,B             ; longer (or new): black, then the colour
        LD      (BARLAST),A
        LD      A,(BARH)
        SUB     B
        LD      E,A             ; h - px lines of black from the top
        LD      A,(BARTOP)
        LD      C,A
        PUSH    BC
        LD      A,(BARX)
        LD      B,A
        LD      D,4
        LD      A,C_BLACK
        CALL    FILL
        POP     BC
        LD      A,B
        OR      A
        RET     Z
        LD      E,B             ; px lines of the colour under it
        LD      A,(BARTOP)
        LD      C,A
        LD      A,(BARH)
        ADD     A,C
        SUB     B
        LD      C,A
        LD      A,(BARX)
        LD      B,A
        LD      D,4
        LD      A,(BARCOL)
        JP      FILL
BD4:    LD      A,(BARX)
        LD      B,A
        LD      D,4
        LD      A,C_BLACK
        JP      FILL
