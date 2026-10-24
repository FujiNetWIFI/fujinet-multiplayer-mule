; p11.inc -- page 11: the development on the colony map (mekkogx develop.c).
; Whose turn it is, our prompt to start, and our walk on the map: the
; player (sprite 0) leads its M.U.L.E. (1); the wampus (2) flashes on its
; mountain. The button catches the wampus, installs the M.U.L.E., lists
; land after the land office, or takes an assay sample. Walking into the
; store goes to town (page 12, DV_TOWN), where every turn starts.

E_STATE equ 0
E_TICK  equ 1
DX_MIN  equ 8                   ; the figure's top left on the map
DX_MAX  equ 152
DY_MIN  equ 48
DY_MAX  equ 160
STORE_X equ 72                  ; the store's plot: x 72-87, y 96-119
STORE_Y equ 96

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp on_state
e_tick:
    jmp tick

    include "sayq.inc"

; on_state: a state has come: the map (a new phase or page puts it up),
; then the turn (its words: page 13's, after this one).
on_state:
    call dv_setup
    call map_build
    jmp dv_turn

; tick: every frame. The map's rows while a state's are building; the
; prompt's button; our walk; the wampus.
tick:
    call dv_setup
    mov r0,#MB_ROW              ; the next row of the map, if any (map_build)
    movx a,@r0
    add a,#256-5
    jc tk_dv
    call map_row
    mov r1,#DIRTY               ; its mountains' right halves are new
    mov a,@r1
    orl a,#DIRTY_STATIC
    mov @r1,a
tk_dv:
    mov r0,#DV_ST
    movx a,@r0
    xrl a,#DV_PROMPT
    jnz tk_walk
    mov r0,#JOYEV               ; the prompt: the button begins our turn
    movx a,@r0
    anl a,#J_FIRE
    jz tk_x
    mov r0,#ACT
    mov a,#ACT_BEGIN
    movx @r0,a
    mov a,#SND_SELECT
    FCALL beep
tk_x:
    ret
tk_walk:
    xrl a,#DV_PROMPT!DV_WALK
    jnz tk_x
    call dv_wampus
    call dv_step
    call dv_draw
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz tk_x
    jmp dv_button

; dv_setup: the map and the figures' shapes, if this page's screen is not up
; (a new phase, a page change, or back from town). A new phase (DV_PH, its
; round) starts the development afresh. Clobbers A, r0-r7.
dv_setup:
    mov r0,#PHS
    movx a,@r0
    mov r6,a
    mov r0,#LASTPH
    movx a,@r0
    xrl a,r6
    jz ds_x
    mov a,r6
    movx @r0,a
    call map_screen
    call dv_shapes
    mov r0,#NSTATE              ; (back from town: the statics again, from
    movx a,@r0                  ; the last state while no request is out)
    jnz ds_t
    call map_build
ds_t:
    mov r0,#RND
    movx a,@r0
    mov r2,a
    mov r3,#0
    SAY T_DEVEL, L_MAP, 1, C_WHITE  ; (the lines under: the turn's words, or
                                    ; blanked by the town on the way out)
    clr a                       ; letters, not production's digits
    mov r0,#MAPDIG
    movx @r0,a
    mov r0,#RND
    movx a,@r0
    mov r5,a
    mov r0,#DV_PH
    movx a,@r0
    xrl a,r5
    jz ds_x
    mov a,r5                    ; a new development
    movx @r0,a
    clr a
    mov r0,#DV_ST
    movx @r0,a
    mov r0,#DV_TOWN
    movx @r0,a
    mov r0,#DV_MSG
    mov a,#0FFh
    movx @r0,a
ds_x:
    ret

; dv_shapes: sprites 0-2: the figure, the M.U.L.E., the wampus; all hidden.
; Clobbers A, r0, r1, r4, r6.
dv_shapes:
    mov a,#P1_VDC
    outl p1,a
    mov r0,#vdc_spr0_shape
    mov r4,#dv_shp&0FFh
    mov r6,#24
dsh_l:
    mov a,r4
    movp a,@a
    movx @r0,a
    inc r0
    inc r4
    djnz r6,dsh_l
    mov a,#P1_MAIN
    outl p1,a
    jmp spr_hide
dv_shp:                         ; bit 0 the leftmost pixel
    db 018h, 018h, 03Ch, 05Ah, 018h, 024h, 024h, 000h   ; the figure
    db 060h, 0E0h, 07Fh, 03Fh, 012h, 012h, 000h, 000h   ; a M.U.L.E.
    db 03Ch, 07Eh, 0DBh, 0FFh, 07Eh, 042h, 000h, 000h   ; the wampus

; dv_onwp: A nonzero if the figure stands within 8 of the wampus.
; Clobbers r0, r5.
dv_onwp:
    mov r0,#DV_WPX
    movx a,@r0
    cpl a
    inc a
    mov r5,a
    mov r0,#DV_X
    movx a,@r0
    add a,r5
    add a,#8
    add a,#256-17
    jc do_no
    mov r0,#DV_WPY
    movx a,@r0
    cpl a
    inc a
    mov r5,a
    mov r0,#DV_Y
    movx a,@r0
    add a,r5
    add a,#8
    add a,#256-17
    jc do_no
    mov a,#1
    ret
do_no:
    clr a
    ret

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "stbyte.inc"

    include "dvturn.inc"

; ===========================================================================
MAPBASE equ 0B00h
    include "map.inc"

; ===========================================================================
    org 0D00h

; dv_button: the button on the map (mekkogx dv_button): the wampus if we
; stand on it and lead nothing; else the plot under us: install the M.U.L.E.
; we lead, list it after the land office, or take an assay sample.
; Clobbers A, r0-r7.
dv_button:
    mov r0,#DV_WP
    movx a,@r0
    jb7 db_plot
    mov r0,#DV_MULE
    movx a,@r0
    jnz db_plot
    call dv_onwp
    jz db_plot
    mov a,#ACT_WAMP
    jmp db_act0
db_plot:
    call dv_plot                ; A = the plot under us (0FFh none)
    mov r5,a
    jb7 db_x
    mov r0,#DV_MULE
    movx a,@r0
    jz db_off
    mov a,#ACT_INST
    jmp db_act
db_off:
    mov r0,#DV_OFFICE
    movx a,@r0
    jz db_samp
    clr a
    movx @r0,a
    mov a,#ACT_SELL
    jmp db_act
db_samp:
    mov r0,#DV_SAMPLE           ; a sample: the assay office reads it
    mov a,r5
    movx @r0,a
    SAY M_SAMPLE, L_MAP+1, 2, C_WHITE
db_x:
    ret
db_act0:
    mov r5,#0
db_act:
    mov r6,a                    ; action A, argument r5: unless one waits
    mov r0,#ACT
    movx a,@r0
    jnz db_busy
    mov a,r6
    movx @r0,a
    inc r0
    mov a,r5
    movx @r0,a
    inc r0
    clr a
    movx @r0,a
db_busy:
    ret

; dv_plot: A = the plot under the figure's middle, 0FFh off the map.
; Clobbers r0, r6, r7.
dv_plot:
    mov r0,#DV_X                ; column: (x + 4 - 8) / 16
    movx a,@r0
    add a,#256-4
    swap a
    anl a,#0Fh
    mov r7,a
    add a,#256-9
    jc dp_off
    inc r0                      ; row: (y + 4 - 48) / 24
    movx a,@r0
    add a,#256-44
    jnc dp_off
    mov r6,#0
dp_r:
    add a,#256-24
    jnc dp_rd
    inc r6
    jmp dp_r
dp_rd:
    mov a,r6
    add a,#256-5
    jc dp_off
    mov a,r6                    ; 9 row + column
    rl a
    rl a
    rl a
    add a,r6
    add a,r7
    ret
dp_off:
    mov a,#0FFh
    ret

; dv_wampus: the wampus, from the state's list of when and where it shows
; (turn.wampus, in turn jiffies): its sprite flashes 8 frames on, 8 off on
; its mountain. Clobbers A, r0-r7.
dv_wampus:
    mov r0,#DV_WP
    movx a,@r0
    jb7 dw_hide
    mov r1,#FRAMES
    mov a,@r1
    anl a,#8
    jz dw_hide
    mov r2,#(SPRTAB+6)>>8
    mov a,#(SPRTAB+6)&0FFh
    call poke_at
    mov r0,#DV_WPY
    movx a,@r0
    call poke_b
    mov r0,#DV_WPX
    movx a,@r0
    call poke_b
    mov a,#C_WHITE<<3
    jmp poke_b
dw_hide:
    mov r2,#(SPRTAB+6)>>8
    mov a,#(SPRTAB+6)&0FFh
    call poke_at
    mov a,#0F8h
    jmp poke_b

; ===========================================================================
    org 0E00h

; dv_step: the stick walks the figure, a step (2 dots, 2 lines) every 6
; frames; into the store is into town. Clobbers A, r0-r7.
dv_step:
    mov r0,#DV_CNT
    movx a,@r0
    jz ds_go
    dec a
    movx @r0,a
    ret
ds_go:
    mov r0,#JOY
    movx a,@r0
    anl a,#J_UP|J_DOWN|J_LEFT|J_RIGHT
    jz ds_rest
    mov r4,a
    mov r0,#DV_CNT
    mov a,#5
    movx @r0,a
    mov r0,#DV_X                ; the M.U.L.E. follows where we were
    movx a,@r0
    mov r1,#DV_MX
    movx @r1,a
    inc r0
    inc r1
    movx a,@r0
    movx @r1,a
    mov r0,#DV_X
    mov a,r4
    anl a,#J_LEFT
    jz ds_r
    movx a,@r0
    add a,#256-DX_MIN-1
    jnc ds_r
    add a,#DX_MIN+1-2
    movx @r0,a
ds_r:
    mov a,r4
    anl a,#J_RIGHT
    jz ds_u
    movx a,@r0
    add a,#256-DX_MAX
    jc ds_u
    add a,#DX_MAX+2
    movx @r0,a
ds_u:
    inc r0
    mov a,r4
    anl a,#J_UP
    jz ds_d
    movx a,@r0
    add a,#256-DY_MIN-1
    jnc ds_d
    add a,#DY_MIN+1-2
    movx @r0,a
ds_d:
    mov a,r4
    anl a,#J_DOWN
    jz ds_s
    movx a,@r0
    add a,#256-DY_MAX
    jc ds_s
    add a,#DY_MAX+2
    movx @r0,a
ds_s:
    mov r0,#DV_FR
    movx a,@r0
    xrl a,#1
    movx @r0,a
    call dv_store               ; into the store?
ds_rest:
    ret

; dv_store: the figure's middle in the store's plot: into town at the
; street's end on that side (page 12 takes over). Clobbers A, r0, r5.
dv_store:
    mov r0,#DV_X
    movx a,@r0
    add a,#4-STORE_X
    mov r5,a
    add a,#256-16
    jc dst_x
    inc r0
    movx a,@r0
    add a,#4-STORE_Y
    add a,#256-24
    jc dst_x
    mov r0,#DV_X                ; the left half: the street's left end
    mov a,r5
    add a,#256-8
    mov a,#8
    jnc dst_put
    mov a,#148
dst_put:
    movx @r0,a
    inc r0
    mov a,#108
    movx @r0,a
    mov r0,#DV_TOWN
    mov a,#1
    movx @r0,a
dst_x:
    ret

; dv_draw: the figure, and the M.U.L.E. a step behind. Clobbers A, r0-r7.
dv_draw:
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov r0,#DV_Y
    movx a,@r0
    call poke_b
    mov r0,#DV_X
    movx a,@r0
    call poke_b
    mov r0,#MYSEAT
    movx a,@r0
    anl a,#3
    add a,#seatcol2&0FFh
    movp a,@a
    call poke_b
    mov r0,#DV_MULE
    movx a,@r0
    jz dd_none
    mov r0,#DV_MY
    movx a,@r0
    call poke_b
    mov r0,#DV_MX
    movx a,@r0
    call poke_b
    mov a,#C_WHITE<<3
    jmp poke_b
dd_none:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    jmp poke_b
seatcol2:                       ; a sprite's colour, attribute bits 3-5
    db C_RED<<3, C_GREEN<<3, C_TAN<<3, C_VIOLET<<3

    PAGE_TAIL
