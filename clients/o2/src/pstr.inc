; pstr.inc -- a strings page, bank STRBANK of src/strings.txt
; (tools/mkstr.py): the say queue's server. While SAYA holds one of this
; bank's strings the main loop farcalls E_SAY; it says the string on its
; lines -- each line centred, '%' the request's number, lines past the
; string's last blank -- then moves SAYB up.

E_SAY   equ 0

    PAGE_HEAD
    db e_say&0FFh
e_say:
    jmp say_serve

; say_serve: SAYA said; SAYB moved up. Clobbers A, r0-r7.
say_serve:
    mov r0,#SAYA+SAY_ID
    movx a,@r0
    anl a,#3Fh
    call sv_find
    mov r4,a                    ; r6:r4, the string; r4 = 0 once it is said
ss_line:
    call lb_clear
    mov a,r4
    jz ss_put                   ; past its last line: blank
    call sv_copy
    jz ss_more
    mov r4,#0                   ; that was its last
ss_more:
    call sv_center
ss_put:
    mov r0,#SV_PTR              ; (tx_build keeps no register)
    mov a,r4
    movx @r0,a
    inc r0
    mov a,r6
    movx @r0,a
    call say_put
    mov r0,#SV_PTR
    movx a,@r0
    mov r4,a
    inc r0
    movx a,@r0
    mov r6,a
    mov r0,#SAYA+SAY_LN         ; the next line, one fewer to go
    movx a,@r0
    add a,#1-10h
    movx @r0,a
    anl a,#0F0h
    jnz ss_line
    mov r0,#SAYB                ; SAYB moves up
    mov r1,#SAYA
    mov r2,#SAY_SIZE
ss_mv:
    movx a,@r0
    movx @r1,a
    inc r0
    inc r1
    djnz r2,ss_mv
    clr a
    mov r0,#SAYB+SAY_ID
    movx @r0,a
    ret

; sv_find: r6 = the data page (0-3) of local string A, A = its offset there.
; Clobbers A, r1, r5.
sv_find:
    mov r5,a
    mov r6,#0
sf_l:
    mov a,r6                    ; past the next page's first? then on
    add a,#(sdir+1)&0FFh
    movp a,@a
    cpl a
    inc a
    add a,r5                    ; id - first carries: id >= first
    jnc sf_got
    inc r6
    jmp sf_l
sf_got:
    mov a,r6
    add a,#sdir&0FFh
    movp a,@a
    cpl a
    inc a
    add a,r5                    ; id - first: its slot after the stub
    add a,#2
    ; fall into sg_get

; sg_get: A = the byte at offset A of data page r6, read by its stub.
; Clobbers r1.
sg_get:
    mov r1,a
    mov a,r6
    add a,#sg_tab&0FFh
    jmpp @a
sg_tab:
    db sg_a&0FFh, sg_b&0FFh, sg_c&0FFh, sg_d&0FFh
sg_a:
    mov a,r1
    jmp 0A00h
sg_b:
    mov a,r1
    jmp 0B00h
sg_c:
    mov a,r1
    jmp 0C00h
sg_d:
    mov a,r1
    jmp 0D00h

; sv_copy: the line at r6:r4 into LINEBUF, '%' the number; r0 its end, r4
; the next line's start, A = 1 if it was the last (0 not). Clobbers r1-r3,
; r5, r7, F0.
sv_copy:
    mov r0,#LINEBUF
svc_l:
    mov a,r4
    inc r4
    call sg_get
    mov r1,a
    add a,#256-2                ; 0 or 1: the line's end
    jnc svc_x
    mov a,r1
    xrl a,#'%'
    jz svc_num
    mov a,r1
    movx @r0,a
    inc r0
    jmp svc_l
svc_num:
    call sv_num
    jmp svc_l
svc_x:
    mov a,r1
    ret

; sv_num: SAYA's number in decimal at r0, no leading zeros; r0 past it.
; Clobbers A, r1-r3, r5, r7, F0.
sv_num:
    mov r1,#SAYA+SAY_NUM
    movx a,@r1
    mov r2,a
    inc r1
    movx a,@r1
    mov r3,a
    clr f0                      ; F0: a digit is out
    mov r7,#pow10&0FFh
sn_dig:
    mov r1,#'0'
sn_sub:
    mov a,r7                    ; r3:r2 + -10^k: no carry, the digit is done
    movp a,@a
    add a,r2
    mov r5,a
    mov a,r7
    inc a
    movp a,@a
    addc a,r3
    jnc sn_put
    mov r3,a
    mov a,r5
    mov r2,a
    inc r1
    jmp sn_sub
sn_put:
    mov a,r1
    xrl a,#'0'
    jnz sn_out
    jf0 sn_out
    jmp sn_next
sn_out:
    clr f0
    cpl f0
    mov a,r1
    movx @r0,a
    inc r0
sn_next:
    inc r7
    inc r7
    mov a,r7
    xrl a,#(pow10+8)&0FFh
    jnz sn_dig
    mov a,r2                    ; the units, always
    add a,#'0'
    movx @r0,a
    inc r0
    ret
pow10:
    db 0F0h,0D8h, 018h,0FCh, 09Ch,0FFh, 0F6h,0FFh   ; -10000 -1000 -100 -10

; sv_center: the text in LINEBUF (r0 its end) moved right to the middle.
; Clobbers A, r0-r3.
sv_center:
    mov a,r0                    ; (LINEBUF is 0: r0 is the length)
    cpl a
    inc a
    add a,#TXT_LEN
    clr c
    rrc a                       ; the shift: (16 - length) / 2
    jz sct_x
    mov r2,a
    mov a,r0
    jz sct_x
    mov r3,a                    ; the characters to move, from the last
    dec r0
    mov a,r0
    add a,r2
    mov r1,a
sct_l:
    movx a,@r0
    movx @r1,a
    mov a,#' '
    movx @r0,a
    dec r0
    dec r1
    djnz r3,sct_l
sct_x:
    ret

    if STRBANK=0
    include "strdir0.inc"
    elseif STRBANK=1
    include "strdir1.inc"
    elseif STRBANK=2
    include "strdir2.inc"
    endif

; ===========================================================================
    org 900h
    include "disp.inc"

    if STRBANK=0
    include "strdat0.inc"
    elseif STRBANK=1
    include "strdat1.inc"
    elseif STRBANK=2
    include "strdat2.inc"
    endif

; ===========================================================================
    org 0E00h

; say_put: LINEBUF onto SAYA's line, in its colour. Clobbers A, r0-r7.
say_put:
    mov r0,#SAYA+SAY_COL
    movx a,@r0
    mov r0,#TXCOL
    movx @r0,a
    inc r0
    movx @r0,a
    mov r0,#SAYA+SAY_LN
    movx a,@r0
    anl a,#0Fh
    mov r4,a
    rl a
    add a,r4                    ; *3
    add a,#slines&0FFh
    mov r6,a
    movp a,@a
    mov r3,a
    inc r6
    mov a,r6
    movp a,@a
    mov r2,a
    inc r6
    mov a,r6
    movp a,@a
    mov r4,a
    jmp tx_build

slines:                         ; L_*: the table, its page, Y
    db T0&0FFh, T0>>8, 30       ; 0-5 the text screen
    db T1&0FFh, T1>>8, 66
    db T2&0FFh, T2>>8, 102
    db T3&0FFh, T3>>8, 138
    db T4&0FFh, T4>>8, 174
    db T5&0FFh, T5>>8, 210
    db T0&0FFh, T0>>8, 30       ; 6-8 the map's title and messages (an
    db T1&0FFh, T1>>8, 188      ; auction's title, labels and message)
    db T2&0FFh, T2>>8, 226
    db T3&0FFh, T3>>8, 66       ; 9 an auction's second line

    PAGE_TAIL
