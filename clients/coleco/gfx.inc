; gfx.inc -- the map's plots, the town, small pictures and dotted lines
; (gfx.c gfx_plot, gfx_town, gfx_pic, gfx_hdots), from the Adam's art
; (gen/gfx.inc). Fixed.

; plot_parts (gfxdata.h PP_*): 96 bytes each, 32 lines of 3 cells
PP_BORDER EQU   0
PP_ICON EQU     1               ; + ICON_* - 1
PP_MOUNT EQU    7               ; + mountains - 1
PP_STORE EQU    10
PP_RIVER EQU    11

ICON_NONE EQU   0
ICON_HOUSE EQU  1
ICON_FOOD EQU   2               ; + GOOD_*
ICON_MULE EQU   6

PIC_STORE EQU   0
PIC_FLAG EQU    1
PIC_FIRE0 EQU   2
PIC_FIRE1 EQU   3

TOWN_CX EQU     6               ; the town: cells 6..24 of cell rows 2..19
TOWN_CW EQU     19
TOWN_CY EQU     2
TOWN_CH EQU     18

; PPART: HL = plot part A. Preserves BC,DE.
PPART:  PUSH    DE
        LD      L,A
        LD      H,0
        ADD     HL,HL           ; * 96 = * 32 * 3
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        LD      D,H
        LD      E,L
        ADD     HL,HL
        ADD     HL,DE
        LD      DE,PLPARTS
        ADD     HL,DE
        POP     DE
        RET

; ---- a plot ---------------------------------------------------------------
; PLOT: plot A as PLK_* say (gfx.h gfx_plot): three cells by four, made
; whole. Each cell line shows the first of the owner's (border, icon,
; dots), white or black (mountains, the store) and the river that it has,
; over the plains or over the next of them, whichever has more pixels there
; (plotcell.asm). Clobbers all.
PL_OWN  EQU     GTMP            ; the layers (ZERO96: none)
PL_ICN  EQU     GTMP+2
PL_WH   EQU     GTMP+4
PL_RV   EQU     GTMP+6
PL_C0   EQU     GTMP+8          ; the owner's colour << 4
PL_C1   EQU     GTMP+9          ; white or black
PL_CX0  EQU     GTMP+10
PL_CY0  EQU     GTMP+11
PL_CX   EQU     GTMP+12
PL_CY   EQU     GTMP+13
PL_LN   EQU     GTMP+14         ; the plot's line (0..31)
PL_DOTS EQU     GTMP+15
PLOT:   LD      C,A             ; the cells: 2 + 3 (p % 9), 2 + 4 (p / 9)
        LD      B,0
PL1:    LD      A,C
        SUB     9
        JR      C,PL2
        LD      C,A
        INC     B
        JR      PL1
PL2:    LD      A,C
        ADD     A,A
        ADD     A,C
        ADD     A,2
        LD      (PL_CX0),A
        LD      A,B
        ADD     A,A
        ADD     A,A
        ADD     A,2
        LD      (PL_CY0),A
        LD      HL,ZERO96
        LD      (PL_OWN),HL
        LD      (PL_ICN),HL
        LD      (PL_WH),HL
        LD      (PL_RV),HL
        XOR     A
        LD      (PL_DOTS),A
        LD      A,(PLK_ST)
        OR      A
        JR      Z,PL3
        LD      A,PP_STORE
        CALL    PPART
        LD      (PL_WH),HL
        JR      PL6
PL3:    LD      A,(PLK_OWN)
        OR      A
        JR      Z,PL4
        LD      A,PP_BORDER
        CALL    PPART
        LD      (PL_OWN),HL
        LD      A,(PLK_DOTS)
        CP      9
        JR      C,PL3A
        LD      A,8
PL3A:   LD      (PL_DOTS),A
        LD      A,(PLK_ICON)
        OR      A
        JR      Z,PL4
        ADD     A,PP_ICON-1
        CALL    PPART
        LD      (PL_ICN),HL
PL4:    LD      A,(PLK_MNT)
        OR      A
        JR      Z,PL5
        ADD     A,PP_MOUNT-1
        CALL    PPART
        LD      (PL_WH),HL
PL5:    LD      A,(PLK_RIV)
        OR      A
        JR      Z,PL6
        LD      A,PP_RIVER
        CALL    PPART
        LD      (PL_RV),HL
PL6:    LD      B,T_WHITE       ; mountains are black, the store white
        LD      A,(PLK_ST)
        OR      A
        JR      NZ,PL7
        LD      A,(PLK_MNT)
        OR      A
        JR      Z,PL7
        LD      B,T_BLACK
PL7:    LD      A,B
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      (PL_C1),A
        LD      A,(PLK_COL)
        AND     0FH
        LD      E,A
        LD      D,0
        LD      HL,TMSOF
        ADD     HL,DE
        LD      A,(HL)
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      (PL_C0),A
        XOR     A               ; (Z: PLSAME goes on)
        LD      HL,ZERO96       ; the bare plains: every cell the same
        LD      DE,(PL_OWN)
        CALL    PLSAME
        LD      DE,(PL_WH)
        CALL    PLSAME
        LD      DE,(PL_RV)
        CALL    PLSAME
        JR      NZ,PL8
        LD      A,(BGC)
        LD      B,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        OR      B
        LD      HL,CELLP
        LD      B,8
PLE0:   LD      (HL),0
        INC     HL
        DJNZ    PLE0
        LD      B,8
PLE1:  LD      (HL),A
        INC     HL
        DJNZ    PLE1
        LD      BC,0400H        ; C = row, B = rows left
PLE2:  LD      A,(PL_CY0)
        ADD     A,C
        LD      D,A
        LD      A,(PL_CX0)
        LD      E,A
        PUSH    BC
        LD      B,3
PLE3:  PUSH    BC
        PUSH    DE
        CALL    CELLAT
        CALL    CELLST
        POP     DE
        POP     BC
        INC     E
        DJNZ    PLE3
        POP     BC
        INC     C
        DJNZ    PLE2
        RET
PL8:    XOR     A
        LD      (PL_CY),A
PLCY:   XOR     A
        LD      (PL_CX),A
PLCX:   LD      A,(PL_CY)       ; this cell's first line
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      (PL_LN),A
        LD      IY,CELLP
PLR:    CALL    PLBYTE          ; the cell's line: CELLP/CELLC
        INC     IY
        LD      HL,PL_LN
        INC     (HL)
        LD      A,(HL)
        AND     7
        JR      NZ,PLR
        LD      A,(PL_CX0)
        LD      HL,PL_CX
        ADD     A,(HL)
        LD      E,A
        LD      A,(PL_CY0)
        LD      HL,PL_CY
        ADD     A,(HL)
        LD      D,A
        CALL    CELLAT
        CALL    CELLST
        LD      HL,PL_CX
        INC     (HL)
        LD      A,(HL)
        CP      3
        JR      C,PLCX
        LD      HL,PL_CY
        INC     (HL)
        LD      A,(HL)
        CP      4
        JR      C,PLCY
        RET

; PLSAME: Z if DE is HL (and NZ stays NZ: call it in a row). Keeps HL.
PLSAME: RET     NZ
        LD      A,D
        CP      H
        RET     NZ
        LD      A,E
        CP      L
        RET

; PLBYTE: line PL_LN of cell PL_CX into (IY+0) pattern, (IY+8) colour.
; (plotcell.asm's byte: the counts are POPC lookups.)
PLBYTE: LD      A,(PL_LN)       ; DE = line * 3 + cx
        LD      E,A
        ADD     A,A
        ADD     A,E
        LD      HL,PL_CX
        ADD     A,(HL)
        LD      E,A
        LD      D,0
        LD      HL,(PL_OWN)
        ADD     HL,DE
        LD      C,(HL)
        LD      HL,(PL_ICN)
        ADD     HL,DE
        LD      A,(HL)
        OR      C
        LD      C,A
        LD      A,(PL_DOTS)
        OR      A
        CALL    NZ,PLDOT        ; C |= the dots here
        LD      HL,(PL_WH)
        ADD     HL,DE
        LD      A,C             ; white & ~owner
        CPL
        AND     (HL)
        LD      B,A
        LD      HL,(PL_RV)
        ADD     HL,DE
        LD      A,C             ; river & ~(owner | white)
        OR      B
        CPL
        AND     (HL)
        LD      E,A             ; C owner, B white, E river
        LD      H,POPC/256
        LD      A,C
        OR      A
        JR      Z,PLY3
        LD      (IY+0),C        ; the owner's pixels, over the one of the
        OR      B               ; others with the most (the plains first)
        OR      E
        CPL
        LD      L,A
        LD      D,(HL)          ; D = the plains' count
        LD      L,B
        LD      A,(HL)
        LD      B,0             ; B = the background: 0 plains, 1 white, 2 river
        CP      D
        JR      C,PLY1
        JR      Z,PLY1
        LD      D,A
        INC     B
PLY1:   LD      L,E
        LD      A,(HL)
        CP      D
        JR      C,PLY2
        JR      Z,PLY2
        LD      B,2
PLY2:   DEC     B
        JP      M,PLY2P
        LD      A,T_RIVER
        JR      NZ,PLY2B
        LD      A,(PL_C1)
        RRCA
        RRCA
        RRCA
        RRCA
        JR      PLY2B
PLY2P:  LD      A,(BGC)
PLY2B:  LD      H,A
        LD      A,(PL_C0)
        OR      H
        LD      (IY+8),A
        RET
PLY3:   LD      A,B
        OR      A
        JR      Z,PLY5
        LD      (IY+0),B        ; white or black, over the plains or the river
        OR      E
        CPL
        LD      L,A
        LD      D,(HL)
        LD      L,E
        LD      A,(HL)
        CP      D
        LD      A,(BGC)
        JR      C,PLY4
        JR      Z,PLY4
        LD      A,T_RIVER
PLY4:   LD      H,A
        LD      A,(PL_C1)
        OR      H
        LD      (IY+8),A
        RET
PLY5:   LD      A,E
        OR      A
        JR      Z,PLY6
        LD      (IY+0),E        ; the river
        LD      A,(BGC)
        OR      T_RIVER*16
        LD      (IY+8),A
        RET
PLY6:   LD      (IY+0),0        ; the plains
        LD      A,(BGC)
        LD      B,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        OR      B
        LD      (IY+8),A
        RET

; PLDOT: C |= the production dots (the first PL_DOTS of 8, each two pixels
; on two lines) on line PL_LN of cell PL_CX. Preserves DE.
PLDOT:  LD      A,(PL_DOTS)
        OR      A
        RET     Z
        PUSH    DE
        LD      B,A
        LD      HL,PLDOTS
PLD1:   LD      A,(PL_LN)       ; its line, or the one under it
        SUB     (HL)
        CP      2
        INC     HL
        JR      NC,PLD3
        LD      A,(HL)          ; its cell
        RRCA
        RRCA
        RRCA
        AND     1FH
        LD      D,A
        LD      A,(PL_CX)
        CP      D
        JR      NZ,PLD3
        LD      A,(HL)
        AND     7
        LD      E,A
        LD      A,0C0H
        JR      Z,PLD2A
PLD2:   SRL     A
        DEC     E
        JR      NZ,PLD2
PLD2A:  OR      C
        LD      C,A
PLD3:   INC     HL
        DJNZ    PLD1
        POP     DE
        RET

ZERO96: DS      96

; ---- the town -------------------------------------------------------------------
; TOWN: the town's picture (gfx.h gfx_town): each cell row's 152 pattern
; bytes, then its 152 colour bytes, unpacked straight into VRAM. The
; packing: n < 128, n + 1 bytes follow; n >= 128, the next byte n - 125
; times. Clobbers all.
RL_SRC  EQU     GTMP
RL_LIT  EQU     GTMP+2
RL_RUN  EQU     GTMP+3
RL_V    EQU     GTMP+4
TOWN:   LD      HL,TOWNRLE
        LD      (RL_SRC),HL
        XOR     A
        LD      (RL_LIT),A
        LD      (RL_RUN),A
        LD      D,TOWN_CY
TWN1:   LD      H,D
        LD      L,TOWN_CX*8
        PUSH    HL
        CALL    TWNROW
        POP     HL
        SET     5,H
        CALL    TWNROW
        INC     D
        LD      A,D
        CP      TOWN_CY+TOWN_CH
        JR      C,TWN1
        RET

; TWNROW: 152 bytes to VRAM HL. Preserves D.
TWNROW: CALL    VLOCK
        CALL    VWA
        LD      B,TOWN_CW*8
TWR1:   CALL    RLGET
        OUT     (VDPD),A
        DJNZ    TWR1
        JP      VFREE

; RLGET: A = the town's next byte. Preserves BC,DE.
RLGET:  LD      HL,RL_RUN
        LD      A,(HL)
        OR      A
        JR      NZ,RLG3
        LD      A,(RL_LIT)
        OR      A
        JR      NZ,RLG2
        LD      HL,(RL_SRC)     ; the next code
        LD      A,(HL)
        INC     HL
        CP      80H
        JR      NC,RLG1
        INC     A
        LD      (RL_LIT),A
        LD      (RL_SRC),HL
        JR      RLG2
RLG1:   SUB     7DH
        LD      (RL_RUN),A
        LD      A,(HL)
        INC     HL
        LD      (RL_V),A
        LD      (RL_SRC),HL
        LD      HL,RL_RUN
RLG3:   DEC     (HL)
        LD      A,(RL_V)
        RET
RLG2:   LD      HL,RL_LIT
        DEC     (HL)
        LD      HL,(RL_SRC)
        LD      A,(HL)
        INC     HL
        LD      (RL_SRC),HL
        RET

; ---- small pictures -----------------------------------------------------------
; PIC: picture A (PIC_*) at x B (colour clocks), line C, colour E (C_*),
; opaque on the background: 8 x 8 clocks, two lines a row, 12 pixels wide
; (gfx.h gfx_pic). Clobbers all.
PC_ROWS EQU     GTMP            ; the picture's rows
PC_PX   EQU     GTMP+2          ; its first pixel
PC_Y    EQU     GTMP+3          ; the line
PC_L    EQU     GTMP+4          ; lines done
PC_COL  EQU     GTMP+5          ; TMS colour
PC_BITS EQU     GTMP+6          ; 3 bytes: this line's pixels, cell by cell
PC_MASK EQU     GTMP+9          ; 3 bytes: the cells' part of the picture
PC_K    EQU     GTMP+12
PIC:    ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      L,A
        LD      H,0
        PUSH    DE
        LD      DE,PICS
        ADD     HL,DE
        POP     DE
        LD      (PC_ROWS),HL
        LD      A,E
        CALL    TCOL
        LD      (PC_COL),A
        LD      A,C
        LD      (PC_Y),A
        LD      A,B
        CALL    PX
        LD      A,H             ; past the right edge: nothing
        OR      A
        RET     NZ
        LD      A,L
        LD      (PC_PX),A
        XOR     A
        LD      (PC_L),A
PCL1:   LD      A,(PC_Y)
        CP      192
        RET     NC
        LD      A,(PC_L)        ; its row's 12 pixels, bit 15 left
        SRL     A
        LD      E,A
        LD      D,0
        LD      HL,(PC_ROWS)
        ADD     HL,DE
        LD      L,(HL)
        LD      H,0
        ADD     HL,HL
        LD      DE,WIDE12
        ADD     HL,DE
        LD      E,(HL)
        INC     HL
        LD      D,(HL)          ; DE = 12 bits
        LD      B,4
PCL2:   SLA     E
        RL      D
        DJNZ    PCL2
        LD      HL,0FFF0H       ; HL = its 12 pixels
        XOR     A
        LD      (PC_MASK+2),A
        LD      A,(PC_PX)       ; shifted to the pixel: 24 bits, D E C
        AND     7
        LD      B,A
        LD      C,0
        JR      Z,PCL4
PCL3:   SRL     D
        RR      E
        RR      C
        SRL     H
        RR      L
        LD      A,(PC_MASK+2)
        RRA
        LD      (PC_MASK+2),A
        DJNZ    PCL3
PCL4:   LD      A,D
        LD      (PC_BITS),A
        LD      A,E
        LD      (PC_BITS+1),A
        LD      A,C
        LD      (PC_BITS+2),A
        LD      A,H
        LD      (PC_MASK),A
        LD      A,L
        LD      (PC_MASK+1),A
        XOR     A
        LD      (PC_K),A
PCL5:   LD      HL,PC_MASK      ; each cell the picture covers on the line
        LD      A,(PC_K)
        LD      E,A
        LD      D,0
        ADD     HL,DE
        LD      A,(HL)
        OR      A
        JR      Z,PCL6
        LD      A,(PC_PX)
        RRCA
        RRCA
        RRCA
        AND     1FH
        ADD     A,E
        CP      TCOLS
        JR      NC,PCL6
        LD      E,A
        LD      A,(PC_Y)
        RRCA
        RRCA
        RRCA
        AND     1FH
        LD      D,A
        CALL    CELLAT
        LD      A,(PC_Y)
        AND     7
        LD      B,A
        CALL    ROWLD
        LD      HL,PC_MASK
        LD      A,(PC_K)
        LD      E,A
        LD      D,0
        ADD     HL,DE
        LD      C,(HL)          ; first the background under it
        LD      A,(BGC)
        LD      D,A
        PUSH    HL
        CALL    PAINT
        POP     HL
        LD      DE,PC_BITS-PC_MASK
        ADD     HL,DE
        LD      A,(HL)          ; then its pixels
        AND     C
        LD      C,A
        LD      A,(PC_COL)
        LD      D,A
        CALL    PAINT
        CALL    ROWST
PCL6:   LD      HL,PC_K
        INC     (HL)
        LD      A,(HL)
        CP      3
        JR      C,PCL5
        LD      HL,PC_Y
        INC     (HL)
        LD      HL,PC_L
        INC     (HL)
        LD      A,(HL)
        CP      16
        JP      C,PCL1
        RET

; ---- dotted lines -------------------------------------------------------------------
; HDOTS: a dotted line at x B (clocks), line C, w D (clocks), colour A
; (C_*): a dot every other clock, one or two pixels as the 3/2 falls
; (gfx.h gfx_hdots). Clobbers all.
HD_X    EQU     GTMP
HD_W    EQU     GTMP+1
HD_C    EQU     GTMP+2          ; clocks done
HD_COL  EQU     GTMP+3
HD_END  EQU     GTMP+4          ; PX(x + w), 2 bytes
HD_CELL EQU     GTMP+6          ; the cell being gathered (0FFH: none)
HD_M    EQU     GTMP+7          ; its mask
HD_Y    EQU     GTMP+8
HDOTS:  CALL    TCOL
        LD      (HD_COL),A
        LD      A,C
        CP      192
        RET     NC
        LD      (HD_Y),A
        LD      A,B
        LD      (HD_X),A
        LD      A,D
        LD      (HD_W),A
        LD      A,B
        ADD     A,D
        CALL    PX
        LD      (HD_END),HL
        XOR     A
        LD      (HD_C),A
        LD      (HD_M),A
        DEC     A
        LD      (HD_CELL),A
HDL1:   LD      A,(HD_C)
        LD      HL,HD_W
        CP      (HL)
        JR      NC,HDL9
        LD      HL,HD_X
        ADD     A,(HL)
        PUSH    AF
        INC     A
        CALL    PX
        EX      DE,HL           ; DE = PX(x + c + 1)
        POP     AF
        CALL    PX              ; HL = PX(x + c)
        PUSH    HL
        CALL    HDPIX
        POP     HL
        EX      DE,HL
        OR      A
        SBC     HL,DE
        LD      A,L
        CP      2
        JR      C,HDL2
        EX      DE,HL
        INC     HL              ; the dot's second pixel
        CALL    HDPIX
HDL2:   LD      HL,HD_C
        INC     (HL)
        INC     (HL)
        JR      HDL1
HDL9:   JP      HDFLUSH

; HDPIX: pixel HL into the gathering (if short of the end). Preserves DE.
HDPIX:  PUSH    DE
        LD      DE,(HD_END)
        LD      A,H
        OR      A
        JR      NZ,HDP9         ; past the screen
        PUSH    HL
        OR      A
        SBC     HL,DE
        POP     HL
        JR      NC,HDP9         ; past the end
        LD      A,L
        RRCA
        RRCA
        RRCA
        AND     1FH
        LD      E,A
        LD      A,(HD_CELL)
        CP      E
        JR      Z,HDP1
        PUSH    HL
        PUSH    DE
        CALL    HDFLUSH
        POP     DE
        POP     HL
        LD      A,E
        LD      (HD_CELL),A
HDP1:   LD      A,L
        AND     7
        LD      B,A
        LD      A,80H
        JR      Z,HDP3
HDP2:   SRL     A
        DJNZ    HDP2
HDP3:   LD      HL,HD_M
        OR      (HL)
        LD      (HL),A
HDP9:   POP     DE
        RET

; HDFLUSH: the gathered dots onto their cell's line.
HDFLUSH: LD     A,(HD_M)
        OR      A
        RET     Z
        LD      A,(HD_CELL)
        LD      E,A
        LD      A,(HD_Y)
        RRCA
        RRCA
        RRCA
        AND     1FH
        LD      D,A
        CALL    CELLAT
        LD      A,(HD_Y)
        AND     7
        LD      B,A
        CALL    ROWLD
        LD      A,(HD_M)
        LD      C,A
        LD      A,(HD_COL)
        LD      D,A
        CALL    PAINT
        CALL    ROWST
        XOR     A
        LD      (HD_M),A
        RET
