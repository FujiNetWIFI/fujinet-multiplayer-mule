; h19cell.asm -- the screen buffer's cell writes (src/h89/h19.c), the hot
; path of every screen change: a cell the same as before costs about 55
; T-states here, against 160 as zsdcc compiled the C. A cell that changes
; is stored and its dirty bit set; a row with a change is marked for the
; next flush.
;
;   void h19_put(uint8_t x, uint8_t y, uint8_t c)
;   void h19_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t c)
;   void h19_putn(uint8_t x, uint8_t y, const uint8_t *s, uint8_t n)
;
; zsdcc's arguments, on the stack (a byte for each uint8_t), the caller's
; to take off. AF, BC, DE, HL and IY are the callee's to use.

        SECTION code_user

        PUBLIC  _h19_put, _h19_fill, _h19_putn
        EXTERN  _rowp, _dirty, _rowdirty, _any_dirty

        defc    SCR_W = 80
        defc    SCR_H = 25

; A = y, C = x (both on the screen): HL = the cell, IY = its dirty byte,
; E = its bit. Keeps C; uses B, D.
cell_at:
        ld      b,a
        add     a,a
        ld      l,a
        ld      h,0
        ld      de,_rowp
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)                  ; DE = rowp[y]
        ld      a,b
        add     a,a
        add     a,a
        add     a,b
        add     a,a                     ; 10 y (ROWB = 10)
        ld      b,c
        srl     b
        srl     b
        srl     b
        add     a,b                     ; + x / 8
        ld      l,a
        ld      h,0
        ld      iy,_dirty
        ex      de,hl
        add     iy,de                   ; IY = &dirty[y][x / 8]
        ld      e,c
        ld      d,0
        add     hl,de                   ; HL = rowp[y] + x
        ld      a,c
        and     7
        ld      e,1
        ret     z
ca_bit:
        sla     e
        dec     a
        jr      nz,ca_bit
        ret

; Row cy has a change.
row_dirty:
        ld      a,(cy)
        ld      l,a
        ld      h,0
        ld      de,_rowdirty
        add     hl,de
        ld      (hl),1
        ld      a,1
        ld      (_any_dirty),a
        ret

_h19_put:
        ld      hl,2
        add     hl,sp
        ld      c,(hl)                  ; x
        inc     hl
        ld      a,(hl)                  ; y
        inc     hl
        ld      d,(hl)                  ; c
        cp      SCR_H
        ret     nc
        ld      (cy),a
        ld      a,c
        cp      SCR_W
        ret     nc
        ld      a,d
        ld      (cchar),a
        ld      a,(cy)
        call    cell_at
        ld      a,(cchar)
        cp      (hl)
        ret     z
        ld      (hl),a
        ld      a,(iy+0)
        or      e
        ld      (iy+0),a
        jp      row_dirty

_h19_fill:
        ld      hl,2
        add     hl,sp
        ld      c,(hl)                  ; x
        inc     hl
        ld      a,(hl)                  ; y
        inc     hl
        ld      b,(hl)                  ; w
        inc     hl
        ld      d,(hl)                  ; c
        cp      SCR_H
        ret     nc
        ld      (cy),a
        ld      a,c
        cp      SCR_W
        ret     nc
        ld      a,SCR_W                 ; w cut at the row's end
        sub     c
        cp      b
        jr      nc,fi_w
        ld      b,a
fi_w:
        ld      a,b
        or      a
        ret     z
        ld      (cn),a
        ld      a,d
        ld      (cchar),a
        ld      a,(cy)
        call    cell_at
        ld      a,(cn)
        ld      b,a
        ld      a,(cchar)
        ld      c,a
        ld      d,0                     ; (a change)
fi_loop:
        ld      a,(hl)
        cp      c
        jr      z,fi_same
        ld      (hl),c
        ld      a,(iy+0)
        or      e
        ld      (iy+0),a
        ld      d,1
fi_same:
        inc     hl
        rlc     e
        jr      nc,fi_next
        inc     iy
fi_next:
        djnz    fi_loop
        ld      a,d
        or      a
        ret     z
        jp      row_dirty

_h19_putn:
        ld      hl,2
        add     hl,sp
        ld      c,(hl)                  ; x
        inc     hl
        ld      a,(hl)                  ; y
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)                  ; s
        inc     hl
        ld      b,(hl)                  ; n
        cp      SCR_H
        ret     nc
        ld      (cy),a
        ld      a,c
        cp      SCR_W
        ret     nc
        ld      a,SCR_W
        sub     c
        cp      b
        jr      nc,pn_w
        ld      b,a
pn_w:
        ld      a,b
        or      a
        ret     z
        ld      (cn),a
        ld      (csrc),de
        ld      a,(cy)
        call    cell_at
        ld      c,e                     ; C = the bit
        ld      a,(cn)
        ld      b,a
        ld      de,(csrc)
        xor     a
        ld      (cchg),a
pn_loop:
        ld      a,(de)
        cp      (hl)
        jr      z,pn_same
        ld      (hl),a
        ld      a,(iy+0)
        or      c
        ld      (iy+0),a
        ld      a,1
        ld      (cchg),a
pn_same:
        inc     hl
        inc     de
        rlc     c
        jr      nc,pn_next
        inc     iy
pn_next:
        djnz    pn_loop
        ld      a,(cchg)
        or      a
        ret     z
        jp      row_dirty

        SECTION bss_user

cy:     defs    1
cn:     defs    1
cchar:  defs    1
cchg:   defs    1
csrc:   defs    2
