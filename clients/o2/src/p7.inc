; p7.inc -- page 7: the status summary and game over (mekkogx status.c
; summary_*, gameover_*), and a good's status before its auction
; (auction.c au_status_*), on the text screen.
;
;   summary                     game over
;   0  STATUS 3      LAND       FIRST FOUNDER
;   1  1 ALICE       8640           ALICE        (in its colour)
;   ..                          (the colony's rating, four lines)
;   5  COLONY       25600
;      FIRE: GO ON              (every other second, until we are ready)
;
; A line a seat, by rank, in its colour. The stick steps the summary's
; column (total, money, land, goods); at game over the button swaps the
; founder and the final table.

E_STATE equ 0
E_TICK  equ 1

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp on_state
e_tick:
    jmp tick

    include "sayq.inc"

; su_title: STATUS <round>, the column's name at the right.
su_title:
    call lb_clear
    mov a,#s_status&0FFh
    call lb_str
    inc r0
    mov r1,#RND
    movx a,@r1
    call lb_dec2
    mov r0,#SU_COL
    movx a,@r0
    add a,#colnames&0FFh
    movp a,@a
    mov r0,#LINEBUF+11
    call lb_at
    mov r4,#0
    jmp say_line
colnames:
    db s_total&0FFh, s_money&0FFh, s_land&0FFh, s_goods&0FFh

; su_line: line 1+TMP0: the TMP0th seat by rank, its name and its value in
; the column, in its colour.
su_line:
    call lb_clear
    mov r0,#TMP0
    movx a,@r0
    mov r5,a
    add a,#'1'
    mov r0,#LINEBUF
    movx @r0,a
    mov a,r5
    add a,#SU_ORD
    mov r0,a
    movx a,@r0
    mov r0,#SU_K
    movx @r0,a
    call pl_off
    mov r1,#LINEBUF+2
    mov r4,#8
sul_n:
    call st_byte
    jz sul_v
    movx @r1,a
    inc r1
    inc r2
    djnz r4,sul_n
sul_v:
    call su_value
    call lb_num
    call su_colour
    mov r0,#TMP0
    movx a,@r0
    inc a
    mov r4,a
    jmp say_line_c

; su_last: line 5: the colony's total; on the summary, until we are ready,
; every other second the prompt instead.
su_last:
    call lb_clear
    mov r0,#PHS
    movx a,@r0
    xrl a,#PHASE_SUMMARY
    jnz sx_col
    mov r0,#MYSEAT
    movx a,@r0
    jb7 sx_col
    call my_flags
    anl a,#PF_READY
    jnz sx_col
    mov r1,#FRAMES
    mov a,@r1
    anl a,#40h
    jz sx_col
    mov a,#s_goon&0FFh
    mov r4,#5
    jmp say
sx_col:
    mov a,#s_colony&0FFh
    call lb_str
    mov r3,#(ST_COLONY+CO_TOTAL)>>8
    mov r2,#(ST_COLONY+CO_TOTAL)&0FFh
    mov r1,#NUMV
    mov r4,#3
sx_b:
    call st_byte
    movx @r1,a
    inc r1
    inc r2
    djnz r4,sx_b
    call lb_num
    mov r4,#5
    jmp say_line

; sv_goods: NUMV = seat SU_K's goods at the store's prices: units (8 bits)
; times price (16), shifted and added. Clobbers A, r0-r7.
sv_goods:
    clr a
    mov r0,#NUMV
    movx @r0,a
    inc r0
    movx @r0,a
    mov r7,#0                   ; the good
sg_l:
    mov r0,#SU_K
    movx a,@r0
    call pl_off
    mov a,r2
    add a,#PL_GOODS
    add a,r7
    mov r2,a
    call st_byte
    mov r4,a                    ; units
    mov a,r7
    rl a
    add a,#(ST_STORE+STO_PRICE)&0FFh
    mov r2,a
    mov r3,#(ST_STORE+STO_PRICE)>>8
    call st_byte
    mov r5,a                    ; r3:r1:r5 = the price, shifted up a bit a
    inc r2                      ; turn
    call st_byte
    mov r1,a
    mov r3,#0
    mov r2,#8
sm_l:
    mov a,r4
    rrc a
    mov r4,a
    jnc sm_sh
    mov r0,#NUMV
    movx a,@r0
    add a,r5
    movx @r0,a
    inc r0
    movx a,@r0
    addc a,r1
    movx @r0,a
    inc r0
    movx a,@r0
    addc a,r3
    movx @r0,a
sm_sh:
    clr c
    mov a,r5
    rlc a
    mov r5,a
    mov a,r1
    rlc a
    mov r1,a
    mov a,r3
    rlc a
    mov r3,a
    djnz r2,sm_l
    inc r7
    mov a,r7
    xrl a,#4
    jnz sg_l
    ret

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

; tick: every frame. On the summary the button says we have seen it
; (every human has to) and the stick steps the column; at game over the
; button swaps the views.
tick:
    call tk_in
    ; fall into su_more

; su_more: the summary's next line, if a state's are not all drawn: the
; title (and the order), the four seats, the colony. A line a tick, so a
; press is never missed while a state draws. Clobbers A, r0-r7.
su_more:
    mov r0,#DRAWQ
    movx a,@r0
    mov r5,a
    add a,#256-6
    jc tk_x
    mov a,r5
    inc a
    movx @r0,a
    mov r0,#PHS
    movx a,@r0
    xrl a,#PHASE_SUMMARY
    jz sm_summ
    xrl a,#PHASE_SUMMARY!PHASE_GAMEOVER
    jz sm_summ
    mov a,r5
    jmp gd_line
sm_summ:
    mov a,r5
    jz sm_title
    xrl a,#5
    jz sm_last
    mov a,r5
    dec a
    mov r0,#TMP0
    movx @r0,a
    jmp su_line
sm_title:
    call su_order
    jmp su_title
sm_last:
    jmp su_last

; tk_in: the stick and the button. Clobbers A, r0, r5.
tk_in:
    call tk_cols
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz tk_x
    mov r0,#PHS
    movx a,@r0
    xrl a,#PHASE_GAMEOVER
    jz tk_over
    mov r0,#MYSEAT
    movx a,@r0
    jb7 tk_x                    ; watching
    mov r0,#ACT
    mov a,#ACT_READY
    movx @r0,a
    mov a,#SND_SELECT
    FCALL beep
tk_x:
    ret
tk_over:
    mov r0,#SU_VIEW
    movx a,@r0
    xrl a,#1
    movx @r0,a
    ret

; su_colour: TXCOL and TXCOL2 = seat SU_K's colour. Clobbers A, r0.
su_colour:
    mov r0,#SU_K
    movx a,@r0
    add a,#seatcol&0FFh
    movp a,@a
    mov r0,#TXCOL
    movx @r0,a
    inc r0
    movx @r0,a
    ret

seatcol:
    db C_RED, C_GREEN, C_TAN, C_VIOLET

; gd_need: r4 = what seat TMP0 needs of good SU_COL next round (auction.c
; au_need): food by the round, 3 to 5 (none after 12); energy one for each
; of its M.U.L.E.s making something else. Clobbers A, r0, r2, r3, r6, r7.
gd_need:
    mov r4,#0
    mov r0,#SU_COL
    movx a,@r0
    jz gn_food
    xrl a,#GOOD_ENERGY
    jnz gn_x
    mov r7,#0
gn_p:
    mov a,r7
    add a,#(ST_MAP+MAP_OWNER)&0FFh
    mov r2,a
    clr a
    addc a,#(ST_MAP+MAP_OWNER)>>8
    mov r3,a
    call st_byte
    mov r6,a
    mov r0,#TMP0
    movx a,@r0
    xrl a,r6
    jnz gn_n
    mov a,r7
    add a,#(ST_MAP+MAP_MULE)&0FFh
    mov r2,a
    clr a
    addc a,#(ST_MAP+MAP_MULE)>>8
    mov r3,a
    call st_byte
    jb7 gn_n                    ; none
    xrl a,#GOOD_ENERGY
    jz gn_n
    inc r4
gn_n:
    inc r7
    mov a,r7
    xrl a,#NUM_PLOTS
    jnz gn_p
gn_x:
    ret
gn_food:
    mov r0,#RND
    movx a,@r0
    inc a                       ; next round
    mov r4,#3
    add a,#256-13
    jc gn_none
    add a,#13-4
    jnc gn_x
    inc r4
    add a,#256-4
    jnc gn_x
    inc r4
    ret
gn_none:
    mov r4,#0
    ret

; ===========================================================================
    org 0B00h
    include "text.inc"
s_status: db "STATUS",0
s_total:  db "TOTAL",0
s_money:  db "MONEY",0
s_land:   db "LAND",0
s_goods:  db "GOODS",0
s_colony: db "COLONY",0
s_goon:   db "FIRE: GO ON",0
s_found:  db "FIRST FOUNDER",0

; gd_line: line A (2-5) of a good's status: seat A-2's name, what it is
; short of (-) or over (+) what it needs next round, and its units, in its
; colour. (SU_COL holds the good while its status is up.) Clobbers A,
; r0-r7.
gd_line:
    add a,#256-2
    mov r0,#TMP0
    movx @r0,a
    call lb_clear
    mov r0,#TMP0
    movx a,@r0
    call pl_off
    mov r1,#LINEBUF
    mov r4,#8
gl_n:
    call st_byte
    jz gl_u
    movx @r1,a
    inc r1
    inc r2
    djnz r4,gl_n
gl_u:
    mov r0,#TMP0                ; its units
    movx a,@r0
    call pl_off
    mov r0,#SU_COL
    movx a,@r0
    add a,#PL_GOODS
    add a,r2
    mov r2,a
    call st_byte
    mov r0,#NUMV
    movx @r0,a
    mov r5,a
    clr a
    inc r0
    movx @r0,a
    inc r0
    movx @r0,a
    call gd_need                ; r4 = its need next round
    mov a,r4
    jz gl_num
    mov r0,#LINEBUF+9
    cpl a                       ; units - need
    inc a
    add a,r5
    jb7 gl_neg
    mov r6,a
    mov a,#'+'
    jmp gl_s
gl_neg:
    cpl a
    inc a
    mov r6,a
    mov a,#'-'
gl_s:
    movx @r0,a
    inc r0
    mov a,r6
    call lb_dec2
gl_num:
    call lb_num
    mov r0,#TMP0                ; in the seat's colour
    movx a,@r0
    mov r0,#SU_K
    movx @r0,a
    call su_colour
    mov r0,#TMP0
    movx a,@r0
    add a,#2
    mov r4,a
    jmp say_line_c

; tk_cols: the stick's left and right step the summary's column (shown
; with the next state). Clobbers A, r0, r5.
tk_cols:
    mov r0,#JOYEV
    movx a,@r0
    mov r5,a
    mov r0,#SU_COL
    anl a,#J_RIGHT
    jz tc_l
    movx a,@r0
    inc a
    jmp tc_put
tc_l:
    mov a,r5
    anl a,#J_LEFT
    jz tc_x
    movx a,@r0
    dec a
tc_put:
    anl a,#3
    movx @r0,a
tc_x:
    ret

; ===========================================================================
    org 0C00h
    include "screen.inc"

; on_state: a state has come. A new phase gets a fresh screen, the first
; column and the founder.
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
    call su_cue
    clr a
    mov r0,#SU_COL
    movx @r0,a
    mov r0,#SU_VIEW
    movx @r0,a
    mov r0,#SU_DRAWN
    mov a,#0FFh
    movx @r0,a
os_same:
    mov r0,#PHS
    movx a,@r0
    xrl a,#PHASE_GAMEOVER
    jz os_over
    xrl a,#PHASE_GAMEOVER!PHASE_SUMMARY
    jnz os_good
    clr a                       ; the summary: drawn by the ticks
    mov r0,#DRAWQ
    movx @r0,a
    ret
os_good:
    jmp gd_state
os_over:
    jmp gameover

; gd_state: a good's status: its name and the store's stock said once (the
; first state of it: SU_DRAWN none), each seat's line by the ticks.
gd_state:
    mov r0,#DRAWQ               ; lines 2-5
    mov a,#2
    movx @r0,a
    mov r0,#SU_DRAWN
    movx a,@r0
    jz gs_x
    clr a
    movx @r0,a
    STB ST_AUCTION+AU_GOOD
    anl a,#3
    mov r0,#SU_COL              ; (the good, for gd_line)
    movx @r0,a
    add a,#S_GOOD0
    mov r5,a
    mov r6,#L_TXT|(1<<4)
    mov r7,#C_CYAN
    call say_q
    mov r0,#SU_COL
    movx a,@r0
    add a,#(ST_STORE+STO_STOCK)&0FFh
    mov r2,a
    mov r3,#(ST_STORE+STO_STOCK)>>8
    call st_byte
    mov r2,a
    mov r3,#0
    mov r5,#M_STORE
    jnz gs_say
    mov r5,#M_STORE0
gs_say:
    mov r6,#(L_TXT+1)|(1<<4)
    mov r7,#C_WHITE
    jmp say_q
gs_x:
    ret

; ===========================================================================
    org 0D00h

; su_value: NUMV = seat SU_K's value in column SU_COL: its score, its money,
; its land (mekkogx su_land_of: 500 a plot, and 25 (good + 1) + 35 for its
; M.U.L.E.), its goods at the store's prices. Clobbers A, r0-r7.
su_value:
    clr a
    mov r0,#NUMV+2
    movx @r0,a
    mov r0,#SU_COL
    movx a,@r0
    mov r5,#PL_SCORE
    jz sv_16
    dec a
    mov r5,#PL_MONEY
    jz sv_16
    dec a
    jz sv_land
    jmp sv_goods
sv_16:
    mov r0,#SU_K
    movx a,@r0
    call pl_off
    mov a,r2
    add a,r5
    mov r2,a
    call st_byte
    mov r1,#NUMV
    movx @r1,a
    inc r2
    call st_byte
    inc r1
    movx @r1,a
    ret

sv_land:
    clr a
    mov r0,#NUMV
    movx @r0,a
    inc r0
    movx @r0,a
    mov r7,#0                   ; the plot
sv_lp:
    mov a,r7
    add a,#(ST_MAP+MAP_OWNER)&0FFh
    mov r2,a
    clr a
    addc a,#(ST_MAP+MAP_OWNER)>>8
    mov r3,a
    call st_byte
    mov r5,a
    mov r0,#SU_K
    movx a,@r0
    xrl a,r5
    jnz sv_ln
    mov r4,#0F4h                ; 500
    mov r5,#01h
    mov a,r7
    add a,#(ST_MAP+MAP_MULE)&0FFh
    mov r2,a
    clr a
    addc a,#(ST_MAP+MAP_MULE)>>8
    mov r3,a
    call st_byte
    jb7 sv_la
    anl a,#3
    add a,#muleval&0FFh
    movp a,@a
    add a,r4
    mov r4,a
    clr a
    addc a,r5
    mov r5,a
sv_la:
    mov r0,#NUMV
    movx a,@r0
    add a,r4
    movx @r0,a
    inc r0
    movx a,@r0
    addc a,r5
    movx @r0,a
    inc r0
    movx a,@r0
    addc a,#0
    movx @r0,a
sv_ln:
    inc r7
    mov a,r7
    xrl a,#NUM_PLOTS
    jnz sv_lp
    ret
muleval:
    db 60, 85, 110, 135

; su_order: SU_ORD = the seats by rank, 1 first (in seat order where two
; tie); seat order if the ranks don't make four. Clobbers A, r0-r7.
su_order:
    mov r5,#1
    mov r4,#SU_ORD
so_k:
    mov r7,#0
so_s:
    mov a,r7
    call pl_off
    mov a,r2
    add a,#PL_RANK
    mov r2,a
    call st_byte
    xrl a,r5
    jnz so_n
    mov a,r4
    mov r0,a
    mov a,r7
    movx @r0,a
    inc r4
so_n:
    inc r7
    mov a,r7
    xrl a,#4
    jnz so_s
    inc r5
    mov a,r5
    xrl a,#5
    jnz so_k
    mov a,r4
    xrl a,#SU_ORD+4
    jz so_x
    mov r0,#SU_ORD
    clr a
so_id:
    movx @r0,a
    inc r0
    inc a
    xrl a,#4
    jz so_x
    xrl a,#4
    jmp so_id
so_x:
    ret

; go_late: A = 1 once the phase is 540 jiffies old ($21C), else 0.
; Clobbers r0.
go_late:
    mov r0,#HPE+1
    movx a,@r0
    add a,#256-2
    jnc gl_no
    jnz gl_yes
    mov r0,#HPE
    movx a,@r0
    add a,#256-1Ch
    jc gl_yes
gl_no:
    clr a
    ret
gl_yes:
    mov a,#1
    ret

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
    org 0E00h

; gameover: FIRST FOUNDER, the winner and the colony's rating for the first
; 540 jiffies (mekkogx gameover_tick), then the final table; the button
; swaps them. The founder's lines are drawn once; the rating's by the say
; queue. Clobbers A, r0-r7.
gameover:
    call su_order
    call go_late
    mov r5,a
    mov r0,#SU_VIEW
    movx a,@r0
    xrl a,r5
    jnz go_table
    mov r0,#SU_DRAWN
    movx a,@r0
    jz go_x                     ; the founder is up
    clr a
    movx @r0,a
    mov r0,#DRAWQ               ; (no more of the table)
    mov a,#6
    movx @r0,a
    call lb_clear
    mov a,#s_found&0FFh
    mov r0,#LINEBUF+1
    call lb_at
    mov r4,#0
    call say_line
    call lb_clear               ; the winner, in its colour
    mov r0,#SU_ORD
    movx a,@r0
    mov r0,#SU_K
    movx @r0,a
    call pl_off
    mov r1,#LINEBUF+4
    mov r4,#8
gn_l:
    call st_byte
    jz gn_c
    movx @r1,a
    inc r1
    inc r2
    djnz r4,gn_l
gn_c:
    call su_colour
    mov r4,#1
    call say_line_c
    STB ST_COLONY+CO_RATING     ; the rating, 0-6, on the four lines left
    add a,#256-7
    jc go_x
    add a,#7+R_0
    mov r5,a
    mov r6,#(L_TXT+2)|(4<<4)
    mov r7,#C_WHITE
    jmp say_q
go_table:
    mov r0,#SU_DRAWN            ; the table: drawn by the ticks
    mov a,#1
    movx @r0,a
    clr a
    mov r0,#DRAWQ
    movx @r0,a
    ret
go_x:
    ret

; lb_num: NUMV (24 bits) in decimal, right-aligned to end at LINEBUF+15,
; no leading zeros (seven columns: from LINEBUF+9). NUMV is used up.
; Clobbers A, r0-r7, F0.
lb_num:
    mov r0,#LINEBUF+9
    mov r7,#pw24&0FFh
    clr f0                      ; F0: a digit is out
ln_dig:
    mov r6,#'0'
ln_sub:
    mov r1,#NUMV                ; NUMV + -10^k: no carry, the digit is done
    mov a,r7
    movp a,@a
    mov r5,a
    movx a,@r1
    add a,r5
    mov r2,a
    inc r1
    mov a,r7
    inc a
    movp a,@a
    mov r5,a
    movx a,@r1
    addc a,r5
    mov r3,a
    inc r1
    mov a,r7
    inc a
    inc a
    movp a,@a
    mov r5,a
    movx a,@r1
    addc a,r5
    jnc ln_put
    movx @r1,a
    dec r1
    mov a,r3
    movx @r1,a
    dec r1
    mov a,r2
    movx @r1,a
    inc r6
    jmp ln_sub
ln_put:
    mov a,r6
    xrl a,#'0'
    jnz ln_out
    jf0 ln_out
    jmp ln_next                 ; (a leading zero: the blank stays)
ln_out:
    clr f0
    cpl f0
    mov a,r6
    movx @r0,a
ln_next:
    inc r0
    mov a,r7
    add a,#3
    mov r7,a
    xrl a,#(pw24+18)&0FFh
    jnz ln_dig
    mov r1,#NUMV                ; the units, always
    movx a,@r1
    add a,#'0'
    movx @r0,a
    ret
pw24:                           ; -10^6 .. -10^1, 24 bits, low byte first
    db 0C0h,0BDh,0F0h, 060h,079h,0FEh, 0F0h,0D8h,0FFh
    db 018h,0FCh,0FFh, 09Ch,0FFh,0FFh, 0F6h,0FFh,0FFh

; su_cue: a new phase's sound (mekkogx status.c): the summary's ALERT, if
; it waits on our button; game over's FOUNDER. Clobbers A, r0, r1.
su_cue:
    mov r0,#PHS
    movx a,@r0
    xrl a,#PHASE_GAMEOVER
    jz suc_go
    xrl a,#PHASE_GAMEOVER!PHASE_SUMMARY
    jnz suc_x
    mov r0,#MYSEAT
    movx a,@r0
    jb7 suc_x
    mov a,#SND_ALERT
    jmp suc_b
suc_go:
    mov a,#SND_FOUNDER
suc_b:
    FCALL beep
suc_x:
    ret

    PAGE_TAIL
