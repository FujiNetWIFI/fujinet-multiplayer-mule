; mappg.inc -- the map's plots, the town, fills and small pictures (gfx.h
; gfx_plot, gfx_town, gfx_fill, gfx_pic). The map page, with its art
; (gen/map.inc).
;
; A plot is 16 pixels by 12 lines at x 8 + 16 * (p % 9), line 6 + 12 * (p /
; 9) (the Atari's 16 x 32 at 3/8). It is built line by line from 1-bit
; layers, each pixel the first of: the owner's (border, icon, production
; dots) in the owner's colour; mountains or the store in white; the river in
; blue; else the black plains. Each layer goes on with the expander in OR
; mode over the cleared plot, masked by the ones above it, so no pixel
; mixes two colours.

PX0     EQU     2               ; the map's first byte column (x 8)
PY0     EQU     6               ; its first line (Atari 16)
PLOTH   EQU     12

; the look (gfx.h PlotLook), filled by the caller: PLK_*
; XPLOT: plot A as PLK_* says.
XPLOT:  LD      C,A
        LD      B,0             ; B = row, C = column
XP1:    LD      A,C
        CP      9
        JR      C,XP2
        SUB     9
        LD      C,A
        INC     B
        JR      XP1
XP2:    ; the screen address: 4000H + (6 + 12 row) * 40 + 2 + 4 col
        LD      A,B
        ADD     A,A
        ADD     A,B
        ADD     A,A
        ADD     A,A             ; 12 row
        ADD     A,PY0
        LD      L,A
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        LD      D,H
        LD      E,L
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,DE           ; *40
        LD      A,C
        ADD     A,A
        ADD     A,A
        ADD     A,PX0
        LD      E,A
        LD      D,0
        ADD     HL,DE
        LD      (PLADR),HL      ; a magic address (the screen less 4000H)
        ; the layers
        LD      HL,ZERO24
        LD      (PLOWN),HL
        LD      (PLICN),HL
        LD      (PLWH),HL
        LD      (PLRV),HL
        XOR     A
        LD      (PLDOTS),A
        LD      A,(PLK_ST)
        OR      A
        JR      Z,XP3
        LD      HL,PL_STORE
        LD      (PLWH),HL
        JR      XP6
XP3:    LD      A,(PLK_OWN)
        OR      A
        JR      Z,XP4
        LD      HL,PL_BORDER
        LD      (PLOWN),HL
        LD      A,(PLK_DOTS)
        LD      (PLDOTS),A
        LD      A,(PLK_ICON)
        OR      A
        JR      Z,XP4
        DEC     A
        LD      L,A
        ADD     A,A
        ADD     A,L             ; *3 ... *24:
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      E,A
        LD      D,0
        LD      HL,PL_ICON_HOUSE
        ADD     HL,DE
        LD      (PLICN),HL
XP4:    LD      A,(PLK_MNT)
        OR      A
        JR      Z,XP5
        DEC     A
        LD      L,A
        ADD     A,A
        ADD     A,L
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      E,A
        LD      D,0
        LD      HL,PL_MOUNT1
        ADD     HL,DE
        LD      (PLWH),HL
XP5:    LD      A,(PLK_RIV)
        OR      A
        JR      Z,XP6
        LD      HL,PL_RIVER
        LD      (PLRV),HL
XP6:    ; the owner's colour pairs (stripes: by line)
        LD      A,(PLK_COL)
        CP      CS
        JR      NZ,XP7
        LD      A,CB*4
        LD      (PLX1),A
        LD      A,CO*4
        JR      XP8
XP7:    ADD     A,A
        ADD     A,A
        LD      (PLX1),A
XP8:    LD      (PLX2),A
        XOR     A
        LD      (PLLN),A
XPLINE: ; this line's masks: own, white, river (2 bytes each)
        LD      A,(PLLN)
        ADD     A,A
        LD      E,A
        LD      D,0
        LD      HL,(PLOWN)
        ADD     HL,DE
        LD      B,(HL)
        INC     HL
        LD      C,(HL)
        LD      HL,(PLICN)
        ADD     HL,DE
        LD      A,(HL)
        OR      B
        LD      B,A
        INC     HL
        LD      A,(HL)
        OR      C
        LD      C,A
        CALL    PLDOT           ; BC |= the dots on this line
        LD      (PLM0),BC
        LD      HL,(PLWH)
        ADD     HL,DE
        LD      A,B
        CPL
        AND     (HL)
        LD      B,A
        INC     HL
        LD      A,C
        CPL
        AND     (HL)
        LD      C,A
        LD      (PLM1),BC
        LD      HL,(PLRV)
        ADD     HL,DE
        LD      A,(PLM0)
        LD      B,A
        LD      A,(PLM1)
        OR      B
        CPL
        AND     (HL)
        LD      B,A
        INC     HL
        LD      A,(PLM0+1)
        LD      C,A
        LD      A,(PLM1+1)
        OR      C
        CPL
        AND     (HL)
        LD      C,A
        LD      (PLM2),BC
        ; clear the plot's 4 bytes on this line, then the layers over them
        LD      HL,(PLADR)
        LD      DE,4000H
        ADD     HL,DE
        XOR     A
        LD      (HL),A
        INC     HL
        LD      (HL),A
        INC     HL
        LD      (HL),A
        INC     HL
        LD      (HL),A
        LD      A,(PLLN)        ; the owner's colour
        AND     1
        LD      A,(PLX1)
        JR      Z,XPL1
        LD      A,(PLX2)
XPL1:   LD      BC,(PLM0)
        CALL    PLLAY
        LD      A,CW*4
        LD      BC,(PLM1)
        CALL    PLLAY
        LD      A,CB*4
        LD      BC,(PLM2)
        CALL    PLLAY
        LD      HL,(PLADR)      ; the next line
        LD      DE,BYTEPL
        ADD     HL,DE
        LD      (PLADR),HL
        LD      HL,PLLN
        INC     (HL)
        LD      A,(HL)
        CP      PLOTH
        JP      C,XPLINE
        RET

; PLLAY: OR 16 pixels (mask B:C, the Atari's order, MSB left) in XPAND pair
; A onto the line at PLADR.
PLLAY:  LD      D,A
        LD      A,B
        OR      C
        RET     Z
        LD      A,D
        OUT     (XPAND),A
        LD      A,MAGOR+MAGEXP
        OUT     (MAGIC),A
        LD      HL,(PLADR)
        LD      (HL),B          ; high nibble
        INC     HL
        LD      (HL),B          ; low nibble
        INC     HL
        LD      (HL),C
        INC     HL
        LD      (HL),C
        RET

; PLDOT: BC |= the production dots (the first PLDOTS of 8) on line PLLN.
; Preserves DE.
PLDOT:  LD      A,(PLDOTS)
        OR      A
        RET     Z
        PUSH    DE
        LD      D,A
        LD      HL,PDOTS
PLD1:   LD      A,(PLLN)
        CP      (HL)
        INC     HL
        JR      NZ,PLD2
        LD      A,(HL)
        OR      B
        LD      B,A
        INC     HL
        LD      A,(HL)
        OR      C
        LD      C,A
        DEC     HL
PLD2:   INC     HL
        INC     HL
        DEC     D
        JR      NZ,PLD1
        POP     DE
        RET

ZERO24: DS      24

; ---- the town: 96 pixels x 54 lines at x 32, line 6 (RLE, see
; tools/mkgfx_astro.py) -------------------------------------------------------
XTOWN:  LD      HL,TOWNRLE
        LD      DE,4000H+PY0*BYTEPL+8
        LD      B,TOWNW         ; bytes left on this line
        LD      C,TOWNH         ; lines left
XT1:    LD      A,(HL)
        INC     HL
        CP      80H
        JR      NC,XTRUN
        INC     A               ; n+1 literals
        LD      (XTN),A
XTL1:   LD      A,(HL)
        INC     HL
        CALL    XTPUT
        RET     C
        LD      A,(XTN)
        DEC     A
        LD      (XTN),A
        JR      NZ,XTL1
        JR      XT1
XTRUN:  SUB     7DH             ; n-125 copies of the next byte
        LD      (XTN),A
        LD      A,(HL)
        INC     HL
        LD      (XTV),A
XTR1:   LD      A,(XTV)
        CALL    XTPUT
        RET     C
        LD      A,(XTN)
        DEC     A
        LD      (XTN),A
        JR      NZ,XTR1
        JR      XT1

; XTPUT: one byte of the town; carry once the last line is full.
XTPUT:  LD      (DE),A
        INC     DE
        DEC     B
        JR      Z,XTP2
        OR      A
        RET
XTP2:   LD      B,TOWNW
        PUSH    HL
        LD      HL,BYTEPL-TOWNW
        ADD     HL,DE
        EX      DE,HL
        POP     HL
        DEC     C
        SCF
        RET     Z
        OR      A
        RET

; ---- fills --------------------------------------------------------------------
; XFILL: bytes: column B (x/4), line C, D wide, E high, colour A (CS:
; stripes by line).
XFILL:  PUSH    AF
        LD      L,C
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        PUSH    DE
        LD      D,H
        LD      E,L
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,DE
        POP     DE
        LD      A,B
        ADD     A,L
        LD      L,A
        JR      NC,XF0
        INC     H
XF0:    PUSH    DE
        LD      DE,4000H
        ADD     HL,DE
        POP     DE
        POP     AF
        CALL    FILLPAT         ; B = even lines' byte, A = odd lines'
        LD      (XFV2),A
XF1:    PUSH    HL
        LD      A,C
        AND     1
        LD      A,B
        JR      Z,XF2
        LD      A,(XFV2)
XF2:    PUSH    BC
        LD      B,D
XF3:    LD      (HL),A
        INC     HL
        DJNZ    XF3
        POP     BC
        POP     HL
        PUSH    DE
        LD      DE,BYTEPL
        ADD     HL,DE
        POP     DE
        INC     C
        DEC     E
        JR      NZ,XF1
        RET

; FILLPAT: colour A -> B = a byte of it for even lines, A for odd ones.
FILLPAT: CP     CS
        JR      NZ,FP1
        LD      B,55H           ; blue
        LD      A,0AAH          ; orange
        RET
FP1:    LD      B,A             ; c * 55H
        ADD     A,A
        ADD     A,A
        OR      B
        LD      B,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        OR      B
        LD      B,A
        RET

; XPIC: picture A (PIC_*: 0 store icon, 1 flag, 2-3 fire) at byte column
; B, line C, colour E, opaque on black.
XPIC:   LD      L,A
        ADD     A,A
        ADD     A,L
        ADD     A,A             ; *6
        LD      L,A
        LD      H,0
        PUSH    DE
        LD      DE,PIC_STOREICON
        ADD     HL,DE
        POP     DE
        LD      A,E
        CP      CS
        JR      NZ,XPI1
        LD      A,CB
XPI1:   ADD     A,A
        ADD     A,A
        OUT     (XPAND),A
        EX      DE,HL           ; DE = the picture
        LD      L,C
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        PUSH    DE
        LD      D,H
        LD      E,L
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,DE
        POP     DE
        LD      A,B
        ADD     A,L
        LD      L,A
        JR      NC,XPI2
        INC     H
XPI2:   LD      B,6
XPI3:   LD      A,MAGEXP
        OUT     (MAGIC),A
        LD      A,(DE)
        INC     DE
        LD      (HL),A
        INC     HL
        LD      (HL),A
        DEC     HL
        LD      A,C
        CP      LINES-1
        RET     NC
        INC     C
        PUSH    DE
        LD      DE,BYTEPL
        ADD     HL,DE
        POP     DE
        DJNZ    XPI3
        RET

; =============================================================================
; the map (map.c): plots from the state, the time bar
; =============================================================================
; Where the state comes from: MAPSRC 1, the window (a state that just
; arrived: the phase's ENTER and STATE); 0, HOT (between states).

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

; SEATCOL: A = seat -> its colour code, from the state.
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

; XMAPPLOT: plot A from the state (map.c map_draw_plot).
XMAPPLOT: LD    (MPP),A
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
        LD      A,CW
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
        JR      NZ,MP9
        LD      A,(MPP)         ; blinking: up for auction, a computer's
        LD      B,A
        LD      A,(MAPBLK)
        CP      B
        JR      NZ,MP1
        LD      A,(MAPBLON)
        OR      A
        JR      NZ,MP2          ; (C: outlined in white)
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
        ADD     A,2             ; ICON_FOOD + good
        LD      (PLK_ICON),A
MP4:    LD      A,(MPBL)
        OR      A
        JR      Z,MP5
        LD      A,(MPO)         ; outlined in white (CW already)
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
MP6:    LD      (PLK_DOTS),A
MP9:    LD      A,(MPP)
        JP      XPLOT

; XMAPALL: every plot.
XMAPALL: XOR    A
MA1:    PUSH    AF
        CALL    XMAPPLOT
        POP     AF
        INC     A
        CP      NPLOTS
        JR      C,MA1
        RET

; XMAPUPD: the plots whose terrain, owner, M.U.L.E. or production changed
; (window against HOT).
XMAPUPD: XOR    A
MU1:    LD      (MPQ),A
        LD      C,3             ; terrain, owner, mule
        LD      E,A
MU2:    LD      D,0             ; terrain / owner / mule / (crystite) / prod
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
        CALL    XMAPPLOT
MU4:    LD      A,(MPQ)
        INC     A
        CP      NPLOTS
        JR      C,MU1
        RET

; XMAPENTER: the map screen (map.c map_enter).
XMAPENTER: XOR  A
        LD      (ANIM),A
        CALL    FXSTOP
        CALL    SPROFF
        CALL    CLRSCR
        LD      A,SCR_MAP
        LD      (SCRNOW),A
        LD      A,NOBODY
        LD      (MAPBLK),A
        XOR     A
        LD      (MAPBLON),A
        LD      A,152
        LD      (BARX),A
        LD      A,16
        LD      (BARTOP),A
        LD      A,160
        LD      (BARH),A
        LD      A,255
        LD      (BARLAST),A
        JP      XMAPALL

; XBARDRAW: the time bar at px lines (Atari) of BARH, in BARCOL (map.c
; bar_draw). Redrawn whole: it is one byte wide.
XBARDRAW: LD    B,A
        LD      A,(BARH)
        CP      B
        JR      NC,BD1
        LD      B,A
BD1:    LD      A,(BARLAST)
        CP      B
        RET     Z
        LD      A,B
        LD      (BARLAST),A
        ; black from the top to top + h - px, the colour from there down
        LD      A,(BARTOP)
        CALL    YCONV
        LD      (BDY0),A
        LD      A,(BARH)
        SUB     B
        LD      C,A
        LD      A,(BARTOP)
        ADD     A,C
        CALL    YCONV
        LD      (BDY1),A
        LD      A,(BARTOP)
        LD      C,A
        LD      A,(BARH)
        ADD     A,C
        CALL    YCONV
        LD      (BDY2),A
        LD      A,(BDY0)
        LD      C,A
        LD      A,(BDY1)
        SUB     C
        JR      Z,BD2
        LD      E,A
        LD      A,(BARX)
        RRCA
        RRCA
        AND     3FH
        LD      B,A
        LD      D,1
        LD      A,CK
        CALL    XFILL
BD2:    LD      A,(BDY1)
        LD      C,A
        LD      A,(BDY2)
        SUB     C
        RET     Z
        LD      E,A
        LD      A,(BARX)
        RRCA
        RRCA
        AND     3FH
        LD      B,A
        LD      D,1
        LD      A,(BARCOL)
        JP      XFILL

; YCONV: Atari line A (16..255) -> screen line 6 + (A - 16) * 3 / 8.
YCONV:  SUB     16
        LD      L,A
        LD      H,0
        LD      D,H
        LD      E,L
        ADD     HL,HL
        ADD     HL,DE
        SRL     H
        RR      L
        SRL     H
        RR      L
        SRL     H
        RR      L
        LD      A,L
        ADD     A,6
        RET

; XHDOTS: a dotted line (every other pixel) at byte column B, Atari line C
; (squeezed), D bytes wide, colour A (black erases). Opaque: it sets the
; line's bytes, so draw it on the black (gfx.h gfx_hdots).
XHDOTS: PUSH    AF
        PUSH    DE              ; (YCONV takes DE)
        LD      A,C
        CALL    YCONV
        LD      C,A
        POP     DE
        POP     AF
        CP      CS
        JR      NZ,XHD1
        LD      A,CB
XHD1:   RLCA                    ; pixels 0 and 2 of each byte:
        RLCA
        LD      E,A             ; c << 2
        RLCA
        RLCA
        RLCA
        RLCA
        OR      E
        LD      E,A             ; c << 6 | c << 2
        LD      L,C
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        PUSH    DE
        LD      D,H
        LD      E,L
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,DE
        POP     DE
        LD      A,B
        ADD     A,L
        LD      L,A
        JR      NC,XHD2
        INC     H
XHD2:   LD      A,H
        ADD     A,40H
        LD      H,A
        LD      A,C
        CP      LINES
        RET     NC
XHD3:   LD      (HL),E
        INC     HL
        DEC     D
        JR      NZ,XHD3
        RET

; XAFILL: GFILL in Atari terms: x B (colour clocks, /4), Atari lines C
; to C+E (squeezed), D bytes wide, colour A.
XAFILL: PUSH    AF
        PUSH    BC
        LD      A,D
        LD      (XFV2),A        ; (YCONV takes DE)
        LD      A,C
        ADD     A,E
        CALL    YCONV
        LD      E,A             ; the end line
        POP     BC
        LD      A,C
        PUSH    DE
        CALL    YCONV
        POP     DE
        LD      C,A
        LD      A,(XFV2)
        LD      D,A
        LD      A,E
        SUB     C
        JR      Z,XAF9
        JR      C,XAF9
        LD      E,A
        LD      A,B
        RRCA
        RRCA
        AND     3FH
        LD      B,A
        POP     AF
        JP      XFILL
XAF9:   POP     AF
        RET

; XAPIC: picture A at x B (colour clocks), Atari line C, colour E.
XAPIC:  PUSH    AF
        PUSH    DE
        LD      A,C
        CALL    YCONV
        LD      C,A
        POP     DE
        LD      A,B
        RRCA
        RRCA
        AND     3FH
        LD      B,A
        POP     AF
        JP      XPIC
