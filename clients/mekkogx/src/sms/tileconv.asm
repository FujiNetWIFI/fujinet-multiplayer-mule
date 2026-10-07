; tileconv.asm -- tiles.c's inner loops: a cell's patterns and colours made a
; Mode 4 tile (and its hash), and a tile compared with what VRAM holds.
; In C they took most of a screen's drawing.

        SECTION code_user

        PUBLIC  _tile_conv, _tile_same
        EXTERN  _tw

        defc    VDP_DATA = $BE
        defc    VDP_CTRL = $BF

; uint8_t tile_conv(const uint8_t *pat, const uint8_t *col): the cell's 8
; rows (pattern p, colours f << 4 | b) into tw as four bit planes each;
; returns their hash (L): each byte xor'd into the hash turned left.
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
        ld      hl,_tw
        ld      b,32
        xor     a
conv_hash:
        rlca
        xor     (hl)
        inc     hl
        djnz    conv_hash
        ld      l,a
        ret

; uint8_t tile_same(uint8_t t) __z88dk_fastcall: does tile t hold tw? The
; reads are 44 cycles apart (the VDP wants 26 while it draws the screen).
_tile_same:
        ld      h,0
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        di                      ; (the interrupt reads the status: the
        ld      a,l             ; address latch)
        out     (VDP_CTRL),a
        ld      a,h
        out     (VDP_CTRL),a    ; (t * 32 < 4000h: a read)
        ei
        ld      hl,_tw
        ld      b,32
same_n:
        in      a,(VDP_DATA)
        cp      (hl)
        jr      nz,same_no
        inc     hl
        djnz    same_n
        ld      l,1
        ret
same_no:
        ld      l,0
        ret
