; gfxa.asm -- gfx.c's loops that run every frame, in 8086 assembly: an
; 8088 at 4.77 MHz spends most of a frame in the C versions' calls,
; multiplies and far pointers (the PCjr more, its memory shared with the
; screen). In gfx_TEXT with gfx.c, so the calls are near. DS is DGROUP,
; as is SS; ES is the screen (gfx.c's vram_p) while drawing.

.8086
                NAME    gfxa

DGROUP          GROUP   _BSS

_BSS            SEGMENT WORD PUBLIC 'BSS'
                EXTRN   _gx_jp:WORD, _gx_nc:BYTE, _gx_y0:BYTE, _gx_y1:BYTE
                EXTRN   _gx_pass:BYTE, _lift_y0:BYTE, _lift_y1:BYTE
                EXTRN   _rowoff:WORD, _cga_w:WORD, _plat_jr:BYTE, _vram_p:DWORD
                EXTRN   _jr_w:WORD, _m4:BYTE
                EXTRN   _gx_gc:BYTE, _gx_gb:WORD, _gx_off:WORD, _gx_n:BYTE
                EXTRN   _gx_rows:BYTE, _gx_y:BYTE, _gx_fg0:BYTE, _gx_fg1:BYTE
                EXTRN   _gx_bgp:BYTE, _gx_kind:BYTE
                EXTRN   _gx_mid:BYTE, _gx_lm:BYTE, _gx_rm:BYTE
                EXTRN   _pl_o1:WORD, _pl_o2:WORD, _pl_o3:WORD, _pl_wh:WORD, _pl_rv:WORD
                EXTRN   _pl_r:WORD, _pl_w:WORD, _pl_bg:WORD, _pl_x:WORD
                EXTRN   _pl_y0:BYTE, _pl_p0:BYTE, _pl_p1:BYTE
                EXTRN   _gx_tsrc:DWORD, _gx_cx0:BYTE, _gx_cy0:BYTE
                EXTRN   _gx_cx1:BYTE, _gx_cy1:BYTE, _cga2:BYTE
                EXTRN   _hq_n:BYTE, _hq_lo:BYTE, _hq_hi:BYTE, _hq_y:BYTE
                EXTRN   _hq_c:BYTE, _hq_fg:BYTE, _hq_lm:BYTE, _hq_off:WORD
                EXTRN   _hd_off:WORD, _hd_n:BYTE
row             DW      ?               ; the line's first byte on the screen
jend            DW      ?               ; past the last job's pointer
jpi             DW      ?               ; the job's pointer
cur             DB      ?               ; the line
hql             DB      ?               ; a dotted line waits on it
cnt             DB      ?               ; clkrow: source bytes
sbuf            DB      6 DUP (?)       ; clkrow: a row shifted a clock right
rofs            DW      ?               ; gx_cells: the row in cell 0's picture
rr              DB      ?               ; gx_cells, gx_rect, gx_plot: the row
pw              DW      ?               ; gx_plot: the owner's pattern
pidx            DW      ?               ; gx_plot: the layers' byte
vo              DW      ?               ; gx_plot: its owner (lo), river (hi) bits
vw              DB      ?               ; gx_plot: its white bits
tline           DB      ?               ; gx_town: the line
tx              DB      ?               ; gx_town: the clock on it
hold            DB      ?               ; gx_town: CGA's left clock
_BSS            ENDS

; gfx.c's SprJob
J_SAVE          EQU     0
J_SRC           EQU     2
J_X             EQU     4
J_NO            EQU     6
J_NST           EQU     8
J_NEND          EQU     10
J_OO            EQU     12
J_OST           EQU     14
J_OEND          EQU     16
J_SHOW          EQU     18
J_SY            EQU     19
J_SH            EQU     20
J_BX            EQU     21
J_N             EQU     22
J_WB            EQU     23
J_PT0           EQU     24
J_PT1           EQU     25
J_DRAWN         EQU     26
J_OSY           EQU     27
J_OSH           EQU     28
J_OBX           EQU     29
J_ON            EQU     30
J_LIFT          EQU     31
J_SAME          EQU     32
J_PUT           EQU     33

gfx_TEXT        SEGMENT BYTE PUBLIC 'CODE'
                ASSUME  CS:gfx_TEXT, DS:DGROUP, SS:DGROUP

; void gx_sprlines(void): the gx_nc sprites at gx_jp[], lines gx_y0 ..
; gx_y1 - 1, a line at a time: (gx_pass 0 or 2) each one's old row back,
; the last drawn first, unless spr_bg put it back already (lifted 2, and
; the line in lift_y0 .. lift_y1 - 1); then (1 or 2) the dotted lines
; waiting on the line (gfx_hdots), and each one's new row saved and
; drawn, in order. A sprite only under waiting lines (lifted 4) stays as
; it is on the others. The rings' offsets and the pictures' rows move on
; in the jobs.
                PUBLIC  gx_sprlines_
gx_sprlines_    PROC    NEAR
                push    bp
                mov     es, word ptr _vram_p+2
                mov     al, _gx_nc
                xor     ah, ah
                shl     ax, 1
                add     ax, offset DGROUP:_gx_jp
                mov     jend, ax
                mov     hql, 0
                mov     al, _gx_y0
                mov     cur, al
sl_line:        mov     al, cur
                cmp     al, _gx_y1
                jb      sl_go
                pop     bp
                ret
                ; (out of the loop, in a short jump's reach: hql for line
                ; AL)
sl_hq:          xor     bx, bx
                cmp     al, _hq_hi
                ja      sl_hql
sl_hqk:         cmp     bl, _hq_n
                jae     sl_hqn
                cmp     al, _hq_y[bx]
                je      sl_hqy
                inc     bx
                jmp     sl_hqk
sl_hqy:         mov     bl, 1
                jmp     short sl_hql
sl_hqn:         xor     bl, bl
sl_hql:         mov     hql, bl
                jmp     short sl_hq0
sl_go:          xor     ah, ah
                mov     bx, ax
                shl     bx, 1
                mov     dx, _rowoff[bx]
                mov     row, dx
                ; is a dotted line waiting on it? (none before hq_lo: hql
                ; is still 0 there)
                cmp     al, _hq_lo
                jae     sl_hq
sl_hq0:
                cmp     _gx_pass, 1
                je      sl_draws
                mov     ax, jend
                mov     jpi, ax
sl_put:         mov     si, jpi
                cmp     si, offset DGROUP:_gx_jp
                jbe     sl_draws
                dec     si
                dec     si
                mov     jpi, si
                mov     bx, [si]
                cmp     byte ptr [bx+J_DRAWN], 0
                je      sl_put
                mov     al, cur
                sub     al, [bx+J_OSY]
                cmp     al, [bx+J_OSH]
                jae     sl_put
                cmp     byte ptr [bx+J_LIFT], 0
                jne     sl_put_lift     ; (rare: out of the loop)
sl_put_do:      mov     al, cur
                mov     [bx+J_PUT], al
                mov     si, [bx+J_SAVE]
                add     si, [bx+J_OO]
                mov     di, row
                mov     al, [bx+J_OBX]
                xor     ah, ah
                add     di, ax
                mov     cl, [bx+J_ON]
                xor     ch, ch
                rep     movsb
sl_put_adv:     mov     ax, [bx+J_OO]
                add     ax, [bx+J_OST]
                cmp     ax, [bx+J_OEND]
                jb      sl_put_st
                xor     ax, ax
sl_put_st:      mov     [bx+J_OO], ax
                jmp     sl_put
sl_next0:       inc     cur             ; (sl_next, in a short jump's reach)
                jmp     sl_line
sl_hook:        call    hq_line
                jmp     short sl_drw0
sl_draws:       cmp     _gx_pass, 0
                je      sl_next0
                cmp     hql, 0
                jne     sl_hook
sl_drw0:        mov     jpi, offset DGROUP:_gx_jp
sl_drw:         mov     si, jpi
                cmp     si, jend
                jae     sl_next
                mov     bx, [si]
                cmp     byte ptr [bx+J_SHOW], 0
                je      sl_drw_next
                mov     al, cur
                sub     al, [bx+J_SY]
                cmp     al, [bx+J_SH]
                jae     sl_drw_next
                ; what is under it, into its ring (screen -> DGROUP),
                ; unless that is what was just put back (same 1), or it
                ; stays here (same 2: only the waiting lines change it)
                cmp     byte ptr [bx+J_SAME], 0
                je      sl_save
                test    byte ptr [bx+J_SAME], 2
                jnz     sl_stay
sl_same:        mov     al, cur
                cmp     al, [bx+J_PUT]
                je      sl_saved
sl_save:        mov     di, [bx+J_SAVE]
                add     di, [bx+J_NO]
                mov     si, row
                mov     al, [bx+J_BX]
                xor     ah, ah
                add     si, ax
                mov     cl, [bx+J_N]
                xor     ch, ch
                push    ds
                push    es
                pop     ds
                push    ss
                pop     es
                rep     movsb
                push    ds
                pop     es
                pop     ds
sl_saved:       mov     ax, [bx+J_NO]
                add     ax, [bx+J_NST]
                cmp     ax, [bx+J_NEND]
                jb      sl_drw_st
                xor     ax, ax
sl_drw_st:      mov     [bx+J_NO], ax
                push    bx
                call    clkrow
                pop     bx
sl_src:         mov     al, [bx+J_WB]
                xor     ah, ah
                add     [bx+J_SRC], ax
sl_drw_next:    add     jpi, 2
                jmp     sl_drw
sl_next:        inc     cur
                jmp     sl_line
                ; (out of the loop: lifted, its old row goes back unless
                ; spr_bg put it back already (2), or only on a waiting
                ; line (4) ...
sl_put_lift:    test    byte ptr [bx+J_LIFT], 4
                jnz     sl_put_hq
                mov     al, cur
                cmp     al, _lift_y0
                jb      sl_put_do0
                cmp     al, _lift_y1
                jb      sl_put_adv0
sl_put_do0:     jmp     sl_put_do
sl_put_hq:      cmp     hql, 0
                jne     sl_put_do0
sl_put_adv0:    jmp     sl_put_adv
                ; ... and same 2: on the others it stays, its rows moving
                ; on)
sl_stay:        cmp     hql, 0
                jne     sl_save
                mov     ax, [bx+J_NO]
                add     ax, [bx+J_NST]
                cmp     ax, [bx+J_NEND]
                jb      sl_stay1
                xor     ax, ax
sl_stay1:       mov     [bx+J_NO], ax
                jmp     sl_src
gx_sprlines_    ENDP

; The dotted lines waiting on line cur, in the order they came, where
; the old rows just put back were.
hq_line         PROC    NEAR
                xor     si, si
hq_k:           mov     ax, si
                cmp     al, _hq_n
                jae     hq_ret
                mov     al, cur
                cmp     al, _hq_y[si]
                jne     hq_nx
                mov     _gx_y, al
                mov     al, _hq_c[si]
                mov     _hd_n, al
                mov     al, _hq_fg[si]
                mov     _gx_fg0, al
                mov     al, _hq_lm[si]
                mov     _gx_lm, al
                mov     bx, si
                shl     bx, 1
                mov     ax, _hq_off[bx]
                mov     _hd_off, ax
                mov     jpi, offset DGROUP:_gx_jp
hq_j:           mov     bx, jpi
                cmp     bx, jend
                jae     hq_nx
                add     jpi, 2
                mov     bx, [bx]
                cmp     byte ptr [bx+J_DRAWN], 0
                je      hq_j
                mov     al, cur
                sub     al, [bx+J_OSY]
                cmp     al, [bx+J_OSH]
                jae     hq_j
                mov     al, [bx+J_OBX]
                mov     dl, al
                add     dl, [bx+J_ON]
                call    gx_hdclip_
                jmp     hq_j
hq_nx:          inc     si
                jmp     hq_k
hq_ret:         ret
hq_line         ENDP

; void gx_hdclip(a AL, b DL; AH, DH whatever): see gfx.c.
                PUBLIC  gx_hdclip_
gx_hdclip_      PROC    NEAR
                xor     ah, ah
                xor     dh, dh
                mov     bx, _hd_off
                xor     cx, cx
                cmp     _plat_jr, 0
                je      hc_s
                inc     cx                  ; the PCjr: a dot every other byte
hc_s:           sub     ax, bx              ; the dots before byte a ...
                jg      hc_a
                xor     ax, ax
                jmp     short hc_b
hc_a:           add     ax, cx
                shr     ax, cl
hc_b:           sub     dx, bx              ; ... and before byte b
                jle     hc_ret
                add     dx, cx
                shr     dx, cl
                mov     di, dx
                mov     dl, _hd_n
                cmp     di, dx
                jbe     hc_e
                mov     di, dx
hc_e:           sub     di, ax
                jle     hc_ret
                mov     dx, di
                mov     _gx_n, dl
                shl     ax, cl
                add     ax, bx
                mov     _gx_off, ax
                jmp     gx_hdots_
hc_ret:         ret
gx_hdclip_      ENDP

; Job BX's picture row (a bit a colour clock, MSB left) at its x on line
; cur: lit clocks become its pattern for the line's parity, the rest stay.
; Clobbers everything but DS and ES.
clkrow          PROC    NEAR
                mov     si, [bx+J_SRC]
                mov     dl, [bx+J_PT0]
                test    cur, 1
                jz      cr_pt
                mov     dl, [bx+J_PT1]
cr_pt:          mov     cl, [bx+J_WB]
                mov     cnt, cl
                xor     ch, ch
                mov     ax, [bx+J_X]
                cmp     _plat_jr, 0
                je      cr_cga
                ; the PCjr: a clock is a byte
                or      ax, ax
                js      cr_slow
                mov     di, cx
                shl     di, 1
                shl     di, 1
                shl     di, 1
                add     di, ax
                cmp     di, 160
                ja      cr_slow
                mov     di, row
                add     di, ax
cr_jbyte:       lodsb
                or      al, al
                jz      cr_jnext
                shl     al, 1
                jnc     cr_j1
                mov     es:[di], dl
cr_j1:          shl     al, 1
                jnc     cr_j2
                mov     es:[di+1], dl
cr_j2:          shl     al, 1
                jnc     cr_j3
                mov     es:[di+2], dl
cr_j3:          shl     al, 1
                jnc     cr_j4
                mov     es:[di+3], dl
cr_j4:          shl     al, 1
                jnc     cr_j5
                mov     es:[di+4], dl
cr_j5:          shl     al, 1
                jnc     cr_j6
                mov     es:[di+5], dl
cr_j6:          shl     al, 1
                jnc     cr_j7
                mov     es:[di+6], dl
cr_j7:          shl     al, 1
                jnc     cr_jnext
                mov     es:[di+7], dl
cr_jnext:       add     di, 8
                loop    cr_jbyte
                ret

; Either edge off the screen: a clock at a time, those on it drawn.
; AX = x, CX = source bytes, SI = the source, DL = the pattern.
cr_slow:        mov     bp, ax
cr_sbyte:       mov     bl, [si]
                inc     si
                mov     bh, 8
cr_sbit:        shl     bl, 1
                jnc     cr_snext
                cmp     bp, 160
                jae     cr_snext
                mov     di, row
                cmp     _plat_jr, 0
                je      cr_scga
                mov     es:[bp+di], dl
                jmp     cr_snext
cr_scga:        mov     ax, bp
                shr     ax, 1
                add     di, ax
                mov     al, 0F0h        ; an even clock: the byte's left half
                test    bp, 1
                jz      cr_smask
                mov     al, 0Fh
cr_smask:       mov     dh, es:[di]
                mov     ah, dh
                xor     ah, dl
                and     ah, al
                xor     ah, dh
                mov     es:[di], ah
cr_snext:       inc     bp
                dec     bh
                jnz     cr_sbit
                loop    cr_sbyte
                ret

; CGA: two clocks a byte, a source byte two words (cga_w's masks). An odd
; x puts a clock of nothing in front: the source a clock to the right, a
; byte longer.
cr_cga:         mov     di, ax
                sar     di, 1           ; the first byte
                js      cr_slow
                mov     bp, cx
                test    al, 1
                jz      cr_c0
                inc     bp
cr_c0:          mov     bx, bp
                shl     bx, 1
                shl     bx, 1
                add     bx, di
                cmp     bx, 80
                ja      cr_slow
                add     di, row
                test    al, 1
                jz      cr_cgo
                mov     bx, offset DGROUP:sbuf
                xor     ah, ah          ; the clock carried over
cr_csh:         lodsb
                mov     ch, al
                shr     ah, 1
                rcr     al, 1
                mov     [bx], al
                inc     bx
                mov     ah, ch
                dec     cl
                jnz     cr_csh
                shr     ah, 1
                mov     al, 0
                rcr     al, 1
                mov     [bx], al
                mov     si, offset DGROUP:sbuf
cr_cgo:         mov     dh, dl          ; DX: the pattern, a word
cr_cbyte:       lodsb
                mov     ch, al
                mov     bl, al
                and     bl, 0F0h
                jz      cr_chi
                shr     bl, 1
                shr     bl, 1
                shr     bl, 1
                xor     bh, bh
                mov     bx, _cga_w[bx]
                mov     ax, es:[di]
                xor     ax, dx
                and     ax, bx
                xor     ax, es:[di]
                mov     es:[di], ax
cr_chi:         mov     bl, ch
                and     bl, 0Fh
                jz      cr_clo
                shl     bl, 1
                xor     bh, bh
                mov     bx, _cga_w[bx]
                mov     ax, es:[di+2]
                xor     ax, dx
                and     ax, bx
                xor     ax, es:[di+2]
                mov     es:[di+2], ax
cr_clo:         add     di, 4
                dec     bp
                jnz     cr_cbyte
                ret
clkrow          ENDP

; gx_cells' next cell: AL = its picture's row byte (SI: the cell list)
CELLV           MACRO
                lodsb
                xor     ah, ah
                shl     ax, 1
                shl     ax, 1
                shl     ax, 1
                add     ax, rofs
                mov     bx, ax
                mov     al, [bx]
                ENDM

; Opaque output: AX = the mask; DX = background ^ pattern, BP = background
PUT             MACRO
                and     ax, dx
                xor     ax, bp
                stosw
                ENDM

; void gx_cells(void): see gfx.c. A line at a time, every cell's row on
; it: the line's address and pattern once, then a few table look-ups a
; cell.
                PUBLIC  gx_cells_
gx_cells_       PROC    NEAR
                push    bp
                mov     es, word ptr _vram_p+2
                mov     rr, 0
gc_row:         mov     al, rr
                cmp     al, _gx_rows
                jb      gc_go
                pop     bp
                ret
gc_go:          add     al, _gx_y
                mov     dl, _gx_fg0
                test    al, 1
                jz      gc_pt
                mov     dl, _gx_fg1
gc_pt:          mov     dh, dl
                xor     ah, ah
                mov     bx, ax
                shl     bx, 1
                mov     di, _rowoff[bx]
                add     di, _gx_off
                mov     al, _gx_bgp
                mov     ah, al
                mov     bp, ax
                xor     dx, ax
                mov     al, rr
                cmp     _gx_rows, 8
                jbe     gc_r
                shr     al, 1           ; 16 lines: each row twice
gc_r:           xor     ah, ah
                add     ax, _gx_gb
                mov     rofs, ax
                mov     si, offset DGROUP:_gx_gc
                mov     cl, _gx_n
                or      cl, cl
                jz      gc_next
                cmp     _gx_kind, 0
                jne     gc_big
                cmp     _plat_jr, 0
                jne     gc_tjr
gc_tc:          CELLV                   ; CGA text: a byte a nibble (m4)
                mov     ch, al
                and     al, 0Fh
                mov     bl, al
                xor     bh, bh
                mov     ah, _m4[bx]
                mov     bl, ch
                shr     bl, 1
                shr     bl, 1
                shr     bl, 1
                shr     bl, 1
                mov     al, _m4[bx]
                PUT
                dec     cl
                jnz     gc_tc
                jmp     gc_next
gc_tjr:         CELLV                   ; PCjr text: a word a nibble (cga_w)
                mov     ch, al
                mov     bl, al
                and     bl, 0F0h
                shr     bl, 1
                shr     bl, 1
                shr     bl, 1
                xor     bh, bh
                mov     ax, _cga_w[bx]
                PUT
                mov     bl, ch
                and     bl, 0Fh
                shl     bl, 1
                mov     ax, _cga_w[bx]
                PUT
                dec     cl
                jnz     gc_tjr
                jmp     gc_next
gc_big:         cmp     _plat_jr, 0
                jne     gc_bjr
gc_bc:          CELLV                   ; CGA big: a word a nibble (cga_w)
                mov     ch, al
                mov     bl, al
                and     bl, 0F0h
                shr     bl, 1
                shr     bl, 1
                shr     bl, 1
                xor     bh, bh
                mov     ax, _cga_w[bx]
                PUT
                mov     bl, ch
                and     bl, 0Fh
                shl     bl, 1
                mov     ax, _cga_w[bx]
                PUT
                dec     cl
                jnz     gc_bc
                jmp     gc_next
gc_bjr:         CELLV                   ; PCjr big: two words a nibble (jr_w)
                mov     ch, al
                mov     bl, al
                and     bl, 0F0h
                shr     bl, 1
                shr     bl, 1
                xor     bh, bh
                mov     ax, _jr_w[bx]
                PUT
                mov     ax, _jr_w[bx+2]
                PUT
                mov     bl, ch
                and     bl, 0Fh
                shl     bl, 1
                shl     bl, 1
                mov     ax, _jr_w[bx]
                PUT
                mov     ax, _jr_w[bx+2]
                PUT
                dec     cl
                jnz     gc_bjr
gc_next:        inc     rr
                jmp     gc_row
gx_cells_       ENDP

; void gx_rect(void): see gfx.c.
                PUBLIC  gx_rect_
gx_rect_        PROC    NEAR
                mov     es, word ptr _vram_p+2
                mov     rr, 0
gr_row:         mov     al, rr
                cmp     al, _gx_rows
                jb      gr_go
                ret
gr_go:          add     al, _gx_y
                mov     dl, _gx_fg0
                test    al, 1
                jz      gr_pt
                mov     dl, _gx_fg1
gr_pt:          xor     ah, ah
                mov     bx, ax
                shl     bx, 1
                mov     di, _rowoff[bx]
                add     di, _gx_off
                mov     al, _gx_lm
                or      al, al
                jz      gr_mid
                call    gr_mask
gr_mid:         mov     cl, _gx_mid
                xor     ch, ch
                mov     al, dl
                mov     ah, dl
                shr     cx, 1
                rep     stosw
                adc     cx, cx
                rep     stosb
                mov     al, _gx_rm
                or      al, al
                jz      gr_next
                call    gr_mask
gr_next:        inc     rr
                jmp     gr_row
gx_rect_        ENDP

; void gx_hdots(void): see gfx.c.
                PUBLIC  gx_hdots_
gx_hdots_       PROC    NEAR
                mov     es, word ptr _vram_p+2
                mov     bl, _gx_y
                xor     bh, bh
                shl     bx, 1
                mov     di, _rowoff[bx]
                add     di, _gx_off
                mov     cl, _gx_n
                xor     ch, ch
                jcxz    gh_ret
                mov     al, _gx_fg0
                cmp     _plat_jr, 0
                je      gh_cga
gh_jr:          stosb
                inc     di
                loop    gh_jr
gh_ret:         ret
gh_cga:         mov     ah, _gx_lm
                and     al, ah
                not     ah
gh_c:           mov     dl, es:[di]
                and     dl, ah
                or      dl, al
                mov     es:[di], dl
                inc     di
                loop    gh_c
                ret
gx_hdots_       ENDP

; ES:DI's byte through mask AL in pattern DL; DI on.
gr_mask         PROC    NEAR
                mov     ah, es:[di]
                mov     bl, ah
                xor     ah, dl
                and     ah, al
                xor     ah, bl
                mov     es:[di], ah
                inc     di
                ret
gr_mask         ENDP

; gx_plot: DX = DX ^ ((DX ^ val) & mask) for the clocks in nibble nib,
; mask = cga_w (SEL2) or a word of jr_w (SEL4, ofs 0 or 2).
SEL2            MACRO   nib, val
                LOCAL   skip
                or      nib, nib
                jz      skip
                mov     bl, nib
                xor     bh, bh
                shl     bx, 1
                mov     bx, _cga_w[bx]
                mov     si, dx
                xor     si, val
                and     si, bx
                xor     dx, si
skip:
                ENDM

SEL4            MACRO   nib, ofs, val
                LOCAL   skip
                or      nib, nib
                jz      skip
                mov     bl, nib
                xor     bh, bh
                shl     bx, 1
                shl     bx, 1
                mov     bx, _jr_w[bx+ofs]
                mov     si, dx
                xor     si, val
                and     si, bx
                xor     dx, si
skip:
                ENDM

; void gx_plot(void): see gfx.c.
                PUBLIC  gx_plot_
gx_plot_        PROC    NEAR
                push    bp
                mov     es, word ptr _vram_p+2
                mov     rr, 0
                mov     pidx, 0
gp_row:         mov     al, rr
                cmp     al, 32
                jb      gp_go
                pop     bp
                ret
gp_go:          add     al, _pl_y0
                mov     dl, _pl_p0
                test    al, 1
                jz      gp_pt
                mov     dl, _pl_p1
gp_pt:          mov     dh, dl
                mov     pw, dx
                xor     ah, ah
                mov     bx, ax
                shl     bx, 1
                mov     di, _rowoff[bx]
                add     di, _pl_x
                call    gp_byte
                call    gp_byte
                inc     rr
                jmp     gp_row
gx_plot_        ENDP

; The layers' byte pidx (8 clocks) at ES:DI; pidx and DI on.
gp_byte         PROC    NEAR
                mov     bx, pidx
                inc     pidx
                mov     si, _pl_o1
                mov     al, [bx+si]
                mov     si, _pl_o2
                or      al, [bx+si]
                mov     si, _pl_o3
                or      al, [bx+si]
                mov     si, _pl_rv
                mov     ah, [bx+si]
                mov     si, _pl_wh
                mov     ch, [bx+si]
                mov     cl, al
                or      cl, ah
                or      cl, ch
                jnz     gp_mix
                mov     ax, _pl_bg      ; plains only
                stosw
                stosw
                cmp     _plat_jr, 0
                je      gp_ret
                stosw
                stosw
gp_ret:         ret
gp_mix:         mov     vo, ax
                mov     vw, ch
                mov     cl, 4
                shr     al, cl
                shr     ah, cl
                shr     ch, cl
                call    gp_nib          ; the left four clocks
                mov     ax, vo
                and     ax, 0F0Fh
                mov     ch, vw
                and     ch, 0Fh         ; then the right four
gp_nib:         mov     dx, _pl_bg      ; AL owner, AH river, CH white
                cmp     _plat_jr, 0
                jne     gp_njr
                SEL2    ah, _pl_r
                SEL2    al, pw
                SEL2    ch, _pl_w
                mov     es:[di], dx
                add     di, 2
                ret
gp_njr:         SEL4    ah, 0, _pl_r
                SEL4    al, 0, pw
                SEL4    ch, 0, _pl_w
                mov     es:[di], dx
                mov     dx, _pl_bg
                SEL4    ah, 2, _pl_r
                SEL4    al, 2, pw
                SEL4    ch, 2, _pl_w
                mov     es:[di+2], dx
                add     di, 4
                ret
gp_byte         ENDP

; void gx_town(void): see gfx.c. The picture is far: DS is its segment
; here, DGROUP is reached through SS.
                PUBLIC  gx_town_
gx_town_        PROC    NEAR
                push    bp
                push    ds
                mov     es, word ptr _vram_p+2
                mov     tline, 0
                mov     tx, 0
                call    tw_addr
                lds     si, _gx_tsrc
                ASSUME  DS:NOTHING
tw_tok:         cmp     ss:tline, 144
                jae     tw_done
                lodsb
                cmp     al, 128
                jae     tw_run
                xor     ah, ah          ; a literal: AL + 1 clocks
                inc     ax
                mov     dx, ax
                xor     bl, bl
                jmp     tw_emit
tw_run:         sub     al, 125         ; a run: AL - 125 clocks of the next
                xor     ah, ah
                mov     dx, ax
                lodsb
                mov     bh, al
                mov     bl, 1
tw_emit:        cmp     ss:_plat_jr, 0
                je      tw_cga
                mov     al, bh
tw_jr:          mov     cx, 96          ; the PCjr: as much as the line has room for
                sub     cl, ss:tx
                cmp     cx, dx
                jbe     tw_jr1
                mov     cx, dx
tw_jr1:         sub     dx, cx
                add     ss:tx, cl
                or      bl, bl
                jz      tw_jlit
                rep     stosb
                jmp     tw_jr2
tw_jlit:        rep     movsb
tw_jr2:         call    tw_eol
                or      dx, dx
                jnz     tw_jr
                jmp     tw_tok
tw_cga:         mov     al, bh          ; CGA: a clock at a time
                or      bl, bl
                jnz     tw_c1
                lodsb
tw_c1:          push    bx
                mov     bl, al
                and     bl, 0Fh
                xor     bh, bh
                mov     ah, ss:_cga2[bx]
                cmp     al, 88h         ; the corral's grey, inside its edge: black
                jne     tw_c2
                mov     al, ss:tx
                cmp     al, ss:_gx_cx0
                jbe     tw_c2
                cmp     al, ss:_gx_cx1
                jae     tw_c2
                mov     al, ss:tline
                cmp     al, ss:_gx_cy0
                jbe     tw_c2
                cmp     al, ss:_gx_cy1
                jae     tw_c2
                xor     ah, ah
tw_c2:          pop     bx
                test    ss:tx, 1
                jnz     tw_c3
                mov     cl, 4
                shl     ah, cl
                mov     ss:hold, ah
                jmp     tw_c4
tw_c3:          or      ah, ss:hold
                mov     es:[di], ah
                inc     di
tw_c4:          inc     ss:tx
                call    tw_eol
                dec     dx
                jnz     tw_cga
                jmp     tw_tok
tw_done:        pop     ds
                ASSUME  DS:DGROUP
                pop     bp
                ret
gx_town_        ENDP

; At the end of a line (tx 96): the next. Keeps AX, BX, DX.
tw_eol          PROC    NEAR
                cmp     ss:tx, 96
                jb      te_ret
                mov     ss:tx, 0
                inc     ss:tline
                cmp     ss:tline, 144
                jae     te_ret
                call    tw_addr
te_ret:         ret
tw_eol          ENDP

; DI = line tline's first clock of the town. Keeps AX, BX, DX.
tw_addr         PROC    NEAR
                push    bx
                mov     bl, ss:tline
                add     bl, 16
                xor     bh, bh
                shl     bx, 1
                mov     di, ss:_rowoff[bx]
                add     di, 16
                cmp     ss:_plat_jr, 0
                je      ta_ret
                add     di, 16
ta_ret:         pop     bx
                ret
tw_addr         ENDP

gfx_TEXT        ENDS
                END
