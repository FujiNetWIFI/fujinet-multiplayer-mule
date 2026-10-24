; demo.inc -- the engine's demonstration map (not built): the port plan's map
; laid out at run time through disp.inc; the map page grew out of it.

    PAGE_HEAD
    db e_boot&0FFh              ; entry 0: the boot (fixed.inc start)
e_boot:
    jmp boot

    org 900h
    include "disp.inc"

; ---------------------------------------------------------------------------
    org 0A00h
; demo_rows: the five row tables and the ten right halves from demo_map,
; whose cells are (code, colour | 8 for a mountain's right half).
demo_rows:
    mov r1,#ROWBUF              ; row 0's Y, the loop's
    mov a,#Y_ROW0
    mov @r1,a
    inc r1                      ; ROWBUF+1: the row
    clr a
    mov @r1,a
    inc r1                      ; ROWBUF+2: right halves written so far
    mov @r1,a
dr_row:
    ; the row table at ROWS + 19*row
    mov r1,#ROWBUF+1
    mov a,@r1
    mov r7,a
    rl a
    rl a
    rl a
    rl a                        ; *16
    add a,r7
    add a,r7
    add a,r7                    ; *19
    add a,#ROWS&0FFh
    mov r3,a
    mov r2,#ROWS>>8
    call poke_at
    mov r1,#ROWBUF
    mov a,@r1
    call poke_b                 ; Y
    mov a,@r1
    rr a
    cpl a
    inc a
    mov r5,a                    ; the bias
    mov r7,#0                   ; cell order: 0,1,2,3, 5,6,7,8, 4
dr_cell:
    mov a,r7
    add a,#cellord&0FFh
    movp a,@a
    call dm_at                  ; r4 = offset of (row, cell) in demo_map
    mov a,r4
    movp a,@a
    call gptr
    mov a,r3
    call poke_b
    mov a,r4
    inc a
    movp a,@a
    anl a,#7
    rl a
    orl a,r1
    call poke_b
    inc r7
    mov a,r7
    xrl a,#9
    jnz dr_cell
    ; the right halves: re-aim POKE at STATTAB for each
    mov r7,#0
dr_rh:
    mov a,r7
    call dm_at
    mov a,r4
    inc a
    movp a,@a
    jb3 dr_rh1
dr_rh2:
    inc r7
    mov a,r7
    xrl a,#9
    jnz dr_rh
    mov r1,#ROWBUF              ; next row
    mov a,@r1
    add a,#ROW_PITCH
    mov @r1,a
    inc r1
    mov a,@r1
    inc a
    mov @r1,a
    xrl a,#5
    jnz dr_row
    ret
dr_rh1:
    mov r6,a                    ; colour | 8
    mov r1,#ROWBUF+2
    mov a,@r1
    inc a
    mov @r1,a
    dec a
    rl a
    rl a                        ; *4
    add a,#STATTAB&0FFh
    mov r2,#STATTAB>>8
    call poke_at
    mov r1,#ROWBUF
    mov a,@r1
    call poke_b                 ; Y
    mov a,r7                    ; X = 16*cell + 18
    swap a
    add a,#18
    call poke_b
    mov a,#2                    ; slope right
    call gptr
    mov a,r3
    call poke_b
    mov a,r6
    anl a,#7
    rl a
    orl a,r1
    call poke_b
    jmp dr_rh2

; dm_at: r4 = the offset in this page of demo_map's (row ROWBUF+1, cell A).
dm_at:
    mov r4,a
    mov r1,#ROWBUF+1
    mov a,@r1
    mov r0,a                    ; row * 18
    rl a
    rl a
    rl a
    add a,r0
    rl a
    add a,r4
    add a,r4
    add a,#demo_map&0FFh
    mov r4,a
    ret

cellord: db 0,1,2,3,5,6,7,8,4

; (code, colour | 8 = a mountain's right half); codes $00 ball, $01 slope
M1 equ C_DGREY|8
M2 equ C_CYAN|8
M3 equ C_WHITE|8
demo_map:
    db ' ',0, 1,M2, 'F',C_RED, ' ',0, '=',C_BLUE, 0,C_GREEN, ' ',0, ' ',0, 1,M3
    db 0,C_TAN, ' ',0, 1,M1, ' ',0, '=',C_BLUE, ' ',0, 'E',C_VIOLET, 1,M3, ' ',0
    db 1,M2, ' ',0, 'C',C_GREEN, ' ',0, '$',C_WHITE, ' ',0, 'S',C_RED|8, ' ',0, 'F',C_TAN
    db 1,C_VIOLET|8, ' ',0, ' ',0, 0,C_RED, 'F',C_GREEN, 1,M3, ' ',0, ' ',0, ' ',0
    db 1,M2, ' ',0, ' ',0, ' ',0, '=',C_BLUE, ' ',0, ' ',0, 1,M1, 0,C_VIOLET


; ---------------------------------------------------------------------------
    org 0B00h

; boot: the map screen with the demonstration map, then idle.
boot:
    call vdc_reset
    mov r1,#MODE                ; no bands, nothing staged while it is built
    clr a
    mov @r1,a
    call scr_map
    call demo_text
    call demo_rows
    call spr_park
    mov r1,#DIRTY
    mov a,#DIRTY_STATIC|DIRTY_GRID
    mov @r1,a
    mov r1,#MODE
    mov a,#MODE_ROWS
    mov @r1,a
    clr a
    mov t,a
    strt cnt
    en tcnti
    en i
idle:
    jmp idle

; vdc_reset: display off, every object parked, the grid empty. Main program, at boot.
; Clobbers A, r0, r2.
vdc_reset:
    mov a,#P1_VDC
    outl p1,a
    mov r0,#vdc_control
    clr a
    movx @r0,a
    mov r0,#0                   ; $00-$9F: sprites, characters, quads, shapes
    mov r2,#0A0h
vr_obj:
    movx @r0,a
    inc r0
    djnz r2,vr_obj
    mov r0,#vdc_gridh0          ; $C0-$C8, $D0-$D8, $E0-$E9
    mov r2,#9
vr_h:
    movx @r0,a
    inc r0
    djnz r2,vr_h
    mov r0,#vdc_gridi0
    mov r2,#9
vr_i:
    movx @r0,a
    inc r0
    djnz r2,vr_i
    mov r0,#vdc_gridv0
    mov r2,#10
vr_v:
    movx @r0,a
    inc r0
    djnz r2,vr_v
    mov a,#P1_MAIN
    outl p1,a
    ret

; spr_park: all four sprites below the screen.
spr_park:
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov r6,#4
spp_l:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    clr a
    call poke_b
    djnz r6,spp_l
    ret

; demo_text: T0, T1, T2 from the strings below.
demo_text:
    mov r4,#s_stat&0FFh
    mov r5,#C_WHITE
    mov r6,#C_GREEN
    call lb_load
    mov r2,#T0>>8
    mov r3,#T0&0FFh
    mov r4,#30
    call tx_build
    mov r4,#s_msg1&0FFh
    mov r5,#C_RED
    mov r6,#C_RED
    call lb_load
    mov r2,#T1>>8
    mov r3,#T1&0FFh
    mov r4,#188
    call tx_build
    mov r4,#s_msg2&0FFh
    mov r5,#C_WHITE
    mov r6,#C_WHITE
    call lb_load
    mov r2,#T2>>8
    mov r3,#T2&0FFh
    mov r4,#226
    jmp tx_build

; lb_load: the NUL-terminated string at offset r4 of this page into
; LINEBUF, space-padded; colours r5 and r6 into TXCOL, TXCOL2.
lb_load:
    mov r0,#TXCOL
    mov a,r5
    movx @r0,a
    inc r0
    mov a,r6
    movx @r0,a
    call lb_clear
    mov r0,#LINEBUF
lbl_l:
    mov a,r4
    movp a,@a
    jz lbl_done
    movx @r0,a
    inc r0
    inc r4
    jmp lbl_l
lbl_done:
    ret

s_stat: db "R3 GRANT   ",'$'|80h,'1'|80h,'2'|80h,'5'|80h,'0'|80h,0
s_msg1: db "ALICES TURN",0
s_msg2: db "F 4 E 2 S 0 C 0",0

; ---------------------------------------------------------------------------
    org 0C00h
; scr_map: the map screen's band list and grid, from scr_map_t.
scr_map:
    mov r2,#SCRCOL>>8           ; SCRCOL, BL_FIRST, then the bands
    mov a,#SCRCOL&0FFh
    call poke_at
    mov r4,#scr_map_t&0FFh
    mov r6,#2+NBANDS*3
    call copy_rom
    mov r2,#GRIDTAB>>8
    mov a,#GRIDTAB&0FFh
    call poke_at
    mov r4,#grid_map_t&0FFh
    mov r6,#28
    jmp copy_rom

; copy_rom: r6 bytes from offset r4 of this page to the POKE pointer.
copy_rom:
    mov a,r4
    movp a,@a
    call poke_b
    inc r4
    djnz r6,copy_rom
    ret

scr_map_t:
    db 002h                     ; black background, dim green grid
    db 256-1                    ; the first band: line 1
    db J_TXTA, T0&0FFh, 256-43                       ; T0 at 30; it ends at 44
    db J_ROW, (ROWS+1*ROW_TAB)&0FFh, 256-ROW_PITCH   ; row 0, stage row 1
    db J_ROW, (ROWS+2*ROW_TAB)&0FFh, 256-ROW_PITCH
    db J_ROW, (ROWS+3*ROW_TAB)&0FFh, 256-ROW_PITCH
    db J_ROW, (ROWS+4*ROW_TAB)&0FFh, 256-ROW_PITCH
    db J_ROW, 0FFh, 256-ROW_PITCH                    ; row 4: nothing to stage
    db J_TXTA, T1&0FFh, 256-38                       ; T1 at 188, then T2's band
    db J_TXTA, T2&0FFh, 0                            ; T2 at 226: the last
    db J_NONE, 0, 0

; The map's grid: horizontal lines 1-6 close grid rows 1-5, except across the
; river (column 4), which keeps the border and the town's box.
grid_map_t:
    db 7Eh,7Eh,7Eh,7Eh,5Ah,7Eh,7Eh,7Eh,7Eh          ; $C0-$C8
    db 0,0,0,0,0,0,0,0,0                             ; $D0-$D8
    db 3Eh,3Eh,3Eh,3Eh,3Eh,3Eh,3Eh,3Eh,3Eh,3Eh      ; $E0-$E9

    PAGE_TAIL
