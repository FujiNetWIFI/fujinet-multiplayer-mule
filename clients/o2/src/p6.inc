; p6.inc -- page 6: the colony map's other phases. The production (mekkogx
; events.c production_*): a colony event first, its title and message, its
; plot boxed, for PR_SHOW frames; then the title and the units each M.U.L.E.
; made, in place of its letter. And a land auction's plot (auction.c, stage
; SHOW): boxed and blinking, with the opening price; the button skips the
; wait. Its bidding is page 8's.

E_STATE equ 0
E_TICK  equ 1
NPLOTS  equ 45
PR_SHOW equ 240
SPR_WHITE equ C_WHITE<<3        ; a sprite's colour, attribute bits 3-5

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp on_state
e_tick:
    jmp tick

    include "sayq.inc"

; on_state: a state has come. A new phase gets the map and the event; then
; the plots, with the units made once the event is over.
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
    call map_screen
    mov r0,#PHS
    movx a,@r0
    xrl a,#PHASE_PRODUCTION
    jnz os_land
    call pr_enter
os_same:
    mov r0,#PHS                 ; the units made: production, after the event
    movx a,@r0
    xrl a,#PHASE_PRODUCTION
    jnz os_let
    mov r0,#PR_WAIT
    movx a,@r0
    jz os_dig
os_let:
    clr a
    jmp os_put
os_dig:
    mov a,#1
os_put:
    mov r0,#MAPDIG
    movx @r0,a
    jmp map_build

; os_land: a land auction's plot: its title, the opening price; the box.
os_land:
    STB ST_AUCTION+AU_PLOT
    mov r0,#PR_PLOT
    movx @r0,a
    mov r0,#RND
    movx a,@r0
    mov r2,a
    mov r3,#0
    SAY T_LAUCT, L_MAP, 1, C_WHITE
    STB ST_AUCTION+AU_START+1
    mov r4,a
    STB ST_AUCTION+AU_START
    mov r2,a
    mov a,r4
    mov r3,a
    SAY M_BIDST, L_MAP+1, 2, C_WHITE
    jmp os_same

; pr_enter: the colony event (1-9), or straight to the production.
pr_enter:
    STB ST_COLONY+CO_PLOT
    mov r0,#PR_PLOT
    movx @r0,a
    STB ST_COLONY+CO_EVENT
    mov r0,#PR_EV
    movx @r0,a
    mov r4,a
    jz pr_none
    add a,#256-10
    jc pr_none
    mov r0,#PR_WAIT
    mov a,#PR_SHOW
    movx @r0,a
    mov a,r4                    ; its title, its message
    add a,#T_EV1-1
    mov r5,a
    mov r6,#L_MAP|(1<<4)
    mov r7,#C_WHITE
    call say_q
    mov a,r4
    add a,#M_EV1-1
    mov r5,a
    mov r6,#(L_MAP+1)|(2<<4)
    call say_q
    ret
pr_none:
    clr a
    mov r0,#PR_WAIT
    movx @r0,a
    ; fall into pr_title

; pr_title: PRODUCTION #round, the messages blank. Clobbers A, r0, r2, r3,
; r5-r7.
pr_title:
    mov r0,#RND
    movx a,@r0
    mov r2,a
    mov r3,#0
    SAY T_PROD, L_MAP, 1, C_WHITE
    SAY S_NONE, L_MAP+1, 2, C_WHITE
    ret

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "stbyte.inc"
    include "cursor.inc"

; ===========================================================================
MAPBASE equ 0B00h
    include "map.inc"

; ===========================================================================
    org 0D00h

; tick: every frame. A row of the map, while the state's are not all built;
; the event's plot boxed while it is up; when it is over, the production
; (its digits come with the next state).
tick:
    mov r0,#MB_ROW
    movx a,@r0
    add a,#256-5
    jc tk_ev
    call map_row
    mov r1,#DIRTY               ; its mountains' right halves are new
    mov a,@r1
    orl a,#DIRTY_STATIC
    mov @r1,a
tk_ev:
    mov r0,#PHS
    movx a,@r0
    xrl a,#PHASE_PRODUCTION
    jz tk_prod
    mov r0,#JOYEV               ; a land auction's plot: the button skips
    movx a,@r0                  ; the wait
    anl a,#J_FIRE
    jz tk_blink
    mov r0,#MYSEAT
    movx a,@r0
    jb7 tk_blink
    mov r0,#ACT
    mov a,#ACT_READY
    movx @r0,a
tk_blink:
    mov r1,#FRAMES              ; the box: 16 frames on, 16 off
    mov a,@r1
    anl a,#10h
    jz tk_hide
    jmp tk_box
tk_prod:
    mov r0,#PR_WAIT
    movx a,@r0
    jz tk_hide
    dec a
    movx @r0,a
    jz tk_done
tk_box:
    mov r0,#PR_PLOT
    movx a,@r0
    mov r5,a
    add a,#256-NPLOTS
    jc tk_hide
    mov a,r5
    mov r0,#G_CUR
    movx @r0,a
    jmp cur_show
tk_done:
    call pr_title
tk_hide:
    jmp cur_hide

    PAGE_TAIL
