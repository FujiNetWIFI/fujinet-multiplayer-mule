; spr.inc -- the sprites: the VDP's own, 16 x 16 and one colour each
; (gfx.c's). Fixed.
;
; A slot (SPRN of them) is up to four VDP sprites side by side and one
; above the other, with four pattern names of its own (slot * 4 + part). A
; slot's patterns are sent when its shape or frame changes (SPRSET), from
; the shape's 1-bit-a-clock rows widened 3/2 by WIDE12; the attribute table
; is written whole by SPRFLUSH, once a frame, when something moved. With
; five on a line the VDP drops the fifth: then the order turns a slot a
; frame, so they flicker instead. The background is drawn under them
; without lifting them (SPRLIFT and SPRDROP do nothing here).

SH_SPEC EQU     0               ; frame = species * 11 + f
SH_FIG  EQU     1               ; frame = species * 10 + f
SH_MULE EQU     2
SH_WAMP EQU     3
SH_CURS EQU     4
SH_NOSE EQU     5
SH_HTOP EQU     6
SH_HBOT EQU     7
SH_DOCK EQU     8
SH_PEST EQU     9
SH_CLOUD EQU    10
SH_BOLT EQU     11
SH_SPARK EQU    12
SH_METEOR EQU   13

SPR_FLIPX EQU   1
SPON    EQU     80H             ; SP_CF: on the screen

SPRLIFT:
SPRDROP: RET

; SPENT: IX = slot A's entry. Preserves BC,DE,HL.
SPENT:  PUSH    DE
        LD      IX,SPRT
        LD      E,A
        ADD     A,A
        ADD     A,E
        ADD     A,A
        ADD     A,E             ; * 7
        LD      E,A
        LD      D,0
        ADD     IX,DE
        POP     DE
        RET

; SHAPE: HL = shape A's SHTAB entry (data, bytes a row, clocks, lines).
; Preserves BC,DE.
SHAPE:  PUSH    DE
        LD      L,A
        LD      H,0
        LD      E,A
        LD      D,H
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,DE           ; * 5
        LD      DE,SHTAB
        ADD     HL,DE
        POP     DE
        RET

; SPGRID: shape A -> B = columns, C = rows of VDP sprites (at most four
; between them). Clobbers A,HL.
SPGRID: CALL    SHAPE
        INC     HL
        INC     HL
        INC     HL
        LD      A,(HL)          ; clocks: (w * 3 + 1) / 2 pixels
        LD      B,A
        ADD     A,A
        ADD     A,B
        INC     A
        SRL     A
        ADD     A,15
        RRCA
        RRCA
        RRCA
        RRCA
        AND     0FH
        LD      B,A
        INC     HL
        LD      A,(HL)          ; lines
        ADD     A,15
        RRCA
        RRCA
        RRCA
        RRCA
        AND     0FH
        LD      C,A
        LD      A,B             ; columns * rows > 4: rows = 4 / columns
        CP      3
        JR      C,SPG1
        LD      C,1
        RET
SPG1:   CP      2
        RET     NZ
        LD      A,C
        CP      3
        RET     C
        LD      C,2
        RET

; SPRSET: slot A as SPA_* say (gfx.h spr_set). Clobbers AF,BC,DE,HL.
SPRSET: PUSH    IX
        LD      C,A
        CALL    SPENT
        LD      A,(SPA_FR)      ; the frame shown: mirrored ones are frames
        LD      B,A             ; of their own (gfx.c frame_of)
        LD      A,(SPA_FL)
        AND     SPR_FLIPX
        JR      Z,SS2
        LD      A,(SPA_SH)
        CP      SH_MULE
        JR      NZ,SS1
        LD      A,B
        ADD     A,4
        LD      B,A
        JR      SS2
SS1:    CP      SH_FIG
        JR      NZ,SS2
        LD      A,B
SS1A:   SUB     10
        JR      NC,SS1A
        ADD     A,10            ; f % 10
        CP      2
        JR      NC,SS2
        INC     B
        INC     B
SS2:    LD      A,(SPA_SH)
        CP      (IX+SP_SH)
        JR      NZ,SS3
        LD      A,B
        CP      (IX+SP_FR)
        JR      Z,SS4
SS3:    LD      A,(SPA_SH)
        LD      (IX+SP_SH),A
        LD      (IX+SP_FR),B
        CALL    SPSEND
        LD      A,1
        LD      (SPDIRTY),A
SS4:    LD      A,(SPA_COL)     ; where, the colour, on
        AND     0FH
        OR      SPON
        LD      B,A
        LD      HL,(SPA_X)
        LD      DE,(SPA_Y)
        LD      A,(IX+SP_CF)
        CP      B
        JR      NZ,SS5
        LD      A,(IX+SP_XL)
        CP      L
        JR      NZ,SS5
        LD      A,(IX+SP_XH)
        CP      H
        JR      NZ,SS5
        LD      A,(IX+SP_YL)
        CP      E
        JR      NZ,SS5
        LD      A,(IX+SP_YH)
        CP      D
        JR      Z,SS6
SS5:    LD      (IX+SP_CF),B
        LD      (IX+SP_XL),L
        LD      (IX+SP_XH),H
        LD      (IX+SP_YL),E
        LD      (IX+SP_YH),D
        LD      A,1
        LD      (SPDIRTY),A
SS6:    POP     IX
        RET

; SPRSETP: slot A from the 8 bytes at HL (shape, frame, x lo/hi, y lo/hi,
; colour, flags) -- the caller's page may hold them.
SPRSETP: PUSH   BC
        PUSH    DE
        LD      DE,SPA_SH
        LD      BC,8
        LDIR
        POP     DE
        POP     BC
        JP      SPRSET

; SPRHIDE: slot A off the screen. Preserves BC,DE,HL.
SPRHIDE: PUSH   IX
        CALL    SPENT
        BIT     7,(IX+SP_CF)
        JR      Z,SPH1
        RES     7,(IX+SP_CF)
        LD      A,1
        LD      (SPDIRTY),A
SPH1:   POP     IX
        RET

; SPROFF: every slot off (gfx.h spr_hide_all). Preserves BC,DE,HL.
SPROFF: PUSH    BC
        LD      B,SPRN
SPO1:   LD      A,B
        DEC     A
        CALL    SPRHIDE
        DJNZ    SPO1
        POP     BC
        RET

; SPRINIT: nothing sent, nothing on.
SPRINIT: LD     HL,SPRT
        LD      B,SPRN
SPI1:   LD      (HL),0FFH       ; SP_SH: nothing sent
        INC     HL
        LD      C,SPRSZ-1
SPI2:   LD      (HL),0
        INC     HL
        DEC     C
        JR      NZ,SPI2
        DJNZ    SPI1
        LD      A,1
        LD      (SPDIRTY),A
        RET

; ---- the patterns -----------------------------------------------------------
; SPSEND: slot C's patterns: shape SP_SH, frame SP_FR of IX (gfx.c send).
; Each part's 16 x 16 pattern is its left column's 16 lines, then its
; right's; a line is widened once and sent to both. Clobbers AF,BC,DE,HL.
SS_DATA EQU     GTMP            ; the frame's first row
SS_WB   EQU     GTMP+2
SS_H    EQU     GTMP+3
SS_VR   EQU     GTMP+4          ; the part's pattern in VRAM
SS_GC   EQU     GTMP+6
SS_GR   EQU     GTMP+7
SS_ROW  EQU     GTMP+8
SS_LN   EQU     GTMP+9          ; 6 bytes: a line widened
SPSEND: LD      A,C             ; the slot's first pattern: 3800H + slot * 128
        RRCA
        LD      H,A
        AND     80H
        LD      L,A
        LD      A,H
        AND     7FH
        ADD     A,VR_SPAT/256
        LD      H,A
        LD      (SS_VR),HL
        LD      A,(IX+SP_SH)
        CALL    SPGRID
        PUSH    BC              ; columns, rows
        LD      A,(IX+SP_SH)
        CALL    SHAPE
        LD      E,(HL)
        INC     HL
        LD      D,(HL)
        INC     HL
        LD      A,(HL)
        LD      (SS_WB),A
        INC     HL
        INC     HL
        LD      A,(HL)
        LD      (SS_H),A
        LD      C,A             ; data + frame * wb * h
        LD      A,(SS_WB)
        LD      B,A
        XOR     A
SPN1:   ADD     A,C
        DJNZ    SPN1
        LD      C,A             ; C = wb * h (64 at most)
        LD      B,(IX+SP_FR)
        LD      HL,0
        LD      A,B
        OR      A
        JR      Z,SPN3
        LD      A,C
        LD      C,B
        LD      B,0
SPN2:   ADD     HL,BC
        DEC     A
        JR      NZ,SPN2
SPN3:   ADD     HL,DE
        LD      (SS_DATA),HL
        POP     BC              ; B columns, C rows
        XOR     A
        LD      (SS_GR),A
SPSGR:  XOR     A
        LD      (SS_GC),A
SPSGC:  PUSH    BC
        CALL    SPPART
        POP     BC
        LD      HL,(SS_VR)      ; the next part's pattern
        LD      DE,32
        ADD     HL,DE
        LD      (SS_VR),HL
        LD      HL,SS_GC
        INC     (HL)
        LD      A,(HL)
        CP      B
        JR      C,SPSGC
        LD      HL,SS_GR
        INC     (HL)
        LD      A,(HL)
        CP      C
        JR      C,SPSGR
        RET

; SPPART: the part (SS_GC, SS_GR) into its pattern at SS_VR.
SPPART: XOR     A
        LD      (SS_ROW),A
SPP1:   LD      A,(SS_GR)       ; the shape's row: gr * 16 + line
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      HL,SS_ROW
        ADD     A,(HL)
        CALL    SPLINE          ; SS_LN = that row widened
        LD      A,(SS_GC)       ; HL = SS_LN + gc * 2
        ADD     A,A
        LD      E,A
        LD      D,0
        LD      HL,SS_LN
        ADD     HL,DE
        LD      D,(HL)          ; the left half
        INC     HL
        LD      E,(HL)          ; the right half
        LD      A,(SS_ROW)
        LD      HL,(SS_VR)
        ADD     A,L
        LD      L,A
        CALL    VLOCK
        CALL    VWA
        LD      A,D
        OUT     (VDPD),A
        LD      A,L
        ADD     A,16
        LD      L,A
        CALL    VWA
        LD      A,E
        OUT     (VDPD),A
        CALL    VFREE
        LD      HL,SS_ROW
        INC     (HL)
        LD      A,(HL)
        CP      16
        JR      C,SPP1
        RET

; SPLINE: shape row A widened into SS_LN (zeros past the last row).
SPLINE: PUSH    IX
        CALL    SPLN0
        POP     IX
        RET
SPLN0:  LD      HL,SS_LN
        LD      B,6
SPL0:   LD      (HL),0
        INC     HL
        DJNZ    SPL0
        LD      HL,SS_H
        CP      (HL)
        RET     NC
        LD      C,A             ; HL = data + row * wb
        LD      A,(SS_WB)
        LD      B,A
        LD      HL,(SS_DATA)
        LD      A,C
        OR      A
        JR      Z,SPL2
        LD      E,B
        LD      D,0
SPL1:   ADD     HL,DE
        DEC     C
        JR      NZ,SPL1
SPL2:   LD      A,B             ; at most 4 bytes a row
        CP      5
        JR      C,SPL3
        LD      B,4
SPL3:   LD      C,0             ; C = b
        LD      IX,SS_LN
SPL4:   PUSH    HL
        PUSH    BC
        LD      A,(HL)          ; DE = wide12[byte]
        LD      L,A
        LD      H,0
        ADD     HL,HL
        LD      DE,WIDE12
        ADD     HL,DE
        LD      E,(HL)
        INC     HL
        LD      D,(HL)
        BIT     0,C
        JR      NZ,SPL5
        LD      A,E             ; even: [k] = v >> 4, [k+1] = v << 4
        RRCA
        RRCA
        RRCA
        RRCA
        AND     0FH
        LD      B,A
        LD      A,D
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        OR      B
        LD      (IX+0),A
        LD      A,E
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      (IX+1),A
        JR      SPL6
SPL5:   LD      A,D             ; odd: [k+1] |= v >> 8, [k+2] = v
        AND     0FH
        OR      (IX+1)
        LD      (IX+1),A
        LD      (IX+2),E
        INC     IX              ; the next pair of bytes: k + 3
        INC     IX
        INC     IX
SPL6:   POP     BC
        POP     HL
        INC     HL
        INC     C
        DJNZ    SPL4
        RET

; ---- the attribute table ---------------------------------------------------------
; SPRFLUSH: the attributes, if a slot changed or the VDP dropped a sprite
; (then the order turns). Once a frame, just after FRWAIT (gfx.c
; spr_flush). Clobbers AF,BC,DE,HL.
SF_K    EQU     GTMP            ; slots gone through
SF_N    EQU     GTMP+1          ; VDP sprites written
SF_GC   EQU     GTMP+2
SF_GR   EQU     GTMP+3
SF_COLS EQU     GTMP+4
SF_ROWS EQU     GTMP+5
SF_COL  EQU     GTMP+6
SF_NAME EQU     GTMP+7
SPRFLUSH: LD    A,(VDPSEEN)
        AND     40H
        LD      B,A
        LD      A,(SPDIRTY)
        OR      B
        RET     Z
        LD      A,B
        OR      A
        JR      Z,SF1
        LD      HL,SPROT        ; a sprite was dropped: take turns
        INC     (HL)
        LD      A,(HL)
        CP      SPRN
        JR      C,SF1
        LD      (HL),0
SF1:    XOR     A
        LD      (VDPSEEN),A
        LD      (SPDIRTY),A
        LD      (SF_K),A
        LD      (SF_N),A
        PUSH    IX
        CALL    VLOCK
        LD      HL,VR_SAT
        CALL    VWA
SFSLOT: LD      A,(SF_K)        ; slot (k + rot) % SPRN
        LD      HL,SPROT
        ADD     A,(HL)
        CP      SPRN
        JR      C,SF2
        SUB     SPRN
SF2:    LD      C,A
        CALL    SPENT
        BIT     7,(IX+SP_CF)
        JP      Z,SFNEXT
        LD      A,C             ; its first name: slot * 16 (* 4 parts, << 2)
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      (SF_NAME),A
        LD      A,(IX+SP_CF)
        AND     0FH
        LD      E,A
        LD      D,0
        LD      HL,TMSOF
        ADD     HL,DE
        LD      A,(HL)
        LD      (SF_COL),A
        LD      A,(IX+SP_SH)
        CALL    SPGRID
        LD      A,B
        LD      (SF_COLS),A
        LD      A,C
        LD      (SF_ROWS),A
        XOR     A
        LD      (SF_GR),A
SFGR:   XOR     A
        LD      (SF_GC),A
SFGC:   LD      A,(SF_GR)       ; DE = y + gr * 16
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      L,A
        LD      H,0
        LD      E,(IX+SP_YL)
        LD      D,(IX+SP_YH)
        ADD     HL,DE
        EX      DE,HL
        LD      HL,15           ; y <= -16 or y >= 192: not shown
        ADD     HL,DE
        BIT     7,H
        JP      NZ,SFPART
        LD      A,D
        INC     A
        JR      Z,SFY1          ; -15..-1: partly shown
        DEC     A
        JP      NZ,SFPART
        LD      A,E
        CP      192
        JP      NC,SFPART
SFY1:   LD      L,(IX+SP_XL)    ; HL = x + x/2 + 4 + gc * 16 (signed)
        LD      H,(IX+SP_XH)
        LD      B,H
        LD      C,L
        SRA     B
        RR      C
        ADD     HL,BC
        LD      BC,4
        ADD     HL,BC
        LD      A,(SF_GC)
        ADD     A,A
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      C,A
        LD      B,0
        ADD     HL,BC
        LD      B,0             ; B = the early clock
        BIT     7,H
        JR      Z,SFX1
        LD      BC,32           ; left of the screen: 32 to the left
        ADD     HL,BC
        LD      B,80H
        BIT     7,H             ; still left of it: not shown
        JR      NZ,SFPART
SFX1:   LD      A,H
        OR      A
        JR      NZ,SFPART       ; right of it
        LD      A,(SF_N)
        CP      32
        JR      NC,SFPART
        INC     A
        LD      (SF_N),A
        LD      A,E             ; y - 1
        DEC     A
        OUT     (VDPD),A
        EX      (SP),HL         ; (the VDP's 8 us)
        EX      (SP),HL
        LD      A,L             ; x
        OUT     (VDPD),A
        LD      A,(SF_COLS)     ; the name: (slot * 4 + gr * cols + gc) << 2
        LD      C,A
        LD      A,(SF_GR)
        LD      D,A
        XOR     A
SFN1:   DEC     D
        JP      M,SFN2
        ADD     A,C
        JR      SFN1
SFN2:   LD      HL,SF_GC
        ADD     A,(HL)
        ADD     A,A
        ADD     A,A
        LD      HL,SF_NAME
        ADD     A,(HL)
        OUT     (VDPD),A
        EX      (SP),HL
        EX      (SP),HL
        LD      A,(SF_COL)
        OR      B
        OUT     (VDPD),A
SFPART: LD      HL,SF_GC
        INC     (HL)
        LD      A,(SF_COLS)
        CP      (HL)
        JP      NZ,SFGC
        LD      HL,SF_GR
        INC     (HL)
        LD      A,(SF_ROWS)
        CP      (HL)
        JP      NZ,SFGR
SFNEXT: LD      HL,SF_K
        INC     (HL)
        LD      A,(HL)
        CP      SPRN
        JP      C,SFSLOT
        LD      A,(SF_N)        ; the end of the list
        CP      32
        JR      NC,SF9
        LD      A,0D0H
        OUT     (VDPD),A
SF9:    CALL    VFREE
        POP     IX
        RET
