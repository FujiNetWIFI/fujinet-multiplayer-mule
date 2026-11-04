; vdp.inc -- the screen: Graphics II as a 256 x 192 bitmap (the Adam's,
; clients/mekkogx/src/tms9918/gfx.c). Fixed.
;
; The name table counts 0..255 in each third, so cell (cx, cy) owns 8
; pattern bytes at cy * 256 + cx * 8 and 8 colour bytes 2000H further on;
; each 8-pixel row of a cell has two colours. Drawing reads a cell's 16
; bytes, paints them (PAINT: a colour the row hasn't got takes the place of
; the one with fewer pixels left) and writes them back.
;
; x is the Atari's colour clocks scaled 3/2, 4 pixels in (PX): 160 clocks
; are pixels 4..243, and a plot (16 clocks from 8 + 16k) is exactly the
; three cells from pixel 16 + 24k. Lines are the Atari's own.
;
; Every address and data burst is inside VLOCK/VFREE (core.inc). Data
; accesses are 29 cycles apart at the least: the VDP needs 8 us between
; them while it draws Graphics II.

; ---- colours ---------------------------------------------------------------
; The game's colours (gfx.h C_*) and the TMS9918's nearest (gfx.c tms_of).
C_BLACK EQU     0
C_WHITE EQU     1
C_BLUE  EQU     2               ; player 0
C_PURPLE EQU    3               ; player 1
C_GREEN EQU     4               ; player 2
C_ORANGE EQU    5
C_RED   EQU     6
C_YELLOW EQU    7
C_GREY  EQU     8
C_CYAN  EQU     9
C_BROWN EQU     10
C_TAN   EQU     11
C_LTBLUE EQU    12
C_DKGREEN EQU   13
C_PINK  EQU     14
C_P3    EQU     15              ; player 3, orange

TMSOF:  DB      T_BLACK,T_WHITE,T_LBLUE,T_MAG,T_MGRN,T_LRED,T_MRED,T_LYEL
        DB      T_GREY,T_CYAN,T_DRED,T_DYEL,T_LBLUE,T_DGRN,T_LRED,T_LRED

; TCOL: A = C_* -> its TMS colour (black is the background, BGC).
; Preserves everything else.
TCOL:   AND     0FH
        JR      NZ,TCL1
        LD      A,(BGC)
        RET
TCL1:   PUSH    HL
        ADD     A,TMSOF AND 0FFH
        LD      L,A
        LD      A,TMSOF/256
        ADC     A,0
        LD      H,A
        LD      A,(HL)
        POP     HL
        RET

; ---- addresses and bursts --------------------------------------------------
; VWA / VRA: the write / read address HL. Clobbers A. (Inside VLOCK.)
VWA:    LD      A,L
        OUT     (VDPC),A
        LD      A,H
        OR      40H
        OUT     (VDPC),A
        RET
VRA:    LD      A,L
        OUT     (VDPC),A
        LD      A,H
        AND     3FH
        OUT     (VDPC),A
        RET

; VOUT: B bytes (0: 256) from HL. Clobbers A,B,HL.
VOUT:   LD      A,(HL)
        OUT     (VDPD),A
        INC     HL
        DJNZ    VOUT
        RET

; VREP: A, B times (0: 256). Clobbers B.
VREP:   OUT     (VDPD),A
        NOP
        NOP
        DJNZ    VREP
        RET

; VWRITE: B bytes (0: 256) from HL to VRAM DE. Clobbers A,B,HL.
VWRITE: CALL    VLOCK
        EX      DE,HL
        CALL    VWA
        EX      DE,HL
        CALL    VOUT
        JP      VFREE

; VFILL: DE bytes of A at VRAM HL, a page at a time (the frame gets a look
; in between). Clobbers A,BC,DE,HL.
VFILL:  LD      C,A
VFL0:   LD      A,D
        OR      E
        RET     Z
        LD      B,E
        LD      A,D
        OR      A
        JR      Z,VFL1
        LD      B,0             ; a whole page
        DEC     D
        JR      VFL2
VFL1:   LD      E,0             ; the last E
VFL2:   CALL    VLOCK
        CALL    VWA
        LD      A,C
        CALL    VREP
        CALL    VFREE
        INC     H
        JR      VFL0

; ---- the set-up --------------------------------------------------------------
; VINIT: Graphics II, 16K, 16 x 16 sprites, the sequential name table, a
; clear screen; the interrupt on. (Not OS7's MODE_1: it leaves the Graphics
; II tables masked.)
VINIT:  LD      BC,0182H        ; R1: 16K, blank, no interrupt, 16 x 16
        CALL    WRITREG
        LD      BC,0002H        ; R0: Graphics II
        CALL    WRITREG
        LD      BC,0206H        ; R2: names at 1800H
        CALL    WRITREG
        LD      BC,03FFH        ; R3: colours at 2000H, all three thirds
        CALL    WRITREG
        LD      BC,0403H        ; R4: patterns at 0000H, all three thirds
        CALL    WRITREG
        LD      BC,0536H        ; R5: sprite attributes at 1B00H
        CALL    WRITREG
        LD      BC,0607H        ; R6: sprite patterns at 3800H
        CALL    WRITREG
        LD      BC,0701H        ; R7: a black border
        CALL    WRITREG
        CALL    VLOCK           ; the names: 0..255, three times
        LD      HL,VR_NAME
        CALL    VWA
        LD      C,3
VIN1:   XOR     A
VIN2:   OUT     (VDPD),A
        INC     A
        NOP
        JR      NZ,VIN2
        DEC     C
        JR      NZ,VIN1
        LD      HL,VR_SAT       ; no sprites
        CALL    VWA
        LD      A,0D0H
        OUT     (VDPD),A
        CALL    VFREE
        LD      A,T_BLACK
        LD      (BGC),A
        CALL    VCLEAR
        LD      BC,01E2H        ; R1: on, the frame interrupt on
        JP      WRITREG

; VCLEAR: every cell blank in white on the background.
VCLEAR: LD      HL,VR_PAT
        LD      DE,1800H
        XOR     A
        CALL    VFILL
        LD      A,(BGC)
        OR      T_WHITE*16
        LD      HL,VR_COL
        LD      DE,1800H
        JP      VFILL

; CLRSCR: the whole screen blank on black, the sprites off (gfx_clear).
CLRSCR: CALL    SPROFF
        CALL    SPRFLUSH
        LD      A,T_BLACK
        LD      (BGC),A
        JR      VCLEAR

; MAPFRAME: the same on the map's plains (gfx_map_frame).
MAPFRAME: CALL  SPROFF
        CALL    SPRFLUSH
        LD      A,T_PLAINS
        LD      (BGC),A
        JR      VCLEAR

; ---- cells ------------------------------------------------------------------
; CELLAT: cell E (column), D (row) -> CELLA. Preserves BC,DE.
CELLAT: LD      A,E
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      L,A
        LD      H,D
        LD      (CELLA),HL
        RET

; CELLLD / CELLST: the cell at CELLA into / out of CELLP and CELLC.
; Clobber A,B,HL.
CELLLD: CALL    VLOCK
        LD      HL,(CELLA)
        CALL    VRA
        LD      HL,CELLP
        CALL    VIN8
        LD      HL,(CELLA)
        SET     5,H             ; + 2000H
        CALL    VRA
        LD      HL,CELLC
        CALL    VIN8
        JP      VFREE
VIN8:   LD      B,8
VIN81:  IN      A,(VDPD)
        LD      (HL),A
        INC     HL
        DJNZ    VIN81
        RET

CELLST: CALL    VLOCK
        LD      HL,(CELLA)
        CALL    VWA
        LD      HL,CELLP
        LD      B,8
        CALL    VOUT
        LD      HL,(CELLA)
        SET     5,H
        CALL    VWA
        LD      HL,CELLC
        LD      B,8
        CALL    VOUT
        JP      VFREE

; ROWLD / ROWST: line B of the cell at CELLA alone (its pattern byte into
; CELLP+B, its colour byte into CELLC+B). Clobber A,HL; keep BC,DE.
ROWLD:  CALL    VLOCK
        CALL    ROWADR
        CALL    VRA
        PUSH    HL
        CALL    ROWBUF
        IN      A,(VDPD)
        LD      (HL),A
        POP     HL
        SET     5,H
        CALL    VRA
        CALL    ROWBUF
        LD      A,L
        ADD     A,8
        LD      L,A
        JR      NC,RWL1
        INC     H
RWL1:   IN      A,(VDPD)
        LD      (HL),A
        JP      VFREE

ROWST:  CALL    VLOCK
        CALL    ROWADR
        CALL    VWA
        PUSH    HL
        CALL    ROWBUF
        LD      A,(HL)
        OUT     (VDPD),A
        POP     HL
        SET     5,H
        CALL    VWA
        CALL    ROWBUF
        LD      A,L
        ADD     A,8
        LD      L,A
        JR      NC,RWS1
        INC     H
RWS1:   LD      A,(HL)
        OUT     (VDPD),A
        JP      VFREE

ROWADR: LD      HL,(CELLA)      ; HL = CELLA + B
        LD      A,L
        ADD     A,B
        LD      L,A
        RET
ROWBUF: LD      HL,CELLP        ; HL = CELLP + B
        LD      A,L
        ADD     A,B
        LD      L,A
        RET     NC
        INC     H
        RET

; POP8: A = the pixels set in A. Preserves everything else.
POP8:   PUSH    HL
        LD      H,POPC/256
        LD      L,A
        LD      A,(HL)
        POP     HL
        RET

; PAINT: line B of the cell: the pixels in mask C become TMS colour D
; (gfx.c paint). Clobbers A,E,HL,IX; keeps BC,D.
PAINT:  LD      A,C
        OR      A
        RET     Z
        LD      IX,CELLP
        PUSH    BC
        LD      C,B
        LD      B,0
        ADD     IX,BC
        POP     BC
        LD      A,C
        INC     A
        JR      NZ,PN1
        LD      (IX+0),0        ; all of it: one colour
        LD      A,D
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        OR      D
        LD      (IX+8),A
        RET
PN1:    PUSH    BC
        LD      A,(IX+8)
        LD      E,A
        AND     0FH
        LD      B,A             ; B = the background
        LD      A,E
        RRCA
        RRCA
        RRCA
        RRCA
        AND     0FH
        LD      E,A             ; E = the foreground
        LD      H,(IX+0)        ; H = the pixels
        CP      B
        JR      NZ,PN2
        LD      H,0             ; one colour: all of it background
        JR      PN3
PN2:    LD      A,D             ; the foreground's colour: set them
        CP      E
        JR      NZ,PN3
        LD      A,H
        OR      C
        JR      PNP
PN3:    LD      A,D             ; the background's: clear them
        CP      B
        JR      NZ,PN4
        LD      A,C
        CPL
        AND     H
        JR      PNP
PN4:    LD      A,C             ; a new colour: it replaces the one with
        CPL                     ; fewer pixels left
        LD      L,A
        AND     H
        CALL    POP8
        EX      AF,AF'
        LD      A,H
        CPL
        AND     L
        CALL    POP8
        LD      L,A
        EX      AF,AF'
        CP      L
        JR      Z,PN5
        JR      NC,PN6
PN5:    LD      A,D             ; the foreground becomes D
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        OR      B
        LD      (IX+8),A
        LD      A,H
        OR      C
        JR      PNP
PN6:    LD      A,E             ; the background becomes D
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        OR      D
        LD      (IX+8),A
        LD      A,C
        CPL
        AND     H
PNP:    LD      (IX+0),A
        POP     BC
        RET

; ---- rectangles ----------------------------------------------------------------
; PX: HL = pixel of colour clock A (A + A/2 + 4). Preserves BC,DE.
PX:     LD      L,A
        LD      H,0
        SRL     A
        ADD     A,4
        ADD     A,L
        LD      L,A
        RET     NC
        INC     H
        RET

; FILL: x B, y C, w D (colour clocks), h E (lines), colour A (C_*): gfx.h
; gfx_fill. Clobbers all.
FILL:   CALL    TCOL
        LD      (FL_COL),A
        LD      A,C
        LD      (FL_Y),A
        LD      A,E
        LD      (FL_H),A
        LD      A,B
        ADD     A,D
        PUSH    BC
        CALL    PX
        POP     BC
        EX      DE,HL           ; DE = px1
        LD      A,B
        CALL    PX              ; HL = px0
        ; fall into FILLPX

; FILLPX: pixels HL .. DE - 1 of lines FL_Y .. FL_Y + FL_H - 1, TMS colour
; FL_COL (gfx.c op without a picture). Clobbers all.
FL_COL  EQU     GTMP
FL_Y    EQU     GTMP+1
FL_H    EQU     GTMP+2
FL_YE   EQU     GTMP+3
FL_CX0  EQU     GTMP+4
FL_CX1  EQU     GTMP+5
FL_LM   EQU     GTMP+6
FL_RM   EQU     GTMP+7
FL_CY   EQU     GTMP+8
FL_CY1  EQU     GTMP+9
FL_R0   EQU     GTMP+10
FL_R1   EQU     GTMP+11
FL_CX   EQU     GTMP+12
FL_M    EQU     GTMP+13
FILLPX: LD      A,D             ; px1 at most 256
        OR      A
        JR      Z,FP1
        LD      DE,256
FP1:    LD      A,(FL_Y)
        CP      192
        RET     NC
        LD      A,(FL_H)
        OR      A
        RET     Z
        PUSH    HL
        EX      DE,HL
        POP     DE              ; HL = px1, DE = px0
        OR      A
        SBC     HL,DE
        RET     Z
        RET     C
        ADD     HL,DE
        DEC     HL              ; HL = px1 - 1 (0..255)
        LD      A,L
        RRCA
        RRCA
        RRCA
        AND     1FH
        LD      (FL_CX1),A
        LD      A,L
        AND     7
        LD      B,A
        LD      A,7
        SUB     B
        LD      B,A
        LD      A,0FFH          ; rm = FF << (7 - (px1-1)&7)
        JR      Z,FP3
FP2:    ADD     A,A
        DJNZ    FP2
FP3:    LD      (FL_RM),A
        LD      A,E             ; px0 (0..255)
        RRCA
        RRCA
        RRCA
        AND     1FH
        LD      (FL_CX0),A
        LD      A,E
        AND     7
        LD      B,A
        LD      A,0FFH          ; lm = FF >> px0&7
        JR      Z,FP5
FP4:    SRL     A
        DJNZ    FP4
FP5:    LD      (FL_LM),A
        LD      A,(FL_Y)        ; ye = min(y + h, 192)
        LD      B,A
        LD      A,(FL_H)
        ADD     A,B
        JR      C,FP6
        CP      192
        JR      C,FP7
FP6:    LD      A,192
FP7:    LD      (FL_YE),A
        DEC     A
        RRCA
        RRCA
        RRCA
        AND     1FH
        LD      (FL_CY1),A
        LD      A,(FL_Y)
        RRCA
        RRCA
        RRCA
        AND     1FH
        LD      (FL_CY),A
FPROW:  LD      A,(FL_CY)       ; r0, r1: this cell row's lines
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      C,A             ; C = cy * 8
        LD      A,(FL_Y)
        CP      C
        LD      A,0
        JR      C,FPR1
        JR      Z,FPR1
        LD      A,(FL_Y)
        AND     7
FPR1:   LD      (FL_R0),A
        LD      A,(FL_YE)
        SUB     C
        CP      8
        JR      C,FPR2
        LD      A,8
FPR2:   LD      (FL_R1),A
        LD      A,(FL_CX0)
        LD      (FL_CX),A
FPCELL: LD      C,0FFH          ; the mask
        LD      A,(FL_CX)
        LD      B,A
        LD      A,(FL_CX0)
        CP      B
        JR      NZ,FPC1
        LD      A,(FL_LM)
        AND     C
        LD      C,A
FPC1:   LD      A,(FL_CX1)
        CP      B
        JR      NZ,FPC2
        LD      A,(FL_RM)
        AND     C
        LD      C,A
FPC2:   LD      A,C
        LD      (FL_M),A
        LD      A,(FL_CY)
        LD      D,A
        LD      E,B
        CALL    CELLAT
        LD      A,(FL_M)        ; all of the cell: no need to look
        INC     A
        JR      NZ,FPC4
        LD      A,(FL_R0)
        OR      A
        JR      NZ,FPC4
        LD      A,(FL_R1)
        CP      8
        JR      NZ,FPC4
        LD      HL,CELLP
        LD      B,8
FPC3:   LD      (HL),0
        INC     HL
        DJNZ    FPC3
        LD      A,(FL_COL)
        LD      C,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        OR      C
        LD      B,8
FPC3B:  LD      (HL),A
        INC     HL
        DJNZ    FPC3B
        JR      FPC6
FPC4:   CALL    CELLLD
        LD      A,(FL_R0)
        LD      B,A
FPC5:   LD      A,(FL_R1)
        CP      B
        JR      Z,FPC6
        LD      A,(FL_M)
        LD      C,A
        LD      A,(FL_COL)
        LD      D,A
        CALL    PAINT
        INC     B
        JR      FPC5
FPC6:   CALL    CELLST
        LD      HL,FL_CX
        LD      A,(HL)
        INC     (HL)
        LD      B,A
        LD      A,(FL_CX1)
        CP      B
        JR      NZ,FPCELL
        LD      HL,FL_CY
        LD      A,(HL)
        INC     (HL)
        LD      B,A
        LD      A,(FL_CY1)
        CP      B
        JP      NZ,FPROW
        RET
