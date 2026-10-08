; tileconv.asm -- tiles.c's inner loops: a cell's patterns and colours put
; in one form (and hashed), compared with a tile's in the cartridge's RAM,
; and made a Mode 4 tile. In C they took most of a screen's drawing.

        SECTION code_user

        PUBLIC  _tile_conv, _tc_canon, _tc_same, _tc_memo
        EXTERN  _tw, _ns, _m_pc

        defc    T_SRC = $8000   ; tile t's 16 bytes at T_SRC + 16t (tiles.c)

; void tile_conv(const uint8_t *pat, const uint8_t *col): the cell's 8
; rows (pattern p, colours f << 4 | b) into tw as four bit planes each.
;
; Plane k of a row is (p & (f ^ b)k) ^ bk: the pattern where just one of
; the colours has bit k (inverted if that one is b), all or nothing where
; both or neither have.
_tile_conv:
        pop     af              ; (the return address)
        pop     de              ; pat
        pop     hl              ; col
        push    hl
        push    de
        push    af
        push    ix
        push    hl
        pop     ix              ; IX: the colours
        ld      hl,_tw
        ld      b,8
conv_row:
        ld      a,(de)
        inc     de
        ld      c,a             ; C: the pattern
        ld      a,(ix+0)
        inc     ix
        push    de
        ld      e,a
        rrca
        rrca
        rrca
        rrca
        xor     e
        and     $0F
        ld      d,a             ; D: f ^ b
        ld      a,e
        and     $0F
        ld      e,a             ; E: b
        srl     d               ; plane 0
        sbc     a,a
        and     c
        srl     e
        jr      nc,conv_0
        cpl
conv_0:
        ld      (hl),a
        inc     hl
        srl     d               ; plane 1
        sbc     a,a
        and     c
        srl     e
        jr      nc,conv_1
        cpl
conv_1:
        ld      (hl),a
        inc     hl
        srl     d               ; plane 2
        sbc     a,a
        and     c
        srl     e
        jr      nc,conv_2
        cpl
conv_2:
        ld      (hl),a
        inc     hl
        srl     d               ; plane 3
        sbc     a,a
        and     c
        srl     e
        jr      nc,conv_3
        cpl
conv_3:
        ld      (hl),a
        inc     hl
        pop     de
        djnz    conv_row
        pop     ix
        ret

; uint8_t tc_canon(const uint8_t *pat, const uint8_t *col): the cell in ns
; (8 pattern, 8 colour bytes) in the one form its pixels have, the form a
; tile is kept in: each row's left pixel is the background (bit clear),
; and a row of one colour is pattern 0 in that colour twice. Returns the
; hash of the 16 bytes (L): each added to the hash turned left.
_tc_canon:
        pop     af
        pop     hl              ; pat
        pop     bc              ; col
        push    bc
        push    hl
        push    af
        push    bc              ; (LDI counts BC down)
        ld      de,_ns
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        pop     hl              ; col
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ldi
        ld      hl,_ns
        ld      de,_ns+8
        ld      b,8
canon_row:
        ld      a,(hl)          ; the pattern
        or      a
        jp      p,canon_pos
        cpl                     ; the left pixel set: the other way round
        ld      (hl),a
        ld      a,(de)
        rrca
        rrca
        rrca
        rrca
        ld      (de),a
canon_pos:
        ld      a,(de)
        ld      c,a
        rrca
        rrca
        rrca
        rrca
        xor     c
        and     $0F
        jr      z,canon_one     ; one colour
        ld      a,(hl)
        or      a
        jr      nz,canon_next
canon_one:
        ld      (hl),0          ; one colour: the background's, twice
        ld      a,c
        and     $0F
        ld      c,a
        rlca
        rlca
        rlca
        rlca
        or      c
        ld      (de),a
canon_next:
        inc     hl
        inc     de
        djnz    canon_row
        ld      hl,_ns
        ld      b,16
        xor     a
canon_hash:
        rlca
        add     a,(hl)
        inc     hl
        djnz    canon_hash
        ld      l,a
        ret

; uint8_t tc_same(uint8_t t) __z88dk_fastcall: does tile t (in the
; cartridge's RAM, which is in slot 2) hold ns?
_tc_same:
        ld      a,l
        rlca
        rlca
        rlca
        rlca
        ld      l,a
        and     $0F
        or      T_SRC >> 8
        ld      h,a
        ld      a,l
        and     $F0
        ld      l,a
        ld      de,_ns
        ld      b,16
same_n:
        ld      a,(de)
        cp      (hl)
        jr      nz,same_no
        inc     hl
        inc     de
        djnz    same_n
        ld      l,1
        ret
same_no:
        ld      l,0
        ret

; uint8_t tc_memo(const uint8_t *pat, const uint8_t *col): are they the
; cell stored last (m_pc: its 8 pattern bytes, then 8 colour bytes)?
_tc_memo:
        pop     af
        pop     hl              ; pat
        pop     bc              ; col
        push    bc
        push    hl
        push    af
        push    bc
        ld      de,_m_pc
        ld      b,8
memo_p:
        ld      a,(de)
        cp      (hl)
        jr      nz,memo_no1
        inc     hl
        inc     de
        djnz    memo_p
        pop     hl              ; col
        ld      b,8
memo_c:
        ld      a,(de)
        cp      (hl)
        jr      nz,memo_no
        inc     hl
        inc     de
        djnz    memo_c
        ld      l,1
        ret
memo_no1:
        pop     hl
memo_no:
        ld      l,0
        ret
