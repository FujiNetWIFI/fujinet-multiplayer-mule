; ---------------------------------------------------------------------------
; map.inc -- the planet (mekkogx map.c and the platforms' gfx_plot,
; gfx_town, gfx_pic): 9 x 5 plots of 10 x 8 at (6 + 10c, 10 + 8r), the time
; bar beside them, the town, the small pictures.
;
; A plot is composed in PLOTBUF (10 x 8, a value a pixel) from the layers
; gen/art.inc lists (the lit pixels' offsets, bit 7 an odd row), in the
; order of the C platforms: ground, river, the owner's border and icon and
; production dots, mountains over all; the store alone. Then one diffed
; DBLIT: a plot that did not change costs only the compare.
;
; The map's rows are palette 2: the ground is light grey, the inks blue,
; red and green; mountains and the river are blue (MNTCOL).
; ---------------------------------------------------------------------------

PLOTW   EQU 10
PLOTH   EQU 8
MNTCOL  EQU C_BLUE
RIVCOL  EQU C_BLUE

ICON_NONE EQU 0
ICON_HOUSE EQU 1
ICON_FOOD EQU 2                 ; + GOOD_*
ICON_MULE EQU 6
LY_BORDER EQU 0
LY_MOUNT EQU 7                  ; + mountains - 1
LY_STORE EQU 10
LY_RIVER EQU 11

; map plot p: x = 6 + 10 * (p % 9), y = 10 + 8 * (p / 9)
PLOTXY: DB 6,10, 16,10, 26,10, 36,10, 46,10, 56,10, 66,10, 76,10, 86,10
        DB 6,18, 16,18, 26,18, 36,18, 46,18, 56,18, 66,18, 76,18, 86,18
        DB 6,26, 16,26, 26,26, 36,26, 46,26, 56,26, 66,26, 76,26, 86,26
        DB 6,34, 16,34, 26,34, 36,34, 46,34, 56,34, 66,34, 76,34, 86,34
        DB 6,42, 16,42, 26,42, 36,42, 46,42, 56,42, 66,42, 76,42, 86,42

PLCOLT: DB C_BLUE, C_PURPLE, C_GREEN, C_P3      ; main.c pl_col

MP_P    EQU DTMP+8              ; the plot being drawn
MP_X    EQU DTMP+9
MP_Y    EQU DTMP+10
MP_COL  EQU DTMP+11
MP_ICON EQU DTMP+12
MP_DOTS EQU DTMP+13
MP_FLG  EQU DTMP+14             ; 1 store, 2 river, 4 owned; mountains << 4

; COLPAIR -- r4, r5 = colour r0's values on rows MP_Y, MP_Y + 1.  Leaf.
;   Clobbers A, r3, DC0.
COLPAIR: LDB MP_Y
        DCI ROWKEY
        ADC
        LM
        AS 0
        LR 3,A
        LM                      ; (the next row's key)
        AS 0
        DCI COLTAB
        ADC
        LM
        LR 5,A
        DCI COLTAB
        LR A,3
        ADC
        LM
        LR 4,A
        POP

; LAYER -- the list at DC1 painted into PLOTBUF: value r4 on its even rows,
;   r5 on its odd ones.  Leaf.  Clobbers A, r3, DC0, DC1.
LAYER:  XDC
        LM
        XDC
        CI 0FFH
        BZ LAYX
        LR 3,A
        NI 07FH
        DCI PLOTBUF
        ADC
        LR A,3
        NI 080H
        LR A,4
        BZ LAY1
        LR A,5
LAY1:   ST
        BR LAYER
LAYX:   POP

; PAINT -- layer r1 (PLYTAB) in colour r0.  Clobbers A, r3-r5, DC0, DC1.
PAINT:  ENTER
        PI COLPAIR
        DCI PLYTAB
        LR A,1
        SL 1
        ADC
        LM
        LR 10,A
        LM
        LR 11,A
        LR DC,H
        XDC
        PI LAYER
        LEAVE

; ---------------------------------------------------------------------------
; MAPPLOT -- plot r0 drawn as the state has it (map.c map_draw_plot).
;   Clobbers A, r0-r9, H, Q, DC0, DC1.
; ---------------------------------------------------------------------------
MAPPLOT: ENTER
        LR A,0
        STB MP_P
        SL 1                    ; its place
        DCI PLOTXY
        ADC
        LM
        STB MP_X
        DCI PLOTXY+1
        LDB MP_P
        SL 1
        DCI PLOTXY+1
        ADC
        LM
        STB MP_Y
        LDB MP_P                ; terrain: store, river, mountains
        DCI ST+M_TERR
        ADC
        LM
        LR 6,A
        NI 3
        CI T_STORE
        LIS 1
        BZ MPT1
        LR A,6
        NI 3
        CI T_RIVER
        LIS 2
        BZ MPT1
        CLR
MPT1:   LR 7,A
        LR A,6                  ; mountains (bits 2-3) into bits 4-5
        SL 1
        SL 1
        NI 030H
        AS 7
        STB MP_FLG
        LIS C_WHITE
        STB MP_COL
        CLR
        STB MP_ICON
        STB MP_DOTS
        ; owned: an owner, or the plot shown outlined
        LDB MP_FLG
        NI 1
        JNZ MPDRAW              ; (the store has neither)
        LDB MP_P
        DCI ST+M_OWNER
        ADC
        LM
        LR 6,A                  ; owner
        LDB MP_P
        DCI ST+M_MULE
        ADC
        LM
        LR 7,A                  ; M.U.L.E.
        PI MPBLINK              ; r8 = 1 if this plot is the one outlined now
        LR A,6
        CI 3
        BC MPO1                 ; an owner
        LR A,8
        NI 0FFH
        JZ MPDRAW
MPO1:   LDB MP_FLG
        OI 4
        STB MP_FLG
        LIS ICON_HOUSE
        LR 9,A
        LR A,6
        CI 3
        BNC MPO2
        LR A,7
        CI 3
        BNC MPO2
        AI ICON_FOOD
        LR 9,A
MPO2:   LR A,8                  ; outlined: in white, and no icon if no one's
        NI 0FFH
        BZ MPO3
        LR A,6
        CI 3
        BC MPO4
        CLR
        LR 9,A
        BR MPO4
MPO3:   LR A,6                  ; the owner's colour, and production dots
        STB GT                  ; (PLCOL: pl_col[players[o].color & 3])
        LR 0,A
        LI PLSIZE
        LR 2,A
        CLR
        LR 1,A
        PI MUL8
        DCI ST+PLAYERS+P_COL
        LR A,0
        ADCU
        LM
        NI 3
        DCI PLCOLT
        ADC
        LM
        STB MP_COL
        LDB MAPDOTS
        NI 0FFH
        BZ MPO4
        LR A,7
        CI 3
        BNC MPO4
        LDB MP_P
        DCI ST+M_PROD
        ADC
        LM
        NI 080H
        BNZ MPO4
        LDB MP_P
        DCI ST+M_PROD
        ADC
        LM
        NI 15
        CI 8
        BC MPO5
        LIS 8
MPO5:   STB MP_DOTS
MPO4:   LR A,9
        STB MP_ICON
MPDRAW: DCI PLOTBUF             ; the ground
        LI PLOTW*PLOTH
        LR 0,A
        CLR
        LR 1,A
        PI MEMZERO
        LDB MP_FLG
        NI 1
        BZ MPD1
        LIS C_WHITE             ; the store alone
        LR 0,A
        LI LY_STORE
        LR 1,A
        PI PAINT
        JMP MPBLIT
MPD1:   LDB MP_FLG
        NI 2
        BZ MPD2
        LIS RIVCOL
        LR 0,A
        LI LY_RIVER
        LR 1,A
        PI PAINT
MPD2:   LDB MP_FLG
        NI 4
        BZ MPD5
        LDB MP_COL
        LR 0,A
        LI LY_BORDER
        LR 1,A
        PI PAINT
        LDB MP_ICON
        NI 0FFH
        BZ MPD3
        LR 1,A
        LDB MP_COL
        LR 0,A
        PI PAINT
MPD3:   LDB MP_DOTS             ; the first n production dots
        NI 0FFH
        BZ MPD5
        LDB MP_COL
        LR 0,A
        PI COLPAIR
        LDB MP_DOTS
        LR 6,A
        DCI PDOTS
        XDC
MPD4:   XDC
        LM
        XDC
        LR 3,A
        NI 07FH
        DCI PLOTBUF
        ADC
        LR A,3
        NI 080H
        LR A,4
        BZ MPD4A
        LR A,5
MPD4A:  ST
        DS 6
        BNZ MPD4
MPD5:   LDB MP_FLG              ; mountains over all
        SR 4
        NI 3
        BZ MPBLIT
        AI LY_MOUNT-1
        LR 1,A
        LIS MNTCOL
        LR 0,A
        PI PAINT
MPBLIT: LDB MP_X
        LR 1,A
        LDB MP_Y
        LR 2,A
        LI PLOTW
        LR 3,A
        LIS PLOTH
        LR 4,A
        DCI PLOTBUF
        XDC
        PI DBLIT
        LEAVE

; MPBLINK -- r8 = 1 if plot MP_P is the one outlined in white now
;   (map_blink, while map_blink_on).  Leaf.  Clobbers A, DC0.
MPBLINK: CLR
        LR 8,A
        LDB MAPBON
        NI 0FFH
        BZ MPBX
        LDB MP_P
        DCI MAPBLNK
        CM
        BNZ MPBX
        LIS 1
        LR 8,A
MPBX:   POP

; PLATXY -- plot r0's corner in Atari units (PLOT_X, PLOT_Y): x = 8 + 16 col
;   in r4:r5, y = 16 + 32 row in r6:r7.  Leaf.  Clobbers A, r0.
PLATXY: CLR
        LR 6,A
PLX1:   LR A,0
        CI 8
        BC PLX2
        AI 0F7H                 ; - 9
        LR 0,A
        LR A,6
        INC
        LR 6,A
        BR PLX1
PLX2:   SL 4
        AI 8
        LR 4,A
        CLR
        LR 5,A
        LR 7,A
        LR A,6
        SL 4
        SL 1
        AI 16
        LR 6,A
        POP

; MAPALL -- every plot (map_draw_all).
MAPALL: ENTER
        CLR
        STB GT+1
MPA1:   LDB GT+1
        LR 0,A
        PI MAPPLOT
        INCB GT+1
        CI NPLOTS
        BNZ MPA1
        LEAVE

; MAPUPD -- the plots whose terrain, owner, M.U.L.E. or production changed
;   since the state drawn last (map_update).
MAPUPD: ENTER
        CLR
        STB GT+1
MPU1:   LDB GT+1
        LR 4,A
        DCI ST+M_TERR           ; terrain, owner, mule are 45 apart
        ADC
        LM
        LR 5,A
        DCI PREV+M_TERR
        LR A,4
        ADC
        LR A,5
        CM
        BNZ MPU2
        DCI ST+M_OWNER
        LR A,4
        ADC
        LM
        LR 5,A
        DCI PREV+M_OWNER
        LR A,4
        ADC
        LR A,5
        CM
        BNZ MPU2
        DCI ST+M_MULE
        LR A,4
        ADC
        LM
        LR 5,A
        DCI PREV+M_MULE
        LR A,4
        ADC
        LR A,5
        CM
        BNZ MPU2
        DCI ST+M_PROD
        LR A,4
        ADC
        LM
        LR 5,A
        DCI PREV+M_PROD
        LR A,4
        ADC
        LR A,5
        CM
        BZ MPU3
MPU2:   LR A,4
        LR 0,A
        PI MAPPLOT
MPU3:   INCB GT+1
        CI NPLOTS
        BNZ MPU1
        LEAVE

; MAPENTER -- the map screen (map.c map_enter).
MAPENTER: ENTER
        CLR
        STB ANIM
        PI FXSTOP
        PI DCLS
        PI BANDS
        LIS SCR_MAP
        STB SCRNOW
        LI NOBODY
        STB MAPBLNK
        CLR
        STB MAPBON
        LI 97                   ; the time bar: beside the plots, 2 wide
        STB BARX
        LI MAPY
        STB BARTOP
        LI 40
        STB BARH
        LI 0FFH
        STB BARLAST
        PI MAPALL
        LEAVE

; ---------------------------------------------------------------------------
; The time bar (map.c bar_draw, bar_tick): a row for every 4 units left (a
; unit is a PTU on a turn, a tick in an auction).  BARX/BARTOP/BARH are the
; Channel F's; BARLAST is the rows shown.
; ---------------------------------------------------------------------------

; BARDRAW -- the bar showing r0 units (map.c bar_draw).
BARROWS EQU DTMP+15             ; the rows wanted (DFILL keeps no register)
BARDRAW: ENTER
        LR A,0                  ; rows = units / 4, at most BARH
        SR 1
        SR 1
        LR 6,A
        LDB BARH
        LR 7,A
        LR A,6
        COM
        AS 7                    ; carries when BARH > rows
        LR A,6
        BC BAR1
        LR A,7
BAR1:   STB BARROWS
        LR 6,A
        LDB BARLAST
        XS 6
        JZ BARX1                ; as it is
        LDB BARLAST
        CI 0FFH
        BZ BAR2
        LR 7,A                  ; fewer than before: the difference off the
        LR A,6                  ; top; else all of it again
        COM
        AS 7                    ; carries when last > rows
        BNC BAR2
        LDB BARH                ; clear (last - rows) from top + h - last
        LR 3,A
        LDB BARLAST
        COM
        INC
        AS 3
        LR 3,A
        LDB BARTOP
        AS 3
        LR 2,A
        LDB BARLAST
        LR 3,A
        LDB BARROWS
        COM
        INC
        AS 3
        LR 4,A
        LDB BARX
        LR 1,A
        LIS 2
        LR 3,A
        CLR
        LR 0,A
        PI DFILL
        BR BAR3
BAR2:   LDB BARH                ; the ground above ...
        LR 3,A
        LDB BARROWS
        COM
        INC
        AS 3                    ; BARH - rows
        LR 4,A
        NI 0FFH
        BZ BAR2A
        LDB BARX
        LR 1,A
        LDB BARTOP
        LR 2,A
        LIS 2
        LR 3,A
        CLR
        LR 0,A
        PI DFILL
BAR2A:  LDB BARROWS             ; ... the colour below
        NI 0FFH
        BZ BAR3
        LR 4,A
        LDB BARH
        LR 3,A
        LDB BARROWS
        COM
        INC
        AS 3
        LR 3,A
        LDB BARTOP
        AS 3
        LR 2,A
        LDB BARX
        LR 1,A
        LIS 2
        LR 3,A
        LDB BARCOL
        LR 0,A
        PI DFILL
BAR3:   LDB BARROWS
        STB BARLAST
BARX1:  LEAVE

; BARTICK -- the original's tick ($5E98): with r0 units of the bar left and
;   under 30, a tick every r0 frames, high and low by turns (map.c bar_tick).
BARTICK: ENTER
        LDB BARFR               ; frames gone by
        LR 4,A
        LDB FRAMES
        LR 5,A
        LR A,4
        COM
        INC
        AS 5
        LR 6,A
        NI 0FFH
        BZ BTKX
        LR A,5
        STB BARFR
        LR A,0                  ; 0 or 30 and over: quiet, the first at 29
        NI 0FFH
        BZ BTK0
        CI 29
        BC BTK1
BTK0:    LIS 1
        STB BARTK
        CLR
        STB BARTG
        BR BTKX
BTK1:    LDB BARTK               ; due yet?
        LR 4,A
        LR A,6
        COM
        AS 4                    ; carries when tk > d
        BNC BTK2
        LR A,6
        COM
        INC
        AS 4
        STB BARTK
        BR BTKX
BTK2:    LR A,0
        STB BARTK
        LDB BARTG
        AI SND_TICK
        LR 0,A
        PI SNDPLAY
        LDB BARTG
        XI 1
        STB BARTG
BTKX:    LEAVE

; ---------------------------------------------------------------------------
; TOWNDRAW -- the town (gfx_town): 60 x 36 at (21, 10), a row at a time
;   from its RLE (n < 128: n+1 values; else the next value n-125 times).
; ---------------------------------------------------------------------------
TOWNDRAW: ENTER
        DCI TOWNRLE
        LR Q,DC
        CLR
        STB GT+2                ; the row
TWR:    LR DC,Q                 ; unpack a row into TOWNBUF
        XDC
        DCI TOWNBUF
        LI TOWNW
        LR 6,A                  ; values to go
TWC:    XDC
        LM
        XDC
        CI 07FH
        BNC TWRUN
        INC                     ; n + 1 values
        LR 5,A
TWL:    XDC
        LM
        XDC
        ST
        DS 6
        DS 5
        BNZ TWL
        BR TWN
TWRUN:  AI 256-125              ; the next value n - 125 times
        LR 5,A
        XDC
        LM
        XDC
TWR1:   ST
        DS 6
        DS 5
        BNZ TWR1
TWN:    LR A,6
        NI 0FFH
        BNZ TWC
        XDC
        LR Q,DC                 ; the next row's data
        XDC
        LI 21
        LR 1,A
        LDB GT+2
        AI MAPY
        LR 2,A
        LI TOWNW
        LR 3,A
        LIS 1
        LR 4,A
        DCI TOWNBUF
        XDC
        PI DBLIT
        INCB GT+2
        CI TOWNH
        BNZ TWR
        LEAVE

; ---------------------------------------------------------------------------
; PICDRAW -- small picture r2 (PIC_*) in colour r0 at the Atari's (r1:r?),
;   see PICAT: 5 x 4, transparent where unlit (gfx_pic).
; PICAT -- the same at the Channel F's (MP_X, MP_Y).
; ---------------------------------------------------------------------------
PICAT:  ENTER
        DCI PLOTBUF             ; transparent
        LI 20
        LR 3,A
        LI 0FFH
PIC1:   ST
        DS 3
        BNZ PIC1
        PI COLPAIR
        DCI PICTAB
        LR A,2
        SL 1
        ADC
        LM
        LR 10,A
        LM
        LR 11,A
        LR DC,H
        XDC
        PI LAYER
        LDB MP_X
        LR 1,A
        LDB MP_Y
        LR 2,A
        LIS 5
        LR 3,A
        LIS 4
        LR 4,A
        DCI PLOTBUF
        XDC
        PI DBLIT
        LEAVE
