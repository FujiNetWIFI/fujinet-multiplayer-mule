; msgpg.inc -- the message line and string drawing (text.c). Page 7, with
; the strings it draws (gen/stra.inc). Pages call the resident wrappers
; (msg.inc); the walker (MWALK) is resident, since a message may come
; from either string page.

; XSDRAW: string kind A, id B at row D, col E, colour C. XSDRAWM (mode in
; (SDMODE)): 0 text, 1 big, 2 text centred, 3 big centred, 4 text padded to
; SDPAD cells, 5 big padded. Clobbers all.
XSDRAW:  PUSH    AF
        XOR     A
        LD      (SDMODE),A
        POP     AF
XSDRAWM: PUSH    BC              ; (kinds 0-4 only: they live in this page)
        LD      C,B
        CALL    STRLOC          ; HL = string
        POP     BC
        LD      A,(SDMODE)
        OR      A
        JR      NZ,SDR1
        CALL    TXTAT
        JR      SDR9
SDR1:   DEC     A
        JR      NZ,SDR2
        CALL    BIGAT
        JR      SDR9
SDR2:   DEC     A
        JR      NZ,SDR3
        CALL    TXTCTR
        JR      SDR9
SDR3:   DEC     A
        JR      NZ,SDR4
        CALL    BIGCTR
        JR      SDR9
SDR4:   DEC     A
        LD      A,(SDPAD)
        LD      B,A
        JR      NZ,SDR5
        CALL    TXTN
        JR      SDR9
SDR5:   CALL    BIGN
SDR9:   RET

; MSGWR: draw MSGMAX characters from index MSGSKP at row D, col E.
MSGWR:  LD      A,(MSGCOL)
        LD      C,A
        CALL    SETCOL
        CALL    TXTPOS
        JP      MWALK

; MSGCNT: A = the message's length (and MSGCUT). Clobbers all.
MSGCNT: XOR     A
        LD      (MSGSKP),A
        LD      (MSGMAX),A
        CALL    MWALK
        LD      A,(MSGK)
        RET

; ---- showing messages -------------------------------------------------------
; XMSGSHOW: message id B, numbers HL, DE.
XMSGSHOW: LD     (MSGV1),HL
        LD      (MSGV2),DE
        LD      HL,0
        LD      (MSGPFX),HL
        LD      A,K_MSG
MSGSK:  LD      C,B
        CALL    STRLOC
        LD      A,B
        LD      (MSGPG),A
        LD      (MSGADR),HL
; XMSGSTART: the message set up in MSGPG/MSGADR/MSGPFX/MSGV1/MSGV2.
XMSGSTART: CALL  MSGCNT
        LD      (MSGLEN),A
        CALL    XMSGCLR
        LD      A,(MSGLEN)
        CP      41
        JR      NC,MSS2
        LD      (MSGMAX),A      ; one row, centred
        LD      B,A
        LD      A,40
        SUB     B
        SRL     A
        LD      E,A
        LD      A,(MSGROW)
        LD      D,A
        JP      MSGWR
MSS2:   CP      81
        JR      NC,MSS5
        LD      A,(MSGROW)      ; two rows (only one fits at the bottom:
        INC     A               ; it scrolls there instead)
        CP      MSGROWS
        JR      NC,MSS5
        LD      A,(MSGCUT)      ; cut at the last space in 40, unless the
        OR      A               ; rest would not fit
        JR      Z,MSS3
        LD      B,A
        LD      A,(MSGLEN)
        SUB     B
        DEC     A
        CP      41
        LD      A,B
        JR      C,MSS4
MSS3:   LD      A,40
MSS4:   LD      (MSGCUT),A
        LD      (MSGMAX),A
        LD      B,A
        LD      A,40
        SUB     B
        SRL     A
        LD      E,A
        LD      A,(MSGROW)
        LD      D,A
        CALL    MSGWR
        LD      A,(MSGCUT)      ; the rest, from after the space
        LD      B,A
        CP      40
        JR      Z,MSS4A
        INC     B
MSS4A:  LD      A,B
        LD      (MSGSKP),A
        LD      A,(MSGLEN)
        SUB     B
        LD      (MSGMAX),A
        LD      B,A
        LD      A,40
        SUB     B
        SRL     A
        LD      E,A
        LD      A,(MSGROW)
        INC     A
        LD      D,A
        JP      MSGWR
MSS5:   LD      A,1             ; scroll
        LD      (MSGSCR),A
        XOR     A
        LD      (MSGPOS),A
        LD      HL,(FRAMES)
        LD      (MSGLST),HL
        RET

; MSGROW1: the row the scroll uses (the second, if there is one).
MSGROW1: LD     A,(MSGROW)
        INC     A
        CP      MSGROWS
        RET     C
        DEC     A
        RET

; XMSGCLR: clear the message rows.
XMSGCLR: XOR     A
        LD      (MSGSCR),A
        LD      A,(MSGROW)
        CALL    CLRROW
        LD      A,(MSGROW)
        INC     A
        CP      MSGROWS
        RET     NC
        JP      CLRROW

; XMSGTICK: every frame: move a scrolling message a character every 4.
XMSGTICK: LD     A,(MSGSCR)
        OR      A
        RET     Z
        LD      HL,(FRAMES)
        LD      DE,(MSGLST)
        OR      A
        SBC     HL,DE
        LD      A,L
        CP      4
        RET     C
        LD      HL,(FRAMES)
        LD      (MSGLST),HL
        ; columns 0..39 show characters pos-40 .. pos-1
        CALL    MSGROW1
        LD      D,A
        LD      E,0
        LD      A,(MSGCOL)
        LD      C,A
        CALL    SETCOL
        CALL    TXTPOS
        LD      A,(MSGPOS)
        LD      B,A             ; leading blanks: 40 - pos, if pos < 40
        LD      A,40
        SUB     B
        JR      NC,MST1
        XOR     A
MST1:   LD      C,A             ; blanks
        OR      A
        JR      Z,MST2
        LD      B,A
MST1B:  LD      A,' '
        CALL    GLYPHC
        DJNZ    MST1B
MST2:   LD      A,40
        SUB     C
        LD      (MSGMAX),A      ; room for characters
        LD      A,(MSGPOS)
        SUB     40
        JR      NC,MST3
        XOR     A
MST3:   LD      (MSGSKP),A
        CALL    MWALK
        LD      A,(MSGMAX)      ; what the message didn't fill: blanks
        OR      A
        JR      Z,MST5
        LD      B,A
MST4:   LD      A,' '
        CALL    GLYPHC
        DJNZ    MST4
MST5:   LD      A,(MSGLEN)
        ADD     A,44
        LD      B,A
        LD      HL,MSGPOS
        INC     (HL)
        LD      A,(HL)
        CP      B
        RET     C
        LD      (HL),0
        RET

; XSHTITLE: title id B with number HL, centred on row 0. The message line's
; walker state is kept (a message may be scrolling). Clobbers all.
XSHTITLE: PUSH   HL
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
        LD      A,K_TITLE
        CALL    STRLOC
        LD      A,B
        LD      (MSGPG),A
        LD      (MSGADR),HL
        CALL    MSGCNT
        LD      (MSGMAX),A
        LD      B,A
        LD      A,40
        SUB     B
        SRL     A
        LD      E,A
        XOR     A
        CALL    CLRROW
        LD      D,0
        LD      C,CW
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
