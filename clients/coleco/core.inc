; core.inc -- the frame interrupt, the VDP's guard, paging and far calls.
; Fixed.

; ---- the frame: the VDP's interrupt is the Z80's NMI ----------------------
; NMI: the BIOS jumps here (0066H: JP 8021H). Reading the VDP's status
; acknowledges the frame, but it also resets the VDP's address latch, which
; would spoil an address being set or a burst under way: while the game is
; at the VDP (VDPBUSY) the status is left and VDPPEND set. The VDP holds its
; interrupt line down meanwhile, so no other NMI comes; VFREE catches up.
; It must never read F800H or above (the mailbox's hotspots).
NMI:    PUSH    AF
        LD      A,(VDPBUSY)
        OR      A
        JR      NZ,NMIDEF
        CALL    NMISAVE
        POP     AF
        RETN
NMIDEF: LD      A,1
        LD      (VDPPEND),A
        POP     AF
        RETN

; NMISAVE: NMIWORK with every register kept but AF.
NMISAVE: PUSH   BC
        PUSH    DE
        PUSH    HL
        PUSH    IX
        PUSH    IY
        EX      AF,AF'
        PUSH    AF
        EXX
        PUSH    BC
        PUSH    DE
        PUSH    HL
        CALL    NMIWORK
        POP     HL
        POP     DE
        POP     BC
        EXX
        POP     AF
        EX      AF,AF'
        POP     IY
        POP     IX
        POP     HL
        POP     DE
        POP     BC
        RET

; NMIWORK: one frame: the status (acknowledged), the clock, the controllers
; (OS7's POLLER) and the sound (OS7's sound engine).
NMIWORK: IN     A,(VDPC)
        LD      HL,VDPSEEN
        OR      (HL)
        LD      (HL),A
        LD      HL,(FRAMES)
        INC     HL
        LD      (FRAMES),HL
        CALL    POLLER
        CALL    PLAYSNG
        JP      SNDMAN

; VLOCK / VFREE: around every address and data burst. Both keep every
; register (a guard that clobbers what it guards is worse than none).
VLOCK:  PUSH    AF
        LD      A,1
        LD      (VDPBUSY),A
        POP     AF
        RET

VFREE:  PUSH    AF
VFR0:   XOR     A
        LD      (VDPBUSY),A
        LD      A,(VDPPEND)
        OR      A
        JR      NZ,VFR1
        POP     AF
        RET
VFR1:   LD      A,1             ; a frame came meanwhile: take it now, with
        LD      (VDPBUSY),A     ; the next one kept out until it is done
        XOR     A
        LD      (VDPPEND),A
        CALL    NMISAVE
        JR      VFR0

; FRWAIT: wait for the next frame (the count to move past its last look).
; Clobbers A,HL,DE.
FRWAIT: LD      HL,(FRAMES)
FRW1:   LD      DE,(FRAMES)
        LD      A,E
        CP      L
        JR      Z,FRW1
        RET

; ---- paging ---------------------------------------------------------------
; SELPG: map page A at C000H. Preserves BC,DE,HL.
SELPG:  LD      (CURPG),A
        PUSH    HL
        ADD     A,FNBKLO
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

; RAND: A = a random byte: eight steps of OS7's RAND_GEN (one bit a call).
; Preserves everything else.
RAND:   PUSH    HL
        PUSH    BC
        LD      HL,(RANDNUM)
        LD      A,H
        OR      L
        JR      NZ,RND1
        LD      HL,(FRAMES)     ; its fixed point is 0
        SET     0,L
        LD      (RANDNUM),HL
RND1:   LD      B,8
RND2:   CALL    RANDGEN
        DJNZ    RND2
        POP     BC
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
