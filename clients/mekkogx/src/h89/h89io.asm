; h89io.asm -- the H89's console without waiting, and its frame clock.
;
; The terminal (the H19 inside the H89) is on the CPU board's 8250 at
; E8h, at 9600 baud: a byte every millisecond. The BIOS's CONOUT waits for
; each, which would take most of a frame. Here the screen's bytes go into a
; ring (tx_ring) and leave from three places, never waited on:
;   - con_pump, which the main loop calls whenever it waits (frame_wait)
;     and in long loops (frame_tick);
;   - the BIOS's 2 ms clock interrupt (RST 1, the jump at 0008h), which
;     we go through first: two bytes a tick at most, so the screen keeps
;     moving while fujinet-lib or a FujiBus call holds the CPU.
; The emulator's 8250 raises no interrupt for an empty transmitter, so the
; transmitter is never interrupt driven.
;
; The keyboard: the 8250's receive interrupt (RST 3, the jump at 0018h) is
; ours while the game runs. Each byte goes into rx_ring, except XOFF and
; XON (13h, 11h): the terminal sends those when its own input queue fills
; (ESC E and the like take it a while), and they stop and start the ring
; at once. The BIOS's handler is out of the way, and with it its habit of
; ringing the bell into the screen's bytes when its buffer fills.
;
; The frame clock: the clock interrupt counts 60ths of a second (3/25 of
; a frame a tick) in frame_count, like the other platforms' interrupts.
;
; Both vectors are read when we start (the BIOS puts them there at cold
; boot; where it is depends on the system's size) and put back on the way
; out. A BIOS without JPs there isn't one we know: con_install says no.

        SECTION code_user

        PUBLIC  _con_install, _con_remove, _con_put, _con_free, _con_pump
        PUBLIC  _con_flushed, _kbd_get, _ticks_now, _cpm_print, _cpm_exit
        PUBLIC  _frame_count, _tx_count, _tx_held

        defc    CON_DATA = $E8          ; RBR / THR
        defc    CON_IER  = $E9
        defc    CON_LSR  = $ED          ; bit 0 a byte in, bit 5 THR empty
        defc    VEC_CLK  = $0008
        defc    VEC_CON  = $0018
        defc    TICKS    = $000B        ; the BIOS's 2 ms count
        defc    TXN      = 64           ; ring sizes (powers of two)
        defc    RXN      = 32
        defc    HOLD_MAX = 125          ; ticks an XOFF holds without an XON

; uint8_t con_install(void): 1 when the vectors are ours
_con_install:
        ld      a,(VEC_CLK)
        cp      $C3
        jr      nz,ci_no
        ld      a,(VEC_CON)
        cp      $C3
        jr      nz,ci_no
        di
        ld      hl,(VEC_CLK+1)
        ld      (clk_old+1),hl
        ld      hl,(VEC_CON+1)
        ld      (con_old),hl
        ld      hl,clk_isr
        ld      (VEC_CLK+1),hl
        ld      hl,rx_isr
        ld      (VEC_CON+1),hl
        in      a,(CON_IER)
        ld      (ier_old),a
        ld      a,1                     ; a byte in: the only interrupt
        out     (CON_IER),a
        ld      (installed),a
        ei
        ld      l,1
        ret
ci_no:
        ld      l,0
        ret

; void con_remove(void): the ring emptied, the BIOS's vectors back
_con_remove:
        ld      a,(installed)
        or      a
        ret     z
cr_drain:
        call    _con_pump
        ld      a,(tx_tail)
        ld      hl,tx_head
        cp      (hl)
        jr      nz,cr_drain
        di
        ld      hl,(clk_old+1)
        ld      (VEC_CLK+1),hl
        ld      hl,(con_old)
        ld      (VEC_CON+1),hl
        ld      a,(ier_old)
        out     (CON_IER),a
        xor     a
        ld      (installed),a
        ei
        ret

; The clock's interrupt, before the BIOS's (which acknowledges it, counts
; TICKS and ends in EI; RET): no EI here.
clk_isr:
        push    af
        push    hl
IF H89_PROF
        ; (a profiling build: the interrupted PC into a ring, for the harness)
        push    de
        ld      hl,6
        add     hl,sp
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      a,(_h89_prof_at)
        ld      l,a
        ld      h,0
        add     hl,hl
        push    bc
        ld      bc,_h89_prof
        add     hl,bc
        pop     bc
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ld      hl,_h89_prof_at
        inc     (hl)
        pop     de
ENDIF
        ld      a,(fr_acc)
        add     a,3
        cp      25
        jr      c,ck_nf
        sub     25
        ld      hl,(_frame_count)
        inc     hl
        ld      (_frame_count),hl
ck_nf:
        ld      (fr_acc),a
        call    tx_send
        call    tx_send
        pop     hl
        pop     af
clk_old:
        jp      0                       ; (the BIOS's handler, set above)

; One byte from the ring to the 8250, if it has room and we aren't held.
; Interrupts off. Keeps everything but AF and HL.
tx_send:
        ld      a,(tx_hold)
        or      a
        ret     nz
        ld      a,(tx_tail)
        ld      hl,tx_head
        cp      (hl)
        ret     z                       ; empty
        in      a,(CON_LSR)
        and     $20
        ret     z                       ; the 8250 is busy
        push    de
        ld      a,(tx_tail)
        ld      e,a
        ld      d,0
        ld      hl,tx_ring
        add     hl,de
        ld      a,(hl)
        out     (CON_DATA),a
        ld      a,e
        inc     a
        and     TXN-1
        ld      (tx_tail),a
        ld      hl,(_tx_count)
        inc     hl
        ld      (_tx_count),hl
        pop     de
        ret

; The 8250's interrupt: the bytes typed. Replaces the BIOS's while we run.
rx_isr:
        push    af
        push    de
        push    hl
rx_more:
        in      a,(CON_LSR)
        rrca
        jr      nc,rx_done
        in      a,(CON_DATA)
        and     $7F
        cp      $13
        jr      z,rx_xoff
        cp      $11
        jr      z,rx_xon
        ld      d,a
        ld      a,(rx_head)
        ld      e,a
        inc     a
        and     RXN-1
        ld      hl,rx_tail
        cp      (hl)
        jr      z,rx_more               ; full: dropped
        ld      (rx_head),a
        ld      a,d
        ld      d,0
        ld      hl,rx_ring
        add     hl,de
        ld      (hl),a
        jr      rx_more
rx_xoff:
        ld      a,1
        ld      (tx_hold),a
        ld      (_tx_held),a
        ld      hl,(TICKS)
        ld      (xoff_at),hl
        jr      rx_more
rx_xon:
        xor     a
        ld      (tx_hold),a
        jr      rx_more
rx_done:
        pop     hl
        pop     de
        pop     af
        ei
        ret

; void con_pump(void): what the 8250 can take now (two bytes at most)
_con_pump:
        ld      a,(tx_hold)
        or      a
        jr      z,cp_go
        ld      hl,(TICKS)              ; held: an XON may have been lost
        ld      de,(xoff_at)
        or      a
        sbc     hl,de
        ld      de,HOLD_MAX
        sbc     hl,de
        ret     c
        xor     a
        ld      (tx_hold),a
cp_go:
        di
        call    tx_send
        call    tx_send
        ei
        ret

; void con_put(uint8_t c) __z88dk_fastcall: into the ring (pumping while
; it is full)
_con_put:
        ld      a,(tx_head)
        ld      e,a
        inc     a
        and     TXN-1
        ld      d,a
cput_wait:
        ld      a,(tx_tail)
        cp      d
        jr      nz,cput_room
        push    de
        push    hl
        call    _con_pump
        pop     hl
        pop     de
        jr      cput_wait
cput_room:
        ld      a,l
        ld      l,e
        ld      h,0
        ld      bc,tx_ring
        add     hl,bc
        ld      (hl),a
        ld      a,d
        ld      (tx_head),a
        ret

; uint8_t con_free(void): room in the ring
_con_free:
        ld      a,(tx_head)
        ld      b,a
        ld      a,(tx_tail)
        sub     b
        dec     a
        and     TXN-1
        ld      l,a
        ret

; uint8_t con_flushed(void): 1 when the ring is empty
_con_flushed:
        ld      a,(tx_tail)
        ld      hl,tx_head
        sub     (hl)
        ld      l,1
        ret     z
        ld      l,0
        ret

; int kbd_get(void): the next byte typed, or -1
_kbd_get:
        ld      a,(rx_tail)
        ld      hl,rx_head
        cp      (hl)
        jr      z,kg_none
        ld      e,a
        ld      d,0
        ld      hl,rx_ring
        add     hl,de
        ld      l,(hl)
        ld      h,0
        inc     a
        and     RXN-1
        ld      (rx_tail),a
        ret
kg_none:
        ld      hl,-1
        ret

; uint16_t ticks_now(void): the BIOS's 2 ms count, read whole
_ticks_now:
        ld      hl,(TICKS)
        ret

; void cpm_print(const char *s) __z88dk_fastcall: a '$'-ended string,
; through the BDOS (before the console is ours, or after)
_cpm_print:
        push    ix
        push    iy
        ex      de,hl
        ld      c,9
        call    5
        pop     iy
        pop     ix
        ret

; void cpm_exit(void): a warm boot (the CCP's memory is the screen's)
_cpm_exit:
        jp      0

        SECTION bss_user

IF H89_PROF
        PUBLIC  _h89_prof, _h89_prof_at
_h89_prof:      defs    512
_h89_prof_at:   defs    1
ENDIF
_frame_count:   defs    2
_tx_count:      defs    2               ; bytes sent (the harness reads it)
_tx_held:       defs    1               ; an XOFF has come (ditto)
fr_acc:         defs    1
installed:      defs    1
ier_old:        defs    1
con_old:        defs    2
tx_hold:        defs    1
xoff_at:        defs    2
tx_head:        defs    1
tx_tail:        defs    1
rx_head:        defs    1
rx_tail:        defs    1
tx_ring:        defs    TXN
rx_ring:        defs    RXN
