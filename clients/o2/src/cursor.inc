; cursor.inc -- the box round a plot: the grant's cursor, an event's plot.
; Sprites 0 and 1 (map.inc cur_shape), through SPRTAB.

; cur_show: the cursor's two sprites on plot G_CUR, or hidden. Clobbers A,
; r0-r7.
cur_hide:
    mov r6,#0F8h
    mov r7,#0
    jmp cs_put
cur_show:
    mov r0,#G_CUR
    movx a,@r0
    mov r6,#0                   ; row
cs_div:
    add a,#256-9
    jnc cs_rem
    inc r6
    jmp cs_div
cs_rem:
    add a,#9                    ; cell: X = 16 * cell + 9
    swap a
    add a,#X_QA-1
    mov r7,a
    mov a,r6                    ; Y = 54 + 24 * row
    rl a
    rl a
    rl a
    mov r6,a                    ; 8 row
    rl a                        ; 16 row
    add a,r6
    add a,#Y_ROW0
    mov r6,a
cs_put:
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov a,r6
    call poke_b                 ; sprite 0: Y, X, white
    mov a,r7
    call poke_b
    mov a,#SPR_WHITE
    call poke_b
    mov a,r6                    ; sprite 1: 8 to the right
    call poke_b
    mov a,r7
    add a,#8
    call poke_b
    mov a,#SPR_WHITE
    jmp poke_b
