; core.inc -- the frame interrupt, paging and far calls. Resident.

; INTON: IM 2 through VECTOR, the line interrupt below the picture.
INTON:  LD      HL,ISR
        LD      (VECTOR),HL
        LD      A,4FH           ; refresh cycles stay in screen RAM
        LD      I,A
        IM      2
        LD      A,VECTOR AND 0FFH
        OUT     (INFBK),A
        LD      A,INLINV
        OUT     (INLIN),A
        LD      A,INMODV
        OUT     (INMOD),A
        EI
        RET

; ISR: count the frame. Touches no cart address (the mailbox pairs its
; hotspot reads; an interrupt between them is harmless as long as the
; handler reads no hotspot).
ISR:    PUSH    AF
        PUSH    HL
        LD      HL,(FRAMES)
        INC     HL
        LD      (FRAMES),HL
        POP     HL
        POP     AF
        EI
        RETI

; FRWAIT: wait for the next frame (the count to move past DONE's last
; look). Clobbers A,HL,DE.
FRWAIT: LD      HL,(FRAMES)
FRW1:   LD      DE,(FRAMES)
        LD      A,E
        CP      L
        JR      Z,FRW1
        RET

; SELPG: map page A at 2000H. Preserves BC,DE,HL.
SELPG:  LD      (CURPG),A
        PUSH    HL
        ADD     A,80H
        LD      L,A
        LD      H,FNRSELP
        LD      A,(HL)
        POP     HL
        RET

; FARCALL: call into another page; the page and address follow the CALL:
;       CALL    FARCALL
;       DB      page
;       DW      routine
; A, BC, DE, HL go in and come back as the routine left them (and F out).
; The caller's page is mapped again on return.
FARCALL: LD     (FCA),A
        LD      (FCHL),HL
        POP     HL
        LD      A,(HL)          ; page
        INC     HL
        PUSH    DE
        LD      E,(HL)
        INC     HL
        LD      D,(HL)
        INC     HL
        LD      (FCTGT),DE
        POP     DE
        PUSH    HL              ; the real return
        LD      H,A
        LD      A,(CURPG)
        PUSH    AF              ; the caller's page
        LD      A,H
        CALL    SELPG
        LD      HL,(FCHL)
        LD      A,(FCA)
        CALL    FCJMP
        LD      (FCHL),HL
        POP     HL              ; H = the caller's page
        PUSH    AF
        LD      A,H
        CALL    SELPG
        POP     AF
        LD      HL,(FCHL)
        RET
FCJMP:  PUSH    HL
        LD      HL,(FCTGT)
        EX      (SP),HL
        RET

; RAND: A = a random byte: a 16-bit Galois LFSR, seeded from the clock.
; Preserves everything else.
RAND:   PUSH    HL
        LD      HL,(RNDS)
        LD      A,H
        OR      L
        JR      NZ,RND1
        LD      HL,(FRAMES)
        SET     0,L
RND1:   LD      A,8             ; eight steps: a fresh byte
RND2:   SRL     H
        RR      L
        JR      NC,RND3
        PUSH    AF
        LD      A,H
        XOR     0B4H
        LD      H,A
        POP     AF
RND3:   DEC     A
        JR      NZ,RND2
        LD      (RNDS),HL
        LD      A,L
        POP     HL
        RET

; FXSTOP: every actor stopped, its sprites off (fx.c fx_stop_all).
FXSTOP: CALL    FARCALL
        DB      PGGRANT
        DW      XFXSTOP
        RET

; ALNUM: carry clear if A is A-Z or 0-9.
ALNUM:  CP      '0'
        RET     C
        CP      '9'+1
        CCF
        RET     NC
        CP      'A'
        RET     C
        CP      'Z'+1
        CCF
        RET
