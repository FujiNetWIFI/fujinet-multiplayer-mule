; macros.inc -- the client's macros, after fujinet.inc and s2macro.inc.
;
; RA.1 is $08 everywhere outside the runtime: RA reaches the variables on
; page $08 in three instructions (VAR/GETV/PUTV/SETV, as Battleship's).
; Page $09 (the text buffer, the stack) goes through R8/R9 pointers.

; Keep `size` bytes on one page, and out of $x000-$x3FF (CONFIG's RFIT).
; Every routine with a short branch starts with one; tools/fitcheck.py
; holds the sizes to the routines. Never fall into an RFIT: the gap is
; zeros, and $00 is IDL.
RFIT    MACRO   size
        IF      ((($) & 0FFH) + (size)) > 100H
        ORG     (($) + 0FFH) & 0FF00H
        ENDIF
        IF      (($) & 0C00H) == 0
        ORG     (($) & 0F000H) + 0400H
        ENDIF
        ENDM

; ---- console RAM --------------------------------------------------------
VAR     MACRO   v               ; RA -> v (page $08)
        LDI     LO(v)
        PLO     RA
        ENDM
GETV    MACRO   v               ; D = v
        VAR     v
        LDN     RA
        ENDM
PUTV    MACRO   v               ; v = D (RD.1 is free between calls)
        PHI     RD
        VAR     v
        GHI     RD
        STR     RA
        ENDM
SETV    MACRO   v, val          ; v = val
        VAR     v
        LDI     val
        STR     RA
        ENDM

; ---- the cart's RAM and the state (read anywhere, written by the cart) ---
; D = the byte at a (any cart page: the state, cart RAM, the reply). R8.
SLD     MACRO   a
        LDI     HI(a)
        PHI     R8
        LDI     LO(a)
        PLO     R8
        LDN     R8
        ENDM
; R8 -> a
SPTR    MACRO   a
        LDI     HI(a)
        PHI     R8
        LDI     LO(a)
        PLO     R8
        ENDM

; ---- hotspots ------------------------------------------------------------
; A text-engine parameter list: RF.1 on TX_PARAM, then P/PD three
; instructions a byte, then GOP runs the operation and parks RF.
PARS    MACRO
        LDI     TX_PARAM
        PHI     RF
        ENDM
P       MACRO   v
        LDI     v
        PLO     RF
        LDN     RF
        ENDM
PD      MACRO                   ; the parameter in D
        PLO     RF
        LDN     RF
        ENDM
GOP     MACRO   code
        LDI     TX_OP
        PHI     RF
        LDI     code
        PLO     RF
        LDN     RF
        LDI     FN_PARK
        PHI     RF
        ENDM
; A register write: REGSEL n, REGDATA v.
REG     MACRO   n, v
        LDI     n
        HOT     HI(FN_REGSEL)
        LDI     v
        HOT     HI(FN_REGDATA)
        ENDM

; HOLD1 / HOLD0: the cart holds the picture as it is while a redraw is
; under way, so it shows whole (CLS lets it go too).
HOLD1   MACRO
        PARS
        P       1
        GOP     TOP_HOLD
        ENDM
HOLD0   MACRO
        PARS
        P       0
        GOP     TOP_HOLD
        ENDM

; ---- calls ----------------------------------------------------------------
; Routine `adr` in bank `bk`, from anywhere; the caller's bank after.
BCALL   MACRO   bk, adr
        SEP     R4
        DW      bcall
        DB      bk
        DW      adr
        ENDM
; The routine at R9 (icall), as if FCALLed.
ICALL   MACRO
        SEP     R4
        DW      icall
        ENDM

; ---- drawing ---------------------------------------------------------------
AT      MACRO   rc              ; cursor to row r, column c ($rc)
        LDI     rc
        HOT     TX_CURSOR
        ENDM
OPN     MACRO   code            ; a text-engine operation, no parameters
        LDI     code
        HOT     TX_OP
        ENDM

; ---- sound -----------------------------------------------------------------
SND     MACRO   id              ; a cue (snd.inc)
        LDI     id
        FCALL   snd_play
        ENDM

; ---- segments ---------------------------------------------------------------
SEGEND  MACRO   lim             ; a segment must stop short of lim
        IF      $ > lim
        ERROR   "segment overflows its window"
        ENDIF
        ENDM
