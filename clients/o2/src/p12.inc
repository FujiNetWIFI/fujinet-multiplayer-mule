; p12.inc -- page 12: the town (mekkogx develop.c town_* / walk_town).
;
;   y 30    $1234   M14 $100      our money; the store's M.U.L.E.s, price
;   48-96   [ C ] [ S ]   [ E ] [ F ]     the outfitters (crystite,
;                                         smithore, energy, food)
;   96-144  the street
;   144-192 [ A ] [ L ]   [ P ] [ M ]     assay, land, pub, corral
;
; The buildings are the grid's, each two cells wide with a door in the cell
; nearer the middle; their letters the text lines at 64 and 160. Up or down
; at a door goes in (the outfitters only with a M.U.L.E.); the building's
; action goes to the server and the figure waits in the door for the
; answer. Off either end of the street: the map (page 11). The row tables
; are left alone, so the map comes back as it was; its mountains' right
; halves (the statics) are built again from the state.

E_STATE equ 0
E_TICK  equ 1
TY_TOP  equ 100                 ; the street: the figure's top 100-116
TY_BOT  equ 116
TY_IN_UP equ 76                 ; in an outfitter
TY_IN_DN equ 140                ; in a lower building
TX_MIN  equ 8
TX_MAX  equ 148

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp on_state
e_tick:
    jmp tick

; on_state: a state has come: the town (if not up), the turn (its words and
; our money: page 13's); the answer we waited for in a door lets us out.
on_state:
    call tw_setup
    call dv_turn
    mov r0,#ACTSENT             ; the building's action answered: back out
    movx a,@r0
    xrl a,#ACT_STATE
    jz os_x
    mov r1,#IR_BW
    mov a,@r1
    jz os_x
    jmp tw_out
os_x:
    ret

; tick: every frame. In a door: wait (255 frames at most); else walk.
tick:
    call tw_setup
    mov r0,#DV_ST
    movx a,@r0
    xrl a,#DV_WALK
    jnz tk_x
    mov r1,#IR_BW
    mov a,@r1
    jz tk_walk
    dec a
    mov @r1,a
    jnz tk_x
    jmp tw_out
tk_walk:
    call tw_step
    jmp tw_draw
tk_x:
    ret

; tw_setup: the town, if this page's screen is not up: its band list, the
; buildings, their letters, the figures' shapes. Clobbers A, r0-r7.
tw_setup:
    mov r0,#PHS
    movx a,@r0
    mov r6,a
    mov r0,#LASTPH
    movx a,@r0
    xrl a,r6
    jz ts_x
    mov a,r6
    movx @r0,a
    mov r1,#MODE                ; no map rows
    clr a
    mov @r1,a
    mov r1,#IR_IN
    mov @r1,a
    mov r1,#IR_BW
    mov @r1,a
    mov r2,#SCRCOL>>8
    mov a,#SCRCOL&0FFh
    call poke_at
    mov r4,#tw_bands&0FFh
    mov r6,#2+NBANDS*3
    call tw_copy
    mov r2,#GRIDTAB>>8
    mov a,#GRIDTAB&0FFh
    call poke_at
    mov r4,#tw_grid&0FFh
    mov r6,#28
    call tw_copy
    mov r2,#STATTAB>>8          ; the map's mountains off (the map builds
    mov a,#STATTAB&0FFh         ; them again)
    call poke_at
    mov r6,#10
ts_s:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    call poke_b
    call poke_b
    djnz r6,ts_s
    mov r1,#DIRTY
    mov a,#DIRTY_GRID|DIRTY_STATIC
    mov @r1,a
    call tw_shapes
    mov a,#tw_top&0FFh          ; the letters
    call tw_label
    mov r2,#T1>>8
    mov r3,#T1&0FFh
    mov r4,#64
    call tx_build
    mov a,#tw_low&0FFh
    call tw_label
    mov r2,#T2>>8
    mov r3,#T2&0FFh
    mov r4,#160
    jmp tx_build
ts_x:
    ret

; tw_copy: r6 bytes from offset r4 of this page to the POKE pointer.
tw_copy:
    mov a,r4
    movp a,@a
    call poke_b
    inc r4
    djnz r6,tw_copy
    ret

tw_bands:
    db 002h                     ; black background, dim green grid
    db 256-1                    ; the first band: line 1
    db J_TXTA, T0&0FFh, 256-43  ; T0 at 30, then T1's band at 44
    db J_TXTA, T1&0FFh, 256-94  ; T1 at 64, then T2's at 138
    db J_TXTA, T2&0FFh, 0       ; T2 at 160: the last
    db J_NONE, 0, 0
    db J_NONE, 0, 0
    db J_NONE, 0, 0
    db J_NONE, 0, 0
    db J_NONE, 0, 0
    db J_NONE, 0, 0

; The buildings: horizontal lines 1, 3 (doors at columns 1, 3, 5, 7), 5
; (the same doors), 7; walls in rows 1-2 and 5-6 at columns 0, 2, 4, 5, 7,
; 9.
tw_grid:
    db 0AAh, 082h, 0AAh, 082h, 000h, 082h, 0AAh, 082h, 0AAh     ; $C0-$C8
    db 0, 0, 0, 0, 0, 0, 0, 0, 0                                ; $D0-$D8
    db 066h, 0, 066h, 0, 066h, 066h, 0, 066h, 0, 066h           ; $E0-$E9

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "stbyte.inc"
    include "dvturn.inc"

; tw_label: LINEBUF = the 16 letters at offset A of this page, white.
; Clobbers A, r0, r1, r6.
tw_label:
    mov r1,a
    mov r0,#LINEBUF
    mov r6,#TXT_LEN
tl_l:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r6,tl_l
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    movx @r0,a
    ret

tw_top: db " C   S     E   F"
tw_low: db " A   L     P   M"


; tw_shapes: sprites 0 and 1: the colonist, the M.U.L.E.; the rest hidden.
; Clobbers A, r0, r1, r4, r6.
tw_shapes:
    mov a,#P1_VDC
    outl p1,a
    mov r0,#vdc_spr0_shape
    mov r4,#tw_shp&0FFh
    mov r6,#16
tsh_l:
    mov a,r4
    movp a,@a
    movx @r0,a
    inc r0
    inc r4
    djnz r6,tsh_l
    mov a,#P1_MAIN
    outl p1,a
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov r6,#4
tsh_h:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    call poke_b
    djnz r6,tsh_h
    ret
tw_shp:                         ; bit 0 the leftmost pixel
    db 01Ch, 01Ch, 008h, 03Eh, 05Dh, 014h, 014h, 036h   ; a colonist
    db 060h, 0E0h, 07Fh, 03Fh, 012h, 012h, 000h, 000h   ; a M.U.L.E.

; ===========================================================================
    org 0B00h

; tw_step: the stick walks the figure, a step every 6 frames: along the
; street 4 dots, across it 4 lines; at a door, on into the building (2 lines
; a step) and its action there. Off either end: the map. Clobbers A, r0-r7.
tw_step:
    mov r0,#DV_CNT
    movx a,@r0
    jz ws_go
    dec a
    movx @r0,a
    ret
ws_go:
    mov r1,#IR_IN
    mov a,@r1
    jz ws_stick
    mov r0,#DV_CNT              ; walking in
    mov a,#5
    movx @r0,a
    mov r0,#DV_Y
    mov a,@r1
    xrl a,#1
    jnz ws_dn
    movx a,@r0
    add a,#256-2
    movx @r0,a
    add a,#256-TY_IN_UP-1
    jc ws_x
    jmp tw_in
ws_dn:
    movx a,@r0
    add a,#2
    movx @r0,a
    add a,#256-TY_IN_DN
    jnc ws_x
    jmp tw_in
ws_stick:
    mov r0,#JOY
    movx a,@r0
    anl a,#J_UP|J_DOWN|J_LEFT|J_RIGHT
    jz ws_x
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
    jz ws_r
    movx a,@r0
    add a,#256-TX_MIN-1
    jnc ws_west
    add a,#TX_MIN+1-4
    movx @r0,a
ws_r:
    mov a,r4
    anl a,#J_RIGHT
    jz ws_u
    movx a,@r0
    add a,#256-TX_MAX
    jc ws_east
    add a,#TX_MAX+4
    movx @r0,a
ws_u:
    inc r0
    mov a,r4
    anl a,#J_UP
    jz ws_d
    movx a,@r0
    add a,#256-TY_TOP-1
    jnc ws_door_up
    add a,#TY_TOP+1-4
    movx @r0,a
ws_d:
    mov a,r4
    anl a,#J_DOWN
    jz ws_x
    movx a,@r0
    add a,#256-TY_BOT
    jc ws_door_dn
    add a,#TY_BOT+4
    movx @r0,a
ws_x:
    ret
ws_west:                        ; the map, west of the store
    mov a,#56
    jmp ws_map
ws_east:
    mov a,#92
ws_map:
    movx @r0,a
    inc r0
    mov a,#100
    movx @r0,a
    call lb_clear               ; the letters off: the map's message lines
    mov r2,#T1>>8
    mov r3,#T1&0FFh
    mov r4,#188
    call tx_build
    mov r2,#T2>>8
    mov r3,#T2&0FFh
    mov r4,#226
    call tx_build
    clr a
    mov r0,#DV_TOWN
    movx @r0,a
    mov r0,#DV_CNT              ; the first map step waits
    mov a,#20
    movx @r0,a
    ret
ws_door_up:                     ; an outfitter: only with a M.U.L.E.
    mov r0,#DV_MULE
    movx a,@r0
    jz ws_x
    call tw_door
    jz ws_x
    mov r1,#IR_BLD              ; columns 1, 3, 5, 7: crystite .. food
    mov a,r5
    cpl a
    add a,#7+1
    clr c
    rrc a
    mov @r1,a
    mov r1,#IR_IN
    mov @r1,#1
    ret
ws_door_dn:
    call tw_door
    jz ws_x
    mov r1,#IR_BLD              ; assay, land, pub, corral
    mov a,r5
    clr c
    rrc a
    add a,#4
    mov @r1,a
    mov r1,#IR_IN
    mov @r1,#2
    ret

; tw_door: A nonzero if the figure's middle is in a door: r5 its column (1,
; 3, 5 or 7). Clobbers r0.
tw_door:
    mov r0,#DV_X
    movx a,@r0
    add a,#4-8
    swap a
    anl a,#0Fh
    mov r5,a
    mov a,r5
    xrl a,#1
    jz td_y
    xrl a,#1!3
    jz td_y
    xrl a,#3!5
    jz td_y
    xrl a,#5!7
    jz td_y
    clr a
    ret
td_y:
    mov a,#1
    ret

; ===========================================================================
    org 0C00h

; tw_in: inside: the building's action (mekkogx dv_building). The corral
; sells a M.U.L.E. (or takes ours back), a shop outfits ours, the assay
; office reads our sample, the land office waits for a plot, the pub ends
; the turn with a win. The figure stands in the door until the answer.
; Clobbers A, r0-r7.
tw_in:
    mov r1,#IR_IN
    mov @r1,#0
    mov r1,#IR_BLD
    mov a,@r1
    mov r5,a                    ; (an outfitter: the good is the argument)
    add a,#256-4
    jnc ti_shop
    mov a,@r1
    xrl a,#4
    jz ti_assay
    xrl a,#4!5
    jz ti_land
    xrl a,#5!6
    jz ti_pub
    mov r0,#DV_MULE             ; the corral: one, or ours back
    movx a,@r0
    jnz ti_ret
    mov a,#ACT_BUYM
    jmp ti_act
ti_ret:
    mov a,#ACT_RETN
    jmp ti_act
ti_shop:
    mov a,#ACT_OUTF
    jmp ti_act
ti_assay:
    mov r0,#DV_SAMPLE
    movx a,@r0
    jb7 ti_nos
    mov r5,a
    mov a,#ACT_ASSAY
    jmp ti_act
ti_nos:
    mov r5,#W_NOSAMP
    jmp ti_say
ti_land:
    mov r0,#DV_OFFICE
    mov a,#1
    movx @r0,a
    mov r5,#W_OFFICE
ti_say:
    mov r6,#L_MAP|(1<<4)
    mov r7,#C_WHITE
    mov r1,#IR_HOLD
    mov @r1,#5
    call say_q
    jmp tw_out
ti_pub:
    mov a,#ACT_PUB
ti_act:
    mov r0,#ACT
    movx @r0,a
    inc r0
    mov a,r5
    movx @r0,a
    inc r0
    clr a
    movx @r0,a
    mov r1,#IR_BW               ; in the door until the answer (4 s)
    mov @r1,#255
    mov a,#SND_SELECT
    FCALL beep
    ret

; tw_out: back on the street, in front of the door.
tw_out:
    mov r1,#IR_BW
    mov @r1,#0
    mov r0,#DV_Y
    mov r1,#IR_BLD
    mov a,@r1
    add a,#256-4
    mov a,#TY_TOP
    jnc to_put
    mov a,#TY_BOT
to_put:
    movx @r0,a
    ret

; tw_draw: the colonist, and the M.U.L.E. a step behind. Clobbers A, r0-r7.
tw_draw:
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
    add a,#twcol&0FFh
    movp a,@a
    call poke_b
    mov r0,#DV_MULE
    movx a,@r0
    jz tdr_none
    mov r0,#DV_MY
    movx a,@r0
    call poke_b
    mov r0,#DV_MX
    movx a,@r0
    call poke_b
    mov a,#C_WHITE<<3
    jmp poke_b
tdr_none:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    jmp poke_b
twcol:                          ; a sprite's colour, attribute bits 3-5
    db C_RED<<3, C_GREEN<<3, C_TAN<<3, C_VIOLET<<3

    include "sayq.inc"

    PAGE_TAIL
