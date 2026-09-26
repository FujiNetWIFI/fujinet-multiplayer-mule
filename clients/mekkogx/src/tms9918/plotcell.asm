; plotcell.asm -- gfx_plot's inner loop (gfx.c): a plot's three layers,
; 32 lines of 3 bytes each, into its 12 cells' pattern and colour bytes.
;
; pl_trip holds the layers one after another: the owner's (border, icon,
; dots) at 0, white or black (mountains, the store) at 96, the river at
; 192. For each byte, a = owner, b = white & ~a, c = river & ~(a | b); the
; first of them that has pixels is the foreground, and the background is
; the plains or a later layer, whichever has more pixels there.
;
; pl_out is 12 cells of 16 bytes (8 pattern, 8 colour), cell cy * 3 + cx.
; IX walks the white layer (the others are at -96 and +96), IY the cells.
; IX and IY are the compiler's: saved.

        SECTION code_user

        PUBLIC  _plot_cells
        EXTERN  _pl_trip, _pl_out, _popc
        EXTERN  _pl_c0, _pl_c1, _pl_c1b, _pl_bg, _pl_bgbg, _pl_rbg

        defc    T_RIVER = 3

_plot_cells:
        push    ix
        push    iy
        ld      ix,_pl_trip+96
        ld      iy,_pl_out
        ld      a,4
        ld      (cy_n),a
cy_loop:
        push    iy                  ; the row's first cell, line 0
        ld      a,8
        ld      (l_n),a
l_loop:
        push    iy                  ; this line of the row's first cell
        call    one
        call    one
        call    one
        pop     iy
        inc     iy                  ; the next line
        ld      hl,l_n
        dec     (hl)
        jr      nz,l_loop
        pop     iy
        ld      de,48               ; the next row of cells
        add     iy,de
        ld      hl,cy_n
        dec     (hl)
        jr      nz,cy_loop
        pop     iy
        pop     ix
        ret

; A = popcount(A) (changes HL)
pop8:
        ld      hl,_popc
        add     a,l
        ld      l,a
        adc     a,h
        sub     l
        ld      h,a
        ld      a,(hl)
        ret

; One byte: IX at its white byte, IY at its cell's line. Leaves IY on the
; next cell (+16), IX on the next byte.
one:
        ld      a,(ix-96)
        ld      d,a                 ; d = a
        cpl
        and     (ix+0)
        ld      e,a                 ; e = b
        or      d
        cpl
        and     (ix+96)
        ld      b,a                 ; b = c
        inc     ix
        ld      a,d
        or      a
        jr      z,no_a
        ld      (iy+0),d            ; the owner's pixels
        ld      a,d
        or      e
        or      b
        cpl
        call    pop8
        ld      c,a                 ; best: the plains
        ld      a,(_pl_bg)
        ld      d,a                 ; bg
        ld      a,e
        call    pop8
        cp      c
        jr      c,a_c
        jr      z,a_c
        ld      c,a                 ; white has more
        ld      a,(_pl_c1b)
        ld      d,a
a_c:
        ld      a,b
        call    pop8
        cp      c
        jr      c,a_done
        jr      z,a_done
        ld      d,T_RIVER           ; the river has more
a_done:
        ld      a,(_pl_c0)
        or      d
        ld      (iy+8),a
        jr      next
no_a:
        ld      a,e
        or      a
        jr      z,no_b
        ld      (iy+0),e            ; white or black
        or      b
        cpl
        call    pop8
        ld      c,a                 ; the plains
        ld      a,b
        call    pop8
        cp      c
        ld      a,(_pl_bg)
        jr      c,b_done
        jr      z,b_done
        ld      a,T_RIVER
b_done:
        ld      d,a
        ld      a,(_pl_c1)
        or      d
        ld      (iy+8),a
        jr      next
no_b:
        ld      a,b
        or      a
        jr      z,none
        ld      (iy+0),b            ; the river
        ld      a,(_pl_rbg)
        ld      (iy+8),a
        jr      next
none:
        ld      (iy+0),0
        ld      a,(_pl_bgbg)
        ld      (iy+8),a
next:
        ld      de,16
        add     iy,de
        ret

        SECTION bss_user
cy_n:   defs    1
l_n:    defs    1
