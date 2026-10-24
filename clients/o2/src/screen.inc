; screen.inc -- the text screen: six lines at Y = 30 + 36n, no grid. One
; page, the template with the code that copies it.

; scr_text: the text screen -- six lines at Y = 30 + 36n, no grid, nothing
; else -- every table blank. Clobbers A, r0-r7.
scr_text:
    mov r1,#MODE                ; no map rows
    clr a
    mov @r1,a
    mov r2,#SCRCOL>>8           ; the colour, then the band list
    mov a,#SCRCOL&0FFh
    call poke_at
    mov r4,#scr_text_t&0FFh
    mov r6,#2+NBANDS*3
    call copy_rom
    mov r2,#GRIDTAB>>8          ; an empty grid
    mov a,#GRIDTAB&0FFh
    call poke_at
    mov r6,#28
    clr a
    call poke_n
    mov r2,#SPRTAB>>8           ; the sprites below the screen
    mov a,#SPRTAB&0FFh
    call poke_at
    mov r6,#4
st_spr:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    clr a
    call poke_b
    djnz r6,st_spr
    mov r2,#STATTAB>>8          ; and the ten static characters
    mov a,#STATTAB&0FFh
    call poke_at
    mov r6,#10
st_st:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    clr a
    call poke_b
    clr a
    call poke_b
    djnz r6,st_st
    mov r1,#DIRTY
    mov a,#DIRTY_STATIC|DIRTY_GRID
    mov @r1,a
    ; fall into scr_clear

; scr_clear: all six lines blank. Clobbers A, r0-r7.
scr_clear:
    clr a
sc_l:
    mov r0,#TMP0                ; (say_line keeps no register)
    movx @r0,a
    call lb_clear
    mov r0,#TMP0
    movx a,@r0
    mov r4,a
    call say_line
    mov r0,#TMP0
    movx a,@r0
    inc a
    mov r6,a
    xrl a,#6
    jz sc_x
    mov a,r6
    jmp sc_l
sc_x:
    ret

; copy_rom: r6 bytes from offset r4 of this page to the POKE pointer.
copy_rom:
    mov a,r4
    movp a,@a
    call poke_b
    inc r4
    djnz r6,copy_rom
    ret

; poke_n: r6 copies of A at the POKE pointer. Clobbers r0, r6.
poke_n:
    call poke_b
    djnz r6,poke_n
    ret

scr_text_t:
    db 000h                     ; black background
    db 256-1                    ; the first band: line 1
    db J_TXTA, T0&0FFh, 256-43  ; T0 at 30, then T1's band at 44
    db J_TXTA, T1&0FFh, 256-36  ; T1 at 66
    db J_TXTA, T2&0FFh, 256-36  ; T2 at 102
    db J_TXTB, T3&0FFh, 256-36  ; T3 at 138
    db J_TXTB, T4&0FFh, 256-36  ; T4 at 174
    db J_TXTB, T5&0FFh, 0       ; T5 at 210: the last
    db J_NONE, 0, 0
    db J_NONE, 0, 0
    db J_NONE, 0, 0

