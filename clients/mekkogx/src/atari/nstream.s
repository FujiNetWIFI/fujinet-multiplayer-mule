;
; nstream.s -- FujiNet netstream on the Atari: the serial port as a raw link
; to the server's socket (stream mode, docs/PROTOCOL.md; nsnet.c above it).
;
; The serial engine is cut down from the FujiNet netstream handler
; (github.com/mozzwald/fujinet-atari-netstream, handler/mads), itself cut
; down from Altirra's replacement 850 firmware:
;
;   Altirra - Atari 800/800XL/5200 emulator
;   Replacement 850 Interface Firmware - R: Device Handler
;   Copyright (C) 2008-2017 Avery Lee
;
;   Copying and distribution of this file, with or without modification,
;   are permitted in any medium without royalty provided the copyright
;   notice and this notice are preserved.  This file is offered as-is,
;   without any warranty.
;
; While the stream is on, the motor line is asserted and POKEY's channels 3
; and 4 clock the serial port (so the sound keeps to voices 1 and 2,
; pokey.s). ns_end drops the motor: FujiNet goes back to plain SIO (the
; disk, appkeys) and holds the socket's data until ns_begin.
;
; Receiving is the serial input IRQ into a 256-byte ring; ns_frame takes
; bytes out a frame at a time (COBS, 0-terminated), decodes it in place and
; checks its Fletcher-16. Sending goes through a 32-byte ring and the
; output-ready IRQ.
;
        .export _ns_begin, _ns_end, _ns_reset, _ns_put, _ns_frame
        .export _ns_on, _ns_audf3, _ns_skctl, _ns_err, _ns_taken, _ns_fbuf
        .importzp ptr1

VSERIN  = $020A         ; VSERIN, VSEROR, VSEROC
VIMIRQ  = $0216
POKMSK  = $10
SSKCTL  = $0232
PACTL   = $D302
AUDF1   = $D200
AUDF3   = $D204
AUDF4   = $D206
AUDCTL  = $D208
SKRES   = $D20A
SEROUT  = $D20D
SERIN   = $D20D
IRQEN   = $D20E
IRQST   = $D20E
SKCTL   = $D20F
SKSTAT  = $D20F

TXSIZE  = 32
FMAX    = 136           ; a frame, encoded (STREAM_MAX_FRAME 128 + COBS)

        .segment "PMGFREE"      ; cleared by gfx_init
_ns_fbuf: .res FMAX             ; the frame coming in, then decoded

        .segment "BSS2"         ; cleared at start-up
_ns_on:   .res 1        ; 1 while the stream is on
_ns_audf3: .res 1       ; the baud rate (AUDF3; AUDF4 is 0)
_ns_skctl: .res 1       ; SKCTL's clock bits
_ns_err:  .res 1        ; sticky: $80 framing, $40 overrun, $10 ring full
_ns_taken: .res 1       ; bytes taken from the ring, mod 256 (SC_CREDIT)
rxhead: .res 1          ; the IRQ writes here
rxtail: .res 1          ; ns_frame reads here
txhead: .res 1
txtail: .res 1
txn:    .res 1          ; bytes in the send ring
txidle: .res 1          ; bit 7: the output is idle
flen:   .res 1          ; bytes in _ns_fbuf
fover:  .res 1          ; the frame overflowed: skip to its 0
vsave:  .res 6
txring: .res TXSIZE
s1:     .res 1
s2:     .res 1

        .bss
rxring: .res 256

        .code

; void ns_reset(void): empty the rings (a new connection)
_ns_reset:
        lda     #0
        sta     rxhead
        sta     rxtail
        sta     txhead
        sta     txtail
        sta     txn
        sta     flen
        sta     fover
        sta     _ns_taken
        sta     _ns_err
        rts

; void ns_begin(void): stream on -- POKEY, vectors, IRQs, motor
_ns_begin:
        lda     _ns_on
        bne     @x
        lda     #$80
        sta     txidle
        lda     txhead          ; what wasn't sent is dropped
        sta     txtail
        lda     #0
        sta     txn
        ldx     #8
@z:     sta     AUDF1,x         ; voices off, AUDCTL 0
        dex
        bpl     @z
        lda     _ns_audf3
        sta     AUDF3
        lda     #0
        sta     AUDF4
        lda     #$28            ; 1.79 MHz, 3+4 joined
        sta     AUDCTL
        sei
        lda     #1
        sta     _ns_on
        lda     #$34            ; motor on
        sta     PACTL
        ldx     #5
@v:     lda     VSERIN,x
        sta     vsave,x
        lda     vecs,x
        sta     VSERIN,x
        dex
        bpl     @v
        jsr     swapirq
        lda     SSKCTL
        and     #$0F
        ora     _ns_skctl
        sta     SSKCTL
        sta     SKCTL
        sta     SKRES
        lda     POKMSK
        ora     #$30            ; serial in, output ready
        sta     POKMSK
        sta     IRQEN
        cli
@x:     rts

; void ns_end(void): stream off, once the send ring is out
_ns_end:
        lda     _ns_on
        beq     @x
        ldx     #0
        ldy     #16
@w:     lda     txn             ; wait for the ring (bounded)
        bne     @d
        bit     txidle
        bmi     @off
@d:     dex
        bne     @w
        dey
        bne     @w
@off:   php
        sei
        lda     POKMSK
        and     #$C7
        sta     POKMSK
        sta     IRQEN
        ldx     #5
@r:     lda     vsave,x
        sta     VSERIN,x
        dex
        bpl     @r
        jsr     swapirq
        lda     #$3C            ; motor off
        sta     PACTL
        lda     #0
        sta     _ns_on
        plp
@x:     rts

; void __fastcall__ ns_put(uint8_t c): send a byte (waits while the ring is full)
_ns_put:
@full:  ldx     txn
        cpx     #TXSIZE
        beq     @full           ; the IRQ empties it
        php
        sei
        bit     txidle
        bpl     @q
        sta     SEROUT          ; idle: straight out
        lsr     txidle
        plp
        rts
@q:     ldx     txhead
        sta     txring,x
        inx
        txa
        and     #TXSIZE-1
        sta     txhead
        inc     txn
        plp
        rts

; uint8_t ns_frame(void): take bytes up to a frame's end. 0: no whole frame
; yet; 255: a bad one (dropped); else the frame is decoded in _ns_fbuf
; (type, payload) and this is its length, the sum left off.
_ns_frame:
@next:  ldx     rxtail
        cpx     rxhead
        bne     @have
        lda     #0
        rts
@have:  lda     rxring,x
        inx
        stx     rxtail
        inc     _ns_taken
        tay
        beq     @end
        ldx     flen
        cpx     #FMAX
        bcs     @ovf
        sta     _ns_fbuf,x
        inc     flen
        bne     @next
@ovf:   sta     fover
        bne     @next
@end:   ldx     flen
        sty     flen            ; 0: the next frame starts
        lda     fover
        sty     fover
        bne     @bad
        txa
        beq     @next           ; an empty frame: nothing
        ; COBS in place: code bytes become 0s, the data moves down one
        stx     ptr1            ; encoded length
        ldx     #0              ; read
        ldy     #0              ; write
@blk:   lda     _ns_fbuf,x      ; the code: 1 + bytes before the next 0
        sta     s1
        sta     ptr1+1
        inx
@cp:    dec     ptr1+1
        beq     @zero
        cpx     ptr1
        bcs     @bad
        lda     _ns_fbuf,x
        sta     _ns_fbuf,y
        inx
        iny
        bne     @cp
@zero:  cpx     ptr1
        bcs     @dec            ; the last block: no 0 after it
        lda     s1              ; nor after a full (255) block
        cmp     #$FF
        beq     @blk
        lda     #0
        sta     _ns_fbuf,y
        iny
        bne     @blk
@dec:   ; y = decoded length; the last two bytes are the sum
        cpy     #3
        bcc     @bad
        dey
        dey
        sty     ptr1
        lda     #0
        sta     s1
        sta     s2
        tax
@sum:   lda     _ns_fbuf,x      ; ones' complement sums: mod 255
        clc
        adc     s1
        adc     #0
        sta     s1
        clc
        adc     s2
        adc     #0
        sta     s2
        inx
        cpx     ptr1
        bne     @sum
        lda     s1
        jsr     norm
        cmp     _ns_fbuf,x
        bne     @bad
        lda     s2
        jsr     norm
        cmp     _ns_fbuf+1,x
        bne     @bad
        lda     ptr1
        rts
@bad:   lda     #255
        rts

norm:   cmp     #255            ; 255 is 0 mod 255
        bne     @n
        lda     #0
@n:     rts

; ---- the IRQs ------------------------------------------------------------
; The OS's IRQ handler pushed A and jumps here through VSERIN / VSEROR.

serin:  lda     SKSTAT          ; framing (bit 7) or overrun (6): note it
        eor     #$FF
        and     #$C0
        beq     @ok
        sta     SKRES
        ora     _ns_err
        sta     _ns_err
@ok:    txa
        pha
        ldx     rxhead
        lda     SERIN
        sta     rxring,x
        inx
        cpx     rxtail
        beq     @full           ; ring full: the byte is lost
        stx     rxhead
        pla
        tax
        pla
        rti
@full:  lda     #$10
        ora     _ns_err
        sta     _ns_err
        pla
        tax
        pla
        rti

serout: lda     txn
        beq     @idle
        dec     txn
        txa
        pha
        ldx     txtail
        lda     txring,x
        sta     SEROUT
        inx
        txa
        and     #TXSIZE-1
        sta     txtail
        pla
        tax
        pla
        rti
@idle:  lda     #$80
        sta     txidle
sercomp: pla
        rti

; The Break key's IRQ is acknowledged here; the rest go on to the OS.
irq:    bit     IRQST
        bpl     brkirq
        jmp     irq
irqnext = * - 2
brkirq: pha
        lda     #$7F
        sta     IRQEN
        lda     POKMSK
        sta     IRQEN
        pla
        rti

swapirq: ldx     #1
@s:     lda     VIMIRQ,x
        pha
        lda     irqnext,x
        sta     VIMIRQ,x
        pla
        sta     irqnext,x
        dex
        bpl     @s
        rts

        .rodata
vecs:   .word   serin, serout, sercomp
