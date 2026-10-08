; tileline.asm -- a line of pixels across a row of cells (gfx.c's op_line,
; the auction's dotted price lines), all in one go: the cartridge's RAM is
; put in slot 2 once, and each cell is
;
;   - its tile's row r read from the RAM (tiles.c's T_SRC: patterns and
;     colours, no VRAM read back),
;   - painted as gfx.c's paint() does it (the colour the cell hasn't got
;     takes the place of the one with fewer pixels left),
;   - left alone if that changed nothing, else looked up in tiles.c's row
;     memo (old tile, row, its pattern and colours -> the new tile): a new
;     name and the tiles' counts. Only what the memo hasn't seen goes to
;     tiles.c (tile_row_miss), which stores the cell the long way and
;     notes the answer.
;
; Along a line the cells are mostly the same tile and the same few masks
; (the dots come round every 3 cells), so first of all a cell is looked
; for among the line's last four (old tile, mask -> new tile): the line's
; row and colour are the same for all of them. They are forgotten when a
; tile is made (tile_row_miss: its pixels are not what they were).
;
; In C, a call or eight a cell, a line was most of a frame.

        SECTION code_user

        PUBLIC  _tile_hline
        EXTERN  _hl_cell, _hl_n, _hl_r, _hl_pic, _hl_lm, _hl_rm, _hl_c
        EXTERN  _hl_p, _hl_cc, _hl_old, _tile_row_miss
        EXTERN  _tile_live, _tile_most, _popc, _made

        defc    T_SRC = $8000   ; tiles.c's layout of the cartridge's RAM
        defc    T_NAME = $9000
        defc    T_REF = $9300
        defc    RM_OLD = $9800  ; tiles.c's row memo, 256 of each
        defc    RM_R = $9900
        defc    RM_P = $9A00
        defc    RM_C = $9B00
        defc    RM_NEW = $9C00
        defc    VDP_DATA = $BE
        defc    VDP_CTRL = $BF
        defc    SV_NAME_W = $3800 | $4000

; void tile_hline(void): hl_n cells from hl_cell (all on one row of the
; screen), their row hl_r, the pixels in hl_pic[0..] (the first cell's
; masked by hl_lm, the last's by hl_rm) colour hl_c.
_tile_hline:
        ld      a,8
        ld      ($FFFC),a       ; the cartridge's RAM in slot 2
        ld      a,1
        ld      (first),a
        xor     a
        ld      (mc_n),a        ; no cells seen yet
hl_loop:
        ld      hl,(_hl_pic)
        ld      c,(hl)          ; C: the pixels in this cell
        inc     hl
        ld      (_hl_pic),hl
        ld      a,(first)
        or      a
        jr      z,hl_nf
        xor     a
        ld      (first),a
        ld      a,(_hl_lm)
        and     c
        ld      c,a
hl_nf:
        ld      a,(_hl_n)
        dec     a
        ld      (_hl_n),a
        jr      nz,hl_nl
        ld      a,(_hl_rm)      ; the last
        and     c
        ld      c,a
hl_nl:
        ld      a,c
        or      a
        jp      z,hl_next
        ld      (mask),a
        ld      hl,(_hl_cell)   ; the cell's tile
        ld      de,T_NAME
        add     hl,de
        ld      a,(hl)
        ld      (_hl_old),a
        ld      e,a             ; E: old
        ld      a,(mc_n)        ; one of the last few?
        or      a
        jr      z,hl_full
        ld      b,a
        ld      hl,mc
        ld      a,(mask)
        ld      c,a             ; C: the mask
mc_look:
        ld      a,(hl)
        inc     hl
        cp      e
        jr      nz,mc_no
        ld      a,(hl)
        cp      c
        jr      nz,mc_no
        inc     hl
        ld      a,(hl)          ; that one's tile
        cp      e
        jp      z,hl_next       ; (left as it was)
        ld      (tile),a
        call    set_name
        jp      hl_next
mc_no:
        inc     hl
        inc     hl
        djnz    mc_look
hl_full:
        ld      a,(_hl_old)
        ld      l,a             ; its row r
        ld      h,0
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        ld      de,T_SRC
        add     hl,de
        ld      a,(_hl_r)
        ld      e,a
        ld      d,0
        add     hl,de
        ld      b,(hl)          ; B: the pattern
        ld      de,8
        add     hl,de
        ld      c,(hl)          ; C: the colours, f << 4 | b
        ld      a,b
        ld      (was_p),a
        ld      a,c
        ld      (was_c),a
        call    paint
        ld      a,(was_p)       ; nothing changed: nothing to do
        cp      b
        jr      nz,hl_changed
        ld      a,(was_c)
        cp      c
        jr      nz,hl_changed
        ld      a,(_hl_old)
        jp      hl_seen
hl_changed:
        ; the memo's entry: old + (r << 3) + p + (c << 1) + (c >> 3)
        ld      a,(_hl_r)
        rlca
        rlca
        rlca
        ld      hl,_hl_old
        add     a,(hl)
        add     a,b
        ld      e,a
        ld      a,c
        add     a,a
        add     a,e
        ld      e,a
        ld      a,c
        rrca
        rrca
        rrca
        and     $1F
        add     a,e
        ld      l,a
        ld      h,RM_NEW >> 8
        ld      a,(hl)
        cp      $FF
        jr      z,hl_miss
        ld      (tile),a
        ld      h,RM_OLD >> 8
        ld      a,(_hl_old)
        cp      (hl)
        jr      nz,hl_miss
        ld      h,RM_R >> 8
        ld      a,(_hl_r)
        cp      (hl)
        jr      nz,hl_miss
        ld      h,RM_P >> 8
        ld      a,b
        cp      (hl)
        jr      nz,hl_miss
        ld      h,RM_C >> 8
        ld      a,c
        cp      (hl)
        jr      nz,hl_miss
        ld      a,(tile)        ; seen before: that tile
        ld      hl,_hl_old
        cp      (hl)
        call    nz,set_name
        ld      a,(tile)
        jr      hl_seen
hl_miss:
        ld      a,b
        ld      (_hl_p),a
        ld      a,c
        ld      (_hl_cc),a
        call    _tile_row_miss  ; (puts the RAM away)
        ld      a,8
        ld      ($FFFC),a
        ld      a,(_made)
        cp      $FF
        jr      z,hl_kept
        xor     a               ; a tile was made: forget the line's cells
        ld      (mc_n),a
        jr      hl_next
hl_kept:
        ld      hl,(_hl_cell)   ; what the cell has now
        ld      de,T_NAME
        add     hl,de
        ld      a,(hl)
hl_seen:
        ; (old, mask) -> A, among the line's last few
        ld      c,a
        ld      a,(mc_n)
        cp      4
        jr      c,mc_add
        ld      a,(mc_at)       ; all four in use: the oldest goes
        inc     a
        and     3
        ld      (mc_at),a
        jr      mc_put
mc_add:
        ld      (mc_at),a
        inc     a
        ld      (mc_n),a
        dec     a
mc_put:
        ld      l,a             ; mc + 3 * A
        add     a,a
        add     a,l
        ld      e,a
        ld      d,0
        ld      hl,mc
        add     hl,de
        ld      a,(_hl_old)
        ld      (hl),a
        inc     hl
        ld      a,(mask)
        ld      (hl),a
        inc     hl
        ld      (hl),c
hl_next:
        ld      hl,(_hl_cell)
        inc     hl
        ld      (_hl_cell),hl
        ld      a,(_hl_n)
        or      a
        jp      nz,hl_loop
        xor     a
        ld      ($FFFC),a       ; the ROM bank back in slot 2
        ret

; The cell (hl_cell) shows tile (tile) in place of hl_old: the counts
; (tiles.c's ref_up, ref_down), the name in the RAM and in VRAM.
set_name:
        ld      a,(tile)
        ld      l,a
        ld      h,T_REF >> 8
        ld      a,(hl)
        inc     a
        jr      z,sn_up         ; 255 sticks
        ld      (hl),a
        dec     a
        jr      nz,sn_up
        ld      hl,_tile_live   ; shown again
        inc     (hl)
        ld      a,(hl)
        ld      hl,_tile_most
        cp      (hl)
        jr      c,sn_up
        jr      z,sn_up
        ld      (hl),a
sn_up:
        ld      hl,(_hl_cell)
        ld      de,T_NAME
        add     hl,de
        ld      a,(tile)
        ld      (hl),a
        ld      hl,(_hl_cell)
        add     hl,hl
        ld      de,SV_NAME_W
        add     hl,de
        di                      ; (the interrupt reads the status: the
        ld      a,l             ; address latch)
        out     (VDP_CTRL),a
        ld      a,h
        out     (VDP_CTRL),a
        ld      a,(tile)
        out     (VDP_DATA),a
        ei
        ld      a,(_hl_old)
        ld      l,a
        ld      h,T_REF >> 8
        ld      a,(hl)
        cp      255
        ret     z
        dec     a
        ld      (hl),a
        ret     nz
        ld      hl,_tile_live   ; shown no more
        dec     (hl)
        ret

; gfx.c's paint() on one row: pattern B, colours C (f << 4 | b), the
; pixels in (mask) become colour hl_c. B and C come back painted.
paint:
        ld      a,(mask)
        cp      $FF
        jr      nz,pt_some
        ld      b,0             ; all of it
        ld      a,(_hl_c)
        ld      c,a
        rlca
        rlca
        rlca
        rlca
        or      c
        ld      c,a
        ret
pt_some:
        ld      a,c
        and     $0F
        ld      e,a             ; E: b
        ld      a,c
        rrca
        rrca
        rrca
        rrca
        and     $0F
        ld      d,a             ; D: f
        cp      e
        jr      nz,pt_two
        ld      b,0             ; one colour: it is all background
pt_two:
        ld      a,(_hl_c)
        cp      d
        jr      nz,pt_notf
        ld      a,d
        cp      e
        jr      z,pt_notf
        ld      a,(mask)        ; the foreground's
        or      b
        ld      b,a
        ret
pt_notf:
        ld      a,(_hl_c)
        cp      e
        jr      nz,pt_new
        ld      a,(mask)        ; the background's
        cpl
        and     b
        ld      b,a
        ret
pt_new:
        ld      a,(mask)        ; a colour it hasn't got: the one with
        cpl                     ; fewer pixels left becomes it
        ld      (rest),a
        and     b
        call    nbits
        ld      (nfg),a
        ld      a,(rest)
        ld      l,a
        ld      a,b
        cpl
        and     l
        call    nbits
        ld      hl,nfg
        cp      (hl)
        jr      c,pt_bg
        ld      a,(_hl_c)       ; the foreground becomes c
        rlca
        rlca
        rlca
        rlca
        or      e
        ld      c,a
        ld      a,(mask)
        or      b
        ld      b,a
        ret
pt_bg:
        ld      a,d             ; the background becomes c
        rlca
        rlca
        rlca
        rlca
        ld      hl,_hl_c
        or      (hl)
        ld      c,a
        ld      a,(mask)
        cpl
        and     b
        ld      b,a
        ret

nbits:
        push    de
        ld      hl,_popc
        ld      e,a
        ld      d,0
        add     hl,de
        ld      a,(hl)
        pop     de
        ret

        SECTION bss_user
first:  defs    1
mask:   defs    1
rest:   defs    1
nfg:    defs    1
tile:   defs    1
was_p:  defs    1
was_c:  defs    1
mc_n:   defs    1               ; the line's last cells: how many,
mc_at:  defs    1               ; the latest,
mc:     defs    12              ; and each old tile, mask, new tile
