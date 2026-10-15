; ---------------------------------------------------------------------------
; macros.inc -- the F8 idioms this client is written in.
;
; Calls. PI keeps exactly one return address (PC1), and PI and JMP both load
; A with the target's high byte, so nothing travels in A across a call. A
; routine that calls anything starts with ENTER and returns with LEAVE (the
; BIOS K-stack, r40-r58, nine deep; both clobber r7). A leaf returns with POP.
; LEAVE carries A back through r10, so a routine may return a value in A.
;
; Compares, unsigned, which the F8 has no instruction for:
;   CI n   sets C when n >= A, Z when n == A          (it computes n - A)
;   CM     sets C when (DC0) >= A, Z when equal       ((DC0) - A; DC0++)
;   LR A,x / COM / AS y   sets C when y > x           (y + ~x carries)
; BM and BP test bit 7 (signed): never use them for an unsigned bound.
; ---------------------------------------------------------------------------

ENTER   MACRO
        LR K,P
        PI PUSHK
        ENDM

LEAVE   MACRO
        LR 10,A
        PI POPK
        LR A,10
        PK
        ENDM

; Leave by jumping to a routine that returns to our caller (a tail call):
; one K-stack level fewer, and no LEAVE afterwards. Clobbers A, r7.
TAIL    MACRO target
        PI POPK
        LR P,K
        JMP target
        ENDM

; --- memory ----------------------------------------------------------------
LDB     MACRO addr              ; A = (addr); DC0 = addr+1
        DCI addr
        LM
        ENDM

STB     MACRO addr              ; (addr) = A; DC0 = addr+1
        DCI addr
        ST
        ENDM

; (addr) = n
SETB    MACRO addr, n
        LI n
        DCI addr
        ST
        ENDM

; r = (addr)
LDR     MACRO r, addr
        DCI addr
        LM
        LR r,A
        ENDM

; (addr) = r
STR     MACRO r, addr
        LR A,r
        DCI addr
        ST
        ENDM

; rlo, rhi = the 16-bit (addr), little-endian
LDW     MACRO rlo, rhi, addr
        DCI addr
        LM
        LR rlo,A
        LM
        LR rhi,A
        ENDM

STW     MACRO rlo, rhi, addr
        DCI addr
        LR A,rlo
        ST
        LR A,rhi
        ST
        ENDM

INCB    MACRO addr              ; (addr)++; A = the new value
        DCI addr
        LM
        INC
        DCI addr
        ST
        ENDM

DECB    MACRO addr
        DCI addr
        LM
        AI 0FFH
        DCI addr
        ST
        ENDM

; --- branches --------------------------------------------------------------
; unsigned A against a constant n (1..255)
BLTI    MACRO n, lab            ; A < n
        CI (n)-1
        BC lab
        ENDM

BGEI    MACRO n, lab            ; A >= n
        CI (n)-1
        BNC lab
        ENDM

BEQI    MACRO n, lab
        CI n
        BZ lab
        ENDM

BNEI    MACRO n, lab
        CI n
        BNZ lab
        ENDM

; Long branches: the condition inverted around a JMP. JMP loads A, so these
; are for places where A is dead.
JZ      MACRO lab
        BNZ m_skip
        JMP lab
m_skip:
        ENDM

JNZ     MACRO lab
        BZ m_skip
        JMP lab
m_skip:
        ENDM

JC      MACRO lab
        BNC m_skip
        JMP lab
m_skip:
        ENDM

JNC     MACRO lab
        BC m_skip
        JMP lab
m_skip:
        ENDM

; A = 0 sets Z (LR and LM do not touch the flags)
TSTA    MACRO
        NI 0FFH
        ENDM

; DC0 = DC0 + A, for any A 0..255 (ADC alone is signed, -128..127)
ADCU    MACRO
        NI 0FFH
        BP m_pos
        AI 080H                 ; A - 128: 0..127
        ADC
        LI 07FH
        ADC
        LIS 1
        ADC                     ; + 128 in all
        BR m_done
m_pos:  ADC
m_done:
        ENDM

; Settle after a VRAM write strobe: the BIOS plotter waits about 94 clocks
; before the next port write. Each NOP is 4 more; disp.inc counts the rest.
SETTLE  MACRO
        NOP
        ENDM

; PRTXT -- a string at text cell tx, pixel row y, in colour col (constants).
PRTXT   MACRO tx, y, str, col
        LI 6+4*(tx)
        LR 1,A
        LI y
        LR 2,A
        LI col
        LR 6,A
        DCI str
        PI DTEXT
        ENDM
