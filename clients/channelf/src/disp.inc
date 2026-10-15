; ---------------------------------------------------------------------------
; disp.inc -- the screen: a shadow of it in RAM, and every write diffed.
;
; VRAM is 128 x 64 at 2 bits a pixel, written one pixel at a time through
; ports 1, 4 and 5 and a strobe on port 0 (the BIOS plotter's sequence,
; $0718-$0765), and it cannot be read back. So SHADOW ($8000) keeps a byte a
; pixel: the BACKGROUND, without the sprites. Every primitive compares the
; value it wants with the shadow and writes only what differs: about 30 us
; for a pixel that is already right, 80 for one that is not. Sprites (spr.inc)
; draw over the background in VRAM only and put the shadow back when they
; move.
;
; Visible: columns 4-105, rows 4-61. Columns 125 and 126 of a row pick its
; palette (only bit 1 of each counts):
;   pal 0  0 black, 1-3 white           pal 2  0 light grey, 1 blue, 2 red, 3 green
;   pal 1  0 light blue, 1 blue, ...    pal 3  0 light green, 1 blue, ...
; A primitive takes the GAME's colour (C_*, gfx.h) and maps it per row
; (COLTAB): black is the ground of any palette, everything else on a pal 0
; row is white, and player 3 is red and green on alternate rows.
;
; Coordinates here are the Channel F's pixels. Callers clip.
; ---------------------------------------------------------------------------

; the visible area
VX0     EQU 4
VX1     EQU 105
VY0     EQU 4
VY1     EQU 61


; ---------------------------------------------------------------------------
; DCOLV -- A, r3 = the value colour r0 has on row r2.  Leaf.
;   Clobbers DC0.
; ---------------------------------------------------------------------------
DCOLV:  DCI ROWKEY
        LR A,2
        ADC
        LM                      ; (palette * 2 + row parity) * 16
        AS 0
        DCI COLTAB
        ADC
        LM
        LR 3,A
        POP

; ---------------------------------------------------------------------------
; PORTS -- point the ports and DC0 at row r2, column r1: port 5 = the row
;   (keeping the sound bits), DC0 = SHADOW + r2*128 + r1, r9 = ~r1 for port 4.
;   Waits out the settle of a write just before.  Inline (leaves stay leaves).
;   Clobbers A, r5, r9, H, DC0.
; ---------------------------------------------------------------------------
PORTS   MACRO
        LIS 3
        LR 5,A
m_wt:   DS 5
        BNZ m_wt
        LR A,2
        COM
        NI 03FH
        LR 5,A
        INS 5
        NI 0C0H
        AS 5
        OUTS 5
        LR A,2
        SR 1
        AI SHADOW>>8
        LR 10,A
        LR A,2
        SL 4
        SL 1
        SL 1
        SL 1                    ; (row & 1) << 7
        AS 1
        LR 11,A
        LR DC,H
        LR A,1
        COM
        LR 9,A
        ENDM

; COLP -- port 1 = value r3.  Clobbers A.
COLP    MACRO
        LR A,3
        SL 4
        SL 1
        SL 1
        COM
        OUTS 1
        ENDM

; MARKNZ -- DROWNZ[r2] = 1, keeping DC0.  Clobbers A, H.
MARKNZ  MACRO
        LR H,DC
        DCI DROWNZ
        LR A,2
        ADC
        LIS 1
        ST
        LR DC,H
        ENDM

; ---------------------------------------------------------------------------
; DSPAN -- r4 pixels of value r3 on row r2 from column r1.  r4 >= 1.
;   Clobbers A, r4, r5, r9, H, DC0.  Leaf.
; ---------------------------------------------------------------------------
DSPAN:  PORTS
        COLP
        LR A,3                  ; a value that is not 0: the row needs clearing
        NI 0FFH
        BZ DSPNZ
        MARKNZ
DSPNZ:
DSPLP:  LM
        XS 3
        BZ DSPSK
        LI 0FFH
        ADC                     ; back to the byte just read
        LR A,3
        ST
        LR A,9
        OUTS 4
        LI 060H
        OUTS 0
        LI 050H
        OUTS 0                  ; the write lands here
        SETTLE
DSPSK:  DS 9
        DS 4
        BNZ DSPLP
        POP

; ---------------------------------------------------------------------------
; DFILL -- a rectangle: r1 = x, r2 = y, r3 = width, r4 = height, r0 = colour.
;   Width and height >= 1.  Clobbers A, r2-r9, H, DC0; keeps r0, r1.
;   Rows past 63 are cut off: they would run the shadow into DATA.
; ---------------------------------------------------------------------------
DFILL:  ENTER
        LR A,2
        CI 63
        JNC DFX
        LR A,4                  ; height <= 64, and y + height <= 64
        CI 64
        BC DF0
        LI 64
        LR 4,A
DF0:    LR A,2
        AS 4
        CI 64
        BC DF1
        LR A,2
        COM
        INC
        AI 64
        LR 4,A
DF1:    PI SPRTOUCH             ; (keeps r0-r8)
        LR A,3
        LR 6,A
        LR A,4
        LR 8,A
DFLP:   PI DCOLV                ; r3 = this row's value of the colour
        LR A,6
        LR 4,A
        PI DSPAN
        LR A,2
        INC
        LR 2,A
        DS 8
        BNZ DFLP
DFX:    LEAVE

; ---------------------------------------------------------------------------
; DDOTS -- a dotted line: every other pixel from column r1, r3 wide, on row
;   r2, in colour r0 (C_BLACK takes one off).  Clobbers A, r3-r9, H, DC0.
; ---------------------------------------------------------------------------
DDOTS:  ENTER
        LIS 1
        LR 4,A
        PI SPRTOUCH
        LR A,3
        INC
        SR 1
        LR 4,A                  ; dots
        NI 0FFH
        BZ DDOTX
        PI DCOLV
        PORTS
        COLP
        LR A,3
        NI 0FFH
        BZ DDTLP
        MARKNZ
DDTLP:  LM
        XS 3
        BZ DDTSK
        LI 0FFH
        ADC
        LR A,3
        ST
        LR A,9
        OUTS 4
        LI 060H
        OUTS 0
        LI 050H
        OUTS 0
        SETTLE
DDTSK:  LIS 1                   ; m_skip the gap
        ADC
        DS 9
        DS 9
        DS 4
        BNZ DDTLP
DDOTX:  LEAVE

; ---------------------------------------------------------------------------
; DBLIT -- a block of values from DC1, a byte a pixel (0-3; $FF leaves the
;   pixel alone): r1 = x, r2 = y, r3 = width, r4 = height.  DC1 is left after
;   the block.  Clobbers A, r2-r9, H, DC0.
; ---------------------------------------------------------------------------
DBLIT:  ENTER
        PI SPRTOUCH
        LR A,3
        LR 6,A
        LR A,4
        LR 8,A
DBLRW:  PORTS
        MARKNZ                  ; this row may hold anything now
        LR A,6
        LR 4,A
DBLLP:  XDC
        LM                      ; the value wanted
        XDC
        LR 3,A
        LM                      ; the value there
        XS 3
        BZ DBLSK
        LR A,3
        CI 3
        BNC DBLSK               ; $FF: transparent
        LI 0FFH
        ADC
        LR A,3
        ST
        SL 4
        SL 1
        SL 1
        COM
        OUTS 1
        LR A,9
        OUTS 4
        LI 060H
        OUTS 0
        LI 050H
        OUTS 0
        SETTLE
DBLSK:  DS 9
        DS 4
        BNZ DBLLP
        LR A,2
        INC
        LR 2,A
        DS 8
        BNZ DBLRW
        LEAVE

; ---------------------------------------------------------------------------
; DPAL -- rows r1..r2 take palette r3 (0-3).  Clobbers A, r0-r9, H, DC0.
; ---------------------------------------------------------------------------
DPAL:   ENTER
        LR A,2                  ; nothing if r1 > r2; never past row 63 (the
        CI 63                   ; shadow ends there: DATA follows)
        BC DPL0
        LI 63
        LR 2,A
DPL0:   LR A,1
        COM
        AS 2                    ; r2 + ~r1 carries: r2 > r1 ...
        BC DPL1
        LR A,1
        XS 2                    ; ... or equal
        JNZ DPLX
DPL1:   LR A,1
        LR 6,A                  ; row
        LR A,2
        LR 8,A                  ; last row
        LR A,3
        LR 0,A                  ; palette
DPLLP:  DCI ROWPAL
        LR A,6
        ADC
        LR A,0
        ST
        SL 1                    ; ROWKEY = (pal * 2 + parity) * 16
        LR 5,A
        LR A,6
        NI 1
        AS 5
        SL 4
        LR 5,A
        DCI ROWKEY
        LR A,6
        ADC
        LR A,5
        ST
        ; column 125: bit 1 = palette bit 0; column 126: bit 1 = palette bit 1
        LR A,6
        LR 2,A
        LI 125
        LR 1,A
        LR A,0
        NI 1
        SL 1
        LR 3,A
        LIS 1
        LR 4,A
        PI DSPAN
        LI 126
        LR 1,A
        LR A,0
        NI 2
        LR 3,A
        LIS 1
        LR 4,A
        PI DSPAN
        LR A,6                  ; next row, until past the last
        LR 5,A
        INC
        LR 6,A
        LR A,8
        XS 5
        BNZ DPLLP
DPLX:   LEAVE

; ---------------------------------------------------------------------------
; DCLS -- the visible area cleared to each row's ground (value 0), the
;   sprites off first: they are not in the shadow, so the diff would miss
;   them. Only rows with something on them cost anything.
;   Clobbers A, r0-r9, H, DC0.
; ---------------------------------------------------------------------------
DCLS:   ENTER
        PI SPROFF
        LI VY0
        LR 2,A
DCLLP:  DCI DROWNZ
        LR A,2
        ADC
        LM
        NI 0FFH
        BZ DCLSK
        LI 0FFH
        ADC
        CLR
        ST                      ; DSPAN writes only zeros: clear from now
        LI VX0
        LR 1,A
        CLR
        LR 3,A
        LI VX1-VX0+1
        LR 4,A
        PI DSPAN
DCLSK:  LR A,2
        INC
        LR 2,A
        CI VY1
        BC DCLLP                ; while row <= VY1
        LEAVE

; ---------------------------------------------------------------------------
; DINIT -- once at power-up: VRAM is unknown, so the BIOS clears all of it
;   (palette 0 on every row), and the shadow and the tables are made to
;   match.  Clobbers A, r0-r9, H, DC0, DC1.
; ---------------------------------------------------------------------------
DINIT:  ENTER
        CLR
        LR 3,A
        PI BCLRSCR
        DCI SHADOW              ; 8K of zeros: 32 x 256
        LI 32
        LR 1,A
        CLR
        LR 0,A
DINL:   CLR
        ST
        DS 0
        BNZ DINL
        DS 1
        BNZ DINL
        DCI ROWKEY              ; ROWKEY, ROWPAL: palette 0
        LI 64
        LR 0,A
DINK:   LR A,0
        NI 1
        SL 4
        ST
        DS 0
        BNZ DINK
        DCI ROWPAL
        LI 64
        LR 0,A
DINP:   CLR
        ST
        DS 0
        BNZ DINP
        DCI DROWNZ
        LI 64
        LR 0,A
DINZ:   CLR
        ST
        DS 0
        BNZ DINZ
        LEAVE

; ---------------------------------------------------------------------------
; COLTAB: the value of each game colour, by (palette * 2 + row parity) * 16.
; On a colour row (palette 1-3): black is the ground, the cool colours and
; white are blue, the warm ones red, the greens green; player 3 is red on
; even rows and green on odd (gfx.h's C_P3 "stripes").
; ---------------------------------------------------------------------------
;          blk wht blu pur grn org red yel gry cyn brn tan lbl dgr pnk P3
CTPAL0  MACRO
        DB  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1
        ENDM
CTEVEN  MACRO
        DB  0,  1,  1,  2,  3,  2,  2,  3,  1,  1,  2,  2,  1,  3,  2,  2
        ENDM
CTODD   MACRO
        DB  0,  1,  1,  2,  3,  2,  2,  3,  1,  1,  2,  2,  1,  3,  2,  3
        ENDM
COLTAB: CTPAL0
        CTPAL0
        CTEVEN
        CTODD
        CTEVEN
        CTODD
        CTEVEN
        CTODD
