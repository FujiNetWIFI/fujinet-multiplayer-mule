; p15.inc -- page 15: the map's set pieces (mekkogx events.c, fx.c): the
; production, its colony event first; the transport ship landing (round 0's
; summary) and leaving (round 1's grant), until it is gone and the phase's
; own page takes over (p2.inc pp_ship).
;
; An event is a script for the actor (an O2 fx.c): one to four sprites side
; by side that it moves, shows and hides by the frame. A ship is all four (32
; dots, two cells), along row 2's band, the store's; the rest one. Sprites
; never cross a row gap but the meteorite's, which blinks in each.

E_STATE equ 0
E_TICK  equ 1
NPLOTS  equ 45
SPR_WHITE equ C_WHITE<<3        ; a sprite's colour, attribute bits 3-5
SHIP_Y  equ Y_ROW0+2*ROW_PITCH  ; the store's row
SHIP_X  equ 16*4+X_QA-1+8-16    ; docked: centred on the store
FX_HID  equ 04h                 ; FX_COL: hidden (the pieces - 1 in bits 6-7)

; The script ops: OP, then its bytes.
OP_END  equ 0                   ; stop
OP_POS  equ 1                   ; x, y: there, shown
OP_WAIT equ 2                   ; n: frames
OP_MOVE equ 3                   ; d, per, n: n steps of d (dx << 4 | dy,
                                ; signed nibbles), one every per frames
OP_HIDE equ 4
OP_SHOW equ 5
OP_LOOK equ 6                   ; shape, colour (attribute): its sprites
OP_LOOP equ 7                   ; count, at: back to `at` count more times
OP_GO   equ 8                   ; at: on from `at`
MV      function dx,dy,((dx)&0Fh)<<4|((dy)&0Fh)
SCR     function lab,(lab)-0E00h    ; a script: its offset in page $E00

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp on_state
e_tick:
    jmp tick

    include "sayq.inc"

; on_state: a state has come. A new phase gets the map and its piece: the
; colony event, or the ship; then the plots, with the units made once the
; event is over. A ship leaving is gone as soon as the grant's cursor runs
; (mekkogx grant.c): the grant's page has to show it.
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
    call fx_stop
    mov r0,#PHS
    movx a,@r0
    xrl a,#PHASE_PRODUCTION
    jz os_prod
    xrl a,#PHASE_PRODUCTION!PHASE_GRANT
    jz os_dep
    SAY T_SHIP, L_MAP, 1, C_WHITE
    SAY M_LANDING, L_MAP+1, 2, C_WHITE
    mov a,#SCR(fx_land)
    jmp os_go
os_dep:
    SAY T_SHIP, L_MAP, 1, C_WHITE
    SAY M_DEPART, L_MAP+1, 2, C_WHITE
    mov a,#SCR(fx_depart)
os_go:
    call fx_start
    jmp os_same
os_prod:
    call pr_enter
os_same:
    mov r0,#PHS
    movx a,@r0
    xrl a,#PHASE_GRANT
    jnz os_dig
    STB ST_GRANT+GR_CURSOR      ; under 45: it runs
    add a,#256-NPLOTS
    jc os_dig
    call fx_stop
    call ship_gone
os_dig:
    clr a                       ; the units made: production, after the event
    mov r6,a
    mov r0,#PHS
    movx a,@r0
    xrl a,#PHASE_PRODUCTION
    jnz os_put
    mov r0,#FX_PC
    movx a,@r0
    jnz os_put
    inc r6
os_put:
    mov a,r6
    mov r0,#MAPDIG
    movx @r0,a
    jmp map_build

; ship_gone: the ship's phase may go to its own page (pp_ship). Clobbers A,
; r0, r6.
ship_gone:
    mov r0,#PHS
    movx a,@r0
    orl a,#80h
    mov r0,#FXSH
    movx @r0,a
    ret

; pr_enter: the colony event (1-9) -- its sound, title, message and piece
; -- or straight to the production.
pr_enter:
    STB ST_COLONY+CO_PLOT
    mov r0,#PR_PLOT
    movx @r0,a
    STB ST_COLONY+CO_EVENT
    mov r0,#PR_EV
    movx @r0,a
    mov r4,a
    jz pr_title
    add a,#256-10
    jc pr_title
    mov a,#SND_EVENT
    FCALL beep
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
    jmp ev_start

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

; fx_tick: TICKN frames of the actor, then its sprites. Clobbers A, r0-r7.
fx_tick:
    mov r0,#TICKN
    movx a,@r0
    jz ft_d
    mov r5,a
ft_l:
    call fx_frame
    mov r0,#FX_PC
    movx a,@r0
    jz ft_d
    djnz r5,ft_l
ft_d:
    jmp fx_draw

; ===========================================================================
    org 900h
DISP_MAP equ 1
    include "disp.inc"

; fx_draw: the actor's sprites from FX_X, FX_Y, FX_COL: its pieces 8 dots
; apart (a ship's middle two white), the rest hidden; a piece off the screen's
; side (x 166 on: right of it, or left as a negative x) hidden too.
; Clobbers A, r0-r7.
fx_draw:
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov r0,#FX_X
    movx a,@r0
    mov r3,a
    mov r0,#FX_Y
    movx a,@r0
    mov r2,a
    mov r0,#FX_COL
    movx a,@r0
    mov r6,a
    rl a
    rl a
    anl a,#3
    inc a
    mov r4,a
    mov a,r6
    anl a,#FX_HID
    jz fd_go
    mov r4,#0
fd_go:
    mov r7,#0
fd_l:
    mov a,r7                    ; k < pieces: pieces + ~k carries
    cpl a
    add a,r4
    jnc fd_hide
    mov a,r3
    add a,#256-166
    jc fd_hide
    mov a,r2
    call poke_b
    mov a,r3
    call poke_b
    mov a,r4
    xrl a,#4
    jnz fd_c
    mov a,r7
    dec a
    anl a,#0FEh
    jnz fd_c
    mov a,#SPR_WHITE
    jmp fd_p
fd_c:
    mov a,r6
    anl a,#38h
fd_p:
    call poke_b
    jmp fd_n
fd_hide:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    call poke_b
fd_n:
    mov a,r3
    add a,#8
    mov r3,a
    inc r7
    mov a,r7
    xrl a,#4
    jnz fd_l
    ret


; fx_stop: the actor stops, its sprites hidden. Clobbers A, r0-r6.
fx_stop:
    clr a
    mov r0,#FX_PC
    movx @r0,a
    jmp spr_hide

; ===========================================================================
    org 0A00h
    include "stbyte.inc"
CUR_XYONLY equ 1
    include "cursor.inc"

; ev_start: event r4's piece: its script, from its plot's cell (or the
; store's, for those without a plot). Clobbers A, r0-r7.
ev_start:
    mov r0,#PR_PLOT
    movx a,@r0
    add a,#256-NPLOTS
    jnc es_xy
    mov a,#STORE_PLOT-NPLOTS
es_xy:
    add a,#NPLOTS
    call plot_xy
    mov r0,#FX_X
    mov a,r7
    movx @r0,a
    inc r0
    mov a,r6
    movx @r0,a
    mov a,r4
    add a,#(evtab-1)&0FFh
    movp a,@a
    jmp fx_start
evtab:                          ; by event: its script
    db fx_pest&0FFh, fx_pirate&0FFh, fx_acid&0FFh, fx_quake&0FFh
    db fx_sun&0FFh, fx_meteor&0FFh, fx_rad&0FFh, fx_fire&0FFh, fx_land&0FFh

; fx_look: shape A into sprites 0 on: its rows, then a blank. A = its
; pieces. Clobbers r0-r4.
fx_look:
    add a,#shp_tab&0FFh
    movp a,@a
    mov r1,a
    movp a,@a                   ; the pieces, then 7 rows each
    mov r4,a
    mov r3,a
    mov a,#P1_VDC
    outl p1,a
    mov r0,#vdc_spr0_shape
fl_p:
    mov r2,#7
fl_r:
    inc r1
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    djnz r2,fl_r
    clr a
    movx @r0,a
    inc r0
    djnz r4,fl_p
    mov a,#P1_MAIN
    outl p1,a
    mov a,r3
    ret

SH_SHIP  equ 0
SH_PEST  equ 1
SH_CLOUDB equ 2
SH_CLOUD equ 3
SH_SPARK equ 4
SH_MULE  equ 5
SH_METEOR equ 6
SH_FLAME equ 7
SH_CRACK equ 8
shp_tab:
    db sh_ship&0FFh, sh_pest&0FFh, sh_cloudb&0FFh, sh_cloud&0FFh
    db sh_spark&0FFh, sh_mule&0FFh, sh_meteor&0FFh, sh_flame&0FFh
    db sh_crack&0FFh
; (bit 0 the leftmost dot; 7 rows of 2 lines: one character band)
sh_ship:
    db 4
    db 000h, 000h, 0F0h, 0FEh, 0FFh, 0FCh, 000h ; the nose
    db 0C0h, 0FCh, 0FFh, 0DBh, 0FFh, 0FFh, 024h ; the hull, its windows
    db 003h, 03Fh, 0FFh, 0DBh, 0FFh, 0FFh, 024h
    db 007h, 00Fh, 03Fh, 07Fh, 0FFh, 03Fh, 000h ; the tail
sh_pest:
    db 1, 03Ch, 0BDh, 07Eh, 0A5h, 0, 0, 0       ; 4 rows: it crawls 6 lines
sh_cloudb:
    db 1, 03Ch, 07Eh, 0FFh, 07Eh, 010h, 008h, 010h ; and its bolt
sh_cloud:
    db 1, 03Ch, 07Eh, 0FFh, 07Eh, 0, 0, 0
sh_spark:
    db 1, 008h, 01Ch, 03Eh, 01Ch, 008h, 0, 0
sh_mule:
    db 1, 000h, 0C0h, 0FEh, 07Ch, 07Ch, 044h, 044h
sh_meteor:
    db 1, 0C0h, 060h, 01Ch, 01Eh, 00Eh, 004h, 0
sh_flame:
    db 1, 008h, 00Ch, 01Ch, 05Eh, 07Eh, 0FFh, 07Eh
sh_crack:
    db 1, 010h, 018h, 00Ch, 018h, 030h, 018h, 008h

; fx_start: the actor runs the script at offset A of page $E00, from
; FX_X, FX_Y, hidden until it shows. Clobbers A, r0.
fx_start:
    mov r0,#FX_PC
    movx @r0,a
    clr a
    mov r0,#FX_CNT
    movx @r0,a
    mov r0,#FX_LP
    movx @r0,a
    mov r0,#FX_COL
    mov a,#FX_HID
    movx @r0,a
    ret

; ===========================================================================
MAPBASE equ 0B00h
    include "map.inc"

; ===========================================================================
    org 0D00h

; tick: every frame. A row of the map, while the state's are not all built;
; the actor, while it runs -- the event over, the production's title (its
; digits come with the next state), the ship gone, its phase's own page.
tick:
    mov r0,#MB_ROW
    movx a,@r0
    add a,#256-5
    jc tk_fx
    call map_row
    mov r1,#DIRTY               ; its mountains' right halves are new
    mov a,@r1
    orl a,#DIRTY_STATIC
    mov @r1,a
tk_fx:
    mov r0,#FX_PC
    movx a,@r0
    jz tk_x
    call fx_tick
    mov r0,#FX_PC
    movx a,@r0
    jnz tk_x
    mov r0,#PHS                 ; it has ended
    movx a,@r0
    xrl a,#PHASE_PRODUCTION
    jz tk_pt
    jmp ship_gone
tk_pt:
    jmp pr_title
tk_x:
    ret

; fx_frame: a frame of the actor: the op under way counts down (a WAIT
; ends, a MOVE steps), or the ops run until one takes time. Keeps r5.
fx_frame:
    mov r0,#FX_CNT
    movx a,@r0
    jz ff_run
    dec a
    movx @r0,a
    jnz ff_x
    mov r0,#FX_PC
    movx a,@r0
    mov r7,a
    call fx_get
    xrl a,#OP_WAIT
    jz ff_w
    mov a,r7                    ; a MOVE's step
    inc a
    call fx_get
    mov r6,a
    swap a
    call fx_sx
    mov r0,#FX_X
    call fx_add
    mov a,r6
    call fx_sx
    mov r0,#FX_Y
    call fx_add
    mov r0,#FX_N
    movx a,@r0
    dec a
    movx @r0,a
    jz ff_mv
    mov a,r7
    add a,#2
    call fx_get
    mov r0,#FX_CNT
    movx @r0,a
ff_x:
    ret
ff_mv:
    mov a,r7
    add a,#4
    jmp ff_go
ff_w:
    mov a,r7
    add a,#2
ff_go:
    mov r7,a
    jmp ff_l
ff_run:
    mov r0,#FX_PC
    movx a,@r0
    mov r7,a
ff_l:                           ; the op at r7
    mov a,r7
    call fx_get
    add a,#optab&0FFh
    jmpp @a
optab:
    db o_end&0FFh, o_pos&0FFh, o_wait&0FFh, o_move&0FFh
    db o_hide&0FFh, o_show&0FFh, o_look&0FFh, o_loop&0FFh, o_go&0FFh
o_end:
    clr a
    mov r0,#FX_PC
    movx @r0,a
    ret
o_pos:
    call fx_next
    mov r0,#FX_X
    movx @r0,a
    call fx_next
    mov r0,#FX_Y
    movx @r0,a
o_show:
    inc r7
    mov r0,#FX_COL
    movx a,@r0
    anl a,#~FX_HID
    jmp o_col
o_hide:
    inc r7
    mov r0,#FX_COL
    movx a,@r0
    orl a,#FX_HID
o_col:
    movx @r0,a
    jmp ff_l
o_wait:
    mov a,r7
    inc a
    call fx_get
    mov r0,#FX_CNT
    movx @r0,a
ff_save:
    mov r0,#FX_PC
    mov a,r7
    movx @r0,a
    ret
o_move:
    mov a,r7
    add a,#3
    call fx_get
    mov r0,#FX_N
    movx @r0,a
    mov a,r7
    add a,#2
    call fx_get
    mov r0,#FX_CNT
    movx @r0,a
    jmp ff_save
o_look:
    call fx_next
    call fx_look
    dec a                       ; the pieces - 1, bits 6-7
    rr a
    rr a
    mov r6,a
    call fx_next
    orl a,r6
    mov r0,#FX_COL
    inc r7
    jmp o_col
o_loop:
    mov r0,#FX_LP               ; first time here: count more times
    movx a,@r0
    jnz ol_c
    mov a,r7
    inc a
    call fx_get
    inc a
ol_c:
    dec a
    movx @r0,a
    jz ol_out
    mov a,r7
    add a,#2
    call fx_get
    mov r7,a
    jmp ff_l
ol_out:
    mov a,r7
    add a,#3
    mov r7,a
    jmp ff_l
o_go:
    call fx_next
    mov r7,a
    jmp ff_l

; fx_sx: A's low nibble, signed. fx_add: XRAM @r0 += A. Clobber A (fx_add
; r1 too).
fx_sx:
    anl a,#0Fh
    xrl a,#08h
    add a,#256-8
    ret
fx_add:
    mov r1,a
    movx a,@r0
    add a,r1
    movx @r0,a
    ret

; ===========================================================================
    org 0E00h

; fx_next: A = the script's byte after r7, r7 on to it. fx_get: A = the
; byte at offset A of this page: the scripts.
fx_next:
    inc r7
    mov a,r7
fx_get:
    movp a,@a
    ret

; Transport ship landing: a fast pass right to left, back slowly from the
; left to the store, settled (mekkogx fx.c fx_land).
fx_land:
    db OP_LOOK, SH_SHIP, C_BLUE<<3
    db OP_POS, 182, SHIP_Y
    db OP_WAIT, 36
    db OP_MOVE, MV(-1,0), 1, 230
    db OP_HIDE, OP_WAIT, 18
    db OP_POS, 256-40, SHIP_Y
    db OP_MOVE, MV(1,0), 6, 60
    db OP_MOVE, MV(5,0), 6, 9
    db OP_WAIT, 32
    db OP_HIDE, OP_END

; Transport ship leaving: up off the store, away to the left.
fx_depart:
    db OP_WAIT, 45
    db OP_LOOK, SH_SHIP, C_BLUE<<3
    db OP_POS, SHIP_X, SHIP_Y
    db OP_WAIT, 32
    db OP_MOVE, MV(-5,0), 8, 9
    db OP_MOVE, MV(-1,0), 8, 52
    db OP_HIDE, OP_END

; Pirate ship: the fast pass, back to the store, the raid, away.
fx_pirate:
    db OP_LOOK, SH_SHIP, C_RED<<3
    db OP_POS, 182, SHIP_Y
    db OP_WAIT, 36
    db OP_MOVE, MV(-1,0), 1, 230
    db OP_HIDE, OP_WAIT, 18
    db OP_POS, 256-40, SHIP_Y
    db OP_MOVE, MV(1,0), 4, 60
    db OP_MOVE, MV(5,0), 6, 9
    db OP_WAIT, 150
    db OP_MOVE, MV(-5,0), 8, 9
    db OP_MOVE, MV(-1,0), 4, 52
    db OP_HIDE, OP_END

; Pest: round its plot, 2 dots every 5 frames, pausing 11 at each turn.
fx_pest:
    db OP_LOOK, SH_PEST, C_WHITE<<3
fp_l:
    db OP_MOVE, MV(2,0), 5, 3, OP_WAIT, 11
    db OP_MOVE, MV(0,2), 5, 3, OP_WAIT, 11
    db OP_MOVE, MV(-2,0), 5, 3, OP_WAIT, 11
    db OP_MOVE, MV(0,-2), 5, 3, OP_WAIT, 11
    db OP_LOOP, 5, fp_l&0FFh
    db OP_HIDE, OP_END

; Acid rain: the cloud on along its row from its plot, its colour cycling,
; its bolt flickering.
fx_acid:
    db OP_LOOK, SH_CLOUDB, C_WHITE<<3, OP_MOVE, MV(1,0), 8, 5
    db OP_LOOK, SH_CLOUD, C_BLUE<<3, OP_MOVE, MV(1,0), 8, 5
    db OP_LOOK, SH_CLOUDB, C_WHITE<<3, OP_MOVE, MV(1,0), 8, 5
    db OP_LOOK, SH_CLOUD, C_CYAN<<3, OP_MOVE, MV(1,0), 8, 5
    db OP_LOOP, 3, fx_acid&0FFh
    db OP_HIDE, OP_END

; Planetquake: cracks over the map, where sunspot's sparks go.
fx_quake:
    db OP_LOOK, SH_CRACK, C_TAN<<3, OP_GO, fs_l&0FFh

; Sunspot: sparks over the map.
fx_sun:
    db OP_LOOK, SH_SPARK, C_WHITE<<3
fs_l:
    db OP_POS, 32, Y_ROW0+2, OP_WAIT, 6
    db OP_POS, 112, Y_ROW0+3*ROW_PITCH+4, OP_WAIT, 6
    db OP_POS, 64, Y_ROW0+ROW_PITCH, OP_WAIT, 6
    db OP_POS, 142, Y_ROW0+2*ROW_PITCH+4, OP_WAIT, 6
    db OP_POS, 12, Y_ROW0+4*ROW_PITCH+2, OP_WAIT, 6
    db OP_POS, 92, Y_ROW0+4, OP_WAIT, 6
    db OP_POS, 128, Y_ROW0+ROW_PITCH+4, OP_WAIT, 6
    db OP_LOOP, 7, fs_l&0FFh
    db OP_HIDE, OP_END

; Radiation: the plot's M.U.L.E. flashes, then bolts off to the right.
fx_rad:
    db OP_LOOK, SH_MULE, C_WHITE<<3
fr_l:
    db OP_WAIT, 6, OP_HIDE, OP_WAIT, 6, OP_SHOW
    db OP_LOOP, 9, fr_l&0FFh
    db OP_MOVE, MV(2,0), 2, 50
    db OP_HIDE, OP_END

; Meteorite: up and to the right of its plot (unseen), then down onto it.
fx_meteor:
    db OP_MOVE, MV(6,-8), 1, 10
    db OP_LOOK, SH_METEOR, C_RED<<3
    db OP_MOVE, MV(-3,4), 3, 20
    db OP_WAIT, 150
    db OP_HIDE, OP_END

; Fire in the store: a flame over it, red and orange by turns.
fx_fire:
    db OP_LOOK, SH_FLAME, C_RED<<3, OP_WAIT, 5
    db OP_LOOK, SH_FLAME, C_TAN<<3, OP_WAIT, 5
    db OP_LOOP, 23, fx_fire&0FFh
    db OP_HIDE, OP_END

    if $ > 0F00h
    error "page 15's scripts have left page $E00"
    endif

    PAGE_TAIL
