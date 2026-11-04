; text.inc -- text, big text, numbers. Fixed.
;
; A character is a whole cell (the Namco font the Adam's games use): its 8
; pattern bytes, then 8 colour bytes (the colour on the background, BGC).
; 32 columns by 24 rows. Big text is the same (gfx.h BIGW 1). Lower case
; prints as upper case.

TCOLS   EQU     32

; SETCOL: C = colour (C_*) -> TXCOL. Preserves everything but A.
SETCOL: LD      A,C
        CALL    TCOL
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        PUSH    HL
        LD      HL,BGC
        OR      (HL)
        POP     HL
        LD      (TXCOL),A
        RET

; TXTPOS: row D, col E -> TXPOS. Preserves the primary registers.
TXTPOS: PUSH    HL
        PUSH    AF
        LD      A,E
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      L,A
        LD      H,D
        LD      A,E             ; past the right edge: nothing is drawn
        CP      TCOLS
        JR      C,TPS1
        LD      H,24
TPS1:   LD      (TXPOS),HL
        POP     AF
        POP     HL
        RET

; TXTAT: string HL (NUL-terminated) at row D, col E, colour C. TXTN:
; exactly B cells (padded with spaces). Clobbers all.
TXTAT:  LD      B,0
        JR      TXT0
TXTN:   LD      A,B
        OR      A
        RET     Z
TXT0:   CALL    SETCOL
        CALL    TXTPOS
TXT1:   LD      A,(HL)
        OR      A
        JR      Z,TXT3
        INC     HL
        CALL    GLYPHC
        LD      A,B
        OR      A
        JR      Z,TXT1
        DJNZ    TXT1
        RET
TXT3:   LD      A,B
        OR      A
        RET     Z
TXT5:   LD      A,' '
        CALL    GLYPHC
        DJNZ    TXT5
        RET
BIGAT   EQU     TXTAT
BIGN    EQU     TXTN

; CHRAT: character A at row D, col E, colour C. Clobbers all.
CHRAT:  PUSH    AF
        CALL    SETCOL
        CALL    TXTPOS
        POP     AF
; GLYPHC: character A at TXPOS in TXCOL; TXPOS moves a cell right (past
; the last column, nothing more is drawn on the row). Preserves BC,DE,HL.
GLYPHC: PUSH    HL
        PUSH    BC
        PUSH    DE
        LD      HL,(TXPOS)
        LD      E,A
        LD      A,H
        CP      24
        JR      NC,GLC9         ; off the screen (or past the edge)
        LD      A,E
        CP      60H
        JR      C,GLC1
        SUB     20H             ; lower case -> upper case
GLC1:   SUB     20H
        CP      40H
        JR      C,GLC2
        LD      A,'?'-20H
GLC2:   EX      DE,HL           ; DE = the cell
        LD      L,A
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        LD      BC,FONT
        ADD     HL,BC           ; HL = the glyph
        CALL    VLOCK
        EX      DE,HL
        CALL    VWA
        EX      DE,HL
        LD      B,8
        CALL    VOUT
        EX      DE,HL
        SET     5,H
        CALL    VWA
        RES     5,H
        LD      A,(TXCOL)
        LD      B,8
        CALL    VREP
        CALL    VFREE
        LD      A,L             ; the next cell
        ADD     A,8
        LD      L,A
        JR      NZ,GLC8
        LD      H,24            ; past the right edge
GLC8:   LD      (TXPOS),HL
GLC9:   POP     DE
        POP     BC
        POP     HL
        RET

; ---- centring, clearing ---------------------------------------------------
; STRLEN: A = length of the string at HL. Preserves HL.
STRLEN: PUSH    HL
        PUSH    BC
        LD      B,0
STRL1:  LD      A,(HL)
        OR      A
        JR      Z,STRL2
        INC     HL
        INC     B
        JR      STRL1
STRL2:  LD      A,B
        POP     BC
        POP     HL
        RET

; TXTCTR: clear row D, then string HL centred on it in colour C. A string
; wider than the screen is cut at a space and goes on, centred, on the next
; row (text.c text_center). BIGCTR: the same (big text is a cell).
BIGCTR:
TXTCTR: PUSH    HL
        PUSH    BC
        PUSH    DE
        LD      A,D
        CALL    CLRROW
        POP     DE
        POP     BC
        POP     HL
        CALL    STRLEN
        CP      TCOLS+1
        JR      C,TXC2
        PUSH    HL              ; find a space at TCOLS or before
        LD      A,L
        ADD     A,TCOLS
        LD      L,A
        JR      NC,TXC0
        INC     H
TXC0:   LD      B,TCOLS
TXC1:   LD      A,(HL)
        CP      ' '
        JR      Z,TXC3
        DEC     HL
        DJNZ    TXC1
        POP     HL              ; none: cut at the edge
        LD      A,TCOLS
TXC2:   LD      B,A             ; B characters, centred
        LD      A,TCOLS
        SUB     B
        SRL     A
        LD      E,A
        JP      TXTN
TXC3:   EX      (SP),HL         ; HL = the string, (SP) = the space
        PUSH    DE              ; (D is the row)
        LD      A,B             ; B = the cut, less the spaces before it
TXC4:   OR      A
        JR      Z,TXC5
        PUSH    HL
        LD      E,A
        DEC     E
        LD      D,0
        ADD     HL,DE
        LD      E,A
        LD      A,(HL)
        POP     HL
        CP      ' '
        LD      A,E
        JR      NZ,TXC5
        DEC     A
        JR      TXC4
TXC5:   POP     DE
        PUSH    DE
        LD      B,A
        LD      A,TCOLS
        SUB     B
        SRL     A
        LD      E,A
        PUSH    BC
        CALL    TXTPOS
        CALL    SETCOL
        POP     BC
        LD      A,B
        OR      A
        JR      Z,TXC7
TXC6:   LD      A,(HL)
        INC     HL
        CALL    GLYPHC
        DJNZ    TXC6
TXC7:   POP     DE
        POP     HL              ; the space
TXC8:   LD      A,(HL)          ; the rest, past its spaces, on the next row
        CP      ' '
        JR      NZ,TXC9
        INC     HL
        JR      TXC8
TXC9:   INC     D
        JP      TXTCTR

; CLRROW: text row A blank. Preserves D,E.
CLRROW: PUSH    DE
        LD      D,A
        LD      E,0
        LD      B,TCOLS
        CALL    CLRTXT
        POP     DE
        RET

; CLRTXT: B cells of row D from col E blank. Clobbers A,BC,HL.
CLRTXT: LD      A,B
        OR      A
        RET     Z
        LD      A,D
        CP      24
        RET     NC
        LD      A,E             ; at most to the edge
        CP      TCOLS
        RET     NC
        ADD     A,B
        CP      TCOLS+1
        JR      C,CLT1
        LD      A,TCOLS
        SUB     E
        LD      B,A
CLT1:   LD      A,E
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      L,A
        LD      H,D
        LD      A,B             ; B cells: 8B bytes
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      C,A             ; (0: 256, the whole row)
        CALL    VLOCK
        CALL    VWA
        LD      B,C
        XOR     A
        CALL    VREP
        SET     5,H
        CALL    VWA
        LD      A,(BGC)
        LD      B,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        OR      B
        LD      B,C
        CALL    VREP
        JP      VFREE

; ---- numbers --------------------------------------------------------------
; NUMSTR: HL in decimal at LINBUF (NUL-terminated); A = its length.
; Powers of ten subtracted (text.c fmt_num). Clobbers BC,DE,HL.
NUMSTR: LD      DE,LINBUF
        LD      B,0             ; digits so far
        PUSH    IX
        LD      IX,POW10
NMS1:   LD      C,(IX+0)
        LD      A,(IX+1)
        OR      C
        JR      Z,NMS4
        PUSH    DE
        LD      D,(IX+1)
        LD      E,C
        LD      A,'0'-1
NMS2:   INC     A
        OR      A
        SBC     HL,DE
        JR      NC,NMS2
        ADD     HL,DE
        POP     DE
        CP      '0'
        JR      NZ,NMS3
        INC     B
        DEC     B
        JR      Z,NMS5          ; leading zero
NMS3:   LD      (DE),A
        INC     DE
        INC     B
NMS5:   INC     IX
        INC     IX
        JR      NMS1
NMS4:   LD      A,L
        ADD     A,'0'
        LD      (DE),A
        INC     DE
        XOR     A
        LD      (DE),A
        INC     B
        LD      A,B
        POP     IX
        RET

POW10:  DW      10000,1000,100,10,0

; NUMR: HL right-aligned in B cells at LINBUF (space padded, the low
; digits kept if too long). Clobbers A,BC,DE,HL.
NUMR:   PUSH    BC
        CALL    NUMSTR          ; A = length
        POP     BC
        LD      C,A
        CP      B
        JR      C,NMR1
        JR      Z,NMR9
        SUB     B               ; too long: keep the last B digits
        LD      E,A
        LD      D,0
        LD      HL,LINBUF
        ADD     HL,DE
        LD      DE,LINBUF
        LD      C,B
        INC     C               ; with the NUL
        LD      B,0
        LDIR
NMR9:   RET
NMR1:   LD      A,B             ; shift right by B-C, spaces in front
        SUB     C
        LD      E,C
        LD      D,0
        LD      HL,LINBUF
        ADD     HL,DE           ; HL = the NUL
        PUSH    AF
        LD      E,B
        LD      D,0
        PUSH    HL
        LD      HL,LINBUF
        ADD     HL,DE
        EX      DE,HL           ; DE = LINBUF+B
        POP     HL
        LD      B,0
        INC     C
        LDDR
        POP     AF
        LD      B,A
        LD      HL,LINBUF
NMR2:   LD      (HL),' '
        INC     HL
        DJNZ    NMR2
        RET

; NUM32R: the 32-bit number at (HL) right-aligned in B cells at LINBUF
; (at most 7), the low digits kept (text.c big_num32). Divides by 10 in
; place in NUMTMP. Clobbers all.
NUM32R: LD      DE,NUMTMP
        PUSH    BC
        LD      BC,4
        LDIR
        POP     BC
        LD      C,B             ; C = cells
        LD      HL,LINBUF
        LD      E,B
        LD      D,0
        ADD     HL,DE
        LD      (HL),0          ; the NUL
N32A:   DEC     HL
        PUSH    HL
        PUSH    BC
        CALL    DIV10T          ; A = remainder
        POP     BC
        POP     HL
        ADD     A,'0'
        LD      (HL),A
        DEC     C
        JR      Z,N32C
        LD      A,(NUMTMP)      ; zero left: spaces
        LD      E,A
        LD      A,(NUMTMP+1)
        OR      E
        LD      E,A
        LD      A,(NUMTMP+2)
        OR      E
        LD      E,A
        LD      A,(NUMTMP+3)
        OR      E
        JR      NZ,N32A
N32B:   DEC     HL
        LD      (HL),' '
        DEC     C
        JR      NZ,N32B
N32C:   RET

; DIV10T: NUMTMP (32 bits) /= 10; A = the remainder.
DIV10T: LD      HL,NUMTMP+3
        LD      B,4
        XOR     A               ; the running remainder
DV10A:  LD      C,(HL)          ; (A:C) / 10, byte by byte from the top
        PUSH    BC
        LD      B,8
        LD      E,0
DV10B:  SLA     C
        RLA
        SLA     E
        CP      10
        JR      C,DV10C
        SUB     10
        INC     E
DV10C:  DJNZ    DV10B
        POP     BC
        LD      (HL),E
        DEC     HL
        DJNZ    DV10A
        RET
