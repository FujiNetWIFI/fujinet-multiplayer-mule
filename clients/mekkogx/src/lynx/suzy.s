;
; suzy.s -- draw a chain of sprites (SCBs) with Suzy into a buffer.
;
; The CPU sleeps while Suzy draws and holds the bus: keep what is drawn in
; one go small (gfx.c splits large fills), or ComLynx bytes are lost.
;
        .pc02
        .export _suzy_draw, _suzy_dst

        .include "lynx.inc"

        .bss
_suzy_dst: .res 2               ; the buffer drawn into

        .code

; void __fastcall__ suzy_draw(const void *scb)
_suzy_draw:
        sta     SCBNEXTL
        stx     SCBNEXTH
        lda     _suzy_dst
        sta     VIDBASL
        lda     _suzy_dst+1
        sta     VIDBASH
        lda     #1
        sta     SPRGO
        stz     SDONEACK
@w:     stz     CPUSLEEP
        lda     SPRSYS
        lsr     a
        bcs     @w
        stz     SDONEACK
        rts
