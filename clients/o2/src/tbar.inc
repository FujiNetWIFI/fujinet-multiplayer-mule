; tbar.inc -- the time bar and its tick (mekkogx map.c bar_draw, bar_tick),
; run by the main loop between states. A state's page sets it (page 10 an
; auction's ticks, page 13 a turn's PTU): TB_U units left, TB_PER frames a
; unit, TB_CNT frames to the next, TB_SEG units a segment (0: no bar on this
; screen). The bar is the grid's bottom line (y 216), nine segments.

; tb_frame: TICKN frames off the unit under way -- a unit gone, the bar
; drawn again -- then the tick: under 30 units left, one every TB_U frames,
; high and low by turns, so it hurries as the bar runs out. Clobbers A,
; r0-r7.
tb_frame:
    mov r0,#TB_SEG
    movx a,@r0
    jz tf_reset                 ; no bar here: the tick starts afresh
    mov r0,#TICKN
    movx a,@r0
    jz tf_x                     ; no frame gone by
    mov r7,a
    mov r0,#TB_U
    movx a,@r0
    jz tf_reset
    mov r6,#0
tf_l:
    mov r0,#TB_CNT
    movx a,@r0
    dec a
    movx @r0,a
    jnz tf_n
    mov r0,#TB_PER
    movx a,@r0
    mov r0,#TB_CNT
    movx @r0,a
    mov r6,#1
    mov r0,#TB_U
    movx a,@r0
    dec a
    movx @r0,a
    jz tf_d
tf_n:
    djnz r7,tf_l
tf_d:
    mov a,r6
    jz tf_tick
    call tb_draw
tf_tick:
    mov r0,#TB_U
    movx a,@r0
    jz tf_reset
    add a,#256-30
    jc tf_reset                 ; 30 or more: none yet
    mov r0,#TICKN               ; due (tk <= d): tk + ~d doesn't carry
    movx a,@r0
    cpl a
    mov r7,a
    mov r0,#TB_TK
    movx a,@r0
    mov r6,a
    anl a,#7Fh
    add a,r7
    jnc tf_ring
    inc a                       ; not yet: tk - d
    mov r7,a
    mov a,r6
    anl a,#80h
    orl a,r7
    movx @r0,a
    ret
tf_ring:
    mov r0,#TB_U                ; the next in TB_U frames, the other note
    movx a,@r0
    mov r7,a
    mov a,r6
    anl a,#80h
    xrl a,#80h
    orl a,r7
    mov r0,#TB_TK
    movx @r0,a
    mov a,r6
    rl a
    anl a,#1
    add a,#SND_TICK
    FCALL beep
    ret
tf_reset:
    mov r0,#TB_TK               ; the first comes at 29, high
    mov a,#1
    movx @r0,a
tf_x:
    ret

; tb_draw: the bar: a segment for every TB_SEG units left, or part of them,
; nine at most; none without TB_SEG. Clobbers A, r0-r6.
tb_draw:
    mov r0,#TB_SEG
    movx a,@r0
    jz tbd_x
    cpl a
    inc a
    mov r6,a
    mov r0,#TB_U
    movx a,@r0
    mov r5,#0
tbd_l:
    jz tbd_put
    inc r5
    add a,r6                    ; u - seg: a borrow, the last (part) segment
    jc tbd_l
tbd_put:
    mov a,r5
    add a,#256-10
    jnc tbd_w
    mov r5,#9
tbd_w:
    mov r0,#FN_W_REGSEL         ; POKE at GRIDTAB+9: $D0-$D8, line 8's
    mov a,#FN_REG_POKE_LO       ; segments, bit 0
    movx @r0,a
    inc r0
    mov a,#(GRIDTAB+9)&0FFh
    movx @r0,a
    dec r0
    mov a,#FN_REG_POKE_HI
    movx @r0,a
    inc r0
    mov a,#((GRIDTAB+9)>>8)-4
    movx @r0,a
    dec r0
    mov a,#FN_REG_POKE
    movx @r0,a
    inc r0
    mov r6,#9
tbd_s:
    mov a,r5
    jz tbd_0
    dec r5
    mov a,#1
tbd_0:
    movx @r0,a
    djnz r6,tbd_s
    mov r1,#DIRTY
    mov a,@r1
    orl a,#DIRTY_GRID
    mov @r1,a
tbd_x:
    ret
