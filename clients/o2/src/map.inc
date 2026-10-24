; map.inc -- the colony map: its screen and its plots, from the state.
;
; Two pages, placed by the includer at MAPBASE (page-aligned): map_build and
; the tables it reads with MOVP; map_screen and its templates.
;
; A plot is one character (plus a mountain's right half, ◣, a static single):
;   the town          $                   white
;   a M.U.L.E.        F E S C             the owner's colour (production: the
;                                         units made, 0-9)
;   owned, no M.U.L.E. ●, or ◢ on a mountain   the owner's colour
;   no one's          ◢◣ a mountain, shaded by its peaks: 1 grey, 2 cyan,
;                     3 white; = the river, blue; a plain, blank

; ===========================================================================
    org MAPBASE

; map_build: the five row tables (ROWS, page $6) and the ten mountains'
; right halves (STATTAB, page $4) from the state in the reply, begun: then
; map_row, a row a call, until MB_ROW is 5. (All five at once would hold
; the main loop up for most of a poll's frames.) Clobbers A, r0.
map_build:
    clr a
    mov r0,#MB_ROW
    movx @r0,a
    mov r0,#MB_M
    movx @r0,a
    mov r0,#MB_Y
    mov a,#Y_ROW0
    movx @r0,a
    ret

; map_row: row MB_ROW (< 5) and its right halves. About a frame.
; Clobbers A, r0-r7.
map_row:
    mov r0,#MB_ROW              ; the row's table: ROWS + 19 * row
    movx a,@r0
    mov r7,a
    swap a                      ; 16
    add a,r7
    add a,r7
    add a,r7                    ; 19
    add a,#ROWS&0FFh
    mov r0,#MB_RB
    movx @r0,a
    mov r2,#ROWS>>8
    call poke_at
    mov r0,#MB_Y
    movx a,@r0
    call poke_b                 ; Y
    clr a
    mov r0,#MB_K
    movx @r0,a
mb_cell:
    mov r0,#MB_K                ; the cells in the writer's order
    movx a,@r0
    add a,#cellord&0FFh
    movp a,@a
    mov r0,#MB_C
    movx @r0,a
    call mb_plot                ; A = the code; MB_COL its colour
    call mb_gptr
    call poke_sel               ; (mb_plot's reads re-selected RXWIN)
    mov a,r3
    call poke_b
    mov a,r1
    call poke_b
    mov r0,#MB_MNT              ; a mountain: its right half, then back to
    movx a,@r0                  ; the row's next cell
    jnz mb_next
    call mb_half
mb_next:
    mov r0,#MB_K
    movx a,@r0
    inc a
    movx @r0,a
    xrl a,#9
    jnz mb_cell
    mov r0,#MB_Y                ; the next row
    movx a,@r0
    add a,#ROW_PITCH
    movx @r0,a
    mov r0,#MB_ROW
    movx a,@r0
    inc a
    movx @r0,a
    ret

; mb_plot: the look of the plot in cell A of row MB_ROW: A = its code,
; MB_COL its colour, MB_MNT 0 for a mountain. Clobbers A, r0-r7.
mb_plot:
    mov r7,a
    mov r0,#MB_ROW              ; p = 9 * row + cell
    movx a,@r0
    mov r6,a
    rl a
    rl a
    rl a
    add a,r6
    add a,r7
    mov r0,#MB_P
    movx @r0,a
    mov r6,a
    mov a,#MAP_TERRAIN          ; the terrain
    call mb_byte
    mov r4,a
    anl a,#3
    xrl a,#TERRAIN_MOUNTAIN     ; MB_MNT: 0 for a mountain
    mov r0,#MB_MNT
    movx @r0,a
    mov a,r4
    anl a,#3
    xrl a,#TERRAIN_STORE
    jnz mp_notown
    mov r0,#MB_COL
    mov a,#C_WHITE
    movx @r0,a
    mov a,#'$'
    ret
mp_notown:
    mov a,#MAP_OWNER            ; the owner
    call mb_byte
    mov r5,a
    mov a,#MAP_MULE             ; a M.U.L.E.?
    call mb_byte
    jb7 mp_nomule
    mov r7,a                    ; the good
    mov a,r5
    call mp_seatcol
    mov r0,#MAPDIG
    movx a,@r0
    jz mp_letter
    mov a,#MAP_PROD             ; production: the units
    call mb_byte
    anl a,#0Fh
    add a,#'0'
    ret
mp_letter:
    mov a,r7
    anl a,#3
    add a,#goodch&0FFh
    movp a,@a
    ret
mp_nomule:
    mov a,r5
    jb7 mp_free
    call mp_seatcol             ; owned: a ball, or the mountain's left half
    mov r0,#MB_MNT
    movx a,@r0
    jz mp_slope
    clr a                       ; ball
    ret
mp_slope:
    mov a,#1
    ret
mp_free:
    mov r0,#MB_MNT
    movx a,@r0
    jnz mp_flat
    mov a,r4                    ; its peaks shade it
    rr a
    rr a
    anl a,#3
    add a,#mtncol&0FFh
    movp a,@a
    mov r0,#MB_COL
    movx @r0,a
    mov a,#1
    ret
mp_flat:
    mov r0,#MB_COL
    mov a,r4
    anl a,#3
    jz mp_plain
    mov a,#C_BLUE               ; the river
    movx @r0,a
    mov a,#'='
    ret
mp_plain:
    mov a,#C_WHITE
    movx @r0,a
    mov a,#' '
    ret

; mp_seatcol: MB_COL = seat A's colour. Clobbers A, r0.
mp_seatcol:
    anl a,#3
    add a,#seatcol&0FFh
    movp a,@a
    mov r0,#MB_COL
    movx @r0,a
    ret

cellord: db 0,1,2,3,5,6,7,8,4
goodch:  db "FESC"
mtncol:  db C_WHITE, C_DGREY, C_CYAN, C_WHITE   ; by peaks (0 unused)
seatcol: db C_RED, C_GREEN, C_TAN, C_VIOLET

; ===========================================================================
    org MAPBASE+100h

; mb_half: cell MB_C's right half (MB_COL) into STATTAB's next entry; then
; POKE aimed back at the row's table, after cell MB_K.
mb_half:
    mov r0,#MB_M
    movx a,@r0
    mov r7,a
    inc a
    movx @r0,a
    mov a,r7
    rl a
    rl a                        ; 4 bytes an entry
    add a,#STATTAB&0FFh
    mov r2,#STATTAB>>8
    call poke_at
    mov r0,#MB_Y
    movx a,@r0
    call poke_b                 ; Y
    mov r0,#MB_C                ; X = 16 * cell + 18
    movx a,@r0
    swap a
    add a,#X_QA+8
    call poke_b
    mov a,#2                    ; slope right
    call mb_gptr
    mov a,r3
    call poke_b
    mov a,r1
    call poke_b
    mov r0,#MB_K                ; the row's table again: RB + 1 + 2 (k + 1)
    movx a,@r0
    rl a
    add a,#3
    mov r6,a
    mov r0,#MB_RB
    movx a,@r0
    add a,r6
    mov r2,#ROWS>>8
    jmp poke_at

; mb_gptr: gptr for code A on this row, with MB_COL: r3 the pointer's low
; byte, r1 the control byte. Clobbers A, r0, r5.
mb_gptr:
    mov r5,a
    mov r0,#MB_Y
    movx a,@r0
    rr a
    cpl a
    inc a
    xch a,r5                    ; r5 = -(Y/2), A = the code
    call gptr
    mov r0,#MB_COL
    movx a,@r0
    rl a
    orl a,r1
    mov r1,a
    ret

; mb_byte: A = the map's array at offset A (MAP_*), plot MB_P (r6).
; Clobbers r0, r2, r3, r6, r7; keeps r4, r5.
mb_byte:
    add a,#ST_MAP&0FFh          ; 16 bits: ST_MAP + array + plot
    mov r2,a
    clr a
    addc a,#ST_MAP>>8
    mov r3,a
    mov r0,#MB_P
    movx a,@r0
    add a,r2
    mov r2,a
    clr a
    addc a,r3
    mov r3,a
    jmp st_byte


; map_screen: the map's band list, grid and colour; the cursor's sprite
; shapes; rows on. The row tables come from map_build. Clobbers A, r0-r7.
map_screen:
    mov r1,#MODE                ; no rows while the list changes
    clr a
    mov @r1,a
    mov r2,#SCRCOL>>8
    mov a,#SCRCOL&0FFh
    call poke_at
    mov r4,#map_bands&0FFh
    mov r6,#2+NBANDS*3
    call ms_copy
    mov r2,#GRIDTAB>>8
    mov a,#GRIDTAB&0FFh
    call poke_at
    mov r4,#map_grid&0FFh
    mov r6,#28
    call ms_copy
    mov a,#P1_VDC               ; the cursor: two sprites, its halves
    outl p1,a
    mov r0,#vdc_spr0_shape
    mov r4,#cur_shape&0FFh
    mov r6,#16
ms_shape:
    mov a,r4
    movp a,@a
    movx @r0,a
    inc r0
    inc r4
    djnz r6,ms_shape
    mov a,#P1_MAIN
    outl p1,a
    call spr_hide
    mov r1,#DIRTY
    mov a,#DIRTY_GRID|DIRTY_STATIC
    mov @r1,a
    mov r1,#MODE
    mov a,#MODE_ROWS
    mov @r1,a
    ret

; spr_hide: all four sprites below the screen. Clobbers A, r0, r1, r6.
spr_hide:
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov r6,#4
sh_l:
    mov a,#0F8h
    call poke_b
    clr a
    call poke_b
    clr a
    call poke_b
    djnz r6,sh_l
    ret

; ms_copy: r6 bytes from offset r4 of this page to the POKE pointer.
ms_copy:
    mov a,r4
    movp a,@a
    call poke_b
    inc r4
    djnz r6,ms_copy
    ret

map_bands:
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

; The grid: horizontal lines 1-6 close grid rows 1-5, except across the river
; (column 4), which keeps the border and the town's box.
map_grid:
    db 7Eh,7Eh,7Eh,7Eh,5Ah,7Eh,7Eh,7Eh,7Eh          ; $C0-$C8
    db 0,0,0,0,0,0,0,0,0                             ; $D0-$D8
    db 3Eh,3Eh,3Eh,3Eh,3Eh,3Eh,3Eh,3Eh,3Eh,3Eh      ; $E0-$E9

; The cursor: a box round a plot, 16 x 14, as two sprites (bit 0 the
; leftmost pixel).
cur_shape:
    db 0FFh,001h,001h,001h,001h,001h,0FFh,000h
    db 0FFh,080h,080h,080h,080h,080h,0FFh,000h
