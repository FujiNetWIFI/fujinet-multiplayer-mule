; fixed.inc -- the fixed half, $400-$7FF: the display engine.
;
; The 8044's 28 characters a frame are re-pointed between lines of text and
; rows of the map by a line-counter interrupt, and the 8048 holds A11 at 0
; for a whole interrupt routine -- so everything here runs below $800, and
; every table an interrupt reads with MOVP sits in the page of the code that
; reads it. The tables are cartridge RAM: the main program writes them through
; the mailbox's POKE register (disp.inc) and never touches the VDC's object
; registers itself.
;
; A screen is a band list (page $7): one counter interrupt per band, each
; re-pointing the characters for the next line. The vblank interrupt draws
; the top line (T0), the sprites, and whatever is dirty, then arms the first
; band. Two kinds of band:
;   h_row   a map row: quads A, B and single 10 from ROWBUF, then the next
;           row's table into ROWBUF -- internal RAM is the only memory fast
;           enough for the 10-line gap (fixed_rowbuf)
;   h_txt   a 16-character text line from a table: T0-T2 page $5, T3-T5
;           page $6 (where, on a map screen, the row tables sit instead);
;           T0 is always the first band, at line 1
;
; Assembled into every page's build; build.sh takes it from page 0's and
; checks the others agree.

CTL_ON  equ vdc_ctrl_grid|vdc_ctrl_fore
CTL_OFF equ vdc_ctrl_grid

; ISR_IN / ISR_FG / ISR_OUT: the frame round every band. Register bank 1;
; P1 saved in r6, A in r7.
ISR_IN  macro
    sel rb1
    mov r7,a
    endm
ISR_VDC macro
    in a,p1
    mov r6,a
    mov a,#P1_VDC
    outl p1,a
    endm
ISR_OFF macro                   ; foreground off: objects may be written
    mov r0,#vdc_control
    mov a,#CTL_OFF
    movx @r0,a
    endm
ISR_ON  macro                   ; foreground on again
    mov r0,#vdc_control
    mov a,#CTL_ON
    movx @r0,a
    endm
ISR_OUT macro
    mov a,r6
    outl p1,a
    mov a,r7
    retr
    endm

; ===========================================================================
;  page $4: vectors, boot, page switching, the map row writer
; ===========================================================================
    org 400h
    jmp start                   ; $400 reset
    jmp vbl_isr                 ; $402 external IRQ: the VDC's vblank
    jmp tim_isr                 ; $404 counter overflow: a band has ended
    jmp start                   ; $406 (the BIOS's own vblank chain: unused)
    jmp start                   ; $408 (after the BIOS's game select: unused)
    retr                        ; $40A
    nop

; start: a console RESET does not reset the cartridge, so the page mapped and
; every table survive it; boot puts back what it needs.
start:                          ; (a reset leaves interrupts off, the counter
    mov a,#P1_MAIN              ; stopped)
    outl p1,a
    clr a
    call sel_page               ; page 0
    mov r0,#ROWBUF              ; internal RAM $20-$3C
    mov r2,#03Dh-ROWBUF
    clr a
st_clr:
    mov @r0,a
    inc r0
    djnz r2,st_clr
st_boot:
    jmp 800h                    ; page 0's entry 0 (A = 0): the boot

; sel_page: map page A at $800-$F1F. Main program, P1 = P1_MAIN.
; Clobbers A, r0, r1.
sel_page:
    mov r1,#CURPG
    mov @r1,a
    mov r0,#FN_W_REGSEL
    mov a,#FN_REG_CODEBANK
    movx @r0,a
    inc r0
    mov a,@r1
    movx @r0,a
    ret

; farcall: entry A of page r7, then the caller's page back. From a page:
;     mov r7,#PG_x / mov a,#E_y / call farcall / sel mb1
; One level: an entry it runs may not farcall. Results come back in r2-r5 or
; external RAM (A and r0, r1, r6 are spent getting home). Clobbers r6.
farcall:
    mov r6,a
    mov r1,#CURPG
    mov a,@r1
    mov r1,#OLDPG
    mov @r1,a
    mov a,r7
    call sel_page
    mov a,r6
    call 800h                   ; the page's dispatcher (page.inc)
    sel mb0
    mov r1,#OLDPG
    mov a,@r1
    call sel_page
    ret

; vdc_reset: display off and every object parked. (The grid and the colour
; come from their tables in the first vblank.) Main program, at boot: FCALL.
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
    mov a,#P1_MAIN
    outl p1,a
    ret

; row_write: one map row out of ROWBUF -- Y, then quad A's four (pointer,
; control), quad B's, single 10's. VDC selected, foreground off. About 145
; cycles, which is why the bytes come from internal RAM: MOVP from a table
; costs two more a byte and misses the gap. Clobbers A, r0, r1, r2.
QCHARS  macro                   ; four (pointer, control): r1 -> r0+2..
    rept 4
    mov a,@r1
    movx @r0,a
    inc r0
    inc r1
    mov a,@r1
    movx @r0,a
    inc r1
    inc r0
    inc r0
    inc r0
    endm
    endm

row_write:
    mov r1,#ROWBUF
    mov a,@r1
    mov r2,a                    ; Y
    inc r1
    mov r0,#vdc_quad0           ; quad A: cells 0-3
    mov a,r2
    movx @r0,a                  ; Y and X are mirrored across a quad
    inc r0
    mov a,#X_QA
    movx @r0,a
    inc r0
    QCHARS
    mov r0,#vdc_quad1           ; quad B: cells 5-8
    mov a,r2
    movx @r0,a
    inc r0
    mov a,#X_QB
    movx @r0,a
    inc r0
    QCHARS
    mov r0,#vdc_chara           ; single 10: the river column
    mov a,r2
    movx @r0,a
    inc r0
    mov a,#X_COL4
    movx @r0,a
    inc r0
    mov a,@r1
    movx @r0,a
    inc r0
    inc r1
    mov a,@r1
    movx @r0,a
    ret

; static_write: singles 0-9 -- the mountains' right halves on a map, parked
; elsewhere -- from STATTAB, (Y, X, pointer, control) each. VDC selected,
; foreground off. Clobbers A, r0-r2.
static_write:
    mov r1,#STATTAB&0FFh
    mov r0,#vdc_char0
    mov r2,#40
stw_l:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r2,stw_l
    ret

STATTAB:                        ; 10 x (Y, X, pointer, control)
    db 40 dup (0)
    if $ > 500h
    error "page $4's tables have left the page"
    endif

; ===========================================================================
;  page $5: text writer A and lines T0-T2; the grid
; ===========================================================================
    org 500h

; TXTWR: a 16-character line from the table at r1 in this page, written
; from a band (h_txta / h_txtb jump here): it leaves through the band's own
; epilogue, so the interrupt never nests deeper than one call. VDC selected,
; foreground off. About 360 cycles. Clobbers A, r0-r4.
TXTWR   macro tw,tq,tql,{GLOBALSYMBOLS}
tw:
    mov a,r1
    movp a,@a
    mov r4,a                    ; Y
    inc r1
    mov r0,#vdc_quad0
    mov r3,#X_TEXT
    call tq
    mov r0,#vdc_quad1
    mov r3,#X_TEXT+8
    call tq
    mov r0,#vdc_quad2
    mov r3,#X_TEXT+64
    call tq
    mov r0,#vdc_quad3
    mov r3,#X_TEXT+72
    call tq
    jmp band_out
tq:
    mov a,r4                    ; Y and X are mirrored across a quad
    movx @r0,a
    inc r0
    mov a,r3
    movx @r0,a
    inc r0
    mov r2,#4
tql:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r1
    inc r0
    inc r0
    inc r0
    djnz r2,tql
    ret
    endm

    TXTWR txta,txta_q,txta_l

; grid_write: the 28 grid registers from GRIDTAB. VDC selected, foreground
; off. The grid has to be off while it is written. Clobbers A, r0-r2.
grid_write:
    mov r0,#vdc_control
    clr a
    movx @r0,a
    mov r1,#GRIDTAB&0FFh
    mov r0,#vdc_gridh0          ; $C0-$C8, then $D0-$D8, then $E0-$E9
    mov r2,#28
gw_l:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r0
    xrl a,#vdc_gridh8+1
    jnz gw_1
    mov r0,#vdc_gridi0
gw_1:
    mov a,r0
    xrl a,#vdc_gridi8+1
    jnz gw_2
    mov r0,#vdc_gridv0
gw_2:
    djnz r2,gw_l
    mov r0,#vdc_control
    mov a,#CTL_OFF
    movx @r0,a
    ret

; fx5_get: A = offset in page $5 -> the byte there (main program: FCALL).
fx5_get:
    movp a,@a
    ret

T0:                             ; text lines: TXT_TAB bytes each
    db TXT_TAB dup (0)
T1: db TXT_TAB dup (0)
T2: db TXT_TAB dup (0)
GRIDTAB:                        ; $C0-$C8, $D0-$D8, $E0-$E9
    db 28 dup (0)
    if $ > 600h
    error "page $5's tables have left the page"
    endif

; ===========================================================================
;  page $6: text writer B and T3-T5, or the map's rows; the sprites
; ===========================================================================
    org 600h

    TXTWR txtb,txtb_q,txtb_l

; row_copy: the row table at offset A of this page into ROWBUF. Main or
; interrupt. Clobbers A, r0-r2.
row_copy:
    mov r1,a
    mov r0,#ROWBUF
    mov r2,#ROW_TAB
rc_l:
    mov a,r1
    movp a,@a
    mov @r0,a
    inc r0
    inc r1
    djnz r2,rc_l
    ret

; spr_write: the four sprites' (Y, X, attribute) from SPRTAB. Their shapes
; the main program writes itself: the VDC takes those with the display on.
; VDC selected, foreground off. Clobbers A, r0-r2.
spr_write:
    mov r1,#SPRTAB&0FFh
    mov r0,#vdc_spr0_ctrl
    mov r2,#4
sw_l:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r0
    inc r1
    djnz r2,sw_l
    ret

; fx6_get: A = offset in page $6 -> the byte there (main program: FCALL).
fx6_get:
    movp a,@a
    ret

T3:                             ; text lines 3-5 ...
    db TXT_TAB dup (0)
T4: db TXT_TAB dup (0)
T5: db TXT_TAB dup (0)
ROWS    equ T3                  ; ... or, on a map screen, the five rows
SPRTAB:                         ; 4 x (Y, X, attribute)
    db 12 dup (0)
; beep: sound A (SND_*): a short blip from the VDC's shift register, its
; pattern, length and control from sndtab; the vblank stops it (SNDCNT).
; Main program: FCALL. Clobbers A, r0, r1.
beep:
    mov r1,a                    ; 3 bytes a sound
    rl a
    add a,r1
    add a,#sndtab&0FFh
    mov r1,a
    mov a,#P1_VDC
    outl p1,a
    mov r0,#vdc_sound0
    mov a,r1
    movp a,@a                   ; the pattern, thrice: 24 bits
    movx @r0,a
    inc r0
    movx @r0,a
    inc r0
    movx @r0,a
    inc r1
    mov a,r1
    movp a,@a
    mov r0,#SNDCNT
    mov @r0,a                   ; its frames
    inc r1
    mov a,r1
    movp a,@a
    mov r0,#vdc_soundctrl
    movx @r0,a                  ; on: bit 7; 6 high; 4 noise; 3-0 volume
    mov a,#P1_MAIN
    outl p1,a
    ret
sndtab:                         ; pattern, frames, control
    db 0AAh, 5, 0CFh            ; SND_SELECT: a high blip
    db 0AAh, 14, 08Fh           ; SND_ALERT: a lower, longer one
    db 0F0h, 12, 09Dh           ; SND_ERROR: a buzz
    db 0CCh, 3, 0CBh            ; SND_TRADE: a click

    if $ > 700h
    error "page $6's tables have left the page"
    endif

; ===========================================================================
;  page $7: the bands and both interrupts; the mountains' right halves
; ===========================================================================
    org 700h

; The band list, poked per screen (disp.inc). BL_FIRST is 256 minus the line
; of the first band; each band is (handler slot, argument, 256 - lines to the
; next band, or 0 for the last).
BL_FIRST equ 701h
BANDS   equ 702h
NBANDS  equ 9
SCRCOL  equ 700h                ; the colour register for this screen
    db 0, 0
    db NBANDS*3 dup (0)

; vbl_isr: once a frame, in the blank. Arms the first band -- exact anywhere
; in the blank, because the counter only counts display lines (T1 is
; vblank|hblank) -- then draws the sprites and whatever is dirty, and on a
; map screen stages row 0. T0 is the first band's, at line 1. The interrupt
; nests one call deep, so the main program may nest six.
vbl_isr:
    ISR_IN
    ISR_VDC
    mov r0,#vdc_status
    movx a,@r0                  ; reading the status drops the IRQ
    mov a,#BL_FIRST&0FFh
    movp a,@a
    mov t,a
    mov r5,#BANDS&0FFh
    mov r1,#SNDCNT              ; a sound's last frame: silence
    mov a,@r1
    jz vb_q
    dec a
    mov @r1,a
    jnz vb_q
    mov r0,#vdc_soundctrl
    movx @r0,a
vb_q:
    mov r1,#FRAMES
    inc @r1
    ISR_OFF
    mov a,#SCRCOL&0FFh
    movp a,@a
    mov r0,#vdc_color
    movx @r0,a
    mov r1,#DIRTY
    mov a,@r1
    jb0 vb_static
vb_s1:
    mov r1,#DIRTY
    mov a,@r1
    jb1 vb_grid
vb_g1:
    call spr_write
    mov r1,#MODE
    mov a,@r1
    jb0 vb_rows
vb_r1:
    ISR_ON
    ISR_OUT
vb_static:
    anl a,#~DIRTY_STATIC
    mov @r1,a
    call static_write
    mov r0,#vdc_char0+40        ; single 10 too: a map's river column, which
    mov a,#0F8h                 ; the text screen would keep (a map's rows
    movx @r0,a                  ; put it back each row)
    jmp vb_g1                   ; the grid, if it waits, next frame: both
                                ; would run far into the display
vb_grid:
    anl a,#~DIRTY_GRID
    mov @r1,a
    call grid_write
    jmp vb_g1
vb_rows:
    mov a,#ROWS&0FFh
    call row_copy
    jmp vb_r1

; tim_isr: a band has ended. The counter is re-armed first, plus what it
; has counted since the overflow: a band that starts late (a long vblank)
; doesn't make the next one late. Then the handler runs.
tim_isr:
    ISR_IN
    mov a,r5
    add a,#2
    movp a,@a
    mov r3,a
    mov a,t
    add a,r3
    mov t,a
    mov a,r5
    inc a
    movp a,@a
    mov r3,a                    ; the argument
    mov a,r5
    add a,#3
    xch a,r5                    ; the next band
    movp a,@a
    jmpp @a                     ; the handler slot -> the handler
jtab:
J_ROW   equ $&0FFh
    db h_row&0FFh
J_TXTA  equ $&0FFh
    db h_txta&0FFh
J_TXTB  equ $&0FFh
    db h_txtb&0FFh
J_NONE  equ $&0FFh
    db h_none&0FFh

; h_none: a band past the end of the list.
h_none:
    clr a
    mov t,a
    mov a,r7
    retr

; h_row: write the row staged in ROWBUF, then stage the one at offset r3 of
; page $6 (0FFh: none).
h_row:
    ISR_VDC
    ISR_OFF
    call row_write
    ISR_ON
    mov a,r3
    inc a
    jz hr_done
    dec a
    call row_copy
hr_done:
    ISR_OUT

; h_txta / h_txtb: the line whose table is at offset r3 of page $5 / $6.
h_txta:
    ISR_VDC
    ISR_OFF
    mov a,r3
    mov r1,a
    jmp txta
h_txtb:
    ISR_VDC
    ISR_OFF
    mov a,r3
    mov r1,a
    jmp txtb
band_out:                       ; where the text writers leave
    ISR_ON
    ISR_OUT

; fx7_get: A = offset in page $7 -> the byte there (main program: FCALL).
fx7_get:
    movp a,@a
    ret

; The rest of page $7 is the main program's: the server, when the lobby's
; room appkey names one (NUL-terminated; empty: the build's).
ENDPT:
    db 38 dup (0)
    if $ > 800h
    error "page $7's tables have left the page"
    endif
