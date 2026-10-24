; text.inc -- the text screen's lines (screen.inc): say, say_line, lb_str.
; The includer puts it at the start of a page and that page's strings after
; it: say reads them with MOVP.

; say: line r4 (0-5) of the text screen = the string at offset A of this
; page, white. Clobbers A, r0-r7.
say:
    mov r6,a
    call lb_clear
    mov a,r6
    call lb_str
    ; fall into say_line

; say_line: LINEBUF onto line r4, white. say_line_c: in TXCOL / TXCOL2 as
; they are. Clobbers A, r0-r7.
say_line:
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    movx @r0,a
say_line_c:
    mov a,r4
    rl a
    add a,r4                    ; *3
    add a,#lines&0FFh
    mov r6,a
    movp a,@a
    mov r3,a                    ; the table's low byte
    inc r6
    mov a,r6
    movp a,@a
    mov r2,a                    ; its page
    inc r6
    mov a,r6
    movp a,@a
    mov r4,a                    ; its Y
    jmp tx_build

; lb_str: the NUL-terminated string at offset A of this page into LINEBUF;
; lb_at: into LINEBUF from r0. Clobbers A, r0, r1.
lb_str:
    mov r0,#LINEBUF
lb_at:
    mov r1,a
lbs_l:
    mov a,r1
    movp a,@a
    jz lbs_x
    movx @r0,a
    inc r0
    inc r1
    jmp lbs_l
lbs_x:
    ret

; The text screen's lines: table (low byte, page) and Y.
lines:
    db T0&0FFh, T0>>8, 30
    db T1&0FFh, T1>>8, 66
    db T2&0FFh, T2>>8, 102
    db T3&0FFh, T3>>8, 138
    db T4&0FFh, T4>>8, 174
    db T5&0FFh, T5>>8, 210

