; sprpg.inc -- the sprites (gfx.h spr_*). The sprite page, with the shapes
; (gen/spr.inc).
;
; No hardware sprites: a sprite is drawn into the screen through magic
; memory with XOR, and drawn again to take it off. Each row: the fg/bg pair
; to XPAND (bg 0, so XOR leaves the background alone where the shape has no
; pixel), MAGIC = XOR + expand + the x shift, then each shape byte twice
; (the expander paints its high nibble, then its low one) and, when
; shifted, a flush write of 0 to push the carried pixels out. MAME's
; funcgen (astrocde_v.cpp) is the reference: the shifter's carry is the
; previous write's data whatever its address, so columns off the screen
; are written to SCRATCH instead of skipped, and the carry is zeroed before
; the first row.
;
; Positions are the Atari's (x colour clocks = pixels; y scanlines, which
; SPRSET squeezes to 3/8 as the art was). A slot keeps what it drew, so
; setting it again the same costs nothing, and a background drawn under a
; sprite must lift it first (SPRLIFT/SPRDROP).

MAGXOR  EQU     20H
MAGOR   EQU     10H
SPRY0   EQU     6               ; Atari line 16 = screen line 6

; slot record (SPRT, SPRSZ bytes)
SR_SH   EQU     0
SR_FR   EQU     1
SR_XL   EQU     2
SR_XH   EQU     3
SR_Y    EQU     4               ; screen line, signed
SR_COL  EQU     5
SR_FL   EQU     6               ; bit 0 flip, bit 7 on the screen, bit 6 lifted
SF_FLIP EQU     1
SF_ON   EQU     80H
SF_LIFT EQU     40H

; XSPRSET: slot A to SPA_* (shape, frame, x, Atari y, colour, flags).
XSPRSET: CALL   SLOTAD          ; IX = the slot
        ; the screen line: 6 + (y - 16) * 3 / 8, off the screen if far out
        LD      HL,(SPA_Y)
        LD      DE,-16
        ADD     HL,DE
        LD      D,H
        LD      E,L
        ADD     HL,HL
        ADD     HL,DE           ; *3
        SRA     H
        RR      L
        SRA     H
        RR      L
        SRA     H
        RR      L               ; /8
        LD      DE,SPRY0
        ADD     HL,DE
        LD      A,H             ; -128..127 or not at all
        OR      A
        JR      Z,SS1
        INC     A
        JP      NZ,XSPRHIDEX
        BIT     7,L
        JP      Z,XSPRHIDEX
        JR      SS2
SS1:    BIT     7,L
        JP      NZ,XSPRHIDEX
SS2:    LD      C,L             ; C = the line
        ; the same as drawn? nothing to do
        BIT     7,(IX+SR_FL)
        JR      Z,SS3
        LD      A,(SPA_SH)
        CP      (IX+SR_SH)
        JR      NZ,SS3
        LD      A,(SPA_FR)
        CP      (IX+SR_FR)
        JR      NZ,SS3
        LD      A,(SPA_X)
        CP      (IX+SR_XL)
        JR      NZ,SS3
        LD      A,(SPA_X+1)
        CP      (IX+SR_XH)
        JR      NZ,SS3
        LD      A,C
        CP      (IX+SR_Y)
        JR      NZ,SS3
        LD      A,(SPA_COL)
        CP      (IX+SR_COL)
        JR      NZ,SS3
        LD      A,(SPA_FL)
        XOR     (IX+SR_FL)
        AND     SF_FLIP
        RET     Z
SS3:    PUSH    BC
        BIT     7,(IX+SR_FL)
        CALL    NZ,BLIT         ; off with the old
        POP     BC
        LD      A,(SPA_SH)
        LD      (IX+SR_SH),A
        LD      A,(SPA_FR)
        LD      (IX+SR_FR),A
        LD      A,(SPA_X)
        LD      (IX+SR_XL),A
        LD      A,(SPA_X+1)
        LD      (IX+SR_XH),A
        LD      (IX+SR_Y),C
        LD      A,(SPA_COL)
        LD      (IX+SR_COL),A
        LD      A,(SPA_FL)
        AND     SF_FLIP
        OR      SF_ON
        LD      (IX+SR_FL),A
        JP      BLIT            ; on with the new

; XSPRHIDE: slot A off the screen.
XSPRHIDE: CALL  SLOTAD
XSPRHIDEX:
        BIT     7,(IX+SR_FL)
        JR      Z,XSH1
        CALL    BLIT
XSH1:   LD      (IX+SR_FL),0
        RET

XSPRALL: LD     B,SPRN          ; all of them
XSA1:   LD      A,B
        DEC     A
        PUSH    BC
        CALL    XSPRHIDE
        POP     BC
        DJNZ    XSA1
        RET

; XSPRLIFT: take every sprite off, to draw the background under it;
; XSPRDROP puts them back.
XSPRLIFT: LD    IX,SPRT
        LD      B,SPRN
XSL1:   BIT     7,(IX+SR_FL)
        JR      Z,XSL2
        PUSH    BC
        CALL    BLIT
        POP     BC
        LD      A,(IX+SR_FL)
        AND     7FH
        OR      SF_LIFT
        LD      (IX+SR_FL),A
XSL2:   LD      DE,SPRSZ
        ADD     IX,DE
        DJNZ    XSL1
        RET
XSPRDROP: LD    IX,SPRT
        LD      B,SPRN
XSD1:   BIT     6,(IX+SR_FL)
        JR      Z,XSD2
        PUSH    BC
        CALL    BLIT
        POP     BC
        LD      A,(IX+SR_FL)
        AND     0BFH
        OR      SF_ON
        LD      (IX+SR_FL),A
XSD2:   LD      DE,SPRSZ
        ADD     IX,DE
        DJNZ    XSD1
        RET

; SLOTAD: IX = SPRT + A * 7.
SLOTAD: LD      E,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        SUB     E
        LD      E,A
        LD      D,0
        LD      IX,SPRT
        ADD     IX,DE
        RET

; BLIT: XOR the slot at IX onto the screen. Preserves IX.
BLIT:   LD      A,(IX+SR_SH)    ; the shape: bytes a row, rows, frame size
        LD      L,A
        ADD     A,A
        ADD     A,A
        ADD     A,L
        LD      L,A
        LD      H,0
        LD      DE,SHTAB
        ADD     HL,DE
        LD      A,(HL)
        LD      (BWB),A
        INC     HL
        LD      A,(HL)
        LD      (BH),A
        INC     HL
        LD      C,(HL)          ; frame size
        INC     HL
        LD      E,(HL)
        INC     HL
        LD      D,(HL)          ; the data
        LD      B,(IX+SR_FR)
        LD      A,B
        OR      A
        JR      Z,BL2
        LD      H,0
        LD      L,C
BL1:    EX      DE,HL
        ADD     HL,DE           ; (DE = size, HL = data: swapped each time)
        EX      DE,HL
        DJNZ    BL1
BL2:    LD      (BSRC),DE
        ; x: the first column (signed) and the shift
        LD      L,(IX+SR_XL)
        LD      H,(IX+SR_XH)
        LD      A,L
        AND     3
        LD      (BSH),A
        SRA     H
        RR      L
        SRA     H
        RR      L
        LD      A,H             ; columns -128..127 are enough
        OR      A
        JR      Z,BL3
        INC     A
        RET     NZ
BL3:    LD      A,L
        LD      (BCOL),A
        ; the colours: fg*4, stripes alternate by line
        LD      A,(IX+SR_COL)
        CP      CS
        JR      NZ,BL4
        LD      A,CB*4
        LD      (BXP1),A
        LD      A,CO*4
        JR      BL5
BL4:    ADD     A,A
        ADD     A,A
        LD      (BXP1),A
BL5:    LD      (BXP2),A
        ; zero the shifter's carry
        LD      A,MAGXOR
        OUT     (MAGIC),A
        XOR     A
        LD      (SCRATCH-4000H),A
        LD      A,(BSH)
        OR      MAGXOR+MAGEXP
        LD      (BMAG),A
        LD      A,(IX+SR_Y)
        LD      (BY),A
        LD      A,(BH)
        LD      B,A
BLROW:  PUSH    BC
        LD      A,(BY)          ; on the screen?
        CP      LINES
        JR      NC,BLNEXT       ; (negative lines too: unsigned)
        LD      C,A
        AND     1               ; stripes: odd lines the other colour
        LD      A,(BXP1)
        JR      Z,BLR1
        LD      A,(BXP2)
BLR1:   OUT     (XPAND),A
        LD      A,(BMAG)
        OUT     (MAGIC),A
        LD      L,C             ; HL = line * 40 (a magic address)
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL           ; *8
        LD      D,H
        LD      E,L
        ADD     HL,HL
        ADD     HL,HL           ; *32
        ADD     HL,DE           ; *40
        LD      (BROW),HL
        LD      A,(BCOL)
        LD      C,A             ; C = the column being written
        LD      A,(BWB)
        LD      B,A
        LD      DE,(BSRC)
        BIT     0,(IX+SR_FL)
        JR      Z,BLB1
        LD      A,E             ; flipped: the row's last byte first
        ADD     A,B
        LD      E,A
        JR      NC,BLB0
        INC     D
BLB0:   DEC     DE
BLB1:   LD      A,(DE)
        BIT     0,(IX+SR_FL)
        JR      Z,BLB2
        PUSH    HL
        LD      HL,REV8
        ADD     A,L
        LD      L,A
        JR      NC,BLB1A
        INC     H
BLB1A:  LD      A,(HL)
        POP     HL
        DEC     DE
        JR      BLB3
BLB2:   INC     DE
BLB3:   CALL    BPUT            ; high nibble
        CALL    BPUT            ; low nibble
        DJNZ    BLB1
        LD      A,(BSH)
        OR      A
        LD      A,0
        CALL    NZ,BPUT         ; flush the carry
BLNEXT: LD      HL,(BSRC)       ; the next row of the shape
        LD      A,(BWB)
        ADD     A,L
        LD      L,A
        JR      NC,BLN1
        INC     H
BLN1:   LD      (BSRC),HL
        LD      HL,BY
        INC     (HL)
        POP     BC
        DJNZ    BLROW
        RET

; BPUT: write A through magic memory at column C of line BROW (SCRATCH if
; off the screen); C moves on. Preserves A, B, DE.
BPUT:   PUSH    AF
        LD      A,C
        CP      40
        JR      NC,BPU1
        LD      HL,(BROW)
        ADD     A,L
        LD      L,A
        JR      NC,BPU2
        INC     H
        JR      BPU2
BPU1:   LD      HL,SCRATCH-4000H
BPU2:   POP     AF
        LD      (HL),A
        INC     C
        RET

; (REV8, each byte mirrored for a flipped sprite, is in gen/spr.inc.)
