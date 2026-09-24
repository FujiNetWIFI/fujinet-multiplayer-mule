; text.inc -- the 4x6 renderer, big text, numbers, fills. Resident.
;
; From fujinet-battleship/astrocade/gfx.inc. At 2bpp four pixels are one
; byte and the character grid is four pixels wide, so a glyph row is one
; byte and nothing shifts. Text goes through the magic EXPANDER: OUT the
; fg/bg pair to XPAND, reset the expander's nibble flip-flop by writing the
; MAGIC register, and the next write through magic memory (0000H + screen
; offset) paints the HIGH nibble as 4 pixels in those two colours; a second
; write without a reset paints the LOW nibble (big text uses that).
;
; Grid: 40 columns x 14 rows of 4x6 cells over the 84 lines, the Atari's
; 40-column grid exactly (its 24 rows of 8 lines don't fit: every screen is
; laid out again on 14).
;
; Colours are the game's four plus the stripes (the CoCo scheme):
;   CK 0 black, CB 1 blue (player 0), CO 2 orange (player 1),
;   CW 3 white (player 2), CS 4 blue/orange stripes (player 3).
; Text is fg on black. The stripes alternate line by line.

CK      EQU     0
CB      EQU     1
CO      EQU     2
CW      EQU     3
CS      EQU     4

MAGEXP  EQU     08H             ; MAGIC: expand, no shift/rotate/or/xor
ROWBYT  EQU     6*BYTEPL        ; 240 bytes in one text row

; SETCOL: C = colour code -> TXCOL (the XPAND pair), TXSTR (the stripe's
; second pair or 0). Preserves everything but A.
SETCOL: LD      A,C
        CP      CS
        JR      Z,SETC1
        ADD     A,A
        ADD     A,A             ; fg*4 + black
        LD      (TXCOL),A
        XOR     A
        LD      (TXSTR),A
        RET
SETC1:  LD      A,CB*4
        LD      (TXCOL),A
        LD      A,CO*4
        LD      (TXSTR),A
        RET

; TXTAT: string HL (NUL-terminated) at row D, col E, colour C. TXTN: exactly
; B cells (pads with spaces). Lower case prints as upper case. Clobbers all.
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

; CHRAT: character A at row D, col E, colour C. Clobbers all.
CHRAT:  PUSH    AF
        CALL    SETCOL
        CALL    TXTPOS
        POP     AF
; GLYPHC: character A at the shadow-DE magic address, which moves one cell
; right. Preserves the primary registers.
GLYPHC: CP      60H
        JR      C,GLC1
        SUB     20H             ; lower case -> upper case
GLC1:   SUB     20H
        CP      40H
        JR      C,GLYPH
        LD      A,'?'-20H
; GLYPH: glyph index A (0-3FH).
GLYPH:  EXX
        CALL    GLYADR
        LD      B,6
        LD      A,(TXCOL)
        LD      C,A
GLYRW:  LD      A,C
        OUT     (XPAND),A
        LD      A,(TXSTR)       ; stripes: the other pair next line
        OR      A
        JR      Z,GLYR0
        LD      C,A
        LD      A,(TXCOL)
        LD      (TXSTR),A
        LD      A,C
        LD      (TXCOL),A
GLYR0:  LD      A,MAGEXP
        OUT     (MAGIC),A
        LD      A,(HL)
        LD      (DE),A
        INC     HL
        LD      A,E
        ADD     A,BYTEPL
        LD      E,A
        JR      NC,GLYRW1
        INC     D
GLYRW1: DJNZ    GLYRW
        LD      A,E             ; back up 6 lines, over 1 cell
        SUB     6*BYTEPL-1
        LD      E,A
        JR      NC,GLYRW2
        DEC     D
GLYRW2: EXX
        RET

; GLYADR: HL = FONT46 + A*6. Clobbers BC.
GLYADR: LD      L,A
        LD      H,0
        ADD     HL,HL
        LD      B,H
        LD      C,L
        ADD     HL,HL
        ADD     HL,BC
        LD      BC,FONT46
        ADD     HL,BC
        RET

; TXTPOS: row D, col E -> shadow DE = magic address (row*240 + col).
; Preserves the primary registers.
TXTPOS: PUSH    HL
        PUSH    BC
        CALL    ROWOFF
        LD      B,0
        LD      C,E
        ADD     HL,BC
        PUSH    HL
        EXX
        POP     DE
        EXX
        POP     BC
        POP     HL
        RET

; ROWOFF: HL = row D * 240. Clobbers A.
ROWOFF: PUSH    DE
        LD      A,D
        ADD     A,A
        LD      E,A
        LD      D,0
        LD      HL,ROWTAB
        ADD     HL,DE
        LD      A,(HL)
        INC     HL
        LD      H,(HL)
        LD      L,A
        POP     DE
        RET

ROWTAB: DW      0*ROWBYT,1*ROWBYT,2*ROWBYT,3*ROWBYT,4*ROWBYT
        DW      5*ROWBYT,6*ROWBYT,7*ROWBYT,8*ROWBYT,9*ROWBYT
        DW      10*ROWBYT,11*ROWBYT,12*ROWBYT,13*ROWBYT

; ---- big text: double width ---------------------------------------------
; BIGAT: string HL at row D, col E (each character two cells), colour C.
; BIGN: exactly B characters, space padded. Clobbers all.
BIGAT:  LD      B,0
        JR      BIG0
BIGN:   LD      A,B
        OR      A
        RET     Z
BIG0:   CALL    SETCOL
        CALL    TXTPOS
BIG1:   LD      A,(HL)
        OR      A
        JR      Z,BIG3
        INC     HL
        CALL    BGLYPH
        LD      A,B
        OR      A
        JR      Z,BIG1
        DJNZ    BIG1
        RET
BIG3:   LD      A,B
        OR      A
        RET     Z
BIG5:   LD      A,' '
        CALL    BGLYPH
        DJNZ    BIG5
        RET

; BGLYPH: character A, double width, at shadow DE (which moves two cells).
BGLYPH: CP      60H
        JR      C,BGL1
        SUB     20H
BGL1:   SUB     20H
        CP      40H
        JR      C,BGL2
        LD      A,'?'-20H
BGL2:   EXX
        CALL    GLYADR
        LD      B,6
BGLRW:  LD      A,(TXCOL)
        OUT     (XPAND),A
        LD      A,(TXSTR)
        OR      A
        JR      Z,BGLR0
        LD      C,A
        LD      A,(TXCOL)
        LD      (TXSTR),A
        LD      A,C
        LD      (TXCOL),A
BGLR0:  PUSH    HL
        LD      A,(HL)          ; high nibble -> each bit doubled
        RRCA
        RRCA
        RRCA
        RRCA
        AND     0FH
        LD      L,A
        LD      H,0
        LD      A,L
        LD      HL,DBLTAB
        ADD     A,L
        LD      L,A
        JR      NC,BGLR1
        INC     H
BGLR1:  LD      A,MAGEXP
        OUT     (MAGIC),A
        LD      A,(HL)
        LD      (DE),A          ; high nibble: the left 4 pixels
        INC     E
        LD      (DE),A          ; low nibble: the right 4
        DEC     E
        POP     HL
        INC     HL
        LD      A,E
        ADD     A,BYTEPL
        LD      E,A
        JR      NC,BGLR2
        INC     D
BGLR2:  DJNZ    BGLRW
        LD      A,E
        SUB     6*BYTEPL-2
        LD      E,A
        JR      NC,BGLR3
        DEC     D
BGLR3:  EXX
        RET

DBLTAB: DB      00H,03H,0CH,0FH,30H,33H,3CH,3FH
        DB      0C0H,0C3H,0CCH,0CFH,0F0H,0F3H,0FCH,0FFH

; ---- centring, clearing ---------------------------------------------------
; STRLEN: A = length of the string at HL. Preserves HL.
STRLEN: PUSH    HL
        LD      B,0
STRL1:  LD      A,(HL)
        OR      A
        JR      Z,STRL2
        INC     HL
        INC     B
        JR      STRL1
STRL2:  LD      A,B
        POP     HL
        RET

; TXTCTR: clear row D, then string HL centred on it in colour C.
TXTCTR: PUSH    HL
        PUSH    BC
        LD      A,D
        CALL    CLRROW
        POP     BC
        POP     HL
        CALL    STRLEN
        CP      40
        JR      C,TXC1
        LD      A,40
TXC1:   LD      B,A
        LD      A,40
        SUB     B
        SRL     A
        LD      E,A
        JP      TXTAT

; BIGCTR: clear row D, then string HL centred in big text (even column).
BIGCTR: PUSH    HL
        PUSH    BC
        LD      A,D
        CALL    CLRROW
        POP     BC
        POP     HL
        CALL    STRLEN
        CP      20
        JR      C,BGC1
        LD      A,20
BGC1:   LD      B,A
        LD      A,20
        SUB     B
        AND     0FEH
        LD      E,A
        JP      BIGAT

; CLRROW: text row A to black. Preserves D,E.
CLRROW: PUSH    DE
        LD      D,A
        CALL    ROWOFF
        LD      DE,NORMEM
        ADD     HL,DE
        LD      D,H
        LD      E,L
        INC     DE
        LD      (HL),0
        LD      BC,ROWBYT-1
        LDIR
        POP     DE
        RET

; CLRTXT: B cells of row D from col E to black. Clobbers A,BC,HL.
CLRTXT: LD      A,B
        OR      A
        RET     Z
        PUSH    DE
        CALL    ROWOFF
        LD      D,0
        ADD     HL,DE
        LD      DE,NORMEM
        ADD     HL,DE
        LD      C,6
CLT1:   PUSH    HL
        PUSH    BC
CLT2:   LD      (HL),0
        INC     HL
        DJNZ    CLT2
        POP     BC
        POP     HL
        LD      DE,BYTEPL
        ADD     HL,DE
        DEC     C
        JR      NZ,CLT1
        POP     DE
        RET

; CLRSCR: the whole screen black. Clobbers A,BC,DE,HL.
CLRSCR: LD      HL,NORMEM
        LD      DE,NORMEM+1
        LD      BC,LINES*BYTEPL-1
        LD      (HL),0
        LDIR
        RET

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
        ; too long: keep the last B digits
        SUB     B
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
NMR1:   ; shift right by B-C, fill with spaces
        LD      A,B
        SUB     C               ; pad count
        LD      E,C             ; move C+1 bytes (with NUL) from the end
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
