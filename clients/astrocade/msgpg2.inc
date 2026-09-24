; msgpg2.inc -- more of the string page: a string of any kind, its '%'
; expanded, centred on a row (text.c text_center of text_expand). The
; message line's walker state is kept (a message may be scrolling).

; XSHTX: kind A, id B, number HL, centred on row D in colour C.
XSHTX:  LD      (SHTXK),A
        LD      A,D
        LD      (SHTXR),A
        LD      A,C
        LD      (SHTXC),A
        PUSH    HL
        LD      HL,(MSGPFX)
        PUSH    HL
        LD      HL,(MSGV1)
        PUSH    HL
        LD      HL,(MSGADR)
        PUSH    HL
        LD      A,(MSGPG)
        PUSH    AF
        LD      HL,0
        LD      (MSGPFX),HL
        LD      HL,8            ; (the stack: pg, adr, v1, pfx, v)
        ADD     HL,SP
        LD      E,(HL)
        INC     HL
        LD      D,(HL)
        LD      (MSGV1),DE
        LD      C,B
        LD      A,(SHTXK)
        CALL    STRLOC
        LD      A,B
        LD      (MSGPG),A
        LD      (MSGADR),HL
        CALL    MSGCNT
        LD      (MSGMAX),A
        LD      B,A
        LD      A,40
        SUB     B
        JR      NC,SHX1
        XOR     A
SHX1:   SRL     A
        LD      E,A
        LD      A,(SHTXR)
        CALL    CLRROW
        LD      A,(SHTXR)
        LD      D,A
        LD      A,(SHTXC)
        LD      C,A
        CALL    SETCOL
        CALL    TXTPOS
        XOR     A
        LD      (MSGSKP),A
        CALL    MWALK
        POP     AF
        LD      (MSGPG),A
        POP     HL
        LD      (MSGADR),HL
        POP     HL
        LD      (MSGV1),HL
        POP     HL
        LD      (MSGPFX),HL
        POP     HL
        RET
