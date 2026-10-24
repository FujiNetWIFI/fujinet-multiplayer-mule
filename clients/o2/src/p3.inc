; p3.inc -- page 3: the lobby (mekkogx status.c lobby_*), on the text
; screen. Phases without a page of their own yet show their number here.
;
;   0  LOBBY IRATA
;   1  1 ALICE      READY       a seat a line, in the seat's colour
;   ..
;   5  FIRE: READY              the button toggles our ready; STARTING IN 12
;                               once the countdown runs

E_STATE equ 0
E_TICK  equ 1

; Seat colours: red, green, tan, violet (blue is the river's).
SEATCOL macro
    db C_RED, C_GREEN, C_TAN, C_VIOLET
    endm

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp on_state
e_tick:
    jmp tick

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "stbyte.inc"

; my_flags: A = our seat's flags (0 watching). Clobbers A, r0, r2, r3, r6, r7.
my_flags:
    mov r0,#MYSEAT
    movx a,@r0
    jb7 mf_none
    call pl_off
    mov a,r2
    add a,#PL_FLAGS
    mov r2,a
    jmp st_byte
mf_none:
    clr a
    ret

; pl_off: r3:r2 = the offset of player A in the state. Clobbers A, r6.
pl_off:
    mov r6,a                    ; 29 * A = 32A - 2A - A
    swap a
    rl a
    anl a,#0E0h                 ; 32A
    xch a,r6
    mov r2,a                    ; A
    rl a                        ; 2A
    add a,r2                    ; 3A
    cpl a
    inc a
    add a,r6                    ; 29A
    add a,#ST_PLAYERS
    mov r2,a
    mov r3,#0
    ret

; tick: every frame. In the lobby the button toggles our ready, then a line
; of the lobby is drawn; in the development (until it has a page) the
; button begins our turn, which then runs out.
tick:
    mov r0,#PHS
    movx a,@r0
    jnz tk_dev
    call tk_lobby
    jmp lb_more
tk_dev:
    xrl a,#PHASE_DEVELOP
    jnz tk_x
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz tk_x
    mov a,#ACT_BEGIN
    jmp tk_send
tk_lobby:
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz tk_x
    mov r0,#MYSEAT
    movx a,@r0
    jb7 tk_x                    ; watching
    call my_flags
    anl a,#PF_READY
    jnz tk_unr
    mov a,#ACT_READY
    jmp tk_send
tk_unr:
    mov a,#ACT_UNRDY
tk_send:
    mov r0,#ACT
    movx @r0,a
    mov a,#SND_SELECT
    FCALL beep
tk_x:
    ret

; ===========================================================================
    org 0B00h
    include "text.inc"
s_lobby:  db "LOBBY",0
s_ready:  db "FIRE: READY",0
s_unrdy:  db "FIRE: NOT READY",0
s_rdy:    db "READY",0
s_cpu:    db "CPU",0
s_open:   db "---",0
s_phase:  db "PHASE",0
s_startn: db "STARTING IN",0
s_watch:  db "WATCHING GAME",0

; ===========================================================================
    org 0C00h
    include "screen.inc"

; lb_secs: LINEBUF = STARTING IN <the lobby's countdown in seconds, up>.
; Clobbers A, r0-r3, r6.
lb_secs:
    call lb_clear
    mov a,#s_startn&0FFh
    call lb_str
    inc r0
    mov r1,#HCD                 ; (countdown - 1) / 60 + 1
    movx a,@r1
    add a,#0FFh
    mov r2,a
    inc r1
    movx a,@r1
    addc a,#0FFh
    mov r3,a
    clr a
    mov r6,a
ls_l:
    inc r6
    mov a,r2
    add a,#256-60
    mov r2,a
    mov a,r3
    addc a,#0FFh
    mov r3,a
    jc ls_l
    mov a,r6
    ; fall into lb_dec2

; lb_dec2: A (0-99) in decimal at r0, one or two digits; r0 past them.
; Clobbers A, r6.
lb_dec2:
    mov r6,#'0'
ld2_l:
    add a,#256-10
    jnc ld2_u
    inc r6
    jmp ld2_l
ld2_u:
    add a,#10+'0'
    xch a,r6
    xrl a,#'0'
    jz ld2_one
    xrl a,#'0'
    movx @r0,a
    inc r0
ld2_one:
    mov a,r6
    movx @r0,a
    inc r0
    ret

; ===========================================================================
    org 0D00h

; on_state: a state has come. A new phase gets a fresh screen.
on_state:
    mov r0,#PHS
    movx a,@r0
    mov r6,a
    mov r0,#LASTPH
    movx a,@r0
    xrl a,r6
    jz os_same
    mov a,r6
    movx @r0,a
    call scr_text
os_same:
    mov r0,#PHS
    movx a,@r0
    jz os_lobby
    ; not ours: the phase's number
    call lb_clear
    mov a,#s_phase&0FFh
    call lb_str
    mov r0,#PHS
    movx a,@r0
    add a,#'0'
    mov r0,#LINEBUF+6
    movx @r0,a
    mov r4,#2
    jmp say_line
os_lobby:
    mov r0,#DRAWQ               ; drawn by the ticks
    movx @r0,a
    ret

; lb_more: the lobby's next line, if a state's are not all drawn: the
; table, the four seats, the prompt. A line a tick, so a press is never
; missed while a state draws. Clobbers A, r0-r7.
lb_more:
    mov r0,#DRAWQ
    movx a,@r0
    mov r5,a
    add a,#256-6
    jc lm_x
    mov a,r5
    inc a
    movx @r0,a
    mov a,r5
    jz lobby
    xrl a,#5
    jz lb_prompt
    mov a,r5
    dec a
    mov r0,#TMP0
    movx @r0,a
    jmp seat_line
lm_x:
    ret

; lobby: line 0, LOBBY and the table's name.
lobby:
    call lb_clear               ; LOBBY <table>   START n
    mov a,#s_lobby&0FFh
    call lb_str
    mov r1,#0
lb_t:
    mov a,r1                    ; SCTABL[i] to LINEBUF[6+i]
    add a,#SCTABL
    mov r0,a
    movx a,@r0
    jz lb_td
    mov r7,a
    mov a,r1
    add a,#LINEBUF+6
    mov r0,a
    mov a,r7
    movx @r0,a
    inc r1
    mov a,r1
    xrl a,#8
    jnz lb_t
lb_td:
    mov r4,#0
    jmp say_line
lb_prompt:
    mov r0,#MYSEAT
    movx a,@r0
    jb7 lb_watch
    mov r0,#HCD+1               ; the countdown: STARTING IN <seconds>
    movx a,@r0
    jb7 lb_rdy                  ; -1: not running
    call lb_secs
    mov r4,#5
    jmp say_line
lb_rdy:
    call my_flags               ; ready? the prompt says the other
    anl a,#PF_READY
    mov r4,#5
    mov r5,#s_ready&0FFh
    jz lb_say
    mov r5,#s_unrdy&0FFh
lb_say:
    mov a,r5
    jmp say
lb_watch:
    mov r4,#5
    mov a,#s_watch&0FFh
    jmp say

; seat_line: line 1+TMP0 for seat TMP0: its number, its name, READY or CPU
; or nothing; "---" for an empty seat. In the seat's colour.
seat_line:
    call lb_clear
    mov r0,#TMP0
    movx a,@r0
    mov r5,a
    add a,#'1'
    mov r0,#LINEBUF
    movx @r0,a
    mov a,r5
    call pl_off
    call st_byte                ; the name's first letter: none, no player
    jnz sl_name
    mov a,#s_open&0FFh
    mov r0,#LINEBUF+2
    call lb_at
    jmp sl_col
sl_name:
    mov r1,#LINEBUF+2
    mov r4,#8
sl_n:
    call st_byte
    jz sl_flags
    movx @r1,a
    inc r1
    inc r2
    djnz r4,sl_n
sl_flags:
    mov r0,#TMP0
    movx a,@r0
    call pl_off
    mov a,r2
    add a,#PL_FLAGS
    mov r2,a
    call st_byte
    mov r6,a
    anl a,#PF_AI
    jnz sl_cpu
    mov a,r6
    anl a,#PF_READY
    jz sl_col
    mov a,#s_rdy&0FFh
    mov r0,#LINEBUF+11
    call lb_at
    jmp sl_col
sl_cpu:
    mov a,#s_cpu&0FFh
    mov r0,#LINEBUF+11
    call lb_at
sl_col:
    mov r0,#TMP0
    movx a,@r0
    add a,#seatcol&0FFh
    movp a,@a
    mov r0,#TXCOL
    movx @r0,a
    inc r0
    movx @r0,a
    mov r0,#TMP0
    movx a,@r0
    inc a
    mov r4,a
    jmp say_line_c

seatcol:
    SEATCOL

    PAGE_TAIL
