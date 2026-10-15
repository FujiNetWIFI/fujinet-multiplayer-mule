; ---------------------------------------------------------------------------
; math.inc -- 16-bit arithmetic. 16-bit values travel little-endian in
; register pairs: (r0 lo, r1 hi) and (r2 lo, r3 hi). All leaves.
;
; The F8 adds without a carry-in, and LNK (add the carry) can itself carry,
; so a 16-bit sum's carry out of the high byte is not to be trusted; nothing
; here relies on it. Subtraction is ~(~v + p), which needs only the one carry
; between the bytes. COM, the logic ops and the shifts all CLEAR the carry:
; nothing may sit between an add and the LNK that takes its carry.
; ---------------------------------------------------------------------------

; ADD16 -- r0:r1 += r2:r3.  Clobbers A.
ADD16:  LR A,0
        AS 2
        LR 0,A
        LR A,1
        LNK
        AS 3
        LR 1,A
        POP

; SUB16 -- r0:r1 -= r2:r3 (mod 65536).  Clobbers A.
;   (COM clears the carry: the high byte is complemented first.)
SUB16:  LR A,1
        COM
        LR 1,A
        LR A,0
        COM
        AS 2                    ; ~v.lo + p.lo, carry into the high byte
        LR 0,A
        LR A,1
        LNK
        AS 3
        COM
        LR 1,A
        LR A,0
        COM
        LR 0,A
        POP

; GE16 -- C set if r0:r1 >= r2:r3, unsigned.  Clobbers A.
GE16:   LR A,3
        COM
        AS 1                    ; v.hi + ~p.hi carries when v.hi > p.hi
        BC GE16X
        LR A,1
        XS 3                    ; (clears C)
        BNZ GE16X               ; v.hi < p.hi
        LR A,0
        COM
        AS 2                    ; p.lo + ~v.lo carries when p.lo > v.lo
        BC GE16N
        LI 0FFH
        AI 1                    ; C set: v >= p
        POP
GE16N:  NI 0                    ; C clear
GE16X:  POP

; MUL8 -- r0:r1 = r0:r1 * r2 (mod 65536).  Clobbers A, r2-r5.
MUL8:   CLR
        LR 4,A                  ; product
        LR 5,A
        LIS 8
        LR 3,A
MUL8L:  LR A,4                  ; product <<= 1
        AS 4
        LR 4,A
        LR A,5
        LNK
        AS 5
        LR 5,A
        LR A,2                  ; multiplier's top bit
        NI 080H
        BZ MUL8N
        LR A,4
        AS 0
        LR 4,A
        LR A,5
        LNK
        AS 1
        LR 5,A
MUL8N:  LR A,2
        SL 1
        LR 2,A
        DS 3
        BNZ MUL8L
        LR A,4
        LR 0,A
        LR A,5
        LR 1,A
        POP

; DIV8 -- r0:r1 = r0:r1 / r2, r3 = the remainder.  r2 >= 1.
;   Clobbers A, r3-r5.
;   The shift moves each bit across by hand (double, then OI 1): LNK then AS
;   would drop the carry out of a high byte of $FF.
DIV8:   CLR
        LR 3,A
        LI 16
        LR 4,A
DIV8L:  CLR
        LR 5,A                  ; the remainder's ninth bit
        LR A,3
        AS 3
        LR 3,A
        BNC DIV8A
        LIS 1
        LR 5,A
DIV8A:  LR A,1
        NI 080H
        BZ DIV8B
        LR A,3
        OI 1
        LR 3,A
DIV8B:  LR A,1
        AS 1
        LR 1,A
        LR A,0
        NI 080H
        BZ DIV8C
        LR A,1
        OI 1
        LR 1,A
DIV8C:  LR A,0
        AS 0
        LR 0,A
        LR A,5
        NI 0FFH
        BNZ DIV8S               ; past 255: certainly >= r2
        LR A,2
        COM
        INC
        AS 3                    ; r3 - r2, carrying when r3 >= r2
        BNC DIV8N
        BR DIV8T
DIV8S:  LR A,2
        COM
        INC
        AS 3                    ; (256 + r3) - r2
DIV8T:  LR 3,A
        LR A,0
        OI 1                    ; a quotient bit
        LR 0,A
DIV8N:  DS 4
        BNZ DIV8L
        POP

; DBL16 -- r0:r1 *= 2.  Clobbers A.
DBL16:  LR A,0
        AS 0
        LR 0,A
        LR A,1
        LNK
        AS 1
        LR 1,A
        POP

; ASR16 -- r0:r1 >>= 1, keeping the sign.  Clobbers A, r4.
ASR16:  LR A,1
        NI 1
        LR 4,A                  ; the bit moving down a byte
        LR A,1
        SR 1
        LR 1,A
        NI 040H                 ; (the old sign, one place down)
        BZ ASR1
        LR A,1
        OI 080H
        LR 1,A
ASR1:   LR A,0
        SR 1
        LR 0,A
        LR A,4
        NI 0FFH
        BZ ASR2
        LR A,0
        OI 080H
        LR 0,A
ASR2:   POP

; RAND -- A = a random byte: a 16-bit Galois LFSR (as the Astrocade's),
;   seeded from the clock.  Leaf.  Clobbers A, r4-r7, DC0.
RAND:   DCI RNDS
        LM
        LR 4,A
        LM
        LR 5,A
        LR A,4
        NI 0FFH
        BNZ RND0
        LR A,5
        NI 0FFH
        BNZ RND0
        DCI FRAMES              ; never 0
        LM
        OI 1
        LR 4,A
RND0:   LIS 8                   ; eight steps: a fresh byte
        LR 6,A
RND1:   LR A,4
        NI 1
        LR 7,A                  ; the bit falling out
        LR A,4
        SR 1
        LR 4,A
        LR A,5
        NI 1
        BZ RND2
        LR A,4
        OI 080H
        LR 4,A
RND2:   LR A,5
        SR 1
        LR 5,A
        LR A,7
        NI 0FFH
        BZ RND3
        LR A,5
        XI 0B4H
        LR 5,A
RND3:   DS 6
        BNZ RND1
        DCI RNDS
        LR A,4
        ST
        LR A,5
        ST
        LR A,4
        POP
