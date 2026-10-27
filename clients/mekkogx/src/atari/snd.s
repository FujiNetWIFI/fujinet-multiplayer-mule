;
; snd.s -- the effects' clock, run by the immediate VBI (gfxa.s; the Lynx's
; VBL, irq.s): sound.c sets a tone going, and each frame this plays it,
; then the second tone if there is one, then quiet. Voice 1 only: SIO and
; the network stream (nstream.s) clock the serial port with voices 3 and 4.
;
; The Lynx assembles this too (src/lynx/snd.s): there voice 1 goes into a
; POKEY shadow, which mikey.s plays on Mikey. So does the 7800
; (src/atari7800/snd.s), on its cart's POKEY, from the frame's DLI, and
; the 5200 (src/atari5200/snd.s), from the VBI as here.
;
        .export _snd_vbi, _snd_f, _snd_fq, _snd_c, _snd_nf, _snd_nfq
.ifdef __LYNX__
        .export _pk_out
        .import _mikey_out
        .define LOWBSS "BSS"
.elseif .defined(__ATARI7800__)
        .define LOWBSS "BSS"
.else
        .define LOWBSS "BSS2"
.endif

.ifdef __ATARI7800__
AUDF1   = $0450                 ; the FujiNet 7800 cart's POKEY
AUDC1   = $0451
.elseif .defined(__ATARI5200__)
AUDF1   = $E800                 ; the 5200's POKEY
AUDC1   = $E801
.else
AUDF1   = $D200
AUDC1   = $D201
.endif

        .segment LOWBSS         ; low memory, cleared by sprite.s's clrlow
_snd_f:   .res 1        ; frames of this tone left (0: none)
_snd_fq:  .res 1        ; its AUDF
_snd_c:   .res 1        ; and AUDC (a pure tone and its volume)
_snd_nf:  .res 1        ; then so many frames (0: none)
_snd_nfq: .res 1        ; of this AUDF
.ifdef __LYNX__
_pk_out: .res 9         ; what POKEY would be given (mikey.s)
.endif

        .code

_snd_vbi:
        lda     _snd_f
        bne     @on
        lda     _snd_nf         ; the tone is over: the second, if any
        beq     @off
        sta     _snd_f
        lda     #0
        sta     _snd_nf
        lda     _snd_nfq
        sta     _snd_fq
@on:    dec     _snd_f
        lda     _snd_fq
        ldx     _snd_c
        bne     @w              ; (never 0 while a tone plays)
@off:   lda     #0
        tax
.ifdef __LYNX__
@w:     sta     _pk_out
        stx     _pk_out+1
        jmp     _mikey_out
.else
@w:     sta     AUDF1
        stx     AUDC1
        rts
.endif
