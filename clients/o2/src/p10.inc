; p10.inc -- page 10: an auction's text (mekkogx auction.c), each state
; after page 8 has placed the players and the lines: the title (a new
; stage), the line under it, each player's price or SELL / BUY, the time
; bar, and the message -- or each player's units of the good.

E_STATE equ 0

    PAGE_HEAD
    db e_state&0FFh, e_none&0FFh
e_state:
    jmp on_state
e_none:
    ret

    include "sayq.inc"

; on_state: page 8 has read this state. A new stage (AV_MSG none) says its
; title and the line under it, its message the state after (the say queue
; holds two: AV_MSG 0FEh meanwhile).
on_state:
    call au_trade
    mov r0,#AV_MSG
    movx a,@r0
    xrl a,#0FFh
    jnz os_t
    call au_cue
    call au_title
    call au_line2
    mov r0,#AV_MSG
    mov a,#0FEh
    movx @r0,a
    call au_labels
    jmp au_bar
os_t:
    call au_line2
    call au_labels
    call au_bar
    jmp au_msg

; au_title: the title, by kind and stage: LAND AUCTION r, or STATUS r while
; declaring, then AUCTION r. Clobbers A, r0, r2, r3, r5-r7.
au_title:
    mov r0,#RND
    movx a,@r0
    mov r2,a
    mov r3,#0
    mov r5,#T_LAUCT
    mov r0,#AV_KIND
    movx a,@r0
    xrl a,#1
    jz at_say
    mov r5,#T_STATUS
    mov r0,#AV_STG
    movx a,@r0
    add a,#256-STAGE_LEAD
    jnc at_say
    mov r5,#T_AUCT
at_say:
    mov r6,#L_MAP|(1<<4)
    mov r7,#C_WHITE
    jmp say_q

; au_msg: the bottom line. A message for the stage, said when it changes;
; otherwise, in a good's bidding, each player's units of it.
; Clobbers A, r0-r7.
au_msg:
    call au_msgid
    mov r5,a
    mov r0,#AV_MSG
    movx a,@r0
    xrl a,r5
    jz am_same
    mov a,r5
    movx @r0,a
    jz am_units
    mov r6,#(L_MAP+2)|(1<<4)
    mov r7,#C_WHITE
    jmp say_q
am_same:
    mov a,r5
    jnz am_x
am_units:
    jmp au_units
am_x:
    ret

; au_line2: the line under the title. Land: what the bidding stands at --
; the opening price, the high bid, what it sold for; a good: its name.
; Clobbers A, r0-r7.
au_line2:
    mov r0,#AV_KIND
    movx a,@r0
    xrl a,#1
    jz al_land
    mov r0,#AV_STG              ; (a good's name once, with the stage)
    movx a,@r0
    mov r0,#AV_MSG
    mov r5,a
    movx a,@r0
    xrl a,#0FFh
    jnz al_x
    mov r0,#AV_GOOD
    movx a,@r0
    add a,#S_GOOD0
    mov r5,a
    mov r6,#L_AUC2|(1<<4)
    mov r7,#C_CYAN
    jmp say_q
al_land:
    mov r0,#AV_STG
    movx a,@r0
    xrl a,#STAGE_DONE
    jnz al_bid
    STB ST_AUCTION+AU_PRICE+1
    mov r4,a
    STB ST_AUCTION+AU_PRICE
    mov r2,a
    orl a,r4
    jz al_bid
    mov a,r4
    mov r3,a
    mov r5,#M_SALEP
    jmp al_say
al_bid:
    call au_high                ; r3:r2 the high bid, r5 its message
al_say:
    mov r6,#L_AUC2|(1<<4)
    mov r7,#C_WHITE
    jmp say_q
al_x:
    ret


; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "stbyte.inc"

; au_labels: T1, a field a seat in its colour: nothing for a seat out of
; it, SELL or BUY before the bidding (and at the ends of the scale), else
; the price its line stands at. Clobbers A, r0-r7.
au_labels:
    call lb_clear
    mov r7,#0
alb_l:
    mov a,r7
    add a,#AV_ROLE
    mov r0,a
    movx a,@r0
    jz alb_n
    mov r6,a                    ; the role
    mov r0,#AV_STG
    movx a,@r0
    add a,#256-STAGE_LEAD
    jnc alb_lbl
    call pos_of
    mov a,r6
    xrl a,#ROLE_SELLER
    jz alb_num                  ; (a seller's price shows)
    mov a,r5                    ; a buyer at the bottom: BUY
    jnz alb_num
    mov a,r4
    jz alb_lbl
alb_num:
    mov a,r7
    rl a
    rl a
    add a,#LINEBUF
    mov r0,a
    call num4
    jmp alb_n
alb_lbl:
    mov a,r7
    rl a
    rl a
    add a,#LINEBUF
    mov r0,a
    mov a,r6
    xrl a,#ROLE_SELLER
    jnz alb_b
    mov a,#s_sell&0FFh
    jmp alb_s
alb_b:
    mov a,#s_buy&0FFh
alb_s:
    call lbl4
alb_n:
    inc r7
    mov a,r7
    xrl a,#4
    jnz alb_l
    mov r0,#TXCOL
    mov a,#TX_SEATS
    movx @r0,a
    mov r2,#T1>>8
    mov r3,#T1&0FFh
    mov r4,#188
    jmp tx_build

; lbl4: the four-letter string at offset A of this page at r0. Clobbers A,
; r0, r1, r6.
lbl4:
    mov r1,a
    mov r6,#4
l4_l:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r6,l4_l
    ret
s_sell: db "SELL"
s_buy:  db " BUY"

; ===========================================================================
    org 0B00h

; au_units: T2, each seat's units of the good -- a land auction's: its
; money -- in its colour. Clobbers A, r0-r7.
au_units:
    call lb_clear
    mov r7,#0
au_ul:
    mov a,r7
    add a,#AV_ROLE
    mov r0,a
    movx a,@r0
    jz au_un
    mov a,r7
    call pl_val
    mov a,r7
    rl a
    rl a
    add a,#LINEBUF
    mov r0,a
    call num4
au_un:
    inc r7
    mov a,r7
    xrl a,#4
    jnz au_ul
    mov r0,#TXCOL
    mov a,#TX_SEATS
    movx @r0,a
    mov r2,#T2>>8
    mov r3,#T2&0FFh
    mov r4,#226
    jmp tx_build

; pl_val: r5:r4 = seat A's money (land), or its units of AV_GOOD.
; Clobbers A, r0, r2, r3, r6.
pl_val:
    mov r6,a                    ; 29 * seat = 32 s - 2 s - s
    swap a
    rl a
    anl a,#0E0h
    xch a,r6
    mov r2,a
    rl a
    add a,r2
    cpl a
    inc a
    add a,r6
    add a,#ST_PLAYERS+PL_MONEY
    mov r2,a
    mov r3,#0
    mov r0,#AV_KIND
    movx a,@r0
    xrl a,#1
    jz pv_m
    mov r0,#AV_GOOD
    movx a,@r0
    add a,#PL_GOODS-PL_MONEY
    add a,r2
    mov r2,a
    call st_byte
    mov r4,a
    mov r5,#0
    ret
pv_m:
    call st_byte
    mov r4,a
    inc r2
    call st_byte
    mov r5,a
    ret

; au_bar: the time bar's units (tbar.inc), as mekkogx au_ticks_now: the
; tick under way has tickLeft jiffies left, each after lasts 25 (6 while
; declaring, or idle), and this state is RXAGE old. While declaring or
; bidding; empty else. Clobbers A, r0-r7.
au_bar:
    mov r0,#TB_SEG              ; a segment: 8 of the bidding's 70 ticks
    mov a,#8
    movx @r0,a
    mov r0,#AV_STG
    movx a,@r0
    mov r7,#6
    xrl a,#STAGE_DECLARE
    jz ab_dec
    xrl a,#STAGE_DECLARE!STAGE_ACTIVE
    jnz ab_none
    STB ST_AUCTION+AU_FLAGS
    anl a,#AF_IDLE
    jnz ab_per
    mov r7,#25
    jmp ab_per
ab_dec:
    mov r0,#TB_SEG              ; 4 of the declaring's 30
    mov a,#4
    movx @r0,a
ab_per:
    mov r0,#RXAGE               ; y = age + per
    movx a,@r0
    add a,r7
    mov r4,a
    STB ST_AUCTION+AU_TICKLEFT
    mov r5,a
    cpl a
    add a,r4                    ; y > tickLeft: a carry
    jc ab_div
    mov a,r4                    ; none gone yet: the next in tickLeft - age
    cpl a
    add a,r5
    inc a
    add a,r7
    mov r5,a
    mov r4,#0
    jmp ab_put
ab_div:
    inc a                       ; x = y - tickLeft: x / per gone, the next in
    mov r4,#0                   ; per - x mod per
ab_dl:
    mov r5,a
    mov a,r7
    cpl a
    inc a
    add a,r5
    jnc ab_rem
    inc r4
    jmp ab_dl
ab_rem:
    mov a,r5
    cpl a
    inc a
    add a,r7
    mov r5,a
ab_put:
    STB ST_AUCTION+AU_TICKS     ; units = ticks - gone, 0 at least
    mov r2,a
    mov a,r4
    cpl a
    add a,r2
    jnc ab_none
    inc a
    mov r0,#TB_U
    movx @r0,a
    mov r0,#TB_PER
    mov a,r7
    movx @r0,a
    mov r0,#TB_CNT
    mov a,r5
    movx @r0,a
    ret
ab_none:
    clr a
    mov r0,#TB_U
    movx @r0,a
    ret

; ===========================================================================
    org 0C00h

; au_high: the land's high bid: r3:r2 the highest buyer's line at or over
; the opening price, r5 M_HIGHBID; none, the opening price and M_BIDAT.
; Clobbers A, r0-r7.
au_high:
    mov r3,#(ST_AUCTION+AU_START)>>8
    mov r2,#(ST_AUCTION+AU_START)&0FFh
    call st_byte
    mov r4,a
    inc r2
    call st_byte
    mov r3,a
    mov a,r4
    mov r2,a
    mov r6,#M_BIDAT
    mov r7,#0
ah_l:
    mov a,r7
    add a,#AV_ROLE
    mov r0,a
    movx a,@r0
    xrl a,#ROLE_BUYER
    jnz ah_n
    call pos_of
    call lt16                   ; under the best so far: no
    jc ah_n
    mov a,r4
    mov r2,a
    mov a,r5
    mov r3,a
    mov r6,#M_HIGHBID
ah_n:
    inc r7
    mov a,r7
    xrl a,#4
    jnz ah_l
    mov a,r6
    mov r5,a
    ret

; pos_of: r5:r4 = seat r7's line. Clobbers A, r0.
pos_of:
    mov a,r7
    rl a
    add a,#AV_POS
    mov r0,a
    movx a,@r0
    mov r4,a
    inc r0
    movx a,@r0
    mov r5,a
    ret

; lt16: C = r5:r4 < r3:r2 (unsigned). Clobbers A.
lt16:
    mov a,r5
    cpl a
    add a,r3                    ; hi: r3 + ~r5 carries: r3 > r5
    jc lt_x
    mov a,r3
    cpl a
    add a,r5                    ; r5 + ~r3 carries: r5 > r3, not less
    jc lt_no
    mov a,r4                    ; the high bytes agree: the low decide
    cpl a
    add a,r2
    ret
lt_no:
    clr c
lt_x:
    ret

; au_msgid: A = the message for the stage: declaring (seated), the end
; (sold, no bids, over), a seller at its critical level, collusion; else
; 0 for a good's units, or the blank line. Clobbers A, r0-r3, r6.
au_msgid:
    mov r0,#AV_STG
    movx a,@r0
    xrl a,#STAGE_DECLARE
    jnz ami_done
    mov r0,#MYSEAT
    movx a,@r0
    jb7 ami_none
    mov a,#M_DECL
    ret
ami_done:
    xrl a,#STAGE_DECLARE!STAGE_DONE
    jnz ami_act
    mov r0,#AV_KIND
    movx a,@r0
    xrl a,#1
    jz ami_land
    mov a,#M_AUOVER
    ret
ami_land:
    STB ST_AUCTION+AU_WINNER
    add a,#256-4
    mov a,#M_NOBIDS
    jc ami_x
    mov a,#M_SOLD
ami_x:
    ret
ami_act:
    mov r0,#AV_KIND             ; land: each bidder's money
    movx a,@r0
    xrl a,#2
    jnz ami_units
    mov r0,#AV_STG
    movx a,@r0
    xrl a,#STAGE_ACTIVE
    jnz ami_units
    STB ST_AUCTION+AU_FLAGS
    mov r6,a
    anl a,#AF_CRITICAL
    jz ami_c
    mov a,#M_CRIT
    ret
ami_c:
    mov a,r6
    anl a,#AF_COLLUDING
    jz ami_units
    mov a,#M_COLLUDE
    ret
ami_units:
    mov r0,#AV_STG
    movx a,@r0
    add a,#256-STAGE_LEAD
    clr a
    jc ami_x
ami_none:
    mov a,#S_NONE
    ret

; ===========================================================================
    org 0D00h

; num4: r5:r4 right-aligned in the four columns from r0; 10000 and more in
; thousands: " 12K". Clobbers A, r0-r3, r5, r6.
num4:
    mov a,r4                    ; 10000 ($2710) or more: no borrow
    add a,#0F0h
    mov a,r5
    addc a,#0D8h
    jc n4_kilo
    mov r1,#n4tab&0FFh
    clr f0
n4_d:
    mov r6,#'0'
n4_s:
    mov a,r1                    ; r5:r4 + -10^k: no carry, the digit is done
    movp a,@a
    add a,r4
    mov r2,a
    mov a,r1
    inc a
    movp a,@a
    addc a,r5
    jnc n4_p
    mov r5,a
    mov a,r2
    mov r4,a
    inc r6
    jmp n4_s
n4_p:
    mov a,r1
    xrl a,#n4tab&0FFh           ; (thousands past 9: the low four)
    jnz n4_q
    mov a,r6
    add a,#256-'9'-1
    jnc n4_q
    mov r6,#'9'
n4_q:
    mov a,r6
    xrl a,#'0'
    jnz n4_o
    jf0 n4_o
    mov a,#' '
    jmp n4_w
n4_o:
    clr f0
    cpl f0
    mov a,r6
n4_w:
    movx @r0,a
    inc r0
    inc r1
    inc r1
    mov a,r1
    xrl a,#(n4tab+6)&0FFh
    jnz n4_d
    mov a,r4
    add a,#'0'
    movx @r0,a
    ret
n4tab:
    db 018h,0FCh, 09Ch,0FFh, 0F6h,0FFh              ; -1000 -100 -10

n4_kilo:
    mov r6,#0                   ; thousands: 1000 off until it borrows
n4_k:
    mov a,r4
    add a,#018h
    mov r2,a
    mov a,r5
    addc a,#0FCh
    jnc n4_kd
    mov r5,a
    mov a,r2
    mov r4,a
    inc r6
    jmp n4_k
n4_kd:
    mov a,#' '
    movx @r0,a
    inc r0
    mov r2,#'0'                 ; 10-65: two digits, then K
    mov a,r6
n4_t:
    add a,#256-10
    jnc n4_u
    inc r2
    jmp n4_t
n4_u:
    add a,#10+'0'
    mov r6,a
    mov a,r2
    movx @r0,a
    inc r0
    mov a,r6
    movx @r0,a
    inc r0
    mov a,#'K'
    movx @r0,a
    ret


; au_cue: a new stage's sound (mekkogx auction.c): declaring, and the lines
; about to go, call a seated player to the stick (ALERT); the end is
; AUCTION OVER. Clobbers A, r0, r1.
au_cue:
    mov r0,#AV_STG
    movx a,@r0
    xrl a,#STAGE_DONE
    jz ac_over
    xrl a,#STAGE_DONE!STAGE_DECLARE
    jz ac_call
    xrl a,#STAGE_DECLARE!STAGE_LEAD
    jnz ac_x
ac_call:
    mov r0,#MYSEAT
    movx a,@r0
    jb7 ac_x
    mov a,#SND_ALERT
    jmp ac_b
ac_over:
    mov a,#SND_AUOVER
ac_b:
    FCALL beep
ac_x:
    ret

; au_trade: a click when the units traded have gone up since the last state
; (AV_TRD 0FFh: the arena's first, only noted). Clobbers A, r0-r3, r6, r7.
au_trade:
    STB ST_AUCTION+AU_TRADED
    mov r7,a
    mov r0,#AV_TRD
    movx a,@r0
    xch a,r7
    movx @r0,a
    mov r6,a
    mov a,r7
    xrl a,#0FFh
    jz at_x
    mov a,r7                    ; now + ~before carries: more than before
    cpl a
    add a,r6
    jnc at_x
    mov a,#SND_TRADE
    FCALL beep
at_x:
    ret

    PAGE_TAIL
