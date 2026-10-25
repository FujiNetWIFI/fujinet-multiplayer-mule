; calls.inc -- calls SCRT can't make by itself: through a register (icall)
; and into a bank (bcall). Fixed code: a bank select takes effect on the
; next fetch, so it must run from outside the window.

; jmp_r9: R3 = R9 and run there; entered with SEP R8 (R8 is spent).
jmp_r9: GHI     R9
        PHI     R3
        GLO     R9
        PLO     R3
        SEP     R3

; icall (ICALL): the routine at R9, as if the caller had FCALLed it: it
; takes over icall's frame, so its FRET goes straight back.
icall:  LDR     R8, jmp_r9
        SEP     R8

; bcall (BCALL bk, adr): adr in bank bk, the caller's bank back after. The
; caller's bank goes on the stack under its return address; the routine
; returns to bk_epi, which puts that bank back. D and DF pass both ways.
bcall:  LDA     R6
        PHI     RD                      ; the bank
        LDA     R6
        PHI     R9
        LDA     R6
        PLO     R9                      ; the routine
        GETV    CURBANK
        STXD                            ; the caller's bank,
        GHI     R6
        STXD
        GLO     R6
        STXD                            ; under its return (SCRT's order)
        GHI     RD
        STR     RA                      ; CURBANK
        ORI     LO(FN_BANKSEL)
        HOT     HI(FN_BANKSEL)
        LDR     R6, bk_epi
        LDR     R8, jmp_r9
        SEP     R8
bk_epi: PHI     RD                      ; the routine's D
        IRX
        LDX                             ; the caller's bank
        PLO     R9
        VAR     CURBANK
        GLO     R9
        STR     RA
        ORI     LO(FN_BANKSEL)
        HOT     HI(FN_BANKSEL)
        GHI     RD
        FRET

; bksel: bank D in the window, from fixed code (the caller must be too).
bksel:  PUTV    CURBANK
        ORI     LO(FN_BANKSEL)
        HOT     HI(FN_BANKSEL)
        FRET
