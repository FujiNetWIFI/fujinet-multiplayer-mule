; p8.inc -- page 8: the auctions' arena (mekkogx auction.c): the land
; auction's bidding, and a good's declaring and trading -- the players, the
; lines, the stick. Each state's text (titles, prices, units, message, the
; time bar) is page 10's, which the main loop runs after this one. (A
; good's status is page 7's text screen; a plot on offer is page 6's map.)
;
;   T0  y 30   LAND AUCTION 3 / STATUS 3 / AUCTION 3
;   T3  y 66   HIGH BID $240 / SMITHORE
;       99-164 the players, in four columns, feet on their price; the
;              sellers' and the buyers' lines dotted across (singles 0-4,
;              5-9); the time bar the grid's right-hand column
;   T1  y 188  each player's price, or SELL / BUY, in its colour
;   T2  y 226  a message, or each player's units of the good
;
; No text band falls inside 83-164, so the players are never blanked by
; one. The lines walk a step every 8 frames towards their targets between
; states, as the server's do.

E_STATE equ 0
E_TICK  equ 1
AY_BASE equ 144                 ; feet at the base price; 1.5 lines a step
AY_UP   equ 30                  ; steps above it (feet at 99: clear of T3)
AY_DOWN equ 13                  ; ... and below
AY_SELL equ 104                 ; before the bidding: the sellers ...
AY_BOT  equ 164                 ; ... and the buyers
AX0     equ 28                  ; seat i stands at 28 + 32 i

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp on_state
e_tick:
    jmp tick

; on_state: a state has come. A new phase (or a page change) puts the
; arena up; a new stage has page 10 say its titles (AV_MSG none); every
; state the players and the lines.
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
    call au_screen
os_same:
    call au_read
    mov r0,#AUSTG
    movx a,@r0
    mov r5,a
    mov r0,#AV_STG
    movx a,@r0
    xrl a,r5
    jz os_st
    mov a,r5
    movx @r0,a
    mov r0,#AV_MSG              ; a new stage: its titles and message
    mov a,#0FFh
    movx @r0,a
os_st:
    call au_snap
    call au_baseset
    call au_draw
    jmp au_lines

; au_read: the kind, the good, dollars a step and the roles. Clobbers A,
; r0-r6.
au_read:
    STB ST_AUCTION+AU_KIND
    mov r0,#AV_KIND
    movx @r0,a
    STB ST_AUCTION+AU_GOOD
    anl a,#3
    mov r0,#AV_GOOD
    movx @r0,a
    STB ST_AUCTION+AU_STEP
    jnz ar_step
    inc a
ar_step:
    mov r0,#AV_STEPV
    movx @r0,a
    mov r3,#(ST_AUCTION+AU_ROLE)>>8
    mov r2,#(ST_AUCTION+AU_ROLE)&0FFh
    mov r1,#AV_ROLE
    mov r4,#4
    ; fall into st_copy

; st_copy: r4 bytes of the state from r3:r2 to XRAM r1. Clobbers A, r0,
; r1, r2, r4, r6.
st_copy:
    call st_byte
    movx @r1,a
    inc r1
    inc r2
    djnz r4,st_copy
    ret

; au_snap: the lines and their targets from the state (the ticks walk them
; on); a seller at its critical level holds its line.
au_snap:
    mov r3,#(ST_AUCTION+AU_POS)>>8
    mov r2,#(ST_AUCTION+AU_POS)&0FFh
    mov r1,#AV_POS
    mov r4,#16                  ; AV_POS then AV_TGT, as in the state
    call st_copy
    STB ST_AUCTION+AU_FLAGS
    anl a,#AF_CRITICAL
    jz asn_x
    STB ST_AUCTION+AU_SELLER
    anl a,#3
    rl a
    mov r5,a
    add a,#AV_POS
    mov r0,a
    mov a,r5
    add a,#AV_TGT
    mov r1,a
    movx a,@r0
    movx @r1,a
    inc r0
    inc r1
    movx a,@r0
    movx @r1,a
asn_x:
    mov r1,#HT                  ; the server steps the lines on every 8th
    movx a,@r1                  ; jiffy: ours in time with it
    mov r0,#RXAGE
    movx a,@r0
    mov r5,a
    movx a,@r1
    add a,r5
    anl a,#7
    cpl a
    add a,#9                    ; 8 - that
    mov r0,#AV_TICK
    movx @r0,a
    ret

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "stbyte.inc"

; au_screen: the arena's band list, an empty grid, the players' shape, the
; lines and players off. Clobbers A, r0-r7.
au_screen:
    mov r1,#MODE                ; no map rows
    clr a
    mov @r1,a
    mov r2,#SCRCOL>>8
    mov a,#SCRCOL&0FFh
    call poke_at
    mov r4,#au_bands&0FFh
    mov r6,#2+NBANDS*3
    call ac_copy
    mov r2,#GRIDTAB>>8
    mov a,#GRIDTAB&0FFh
    call poke_at
    mov r6,#28
asc_g:
    clr a
    call poke_b
    djnz r6,asc_g
    mov r2,#STATTAB>>8          ; the lines' ten dashes off
    mov a,#STATTAB&0FFh
    call poke_at
    mov r6,#10
asc_s:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    call poke_b
    call poke_b
    djnz r6,asc_s
    mov r7,#0                   ; the four players: their species' figures
    mov r2,#ST_PLAYERS+PL_SPECIES
asc_p:
    mov r3,#0
    call st_byte
    anl a,#7
    rl a
    rl a
    rl a
    mov r5,a
    mov a,r7
    rl a
    rl a
    rl a
    add a,#vdc_spr0_shape
    mov r0,a
    mov a,#P1_VDC
    outl p1,a
    mov a,r5
    call af_copy
    mov a,#P1_MAIN
    outl p1,a
    mov a,r2
    add a,#PL_SIZE
    mov r2,a
    inc r7
    mov a,r7
    xrl a,#4
    jnz asc_p
    mov r0,#AV_LN1              ; nothing on screen yet
    mov a,#0FFh
    movx @r0,a
    inc r0
    movx @r0,a
    inc r0
    movx @r0,a                  ; AV_MSG
    inc r0
    movx @r0,a                  ; AV_TRD
    mov r0,#AV_STG
    movx @r0,a
    clr a
    mov r0,#AV_PUSH
    movx @r0,a
    call lb_clear               ; T3 blank: the text screen left a line in it
    mov r2,#T3>>8
    mov r3,#T3&0FFh
    mov r4,#66
    call tx_build
    call spr_off
    mov r1,#DIRTY
    mov a,#DIRTY_GRID|DIRTY_STATIC
    mov @r1,a
    ret

; spr_off: the four players below the screen. Clobbers A, r0, r1, r6.
spr_off:
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov r6,#4
so_l:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    call poke_b
    djnz r6,so_l
    ret

; ac_copy: r6 bytes from offset r4 of this page to the POKE pointer.
ac_copy:
    mov a,r4
    movp a,@a
    call poke_b
    inc r4
    djnz r6,ac_copy
    ret

au_bands:
    db 007h                     ; black background, a white grid (the bar)
    db 256-1                    ; the first band: line 1
    db J_TXTA, T0&0FFh, 256-43  ; T0 at 30, then T3's band at 44
    db J_TXTB, T3&0FFh, 256-122 ; T3 at 66, then T1's band at 166
    db J_TXTA, T1&0FFh, 256-36  ; T1 at 188, then T2's at 202
    db J_TXTA, T2&0FFh, 0       ; T2 at 226: the last
    db J_NONE, 0, 0
    db J_NONE, 0, 0
    db J_NONE, 0, 0
    db J_NONE, 0, 0
    db J_NONE, 0, 0


; ===========================================================================
    org 0B00h

; price_y: A = the feet's line for price r3:r2: 1.5 lines a step from
; AY_BASE, up to AY_UP steps over the base and AY_DOWN under. (Land steps
; are $4.) Clobbers A, r2, r3, r5, r6.
price_y:
    mov r0,#AV_BASE             ; r3:r2 - base
    movx a,@r0
    cpl a
    add a,#1
    mov r5,a
    inc r0
    movx a,@r0
    cpl a
    addc a,#0
    mov r6,a
    mov a,r2
    add a,r5
    mov r2,a
    mov a,r3
    addc a,r6
    mov r3,a
    jb7 py_dn
    call py_steps
    add a,#256-AY_UP-1
    jnc py_up
    mov a,#AY_UP
    jmp py_up1
py_up:
    add a,#AY_UP+1
py_up1:
    call py_half                ; 1.5 steps
    cpl a
    inc a
    add a,#AY_BASE
    ret
py_dn:
    mov a,r2                    ; base - price
    cpl a
    add a,#1
    mov r2,a
    mov a,r3
    cpl a
    addc a,#0
    mov r3,a
    call py_steps
    add a,#256-AY_DOWN-1
    jnc py_d
    mov a,#AY_DOWN
    jmp py_d1
py_d:
    add a,#AY_DOWN+1
py_d1:
    call py_half
    add a,#AY_BASE
    ret

; py_steps: A = r3:r2 in steps (dollars / AV_STEPV for a $4 step), 255 at
; most. Clobbers r0.
py_steps:
    mov r0,#AV_STEPV
    movx a,@r0
    xrl a,#4
    jnz ps_one
    mov a,r3                    ; / 4
    rr a
    rr a
    anl a,#0C0h
    mov r0,a
    mov a,r2
    rr a
    rr a
    anl a,#3Fh
    orl a,r0
    mov r2,a
    mov a,r3
    rr a
    rr a
    anl a,#3Fh
    mov r3,a
ps_one:
    mov a,r3
    jz ps_lo
    mov a,#0FFh
    ret
ps_lo:
    mov a,r2
    ret

; py_half: A = A * 3 / 2. Clobbers r0.
py_half:
    mov r0,a
    clr c
    rrc a
    add a,r0
    ret

; au_draw: the players. Before the bidding the sellers stand at the top,
; the buyers at the bottom; then each on its line. Clobbers A, r0-r7.
au_draw:
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov r7,#0
ad_l:
    mov a,r7
    add a,#AV_ROLE
    mov r0,a
    movx a,@r0
    jz ad_off
    mov r4,a                    ; the role
    mov r0,#AV_STG
    movx a,@r0
    add a,#256-STAGE_LEAD
    jc ad_line
    mov a,r4
    xrl a,#ROLE_SELLER
    jnz ad_bot
    mov a,#AY_SELL
    jmp ad_put
ad_bot:
    mov a,#AY_BOT
    jmp ad_put
ad_line:
    mov a,r7
    rl a
    add a,#AV_POS
    mov r0,a
    movx a,@r0
    mov r2,a
    inc r0
    movx a,@r0
    mov r3,a
    call price_y
ad_put:
    add a,#256-16               ; Y: the feet less the man's height
    mov r4,a
    call poke_sel               ; (price_y's reads kept POKE)
    mov a,r4
    call poke_b
    mov a,r7                    ; X
    swap a
    rl a
    add a,#AX0
    call poke_b
    mov a,r7                    ; its colour
    add a,#seatcol&0FFh
    movp a,@a
    rl a
    rl a
    rl a
    call poke_b
    jmp ad_n
ad_off:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    call poke_b
ad_n:
    inc r7
    mov a,r7
    xrl a,#4
    jnz ad_l
    ret
seatcol:
    db C_RED, C_GREEN, C_TAN, C_VIOLET

; tk_decl: declaring: up a seller, down a buyer, the button out.
tk_decl:
    mov r0,#JOYEV
    movx a,@r0
    mov r5,a
    mov r4,#ROLE_SELLER
    anl a,#J_UP
    jnz td_role
    mov r4,#ROLE_BUYER
    mov a,r5
    anl a,#J_DOWN
    jnz td_role
    mov r4,#ROLE_NONE
    mov a,r5
    anl a,#J_FIRE
    jz td_x
td_role:
    mov r0,#MYSEAT              ; ours, at once
    movx a,@r0
    add a,#AV_ROLE
    mov r0,a
    mov a,r4
    movx @r0,a
    mov r0,#ACT
    mov a,#ACT_DECL
    movx @r0,a
    inc r0
    mov a,r4                    ; arg: 0 out, 1 buyer, 2 seller
    movx @r0,a
    inc r0
    clr a
    movx @r0,a
    mov a,#SND_SELECT
    FCALL beep

    jmp au_draw                 ; (its label with the next state)
td_x:
    ret

; ===========================================================================
    org 0C00h

; au_lines: the two dotted lines: the sellers' (the lowest ask, or the
; store's selling price while it has stock) and the buyers' (the highest
; bid, or the store's buying price / the land's opening). Rewritten when
; either moves. Clobbers A, r0-r7.
au_lines:
    mov r0,#AV_STG
    movx a,@r0
    add a,#256-STAGE_LEAD
    jc ln_on
    mov r4,#0FFh                ; before the bidding: none
    mov r5,#0FFh
    jmp ln_set
ln_on:
    call ask_low                ; r3:r2 the sellers' price, or none (C)
    mov a,#0FFh
    jc ln_buy
    call price_y
ln_buy:
    mov r0,#WX_F                ; (bid_high keeps no register)
    movx @r0,a
    call bid_high               ; r3:r2 the buyers' price
    call price_y
    mov r5,a
    mov r0,#WX_F
    movx a,@r0
    mov r4,a
ln_set:
    mov r0,#AV_LN1
    movx a,@r0
    xrl a,r4
    jnz ln_new
    inc r0
    movx a,@r0
    xrl a,r5
    jz ln_x
ln_new:
    mov r0,#AV_LN1
    mov a,r4
    movx @r0,a
    inc r0
    mov a,r5
    movx @r0,a
    mov r2,#STATTAB>>8
    mov a,#STATTAB&0FFh
    call poke_at
    mov a,r4
    call al_dots
    mov r0,#AV_LN2              ; (al_dots keeps no register)
    movx a,@r0
    call al_dots
    mov r1,#DIRTY
    mov a,@r1
    orl a,#DIRTY_STATIC
    mov @r1,a
ln_x:
    ret

; al_dots: five dashes across at line A (0FFh: off). Clobbers A, r0-r3,
; r6, r7.
al_dots:
    mov r7,a
    mov r6,#16                  ; X
ald_l:
    mov a,r7
    xrl a,#0FFh
    jz ald_off
    mov a,r7                    ; the dash's bar is the glyph's middle row;
    add a,#256-6                ; an even Y (the pointer's bias is Y / 2:
    anl a,#0FEh                 ; an odd one shows another glyph)
    mov r2,a
    call poke_b                 ; Y
    mov a,r6
    call poke_b                 ; X
    mov a,r2                    ; the dash for that Y
    rr a
    cpl a
    inc a
    mov r5,a
    mov a,#'-'
    call gptr
    mov a,r3
    call poke_b
    mov a,#C_WHITE<<1
    orl a,r1
    call poke_b
    jmp ald_n
ald_off:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    call poke_b
    call poke_b
ald_n:
    mov a,r6
    add a,#32
    mov r6,a
    xrl a,#16+5*32
    jnz ald_l
    ret

; ask_low: r3:r2 = the lowest asking price -- the store's while it has the
; good, any seller's lower -- or C set: none. Clobbers A, r0-r7.
ask_low:
    mov r2,#0FFh
    mov r3,#0FFh
    mov r0,#AV_KIND
    movx a,@r0
    xrl a,#2
    jnz ak_s
    mov r0,#AV_GOOD             ; the store's stock of it
    movx a,@r0
    add a,#(ST_STORE+STO_STOCK)&0FFh
    mov r2,a
    mov r3,#(ST_STORE+STO_STOCK)>>8
    call st_byte
    mov r2,#0FFh
    mov r3,#0FFh
    jz ak_s
    mov r3,#(ST_AUCTION+AU_STORESELL)>>8
    mov r2,#(ST_AUCTION+AU_STORESELL)&0FFh
    call st_byte
    mov r4,a
    inc r2
    call st_byte
    mov r3,a
    mov a,r4
    mov r2,a
ak_s:
    mov r7,#0
ak_l:
    mov a,r7
    add a,#AV_ROLE
    mov r0,a
    movx a,@r0
    xrl a,#ROLE_SELLER
    jnz ak_n
    call pos_of                 ; r5:r4 = seat r7's line
    call lt16                   ; r5:r4 < r3:r2? (C)
    jnc ak_n
    mov a,r4
    mov r2,a
    mov a,r5
    mov r3,a
ak_n:
    inc r7
    mov a,r7
    xrl a,#4
    jnz ak_l
    mov a,r3                    ; none: still $FFFF
    anl a,r2
    cpl a
    clr c
    jnz ak_x
    cpl c
ak_x:
    ret

; ===========================================================================
    org 0D00h

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

; au_baseset: the price at AY_BASE: a good's, the store's buying price; a
; land's, its opening price -- or, once the high bid runs past AY_UP steps
; over that, the high bid less AY_UP steps: the scale scrolls up, the
; leader at the top (mekkogx au_step). Clobbers A, r0-r7.
au_baseset:
    mov r0,#AV_KIND
    movx a,@r0
    xrl a,#1
    mov r3,#(ST_AUCTION+AU_STOREBUY)>>8
    mov r2,#(ST_AUCTION+AU_STOREBUY)&0FFh
    jnz abs_copy
    mov r2,#(ST_AUCTION+AU_START)&0FFh
    call abs_copy
    call bid_high               ; r3:r2 = the high bid (the opening at least)
    mov a,r2                    ; less AY_UP $4 steps
    add a,#256-AY_UP*4
    mov r2,a
    mov a,r3
    addc a,#0FFh
    mov r3,a
    jnc abs_x                   ; (under: no scroll)
    mov r0,#AV_BASE
    movx a,@r0
    mov r4,a
    inc r0
    movx a,@r0
    mov r5,a
    call lt16                   ; opening < that: the base moves up
    jnc abs_x
    mov r0,#AV_BASE
    mov a,r2
    movx @r0,a
    inc r0
    mov a,r3
    movx @r0,a
abs_x:
    ret
abs_copy:
    mov r1,#AV_BASE
    mov r4,#2
    jmp st_copy

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

; tk_act: send action A (no argument) unless one waits. Clobbers r0, r6.
tk_act:
    mov r6,a
    mov r0,#ACT
    movx a,@r0
    jnz ta_x
    mov a,r6
    movx @r0,a
    inc r0
    clr a
    movx @r0,a
    inc r0
    movx @r0,a
ta_x:
    ret

; au_step: seat r7's line a step (AV_STEPV) towards its target. (A land
; scale scrolls with the next state: au_baseset.) Clobbers A, r0-r6.
au_step:
    call pos_of                 ; r5:r4 = the line
    mov a,r7
    rl a
    add a,#AV_TGT
    mov r1,a
    movx a,@r1
    mov r2,a
    inc r1
    movx a,@r1
    mov r3,a                    ; r3:r2 = the target
    call lt16                   ; line < target: up
    jc st_up
    mov a,r2                    ; target < line: down
    xch a,r4
    mov r2,a
    mov a,r3
    xch a,r5
    mov r3,a                    ; (swapped: r5:r4 the target)
    call lt16
    jnc st_x                    ; equal
    mov r0,#AV_STEPV            ; line - step, not under the target
    movx a,@r0
    cpl a
    inc a
    add a,r2
    mov r2,a
    mov a,#0FFh
    addc a,r3
    mov r3,a
    call lt16                   ; target < line still?
    jc st_put
    mov a,r4
    mov r2,a
    mov a,r5
    mov r3,a
    jmp st_put
st_up:
    mov r0,#AV_STEPV            ; line + step, not over the target
    movx a,@r0
    add a,r4
    mov r4,a
    clr a
    addc a,r5
    mov r5,a
    call lt16                   ; still under it?
    jc st_upp
    mov a,r2
    mov r4,a
    mov a,r3
    mov r5,a
st_upp:
    mov a,r4
    mov r2,a
    mov a,r5
    mov r3,a
st_put:                         ; r3:r2 the line now
    mov a,r7
    rl a
    add a,#AV_POS
    mov r0,a
    mov a,r2
    movx @r0,a
    inc r0
    mov a,r3
    movx @r0,a
st_x:
    ret

; bid_high: r3:r2 = the highest bid: the store's buying price (land: the
; opening price), any buyer's higher. Clobbers A, r0-r7.
bid_high:
    mov r0,#AV_BASE
    mov r3,#(ST_AUCTION+AU_STOREBUY)>>8
    mov r2,#(ST_AUCTION+AU_STOREBUY)&0FFh
    mov r0,#AV_KIND
    movx a,@r0
    xrl a,#2
    jz bh_rd
    mov r2,#(ST_AUCTION+AU_START)&0FFh
bh_rd:
    call st_byte
    mov r4,a
    inc r2
    call st_byte
    mov r3,a
    mov a,r4
    mov r2,a
    mov r7,#0
bh_l:
    mov a,r7
    add a,#AV_ROLE
    mov r0,a
    movx a,@r0
    xrl a,#ROLE_BUYER
    jnz bh_n
    call pos_of
    call lt16                   ; r5:r4 < r3:r2?
    jc bh_n
    mov a,r4
    mov r2,a
    mov a,r5
    mov r3,a
bh_n:
    inc r7
    mov a,r7
    xrl a,#4
    jnz bh_l
    ret

; ===========================================================================
    org 0E00h

; tick: every frame. Our stick and button; the lines' walk, a step every 8
; frames (the players and the lines move with it).
tick:
    mov r0,#MYSEAT
    movx a,@r0
    jb7 tk_walk                 ; watching
    mov r0,#AV_STG
    movx a,@r0
    xrl a,#STAGE_DECLARE
    jnz tk_nd
    call tk_decl
    jmp tk_walk
tk_nd:
    xrl a,#STAGE_DECLARE!STAGE_LEAD
    jz tk_stick
    xrl a,#STAGE_LEAD!STAGE_ACTIVE
    jnz tk_walk
    mov r0,#AV_KIND             ; a good's bidding: the button colludes
    movx a,@r0
    xrl a,#2
    jnz tk_stick
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz tk_stick
    mov a,#ACT_COLL
    clr c
    call tk_act
tk_stick:
    call au_stick
tk_walk:
    mov r0,#AV_STG
    movx a,@r0
    add a,#256-STAGE_LEAD
    jnc tk_x
    mov r0,#TICKN               ; every 8 frames
    movx a,@r0
    cpl a
    inc a
    mov r0,#AV_TICK
    mov r5,a
    movx a,@r0
    add a,r5
    jz tk_step
    jb7 tk_step
    movx @r0,a
tk_x:
    ret
tk_step:
    add a,#8
    jz tk_s8
    jb7 tk_s8
    jmp tk_s
tk_s8:
    mov a,#8
tk_s:
    movx @r0,a
    mov r7,#0
tk_sl:
    call au_step
    inc r7
    mov a,r7
    xrl a,#4
    jnz tk_sl
    call au_draw
    jmp au_lines

; au_stick: our stick, sent when it changes: up and down push our line
; (the server moves it; the computers' start with ours), let go holds it
; where we see it (ACT_HOLD).
au_stick:
    mov r0,#JOY
    movx a,@r0
    mov r5,a
    mov r4,#1
    anl a,#J_UP
    jnz as_k
    mov r4,#2
    mov a,r5
    anl a,#J_DOWN
    jnz as_k
    mov r4,#0
as_k:
    mov r0,#AV_PUSH
    movx a,@r0
    xrl a,r4
    jz as_x
    mov r0,#ACT
    movx a,@r0
    jnz as_x                    ; one at a time
    mov r0,#AV_PUSH
    mov a,r4
    movx @r0,a
    mov r0,#ACT+1
    movx @r0,a                  ; ACTARG: the push
    inc r0
    clr a
    movx @r0,a
    mov r0,#ACT
    mov a,#ACT_PUSH
    movx @r0,a
    mov a,r4
    jnz as_x
    mov r0,#AV_STG              ; let go while bidding: hold it here
    movx a,@r0
    xrl a,#STAGE_ACTIVE
    jnz as_x
    mov r0,#MYSEAT
    movx a,@r0
    mov r7,a
    call pos_of
    mov r0,#ACT
    mov a,#ACT_HOLD
    movx @r0,a
    inc r0
    mov a,r4
    movx @r0,a
    inc r0
    mov a,r5
    movx @r0,a
    mov a,r7                    ; and ours stays put
    rl a
    add a,#AV_TGT
    mov r0,a
    mov a,r4
    movx @r0,a
    inc r0
    mov a,r5
    movx @r0,a
as_x:
    ret

; af_copy: the figure at offset A of afigs into the shape at r0 (VDC
; selected). Clobbers A, r0, r1, r4.
af_copy:
    add a,#afigs&0FFh
    mov r1,a
    mov r4,#8
afc_l:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r4,afc_l
    ret
afigs:                          ; by species, standing
    FIGS_STAND

    PAGE_TAIL
