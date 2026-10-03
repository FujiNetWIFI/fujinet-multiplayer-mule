; uartisr.asm -- the serial port's interrupt, for stream mode's packets
; (src/msdos/fbport.c, src/nstrans_fb.c).
;
; FUJINET.SYS polls the UART inside each INT F5h call, so the game stops
; until the FujiNet's reply is in. Instead the game puts a request in
; _fb_tring and goes on; this handler sends it a byte at a time as the
; transmitter empties and keeps what comes back in _fb_rring, which the
; game reads on the frames after. The receiving ring is 256 bytes with a
; byte index (it wraps by itself), the sending one 64 (the index masked);
; the game owns _fb_thead and _fb_rtail,
; this handler _fb_ttail and _fb_rhead. It is short: at 115200 bps a byte
; comes every 87 us, and an 8250 holds only one.

.8086
                NAME    uartisr

DGROUP          GROUP   _BSS

_BSS            SEGMENT WORD PUBLIC 'BSS'
                PUBLIC  _fb_base, _fb_rhead, _fb_rtail, _fb_thead, _fb_ttail
                PUBLIC  _fb_lsr, _fb_rring, _fb_tring
_fb_base         DW      ?               ; the UART (0: none)
_fb_rhead        DB      ?
_fb_rtail        DB      ?
_fb_thead        DB      ?
_fb_ttail        DB      ?
_fb_lsr          DB      ?               ; line errors seen (overrun: 2)
                DB      ?
_fb_rring        DB      256 DUP (?)
_fb_tring        DB      64 DUP (?)
_BSS            ENDS

uartisr_TEXT    SEGMENT BYTE PUBLIC 'CODE'
                ASSUME  CS:uartisr_TEXT, DS:DGROUP
                PUBLIC  _fb_isr

_fb_isr          PROC    FAR
                push    ax
                push    bx
                push    dx
                push    ds
                mov     ax, DGROUP
                mov     ds, ax
                xor     bh, bh
isr_next:       mov     dx, _fb_base
                add     dx, 2           ; IIR
                in      al, dx
                test    al, 1
                jnz     isr_done        ; nothing (more) pending
                and     al, 6
                cmp     al, 4
                je      isr_rx          ; data (or a 16550's timeout)
                cmp     al, 2
                je      isr_tx          ; the transmitter is empty
                cmp     al, 6
                je      isr_line
                add     dx, 4           ; modem status: MSR (base+6) clears it
                in      al, dx
                jmp     isr_next
isr_line:       add     dx, 3           ; LSR (base+5)
                in      al, dx
                or      _fb_lsr, al
                jmp     isr_next
isr_rx:         sub     dx, 2           ; RBR
                in      al, dx
                mov     bl, _fb_rhead
                mov     _fb_rring[bx], al
                inc     bl
                cmp     bl, _fb_rtail
                je      isr_full
                mov     _fb_rhead, bl
                jmp     isr_next
isr_full:       or      _fb_lsr, 2       ; (as an overrun: the byte is lost)
                jmp     isr_next
isr_tx:         mov     bl, _fb_ttail
                cmp     bl, _fb_thead
                je      isr_txoff
                mov     al, _fb_tring[bx]
                inc     bl
                and     bl, 3Fh
                mov     _fb_ttail, bl
                sub     dx, 2           ; THR
                out     dx, al
                jmp     isr_next
isr_txoff:      dec     dx              ; IER (base+1): the receiver only
                mov     al, 1
                out     dx, al
                jmp     isr_next
isr_done:       mov     al, 20h         ; EOI
                out     20h, al
                pop     ds
                pop     dx
                pop     bx
                pop     ax
                iret
_fb_isr          ENDP

uartisr_TEXT    ENDS
                END
