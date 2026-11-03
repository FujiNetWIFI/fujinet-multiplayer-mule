; p13.inc -- page 13: the development's words, each state after its walking
; page (11 the map, 12 the town) has read the turn (mekkogx develop.c
; develop_state): our prompt, whose turn it is, how ours ended; what the
; server said to our last action; the wampus's window; in town, our money
; and the store's M.U.L.E.s.

E_STATE equ 0
SPR_WHITE equ C_WHITE<<3        ; a sprite's colour, attribute bits 3-5

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp on_state
e_tick:
    jmp as_tick                 ; (the main loop runs it each frame of the
                                ; development, after the walking page's)

    include "sayq.inc"

CUR_XYONLY equ 1
    include "cursor.inc"

; on_state: the turn's words (on_state2), then the assays seen (as_scan).
on_state:
    call on_state2
    jmp as_scan

; on_state2: the time bar; by DV_ST: our turn under way, the answers and the
; wampus (map) or the money line (town); else what to say of the turn,
; once a seat and kind (DV_MSG = seat * 4 + kind).
on_state2:
    call dv_tbar
    mov r0,#DV_ST
    movx a,@r0
    xrl a,#DV_WALK
    jnz os_turn
    call dv_event
    call dv_answer
    mov r0,#DV_TOWN
    movx a,@r0
    jz os_map
    jmp dv_money
os_map:
    jmp dv_wpcalc
os_turn:
    mov r0,#DV_SEAT
    movx a,@r0
    mov r5,a
    add a,#256-4
    jc ot_x
    mov r0,#DV_ST               ; kind: 3 our prompt, 1 a computer, 2 over,
    movx a,@r0                  ; 0 playing
    mov r6,#3
    xrl a,#DV_PROMPT
    jz ot_k
    mov a,r5                    ; a computer's: on the server it is over at
    call pl_flags               ; once, but it is "taking his turn" until
    mov r6,#1                   ; the next seat's (mekkogx develop_state)
    anl a,#PF_AI
    jnz ot_k
    mov r6,#2
    mov r0,#DV_FLAGS
    movx a,@r0
    anl a,#TF_ENDED
    jnz ot_k
    mov r6,#0
ot_k:
    mov a,r6
    mov r7,a
    mov a,r5
    rl a
    rl a
    orl a,r6
    mov r6,a
    mov r0,#DV_MSG
    movx a,@r0
    anl a,#0EFh                 ; bit 4: another's event said
    xrl a,r6
    jz ot_same
    mov a,r6
    movx @r0,a
    mov a,r7
    jz ot_nm
    xrl a,#1
    jz ot_cpu
    xrl a,#1!2
    jz ot_over
    mov a,#SND_ALERT
    FCALL beep
    SAY M_START, L_MAP+1, 2, C_WHITE
ot_x:
    ret
ot_same:
    mov a,r7                    ; still the same turn: its event may come
    jnz ot_x
    jmp ot_ev
ot_cpu:
    mov r0,#AS_F                ; its assays are in this state (as_scan)
    mov a,#1
    movx @r0,a
    SAY M_CPU, L_MAP+1, 2, C_WHITE
    ret
ot_over:                        ; ours: what the pub paid, or out of time
    mov r0,#MYSEAT
    movx a,@r0
    xrl a,r5
    jnz ot_blank
    mov r0,#ACTSENT
    movx a,@r0
    xrl a,#ACT_PUB
    jnz ot_time
    mov r0,#HRES
    movx a,@r0
    mov r2,a
    inc r0
    movx a,@r0
    mov r3,a
    SAY M_GAMBLE, L_MAP+1, 2, C_WHITE
    ret
ot_time:
    STB ST_TURN+TU_LEFT
    mov r4,a
    STB ST_TURN+TU_LEFT+1
    orl a,r4
    jnz ot_blank
    SAY M_TIMEUP, L_MAP+1, 2, C_WHITE
    ret
ot_blank:
    SAY S_NONE, L_MAP+1, 2, C_WHITE
    ret
ot_nm:
    jmp ot_name                 ; ends in ot_ev
; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "stbyte.inc"

; dv_answer: what the server said to the action this state answers
; (ACTSENT): in town (mekkogx dv_building_result) no money, no M.U.L.E.s,
; the assay's level; on the map (dv_map_result) a M.U.L.E. ran off, the
; wampus caught, land listed. Town words go on the money line for a few
; states. Clobbers A, r0-r7.
dv_answer:
    mov r0,#ACTSENT
    movx a,@r0
    mov r7,a
    mov r0,#HERR
    movx a,@r0
    jz da_ok
    mov r5,#W_NOMONEY
    xrl a,#ERR_MONEY
    jz da_town
    mov r5,#W_NOMULES
    xrl a,#ERR_MONEY!ERR_NOSTOCK
    jz da_town
    mov a,#SND_ERROR            ; anything else turned down: a buzz
    jmp da_snd
da_ok:
    mov a,r7
    xrl a,#ACT_ASSAY
    jz da_as
    xrl a,#ACT_ASSAY!ACT_BUYM   ; a M.U.L.E. bought, outfitted
    jz da_buy
    xrl a,#ACT_BUYM!ACT_OUTF
    jnz da_map
da_buy:
    mov a,#SND_BUY
da_snd:
    FCALL beep
    ret
da_as:
    mov r0,#HRES                ; 0 none .. 4 very high
    movx a,@r0
    anl a,#7
    add a,#256-5
    jnc da_lvl
    mov a,#4-5
da_lvl:
    add a,#5+W_ASSAY0
    mov r5,a
da_town:
    mov a,r5                    ; an error buzzes (an assay doesn't)
    add a,#256-W_ASSAY0
    jc da_hold
    mov a,#SND_ERROR
    FCALL beep
da_hold:
    mov r1,#IR_HOLD
    mov @r1,#5
    mov r6,#L_MAP|(1<<4)
    jmp da_say
da_map:
    mov a,r7
    xrl a,#ACT_INST
    jnz da_w
    mov r0,#HRES
    movx a,@r0
    xrl a,#1
    jnz da_buy                  ; installed
    mov a,#SND_ERROR
    FCALL beep
    mov r5,#M_RANOFF
    jmp da_m
da_w:
    xrl a,#ACT_INST!ACT_WAMP
    jnz da_s
    mov a,#SND_FOUNDER
    FCALL beep
    mov r5,#M_CAUGHT
    jmp da_m
da_s:
    xrl a,#ACT_WAMP!ACT_SELL
    jnz da_x
    mov r5,#M_LISTED
da_m:
    mov r6,#(L_MAP+1)|(2<<4)
da_say:
    mov r0,#HRES
    movx a,@r0
    mov r2,a
    inc r0
    movx a,@r0
    mov r3,a
    mov r7,#C_WHITE
    jmp say_q
da_x:
    ret

; dv_money: the town's top line, unless a message holds it: our money, and
; the store's M.U.L.E.s and their price: $1234  M14 $100. Clobbers A, r0-r7.
dv_money:
    mov r1,#IR_HOLD
    mov a,@r1
    jz dm_go
    dec a
    mov @r1,a
    ret
dm_go:
    call lb_clear
    mov r0,#LINEBUF
    mov a,#'$'
    movx @r0,a
    mov r0,#MYSEAT
    movx a,@r0
    anl a,#3
    call pl_off
    mov a,r2
    add a,#PL_MONEY
    mov r2,a
    call st_byte
    mov r4,a
    inc r2
    call st_byte
    mov r5,a
    mov r0,#LINEBUF+1
    call num5
    mov r0,#LINEBUF+8
    mov a,#'M'
    movx @r0,a
    mov r3,#(ST_STORE+STO_MULES)>>8
    mov r2,#(ST_STORE+STO_MULES)&0FFh
    call st_byte
    mov r4,a
    mov r5,#0
    mov r0,#LINEBUF+9
    call num2
    mov r0,#LINEBUF+12
    mov a,#'$'
    movx @r0,a
    mov r3,#(ST_STORE+STO_MULEPRICE)>>8
    mov r2,#(ST_STORE+STO_MULEPRICE)&0FFh
    call st_byte
    mov r4,a
    inc r2
    call st_byte
    mov r5,a
    mov r0,#LINEBUF+13
    call num3
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    movx @r0,a
    mov r2,#T0>>8
    mov r3,#T0&0FFh
    mov r4,#30
    jmp tx_build

; ===========================================================================
    org 0B00h

; num5/num3/num2: r5:r4 in decimal from r0, left-aligned, no leading zeros,
; at most 5/3/2 digits (the low ones). Clobbers A, r0-r3, r6, F0.
num5:
    mov r1,#n5tab&0FFh
    jmp nn_go
num3:
    mov r1,#n3tab&0FFh
    jmp nn_go
num2:
    mov r1,#n2tab&0FFh
nn_go:
    clr f0
nn_d:
    mov r6,#'0'
nn_s:
    mov a,r1                    ; r5:r4 + -10^k: no carry, the digit is done
    movp a,@a
    add a,r4
    mov r2,a
    mov a,r1
    inc a
    movp a,@a
    addc a,r5
    jnc nn_p
    mov r5,a
    mov a,r2
    mov r4,a
    inc r6
    mov a,r6
    xrl a,#'9'+1                ; (a digit past 9: the low digits only)
    jnz nn_s
    mov r6,#'9'
    jmp nn_s
nn_p:
    mov a,r6
    xrl a,#'0'
    jnz nn_o
    jf0 nn_o
    jmp nn_n
nn_o:
    clr f0
    cpl f0
    mov a,r6
    movx @r0,a
    inc r0
nn_n:
    inc r1
    inc r1
    mov a,r1
    xrl a,#nnend&0FFh
    jnz nn_d
    mov a,r4
    add a,#'0'
    movx @r0,a
    ret
n5tab:
    db 0F0h,0D8h, 018h,0FCh                         ; -10000 -1000
n3tab:
    db 09Ch,0FFh                                    ; -100
n2tab:
    db 0F6h,0FFh                                    ; -10
nnend:

; pl_off: r3:r2 = the offset of player A in the state. Clobbers A, r6.
pl_off:
    mov r6,a                    ; 29 * A = 32A - 2A - A
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
    add a,#ST_PLAYERS
    mov r2,a
    mov r3,#0
    ret

; pl_flags: A = player A's flags. Clobbers r0, r2, r3, r6.
pl_flags:
    call pl_off
    mov a,r2
    add a,#PL_FLAGS
    mov r2,a
    jmp st_byte

; dv_wpcalc: the wampus showing now (the turn's list, sent only to whoever
; plays it: from <= the turn's clock < to, in sixteenths), and where on its
; mountain; DV_WP 0FFh: none. Clobbers A, r0-r7.
dv_wpcalc:
    mov r0,#DV_WP
    mov a,#0FFh
    movx @r0,a
    STB ST_TURN+TU_ELAPSED
    mov r4,a
    STB ST_TURN+TU_ELAPSED+1
    mov r5,a
    call q16
    mov r1,#WX_W
    movx @r1,a
    STB ST_TURN+TU_NWAMPUS
    jz wc_x
    mov r7,a
    mov r3,#(ST_TURN+TU_WAMPUS)>>8
    mov r2,#(ST_TURN+TU_WAMPUS)&0FFh
wc_l:
    call st_inc
    mov r1,#WX_P
    movx @r1,a
    call st_inc
    mov r1,#WX_M
    movx @r1,a
    call st_inc                 ; from
    mov r4,a
    call st_inc
    mov r5,a
    call q16
    mov r1,#WX_F
    movx @r1,a
    call st_inc                 ; to
    mov r4,a
    call st_inc
    mov r5,a
    call q16
    mov r4,a
    mov r1,#WX_W                ; w < to: to + ~w carries
    movx a,@r1
    mov r6,a
    cpl a
    add a,r4
    jnc wc_n
    mov r1,#WX_F                ; from <= w: from + ~w doesn't carry
    movx a,@r1
    mov r4,a
    mov a,r6
    cpl a
    add a,r4
    jc wc_n
    jmp wc_got
wc_n:
    djnz r7,wc_l
wc_x:
    ret

; ===========================================================================
; as_put: the mask byte just done (r4) to its place: the one before plot r7's.
; Clobbers A, r0-r2.
as_put:
    mov a,r7
    add a,#255
    rr a
    rr a
    rr a
    anl a,#1Fh
    add a,#ASMASK&0FFh
    mov r2,#ASMASK>>8
    call poke_at
    mov a,r4
    jmp poke_b

; as_off: nothing under way: the box away. Clobbers A, r0-r2.
as_off:
    mov r0,#AS_I
    movx a,@r0
    jz ao_x
    clr a
    movx @r0,a
    mov r2,#SPRTAB>>8
    mov a,#(SPRTAB+9)&0FFh
    call poke_at
    mov a,#0F8h
    jmp poke_b
ao_x:
    ret

; as_load: sprite 3's shape: a box 8 x 8. Clobbers A, r0, r1, r6.
as_load:
    mov a,#P1_VDC
    outl p1,a
    mov r0,#vdc_spr0_shape+24
    mov r1,#as_shp&0FFh
    mov r6,#8
al_l:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r6,al_l
    mov a,#P1_MAIN
    outl p1,a
    ret
as_shp:
    db 0FFh, 081h, 081h, 081h, 081h, 081h, 081h, 0FFh

    org 0C00h

; wc_got: the wampus on plot WX_P's mountain WX_M: x 10 + 16 col + 2 m,
; y 52 + 24 row.
wc_got:
    mov r0,#DV_WP
    clr a
    movx @r0,a
    mov r1,#WX_P
    movx a,@r1
    mov r6,#52
wg_r:
    add a,#256-9
    jnc wg_c
    mov r5,a
    mov a,r6
    add a,#24
    mov r6,a
    mov a,r5
    jmp wg_r
wg_c:
    add a,#9                    ; the column
    swap a
    add a,#10
    mov r5,a
    mov r1,#WX_M
    movx a,@r1
    anl a,#3
    rl a
    add a,r5
    mov r0,#DV_WPX
    movx @r0,a
    inc r0
    mov a,r6
    movx @r0,a
    ret

; st_inc: A = the state's byte at r3:r2, and r3:r2 on. Clobbers r0, r6.
st_inc:
    call st_byte
    inc r2
    xch a,r2
    jnz si_x
    inc r3
si_x:
    xch a,r2
    ret

; q16: A = r5:r4 / 16, the low 8 bits (the turn's clock in sixteenths).
q16:
    mov a,r4
    swap a
    anl a,#0Fh
    mov r6,a
    mov a,r5
    swap a
    anl a,#0F0h
    orl a,r6
    ret

; ot_name: (on_state) TURN: <name of seat r5>, in its colour.
ot_name:                        ; TURN: <name>, in its colour
    call lb_clear
    mov r0,#LINEBUF+3
    mov r1,#5
    mov r4,#s_turn&0FFh
on_t:
    mov a,r4
    movp a,@a
    movx @r0,a
    inc r0
    inc r4
    djnz r1,on_t
    mov a,r5
    call pl_off
    mov r1,#LINEBUF+9
    mov r4,#7
on_n:
    call st_byte
    jz on_c
    movx @r1,a
    inc r1
    inc r2
    djnz r4,on_n
on_c:
    mov a,r5
    add a,#dvcol&0FFh
    movp a,@a
    mov r0,#TXCOL
    movx @r0,a
    inc r0
    movx @r0,a
    mov r2,#T1>>8
    mov r3,#T1&0FFh
    mov r4,#188
    call tx_build
    SAY S_NONE, L_MAP+2, 1, C_WHITE
; ot_ev: another seat's turn event (the first state that has one), on the
; second message line, with its amount for that seat's M.U.L.E.s; the turn
; name is on the line above. DV_MSG bit 4 says it was shown. Clobbers all.
ot_ev:
    mov r0,#DV_MSG
    movx a,@r0
    jb4 oe_x
    STB ST_TURN+TU_EVENT
    jz oe_x
    mov r7,a
    mov r0,#DV_MSG
    movx a,@r0
    orl a,#10h
    movx @r0,a
    mov a,r7
    jmp de_ok
oe_x:
    ret
s_turn: db "TURN:"
dvcol:
    db C_RED, C_GREEN, C_TAN, C_VIOLET

; ===========================================================================
; ---------------------------------------------------------------------------
; A computer's assays. The server plays its turn at once, so the first state
; of it has every plot it assayed (the map's crystite, -1 till anyone does);
; each state's assayed plots are kept as bits (ASMASK) to tell those from the
; ones seen before. From 120 frames in, every 120 a plot is boxed (sprite 3)
; and its level said, as the Atari does; the server's turn display allows it.

; as_new: plot r7, level r6 is new: queued (ASQ, ASQL, AS_N) while a computer's
; first state is read (AS_F), 8 at most. Clobbers A, r0-r3.
as_new:
    mov r0,#AS_F
    movx a,@r0
    jz an_x
    mov r0,#AS_N
    movx a,@r0
    mov r3,a
    add a,#256-8
    jc an_x
    mov a,r3
    add a,#ASQ&0FFh
    mov r2,#ASQ>>8
    call poke_at
    mov a,r7
    call poke_b
    mov a,r3
    add a,#ASQL&0FFh
    mov r2,#ASQL>>8
    call poke_at
    mov a,r6
    anl a,#7
    call poke_b
    mov r0,#AS_N
    movx a,@r0
    inc a
    movx @r0,a
an_x:
    ret

at_end:
    call as_off
    SAY M_CPU, L_MAP+1, 2, C_WHITE
    ret

    org 0D00h

; dv_tbar: the turn's time bar (tbar.inc), as mekkogx dv_bar: while a turn
; is under way, its PTU left, (left - age) / pTU, a unit every pTU frames;
; a segment is 320 jiffies' worth (320 / pTU units). Clobbers A, r0-r7.
dv_tbar:
    clr a
    mov r0,#TB_U
    movx @r0,a
    STB ST_TURN+TU_PTU
    jz dt_x
    mov r7,a
    mov r5,#1
    mov r4,#40h
    call dv_div
    mov a,r3
    mov r0,#TB_SEG
    movx @r0,a
    mov r0,#DV_FLAGS
    movx a,@r0
    anl a,#TF_STARTED|TF_ENDED
    xrl a,#TF_STARTED
    jnz dt_x
    STB ST_TURN+TU_LEFT
    mov r4,a
    STB ST_TURN+TU_LEFT+1
    mov r5,a
    mov r0,#RXAGE               ; left - age - 1: no carry, none left
    movx a,@r0
    cpl a
    add a,r4
    mov r4,a
    mov a,r5
    addc a,#0FFh
    mov r5,a
    jnc dt_x
    inc r4
    mov a,r4
    jnz dt_d
    inc r5
dt_d:
    call dv_div
    mov a,r3
    mov r0,#TB_U
    movx @r0,a
    mov a,r4                    ; the next unit in the remainder + 1 frames
    inc a
    mov r0,#TB_CNT
    movx @r0,a
    mov a,r7
    mov r0,#TB_PER
    movx @r0,a
dt_x:
    ret

; dv_div: r3 = r5:r4 / r7 (255 at most), r4 the remainder. Clobbers A, r6.
dv_div:
    mov r3,#0
dd_l:
    mov a,r7
    cpl a
    inc a
    add a,r4
    mov r6,a
    mov a,r5
    addc a,#0FFh
    jnc dd_x
    mov r5,a
    mov a,r6
    mov r4,a
    inc r3
    mov a,r3
    inc a
    jnz dd_l
dd_x:
    ret

; as_scan: every state, last in on_state: the map's crystite, a plot a bit.
; The first state of a computer's turn (AS_F) also queues the plots not in
; ASMASK and starts the show. Any other state of another kind of turn stops
; one. Clobbers A, r0-r7.
as_scan:
    mov r0,#AS_F
    movx a,@r0
    jz as_nc
    clr a
    mov r0,#AS_N
    movx @r0,a
    call as_off
    jmp as_go
as_nc:
    mov r0,#DV_MSG
    movx a,@r0
    anl a,#3
    xrl a,#1
    jz as_go
    call as_off
as_go:
    mov r7,#0                   ; the plot
as_b:
    mov a,r7                    ; its byte of the old mask
    rr a
    rr a
    rr a
    anl a,#1Fh
    add a,#ASMASK&0FFh
    FCALL fx6_get
    mov r5,a
    clr a
    mov r4,a                    ; the new one
as_p:
    mov a,r7
    add a,#(ST_MAP+MAP_CRYSTITE)&0FFh
    mov r2,a
    mov r3,#(ST_MAP+MAP_CRYSTITE)>>8
    call st_byte
    jb7 as_nx                   ; -1: not assayed
    mov r6,a
    mov a,r7
    anl a,#7
    add a,#as_bits&0FFh
    movp a,@a
    mov r1,a
    orl a,r4
    mov r4,a
    mov a,r5
    anl a,r1
    jnz as_nx                   ; seen before
    call as_new
as_nx:
    inc r7
    mov a,r7
    xrl a,#NUM_PLOTS
    jz as_last
    mov a,r7
    anl a,#7
    jnz as_p
    call as_put
    jmp as_b
as_last:
    call as_put
    mov r0,#AS_F
    movx a,@r0
    jz as_x
    clr a
    movx @r0,a
    mov r0,#AS_N
    movx a,@r0
    jz as_x                     ; none assayed: nothing to show
    call as_load
    clr a
    mov r0,#AS_T
    movx @r0,a
    inc a
    mov r0,#AS_I
    movx @r0,a
as_x:
    ret
as_bits:
    db 1, 2, 4, 8, 10h, 20h, 40h, 80h

; as_tick: every frame of the development: the show's next step is due 120
; frames after the last: the box on the next plot with what was found there
; (W_ASSAY0 + level), after the last the box off and the turn's words back.
; Clobbers A, r0-r7.
as_tick:
    mov r0,#AS_I
    movx a,@r0
    jz at_x
    mov r0,#TICKN
    movx a,@r0
    mov r2,a
    mov r0,#AS_T
    movx a,@r0
    add a,r2
    add a,#256-120
    jc at_f
    add a,#120
    movx @r0,a
at_x:
    ret
at_f:
    jmp at_fire

    org 0E00h                   ; (its table must sit on its MOVP's page)

; dv_event: once a turn of ours (DV_MSG 0FEh after), its player event
; (turn.event 1-22) on the town's top line, held a few states, with its
; amount (mekkogx dv_event_msg): 25 (round / 4 + 1) times the event's
; factor, or per M.U.L.E. of ours of a kind (food, mining, energy).
; Clobbers A, r0-r7.
dv_event:
    mov r0,#DV_MSG
    movx a,@r0
    xrl a,#0FEh
    jz de_x
    mov a,#0FEh
    movx @r0,a
    STB ST_TURN+TU_EVENT
    jz de_x
de_ok:
    mov r7,a                    ; k
    add a,#256-23
    jc de_x
    mov r0,#RND                 ; unit = 25 (round / 4 + 1)
    movx a,@r0
    rr a
    rr a
    anl a,#3Fh
    inc a
    mov r5,a
    clr a
de_u:
    add a,#25
    djnz r5,de_u
    mov r4,a                    ; unit
    mov a,r7
    add a,#(pev_x-1)&0FFh
    movp a,@a
    mov r5,a                    ; the factor
    jb7 de_mule
de_mul:                         ; r3:r2 = unit * r5
    mov r2,#0
    mov r3,#0
    mov a,r5
    jz de_say
de_m:
    mov a,r2
    add a,r4
    mov r2,a
    clr a
    addc a,r3
    mov r3,a
    djnz r5,de_m
de_say:
    mov a,r7
    add a,#E_1-1
    mov r5,a
    mov r6,#L_MAP|(1<<4)
    mov r0,#DV_MSG              ; another's event: the second message line
    movx a,@r0
    xrl a,#0FEh
    jz de_own
    mov r6,#(L_MAP+2)|(1<<4)
de_own:
    mov r7,#C_WHITE
    mov r1,#IR_HOLD
    mov @r1,#6
    jmp say_q
de_x:
    ret
de_mule:                        ; per M.U.L.E. of ours: 80h food, 81h
    mov r1,#WX_F                ; mining (smithore, crystite), 82h energy
    mov a,r5
    movx @r1,a
    mov r1,#0                   ; n
    mov r5,#0                   ; the plot
de_p:
    mov a,r5
    add a,#(ST_MAP+MAP_OWNER)&0FFh
    mov r2,a
    clr a
    addc a,#(ST_MAP+MAP_OWNER)>>8
    mov r3,a
    call st_byte
    mov r6,a
    mov r0,#DV_SEAT             ; the seat whose turn it is
    movx a,@r0
    xrl a,r6
    jnz de_n
    mov a,r5
    add a,#(ST_MAP+MAP_MULE)&0FFh
    mov r2,a
    clr a
    addc a,#(ST_MAP+MAP_MULE)>>8
    mov r3,a
    call st_byte
    jb7 de_n                    ; none
    mov r6,a
    mov r0,#WX_F
    movx a,@r0
    xrl a,#80h
    jnz de_k1
    mov a,r6                    ; food
    jz de_c
    jmp de_n
de_k1:
    xrl a,#80h!81h
    jnz de_k2
    mov a,r6                    ; mining
    anl a,#2
    jnz de_c
    jmp de_n
de_k2:
    mov a,r6                    ; energy
    xrl a,#1
    jnz de_n
de_c:
    inc r1
de_n:
    inc r5
    mov a,r5
    xrl a,#NUM_PLOTS
    jz de_pe
    jmp de_p
de_pe:
    mov a,r1                    ; unit * n, twice that but for energy
    mov r5,a
    mov r0,#WX_F
    movx a,@r0
    xrl a,#82h
    jz de_go
    mov a,r5
    rl a
    mov r5,a
de_go:
    jmp de_mul

pev_x:                          ; factors: 0 none; 80h-82h per M.U.L.E.
    db 0, 0, 2, 4, 80h, 4, 8, 2, 3, 6, 4, 2, 0, 0, 3, 81h, 82h, 6, 4, 4, 4, 0


at_fire:
    movx @r0,a                  ; (what went over 120 counts to the next)
    mov r0,#AS_I
    movx a,@r0
    mov r3,a
    inc a
    movx @r0,a                  ; (r3: this step + 1)
    mov r0,#AS_N
    movx a,@r0
    inc a
    xrl a,r3
    jnz at_s
    jmp at_end
at_s:
    mov a,r3
    add a,#(ASQ-1)&0FFh
    FCALL fx6_get
    call plot_xy                ; r7 X, r6 Y: the 16 x 14 box's corner
    mov r2,#SPRTAB>>8
    mov a,#(SPRTAB+9)&0FFh
    call poke_at
    mov a,r6                    ; our 8 x 8 one is in the middle of it
    add a,#3
    call poke_b
    mov a,r7
    add a,#4
    call poke_b
    mov a,#SPR_WHITE
    call poke_b
    mov a,r3
    add a,#(ASQL-1)&0FFh
    FCALL fx6_get
    add a,#W_ASSAY0
    mov r5,a
    mov r6,#(L_MAP+1)|(2<<4)
    mov r7,#C_WHITE
    jmp say_q


    PAGE_TAIL
