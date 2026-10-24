; sayq.inc -- the say queue's end in a phase page: what to say, said by the
; strings page (pstr.inc) when the main loop gets to it, this frame.

; SAY: queue string `id` on `n` lines from line `ln` (L_*), in colour `col`.
; Its '%' is r3:r2. Clobbers A, r0, r5-r7.
SAY     macro id, ln, n, col
    mov r5,#id
    mov r6,#(ln)|((n)<<4)
    mov r7,#col
    call say_q
    endm

; say_q: queue string r5 on lines r6 (first | count << 4), colour r7, '%'
; r3:r2. The second slot if the first is taken. Clobbers A, r0.
say_q:
    mov r0,#SAYA
    movx a,@r0
    jz sq_put
    mov r0,#SAYB
sq_put:
    mov a,r5
    movx @r0,a
    inc r0
    mov a,r6
    movx @r0,a
    inc r0
    mov a,r7
    movx @r0,a
    inc r0
    mov a,r2
    movx @r0,a
    inc r0
    mov a,r3
    movx @r0,a
    ret
