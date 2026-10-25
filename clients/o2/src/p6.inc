; p6.inc -- page 6: a land auction's plot (mekkogx auction.c, stage SHOW),
; on the colony map: boxed and blinking, with the opening price; the button
; skips the wait. Its bidding is page 8's. (The production is page 15's.)

E_STATE equ 0
E_TICK  equ 1
NPLOTS  equ 45
SPR_WHITE equ C_WHITE<<3        ; a sprite's colour, attribute bits 3-5

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp on_state
e_tick:
    jmp tick

    include "sayq.inc"

; on_state: a state has come. A new phase gets the map; then the plots.
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
    call os_land
os_same:
    clr a                       ; the M.U.L.E.s' letters
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
; the plot boxed, 16 frames on, 16 off; the button skips the wait.
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
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz tk_blink
    mov r0,#MYSEAT
    movx a,@r0
    jb7 tk_blink
    mov r0,#ACT
    mov a,#ACT_READY
    movx @r0,a
tk_blink:
    mov r1,#FRAMES
    mov a,@r1
    anl a,#10h
    jz tk_hide
    mov r0,#PR_PLOT
    movx a,@r0
    mov r5,a
    add a,#256-NPLOTS
    jc tk_hide
    mov a,r5
    mov r0,#G_CUR
    movx @r0,a
    jmp cur_show
tk_hide:
    jmp cur_hide

    PAGE_TAIL
