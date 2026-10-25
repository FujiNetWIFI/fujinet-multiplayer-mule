; p2.inc -- page 2: the game's main loop (mekkogx main.c). Each frame: the
; stick, a step of the request in flight (page 1), and the phase's own page --
; its on_state when a state has come, its tick every frame. All by farcall
; from here, the top level (tools/checkstack.py).

E_SAY   equ 0                   ; a strings page's entry (pstr.inc)
PG_NET  equ 1                   ; the network page's entries
E_NBEGIN equ 0
E_NSTEP equ 1
E_STATE equ 0                   ; a phase page's: a state has come ...
E_TICK  equ 1                   ; ... and every frame
POLLN   equ 20                  ; frames between polls: a third of a second

    PAGE_HEAD
    db e_main&0FFh              ; entry 0: from the boot, one way
e_main:
    jmp main

; ===========================================================================
    org 900h
    include "stbyte.inc"

; ===========================================================================
    org 0A00h

main:
    mov r0,#LASTPH              ; nothing shown yet
    mov a,#0FFh
    movx @r0,a
    mov r0,#LASTPG
    movx @r0,a
    clr a
    mov r0,#POLLW
    movx @r0,a
    mov r0,#SAYA+SAY_ID         ; nothing to say
    movx @r0,a
    mov r0,#SAYB+SAY_ID
    movx @r0,a
ml:
    call frame_wait
    call joy_poll
    mov r7,#PG_NET
    mov a,#E_NSTEP
    FCALL farcall
    mov r0,#NSTATE
    movx a,@r0
    jnz ml_tick                 ; a request is out
    mov r0,#NETHAVE
    movx a,@r0
    xrl a,#NH_STATE
    jnz ml_idle
    clr a                       ; a state: taken
    movx @r0,a
    call st_header
    call rx_age
    clr a                       ; no time bar, unless its page sets one
    mov r0,#TB_SEG
    movx @r0,a
    call st_page
    mov a,#E_STATE
    FCALL farcall
    call st_page2               ; its phase's second page, if any
    mov a,r7
    jz ml_drawn
    mov a,#E_STATE
    FCALL farcall
ml_drawn:
    call tb_draw
ml_idle:
    mov r0,#ACT                 ; an action goes at once; a poll waits its turn
    movx a,@r0
    jnz ml_begin
    mov r0,#POLLW
    movx a,@r0
    jz ml_begin
    dec a
    movx @r0,a
    jmp ml_tick
ml_begin:
    mov r0,#POLLW
    mov a,#POLLN
    movx @r0,a
    mov r7,#PG_NET
    mov a,#E_NBEGIN
    FCALL farcall
ml_tick:
    call pg_tick                ; (a walk can change the page)
    mov r1,#FRAMES              ; TICKN: the frames this tick stands for
    mov a,@r1
    mov r2,a
    mov r0,#TICKF
    movx a,@r0
    cpl a
    inc a
    add a,r2
    mov r0,#TICKN
    movx @r0,a
    mov a,r2
    mov r0,#TICKF
    movx @r0,a
    call phase_page
    mov a,#E_TICK
    FCALL farcall
    call tb_frame
    call snd_go
ml_say:
    mov r0,#SAYA+SAY_ID         ; what the phase asked to be said: by the
    movx a,@r0                  ; strings page of its bank (id / 64)
    jz ml
    swap a
    rr a
    rr a
    anl a,#3
    add a,#str_pages&0FFh
    movp a,@a
    mov r7,a
    mov a,#E_SAY
    FCALL farcall
    jmp ml_say
str_pages:                      ; a strings bank's page (tools/mkstr.py)
    db 5, 9, 14

; st_header: the header's fields the frame code reads, into HOT.
; Clobbers A, r0-r7.
st_header:
    mov r5,#hdr_tab&0FFh
sh_l:
    mov a,r5
    movp a,@a                   ; the field's offset (0: done)
    jz sh_x
    mov r2,a
    mov r3,#0
    call st_byte
    mov r4,a
    inc r5
    mov a,r5
    movp a,@a                   ; where it goes
    mov r0,a
    mov a,r4
    movx @r0,a
    inc r5
    jmp sh_l
sh_x:
    ret
hdr_tab:
    db HDR_PHASE, PHS
    db HDR_ROUND, RND
    db HDR_MYSEAT, MYSEAT
    db HDR_ERR, HERR
    db HDR_T, HT
    db HDR_T+1, HT+1
    db HDR_ELAPSED, HPE
    db HDR_ELAPSED+1, HPE+1
    db HDR_COUNTDOWN, HCD
    db HDR_COUNTDOWN+1, HCD+1
    db HDR_RESULT, HRES
    db HDR_RESULT+1, HRES+1
    db 0

; ===========================================================================
    org 0B00h
    include "input.inc"

; st_page: r7 = the page for this state, its auction stage noted first. A
; state for another page than the last one's starts that page afresh: its
; screen is not up (LASTPH none). Clobbers A, r0-r3, r6.
st_page:
    STB ST_AUCTION+AU_STAGE
    mov r0,#AUSTG
    movx @r0,a
    call phase_page
sp_chk:
    mov r0,#LASTPG
    movx a,@r0
    xrl a,r7
    jz sp_x
    mov a,r7
    movx @r0,a
    mov r0,#LASTPH
    mov a,#0FFh
    movx @r0,a
sp_x:
    ret

; pg_tick: the tick's page: one other than the last starts afresh (a walk
; into town or out of it), as a state's does. Clobbers A, r0, r7.
pg_tick:
    call phase_page
    jmp sp_chk

; st_page2: r7 = the page that runs after this state's, or 0: an
; auction's text (page 10) after its arena (8); the development's words
; (13) after its map or town (11, 12). Clobbers A, r0.
st_page2:
    mov r0,#PHS
    movx a,@r0
    mov r7,#13
    xrl a,#PHASE_DEVELOP
    jz sp2_x
    call phase_page
    mov a,r7
    xrl a,#8
    mov r7,#0
    jnz sp2_x
    mov r7,#10
sp2_x:
    ret



; ===========================================================================
    org 0C00h

; rx_age: RXAGE = how much older this state is than the fastest reply yet
; (mekkogx clock_sync): the server's clock less ours is largest for the
; fastest, and FRAMES never misses a vblank, so a larger one is a faster
; reply. The lobby has no clock: a game starts afresh. TICKF too: the
; state's countdowns run from its arrival. Clobbers A, r0-r2.
rx_age:
    mov r1,#FRAMES              ; lag = t - FRAMES (low bytes)
    mov a,@r1
    mov r0,#TICKF
    movx @r0,a
    cpl a
    inc a
    mov r0,#HT
    mov r2,a
    movx a,@r0
    add a,r2
    mov r2,a
    clr a
    mov r0,#RXAGE
    movx @r0,a
    mov r0,#PHS
    movx a,@r0
    mov r0,#LAGH
    jnz ra_game
    movx @r0,a                  ; the lobby: none yet
    ret
ra_game:
    movx a,@r0
    jz ra_base                  ; the first state of a game
    mov r0,#LAGB                ; d = lag - base
    movx a,@r0
    cpl a
    inc a
    add a,r2
    jz ra_x
    jb7 ra_old
ra_base:                        ; the fastest yet (or the first)
    mov r0,#LAGH
    mov a,#1
    movx @r0,a
    mov r0,#LAGB
    mov a,r2
    movx @r0,a
ra_x:
    ret
ra_old:
    cpl a                       ; older by -d jiffies, 30 at most; more than
    inc a                       ; 120: the server's clock went back
    mov r1,a
    add a,#256-121
    jc ra_base
    mov a,r1
    add a,#256-31
    mov a,r1
    jnc ra_put
    mov a,#30
ra_put:
    mov r0,#RXAGE
    movx @r0,a
    ret

; phase_page: r7 = the page that runs phase PHS: an auction's plot is shown
; on the map (page 6) and a good's status on the text screen (7) until
; their bidding; the ship (15) before round 0's summary and round 1's
; grant. Clobbers A, r0, r1, r6.
phase_page:
    mov r0,#AUSTG               ; (an auction showing its plot or good)
    movx a,@r0
    mov r0,#PHS
    xrl a,#STAGE_SHOW
    jnz pp_tab
    movx a,@r0
    mov r7,#6
    xrl a,#PHASE_LANDAUCTION
    jz pp_x
    mov r7,#7
    xrl a,#PHASE_LANDAUCTION!PHASE_DECLARE
    jz pp_x
    xrl a,#PHASE_DECLARE!PHASE_TRADING
    jz pp_x
pp_tab:
    movx a,@r0
    anl a,#0Fh
    xrl a,#PHASE_DEVELOP
    jz pp_dev
    xrl a,#PHASE_DEVELOP!PHASE_SUMMARY
    jz pp_sum
    xrl a,#PHASE_SUMMARY!PHASE_GRANT
    jz pp_gr
pp_own:
    mov r0,#PHS
    movx a,@r0
    anl a,#0Fh
    add a,#phase_tab&0FFh
    movp a,@a
    mov r7,a
pp_x:
    ret
pp_sum:                         ; round 0's summary: the ship lands first
    mov r0,#RND                 ; if it began under 300 jiffies ago
    movx a,@r0
    jnz pp_own
    mov r1,#~(300-1)&0FFh
    mov r6,#~((300-1)>>8)&0FFh
    jmp pp_ship
pp_gr:                          ; round 1's grant: it leaves first, within
    mov r0,#RND                 ; 700 (mekkogx summary_enter, grant_enter)
    movx a,@r0
    xrl a,#1
    jnz pp_own
    mov r1,#~(700-1)&0FFh
    mov r6,#~((700-1)>>8)&0FFh
; pp_ship: the ship's page (15) while it shows: FXSH is the phase then, and
; the phase | 80h once it has gone; else (a new phase) it shows if the
; phase is under r6:r1 = ~(limit - 1) jiffies old. Clobbers A, r0, r7.
pp_ship:
    mov r0,#PHS
    movx a,@r0
    mov r7,a
    mov r0,#FXSH
    movx a,@r0
    xrl a,r7
    jz pp_15
    xrl a,#80h
    jz pp_own
    mov r0,#HPE                 ; HPE + ~(limit - 1) carries: too late
    movx a,@r0
    add a,r1
    inc r0
    movx a,@r0
    addc a,r6
    mov a,r7
    jc pp_late
    mov r0,#FXSH
    movx @r0,a
pp_15:
    mov r7,#15
    ret
pp_late:
    orl a,#80h
    mov r0,#FXSH
    movx @r0,a
    jmp pp_own
pp_dev:                         ; the development: the town while we walk
    mov r0,#RND                 ; it (this round's: DV_PH), else the map
    movx a,@r0
    mov r7,a
    mov r0,#DV_PH
    movx a,@r0
    xrl a,r7
    mov r7,#11
    jnz pp_x
    mov r0,#DV_TOWN
    movx a,@r0
    xrl a,#1
    jnz pp_x
    mov r7,#12
    ret

phase_tab:                      ; PHASE_* -> its page
    db 3, 4, 8, 11, 15, 8, 8, 7, 7, 3, 3, 3, 3, 3, 3, 3

; ===========================================================================
    org 0D00h
    include "snd.inc"

; ===========================================================================
    org 0E00h
    include "tbar.inc"

    PAGE_TAIL
