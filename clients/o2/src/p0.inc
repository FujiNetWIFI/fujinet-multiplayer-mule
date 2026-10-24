; p0.inc -- page 0: the boot (mekkogx boot.c). The cart, who we are, which
; table; then the game's pages.
;
; The FujiNet lobby hands a game its details in appkeys (creator 1, app 1):
;   key 0    the player's name (shared by every FujiNet game)
;   key 10   the table's URL, "http://host/?table=<id>"
; Without them we ask for a name and list the build's server's tables.
;
; The top level runs at stack depth 0: it is where the farcalls into the
; network page are made (tools/checkstack.py).

PG_MAIN equ 2                   ; the game's main loop, entered one way
PG_NET  equ 1                   ; the network page's entries
E_NBEGIN equ 0
E_NSTEP equ 1

    PAGE_HEAD
    db e_boot&0FFh              ; entry 0: the boot (fixed.inc start)
e_boot:
    jmp boot

; ak_write_name: key 0 = SCNAME. Best effort. Clobbers A, r0-r7.
ak_write_name:
    mov r5,#0
    mov r1,#1
    call ak_open
    mov r2,#FUJI_DEVICEID
    mov r3,#FC_AKWRITE
    mov r4,#0
    call fn_begin
    mov r1,#SCNAME
akw_l:
    mov a,r1
    mov r0,a
    movx a,@r0
    jz akw_go
    call fn_tx
    inc r1
    jmp akw_l
akw_go:
    jmp fn_go

; up_alnum: A upper-cased if a letter; 0 if not a letter or a digit.
; Clobbers r6.
up_alnum:
    mov r6,a
    add a,#256-'a'              ; 'a'..'z': up
    jnc ua_1
    add a,#256-26
    jc ua_1
    mov a,r6
    add a,#256-32
    ret
ua_1:
    mov a,r6                    ; 'A'..'Z'
    add a,#256-'A'
    jnc ua_2
    add a,#256-26
    jnc ua_ok
ua_2:
    mov a,r6                    ; '0'..'9'
    add a,#256-'0'
    jnc ua_no
    add a,#256-10
    jnc ua_ok
ua_no:
    clr a
    ret
ua_ok:
    mov a,r6
    ret

; tbl_show: the table list in the reply (a count, then 33-byte MuleTables)
; on lines 1-5: name and players. NTBL = how many (at most 5).
; Clobbers A, r0-r7.
tbl_show:
    clr a
    call rx_win
    clr a
    call rx_get
    mov r6,a
    add a,#256-6
    jnc ts_n
    mov r6,#5
ts_n:
    mov a,r6
    mov r0,#NTBL
    movx @r0,a
    clr a
ts_l:
    mov r0,#TMP0                ; (tbl_line keeps no register)
    movx @r0,a
    mov r6,a
    mov r0,#NTBL
    movx a,@r0
    xrl a,r6
    jz ts_x
    mov a,r6
    call tbl_line
    mov r0,#TMP0
    movx a,@r0
    inc a
    jmp ts_l
ts_x:
    ret

; tbl_line: line 1+A = table A: ' ', its name (9), ' ', its players (5);
; '>' first when it is SEL. Clobbers A, r0-r7.
tbl_line:
    mov r5,a
    call lb_clear
    mov a,r5
    call tbl_off                ; r3:r2 = 1 + 33*A + 8: the name
    mov r1,#LINEBUF+1
    mov r4,#9
tl_n:
    call tl_ch
    djnz r4,tl_n
    mov a,r2                    ; on to the players: 20 - 9 further
    add a,#11
    mov r2,a
    clr a
    addc a,r3
    mov r3,a
    mov r1,#LINEBUF+11
    mov r4,#5
tl_p:
    call tl_ch
    djnz r4,tl_p
    mov r0,#SEL
    movx a,@r0
    xrl a,r5
    jnz tl_put
    mov r0,#LINEBUF
    mov a,#'>'
    movx @r0,a
tl_put:
    mov a,r5
    inc a
    mov r4,a
    jmp say_line

; tl_ch: reply byte r3:r2 into LINEBUF at r1 (NUL as a space), both advanced.
tl_ch:
    call st_byte
    jnz tlc_1
    mov a,#' '
tlc_1:
    movx @r1,a
    inc r1
    inc r2
    mov a,r2
    jnz tlc_2
    inc r3
tlc_2:
    ret

; tbl_off: r3:r2 = 1 + 33*A + 8, a table's name in the reply.
tbl_off:
    mov r2,a                    ; 33A = 32A + A
    swap a
    rl a
    anl a,#0E0h
    add a,r2
    add a,#9
    mov r2,a
    mov r3,#0
    ret

; tbl_choose: the stick picks one of NTBL tables; SCTABL = its id.
; Clobbers A, r0-r7.
tbl_choose:
    clr a
    mov r0,#SEL
    movx @r0,a
tc_draw:
    call tbl_show
tc_l:
    call frame_wait
    call joy_poll
    mov r0,#JOYEV
    movx a,@r0
    jb4 tc_take
    jb0 tc_up
    jb1 tc_down
    jmp tc_l
tc_up:
    mov r0,#SEL
    movx a,@r0
    jz tc_l
    dec a
    movx @r0,a
    jmp tc_draw
tc_down:
    mov r0,#SEL
    movx a,@r0
    inc a
    mov r6,a
    mov r0,#NTBL
    movx a,@r0
    xrl a,r6
    jz tc_l
    mov a,r6
    mov r0,#SEL
    movx @r0,a
    jmp tc_draw
tc_take:
    mov r0,#SEL                 ; its id: 1 + 33*SEL, 8 bytes, NUL-padded
    movx a,@r0
    call tbl_off
    mov a,r2
    add a,#256-8
    mov r2,a
    mov r1,#SCTABL
    mov r4,#8
tc_id:
    call st_byte
    movx @r1,a
    inc r1
    inc r2
    djnz r4,tc_id
    clr a
    movx @r1,a
    ret

    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "mail.inc"

; fn_go: post and wait (the boot only: the game never waits). A = 0 if it
; worked and was ACKed. Clobbers A, r0, r2, r6.
fn_go:
    call fn_post
    mov r2,#0
fg_l:
    call fn_done
    jz fg_done
    djnz r2,fg_l
    mov a,#0FFh
    ret
fg_done:
    jmp fn_ack

; ak_open: OPEN_APPKEY key r5, mode r1 (0 read, 1 write): creator 1, app 1,
; no parameters, a 6-byte payload. A = 0 if it worked. Clobbers A, r0, r2-r4,
; r6, r7.
ak_open:
    mov r2,#FUJI_DEVICEID
    mov r3,#FC_AKOPEN
    mov r4,#0
    call fn_begin
    mov a,#1                    ; creator, LE
    call fn_tx
    clr a
    call fn_tx
    mov a,#1                    ; app
    call fn_tx
    mov a,r5                    ; key
    call fn_tx
    mov a,r1                    ; mode
    call fn_tx
    clr a                       ; reserved
    call fn_tx
    jmp fn_go

; ak_read: key r5 into the reply: its length (2 bytes), then the key. A = the
; length, at most 64; 0: none. Clobbers A, r0-r7.
ak_read:
    mov r1,#0
    call ak_open
    jnz ak_none
    mov r2,#FUJI_DEVICEID
    mov r3,#FC_AKREAD
    mov r4,#0
    call fn_begin
    call fn_go
    jnz ak_none
    clr a
    call rx_win
    mov a,#1
    call rx_get
    jnz ak_64
    clr a
    call rx_get
    mov r6,a
    add a,#256-65
    jc ak_64
    mov a,r6
    ret
ak_64:
    mov a,#64
    ret
ak_none:
    clr a
    ret

; ak_name: SCNAME from key 0. Only letters and digits go into a URL
; unescaped, so anything else means no name. Clobbers A, r0-r7.
ak_name:
    clr a
    mov r0,#SCNAME
    movx @r0,a
    mov r5,#0
    call ak_read
    jz akn_x
    mov r4,a                    ; the length, at most 8
    add a,#256-9
    jnc akn_ok
    mov r4,#8
akn_ok:
    mov r3,#0
akn_l:
    mov a,r3                    ; reply byte 2 + i
    add a,#2
    call rx_get
    call up_alnum
    jz akn_bad
    mov r6,a
    mov a,r3
    add a,#SCNAME
    mov r0,a
    mov a,r6
    movx @r0,a
    inc r3
    mov a,r3
    xrl a,r4
    jnz akn_l
    mov a,r3
    add a,#SCNAME
    mov r0,a
    clr a
    movx @r0,a
akn_x:
    ret
akn_bad:
    clr a
    mov r0,#SCNAME
    movx @r0,a
    ret

; ===========================================================================
    org 0B00h

; boot: the top level. Every farcall into the network page is made here.
boot:
    FCALL vdc_reset
    call scr_text
    call ints_on
    mov r4,#0
    mov a,#s_title&0FFh
    call say
    mov r4,#1
    mov a,#s_sub&0FFh
    call say
    mov r4,#5
    mov a,#s_fuji&0FFh
    call say
    mov r7,#120                 ; the title, two seconds
bt_ttl:
    call frame_wait
    djnz r7,bt_ttl

    mov a,#FN_R_MAGIC0&0FFh     ; a FujiNet cartridge, speaking protocol 2?
    call mb_get
    xrl a,#'F'
    jnz bt_nocart
    mov a,#FN_R_PROTO_VER&0FFh
    call mb_get
    xrl a,#FN_PROTO_VER
    jz bt_cart
bt_nocart:
    mov r4,#3
    mov a,#s_nocart&0FFh
    call say
bt_stop:
    jmp bt_stop

bt_cart:
    call ak_name
    mov r0,#SCNAME
    movx a,@r0
    jnz bt_named
    call pick_name
    call ak_write_name
bt_named:
    clr a                       ; no room appkey read yet: the build's
    mov r0,#SCTABL              ; server, and its tables
    movx @r0,a

; The table list.
bt_tables:
    call scr_clear
    mov r4,#0
    mov a,#s_tables&0FFh
    call say
    mov r4,#5
    mov a,#s_load&0FFh
    call say
    mov r0,#ACT
    mov a,#ACT_TABLS
    movx @r0,a
    mov r7,#PG_NET
    mov a,#E_NBEGIN
    FCALL farcall
bt_tw:
    call frame_wait
    mov r7,#PG_NET
    mov a,#E_NSTEP
    FCALL farcall
    mov r0,#NSTATE
    movx a,@r0
    jnz bt_tw
    mov r0,#NETHAVE
    movx a,@r0
    xrl a,#NH_TABLES
    jnz bt_none
    call tbl_show               ; NTBL = how many
    mov r0,#NTBL
    movx a,@r0
    jnz bt_pick
bt_none:
    mov r4,#5
    mov a,#s_notbl&0FFh
    call say
    call wait_fire
    jmp bt_tables
bt_pick:
    call tbl_choose             ; SCTABL

; Join: the first /state.
bt_join:
    call scr_clear
    mov r4,#2
    mov a,#s_join&0FFh
    call say
bt_jagain:
    mov r7,#PG_NET
    mov a,#E_NBEGIN
    FCALL farcall
bt_jw:
    call frame_wait
    mov r7,#PG_NET
    mov a,#E_NSTEP
    FCALL farcall
    mov r0,#NSTATE
    movx a,@r0
    jnz bt_jw
    mov r0,#NETHAVE
    movx a,@r0
    xrl a,#NH_STATE
    jnz bt_jagain               ; until it answers
    clr a                       ; the state is in: the game takes it from here
    mov r0,#NETHAVE
    mov a,#NH_STATE
    movx @r0,a
    mov r7,#0
    mov a,#PG_MAIN
    jmp far_go

; ints_on: the line counter and the vblank interrupt.
ints_on:
    clr a
    mov t,a
    strt cnt
    en tcnti
    en i
    ret

; wait_fire: until the button is pressed. Clobbers A, r0-r4.
wait_fire:
    call frame_wait
    call joy_poll
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz wait_fire
    ret

; ===========================================================================
    org 0C00h
    include "text.inc"

s_title:  db "    M.U.L.E.",0
s_sub:    db "   ODYSSEY 2",0
s_fuji:   db "    FUJINET",0
s_nocart: db "NO FUJINET CART",0
s_tables: db "CHOOSE A TABLE",0
s_load:   db "  LOADING...",0
s_notbl:  db "NO TABLES FIRE",0
s_join:   db "  JOINING...",0
s_yname:  db "  YOUR NAME?",0
s_pk1:    db "STICK: A LETTER",0
s_pk2:    db "FIRE TAKES IT",0
s_pk3:    db "TAKE . TO END",0

; ===========================================================================
    org 0D00h
    include "screen.inc"

; lb_name: SCNAME into a clear LINEBUF from column 4. Clobbers A, r0-r2, r6.
lb_name:
    call lb_clear
    mov r1,#0
lbn_l:
    mov a,r1
    add a,#SCNAME
    mov r0,a
    movx a,@r0
    jz lbn_x
    mov r6,a
    mov a,r1
    add a,#LINEBUF+4
    mov r0,a
    mov a,r6
    movx @r0,a
    inc r1
    mov a,r1
    xrl a,#8
    jnz lbn_l
lbn_x:
    ret

    include "input.inc"

; ===========================================================================
    org 0E00h

; pick_name: the stick's letter picker into SCNAME (mekkogx boot.c, no
; keyboard): up and down turn the letter, the button takes it, left rubs
; out; taking '.' ends. Clobbers A, r0-r7.
pick_name:
    call scr_clear
    mov r4,#0
    mov a,#s_yname&0FFh
    call say
    mov r4,#3
    mov a,#s_pk1&0FFh
    call say
    mov r4,#4
    mov a,#s_pk2&0FFh
    call say
    mov r4,#5
    mov a,#s_pk3&0FFh
    call say
    clr a
    mov r0,#SCNAME
    movx @r0,a
    mov r0,#NAMEN
    movx @r0,a
    mov r0,#PICK
    movx @r0,a
    mov r0,#REP
    movx @r0,a
pn_l:
    call frame_wait
    call joy_poll
    mov r0,#JOYEV
    movx a,@r0
    jb4 pn_fire
    jb2 pn_rub
    mov r0,#REP                 ; the stick held: turn the letter
    movx a,@r0
    jz pn_turn
    dec a
    movx @r0,a
    jmp pn_draw
pn_turn:
    mov r0,#JOY
    movx a,@r0
    jb0 pn_up
    jb1 pn_down
    jmp pn_draw
pn_up:
    mov r0,#PICK
    movx a,@r0
    jnz pn_u1
    mov a,#PICKN
pn_u1:
    dec a
    movx @r0,a
    jmp pn_rep
pn_down:
    mov r0,#PICK
    movx a,@r0
    inc a
    xrl a,#PICKN
    jz pn_d1
    xrl a,#PICKN
pn_d1:
    movx @r0,a
pn_rep:
    mov r0,#REP
    mov a,#8
    movx @r0,a
    jmp pn_draw
pn_rub:
    mov r0,#NAMEN
    movx a,@r0
    jz pn_draw
    dec a
    movx @r0,a
    add a,#SCNAME
    mov r0,a
    clr a
    movx @r0,a
    jmp pn_draw
pn_fire:
    mov r0,#PICK
    movx a,@r0
    add a,#pickset&0FFh
    movp a,@a
    xrl a,#'.'
    jz pn_end
    xrl a,#'.'
    mov r6,a
    mov r0,#NAMEN
    movx a,@r0
    xrl a,#8
    jz pn_draw                  ; full
    xrl a,#8
    mov r1,a
    inc a
    movx @r0,a
    mov a,r1
    add a,#SCNAME
    mov r0,a
    mov a,r6
    movx @r0,a
    inc r0
    clr a
    movx @r0,a
    jmp pn_draw
pn_end:
    mov r0,#NAMEN
    movx a,@r0
    jz pn_draw
    ret
pn_draw:
    call lb_name                ; the name, then the letter blinking after it
    mov r0,#NAMEN
    movx a,@r0
    add a,#LINEBUF+4
    mov r1,a
    mov r0,#FRAMES
    mov a,@r0
    anl a,#16
    jnz pn_letter
    mov a,#'_'
    jmp pn_blink
pn_letter:
    mov r0,#PICK
    movx a,@r0
    add a,#pickset&0FFh
    movp a,@a
pn_blink:
    movx @r1,a                  ; LINEBUF+4+n
    mov r4,#2
    call say_line
    jmp pn_l

pickset:
    db "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789."
PICKN   equ $-pickset


    PAGE_TAIL
