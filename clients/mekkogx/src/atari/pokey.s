;
; pokey.s -- the sound driver, run by the immediate VBI (gfxa.s): the
; original's sounds, played the way the original plays them.
;
; Each frame, in the original's order:
;   music   the 4-voice tune engine (title theme, the round's tune): per
;           voice, notes from a stream (bits 0-5 a pitch in a table, bit 6
;           a rest, bit 7 a new length follows), each with an attack held
;           for a while and a decay; voice 4 plays raw AUDF bytes
;   seq     a script for the longer effects (ship, colony events, jingles,
;           production, outfitting; tools/mksnd_atari.py writes them)
;   timer   the time bar's tick on voices 1 and 2
;   step    a footstep: voice 3 for one frame
;   v4      the original's voice-4 effects (the player blips, the message
;           beep): tables of pitch, length and volume
; into shadow registers, then POKEY. While SIO is busy (CRITIC), or the
; network stream is on (nstream.s), only voices 1 and 2 are written: the
; serial port is clocked with 3 and 4 (and AUDCTL).
;
; The Lynx assembles this too (src/lynx/pokey.s): there the shadow goes to
; mikey.s, which plays it on Mikey's four channels, and nothing holds
; voices back (ComLynx has its own clock).
;
        .export _snd_vbi, _snd_mute, _mus_desc, _mus_start, _mus_stop
        .export _seq_start, _seq_stop, _v4_play, _tm_set, _step_play
        .export _seq_ptr
        .import _v4_tab
.ifdef __LYNX__
        .export _pk_out
        .import _mikey_out, _snd_rand
        .define LOWBSS "BSS"
.macro  RND
        jsr     _snd_rand
.endmacro
.else
        .import _ns_on
        .define LOWBSS "BSS2"
.macro  RND
        lda     RANDOM
.endmacro
.endif

AUDF1   = $D200
AUDCTL  = $D208
SKCTL   = $D20F
RANDOM  = $D20A
CRITIC  = $42

; the tune descriptor (sound.c fills _mus_desc from gen/atari/snd.s)
D_V1S   = 0             ; voice 1 start, end
D_V1E   = 2
D_V2S   = 4             ; voices 2-4: start, where it wraps, where to
D_V2W   = 6
D_V2R   = 8
D_V3S   = 10
D_V3W   = 12
D_V3R   = 14
D_V4S   = 16
D_V4W   = 18
D_V4R   = 20
D_BASE  = 22            ; per voice: AUDC to start a note with
D_HOLD  = 26            ; frames before the decay
D_DECAY = 30            ; frames a volume step (0: cut)
D_DUR   = 34            ; the first notes' length
D_FT1   = 38            ; voice 1's pitch table
D_FT23  = 40            ; voices 2 and 3's
D_ALT   = 42            ; 1: after the first time through, start 2 bytes on
D_SIZE  = 43

        .segment "ZEROPAGE"
sp1:    .res 2          ; the VBI's own pointers
sp2:    .res 2

        .segment LOWBSS         ; low memory, cleared by sprite.s's clrlow
_snd_mute: .res 1
sh:     .res 9          ; AUDF1 AUDC1 ... AUDF4 AUDC4 AUDCTL
.ifdef __LYNX__
_pk_out: .res 9         ; what POKEY would be given (mikey.s)
.endif
_mus_desc: .res D_SIZE
mus_on: .res 1
vp:     .res 8          ; voice pointers
vact:   .res 4
vcur:   .res 4
vhold:  .res 4
vdec:   .res 4
vdurc:  .res 4
vdur:   .res 4
_seq_ptr: .res 2        ; 0: none
sq_wait: .res 1
sq_lp:  .res 4          ; two loops: where, count
sq_lc:  .res 2
sq_dep: .res 1
sq_mask: .res 2         ; registers it set (bit 8: AUDCTL)
tm_per: .res 1          ; timer: ticks every tm_per frames (0: off)
tm_cnt: .res 1
tm_ph:  .res 1
tm_tog: .res 1
tm_lease: .res 1
st_f:   .res 1          ; a footstep: AUDF3, AUDC3, frames left
st_c:   .res 1
st_n:   .res 1
v4_id:  .res 1          ; $80: none
v4_c8:  .res 1
v4_c9:  .res 1
v4_rep: .res 1
v4_gap: .res 1
vx:     .res 1
ev:     .res 1

        .rodata
wrapoff: .byte  D_V2W, D_V3W, D_V4W     ; voices 2-4's wrap points
; pow2 masks for sq_mask
bitlo:  .byte   1, 2, 4, 8, 16, 32, 64, 128, 0
bithi:  .byte   0, 0, 0, 0, 0, 0, 0, 0, 1

        .code

; ---- the C side's controls (sound.c) ------------------------------------
; void mus_start(void): play _mus_desc from its starts
_mus_start:
        lda     #0
        sta     mus_on
        jsr     mus_rewind
.ifndef __LYNX__
        lda     _ns_on          ; the stream owns SKCTL (its clock bits)
        bne     @on
        lda     #3
        sta     SKCTL
.endif
@on:    lda     #1
        sta     mus_on
        rts

; void mus_stop(void)
_mus_stop:
        lda     #0
        sta     mus_on
        ldx     #7
@z:     sta     sh,x
        dex
        bpl     @z
        rts

mus_rewind:
        lda     _mus_desc+D_V1S
        sta     vp
        lda     _mus_desc+D_V1S+1
        sta     vp+1
        lda     _mus_desc+D_V2S
        sta     vp+2
        lda     _mus_desc+D_V2S+1
        sta     vp+3
        lda     _mus_desc+D_V3S
        sta     vp+4
        lda     _mus_desc+D_V3S+1
        sta     vp+5
        lda     _mus_desc+D_V4S
        sta     vp+6
        lda     _mus_desc+D_V4S+1
        sta     vp+7
        ldx     #3
@v:     lda     _mus_desc+D_DUR,x
        sta     vdur,x
        lda     #0
        sta     vact,x
        dex
        bpl     @v
        rts

; void __fastcall__ seq_start(const uint8_t *script)
_seq_start:
        ldy     #0
        sty     _seq_ptr+1      ; (atomic against the VBI: hi byte last)
        sta     _seq_ptr
        sty     sq_wait
        sty     sq_dep
        sty     sq_mask
        sty     sq_mask+1
        stx     _seq_ptr+1
        rts

; void seq_stop(void)
_seq_stop:
        lda     #0
        sta     _seq_ptr+1
        jmp     seq_quiet

; void __fastcall__ v4_play(uint8_t id_and_repeats): id in bits 0-2,
; repeats in bits 4-7
_v4_play:
        pha
        and     #7
        tax
        lda     #$80
        sta     v4_id           ; (off while it changes)
        pla
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        sta     v4_rep
        lda     #0
        sta     v4_c8
        sta     v4_gap
        stx     v4_id
        rts

; void __fastcall__ tm_set(uint8_t left): tick every `left` frames (0: stop)
_tm_set:
        sta     tm_per
        tax
        bne     @on
        lda     #0
        sta     sh
        sta     sh+1
        sta     sh+2
        sta     sh+3
        rts
@on:    lda     #64
        sta     tm_lease
        lda     tm_cnt
        beq     @s
        cmp     tm_per
        bcc     @r
@s:     stx     tm_cnt
@r:     rts

; void __fastcall__ step_play(uint16_t fc): AUDF3 = low byte, AUDC3 = high
_step_play:
        sta     st_f
        stx     st_c
        lda     #1
        sta     st_n
        rts

; ---- the VBI -----------------------------------------------------------
_snd_vbi:
        lda     mus_on
        beq     @1
        jsr     music
@1:     lda     _seq_ptr+1
        beq     @2
        jsr     seq
@2:     lda     tm_per
        beq     @3
        jsr     timer
@3:     jsr     step
        lda     v4_id
        bmi     @4
        jsr     v4
@4:     ldx     #8              ; out: all, or while SIO is busy
.ifndef __LYNX__
        lda     CRITIC          ; voices 1 and 2 only
        ora     _ns_on
        beq     @w
        ldx     #3
.endif
@w:     lda     sh,x
        ldy     _snd_mute
        beq     @p
        cpx     #8              ; muted: the volumes are 0
        beq     @p
        pha
        txa
        lsr     a
        pla
        bcc     @p
        lda     #0
.ifdef __LYNX__
@p:     sta     _pk_out,x
        dex
        bpl     @w
        jmp     _mikey_out
.else
@p:     sta     AUDF1,x
        dex
        bpl     @w
        rts
.endif

; ---- music ---------------------------------------------------------------
music:  ldx     #0
@voice: lda     vact,x
        beq     @fetch
        jmp     @env
@fetch: txa
        asl     a
        tay                     ; y = 2 * voice
        cpx     #0
        bne     @wrap
        ; voice 1: at its end the tune starts over (silent this frame)
        lda     vp
        cmp     _mus_desc+D_V1E
        bne     @get
        lda     vp+1
        cmp     _mus_desc+D_V1E+1
        bne     @get
        lda     _mus_desc+D_ALT
        beq     @rew
        lda     #0              ; the round's tune: after the first time
        sta     _mus_desc+D_ALT ; through, voices 1-3 start 2 bytes on
        ldx     #D_V1S
        jsr     add2
        ldx     #D_V2S
        jsr     add2
        ldx     #D_V3S
        jsr     add2
@rew:   jsr     mus_rewind
        ldx     #7
        lda     #0
@q:     sta     sh,x
        dex
        bpl     @q
        rts
@wrap:  ; voices 2-4: back to the loop's start at its end
        stx     vx
        lda     wrapoff-1,x
        tax
        lda     vp,y
        cmp     _mus_desc,x
        bne     @nw
        lda     vp+1,y
        cmp     _mus_desc+1,x
        bne     @nw
        lda     _mus_desc+2,x
        sta     vp,y
        lda     _mus_desc+3,x
        sta     vp+1,y
@nw:    ldx     vx
@get:   lda     vp,y
        sta     sp1
        lda     vp+1,y
        sta     sp1+1
        cpx     #3
        bne     @note
        ldy     #0              ; voice 4: a raw AUDF byte
        lda     (sp1),y
        sta     sh+6
        lda     #1
        jsr     advance
        lda     _mus_desc+D_BASE+3
        jmp     @start
@note:  lda     _mus_desc+D_FT23
        sta     sp2
        lda     _mus_desc+D_FT23+1
        sta     sp2+1
        cpx     #0
        bne     @t
        lda     _mus_desc+D_FT1
        sta     sp2
        lda     _mus_desc+D_FT1+1
        sta     sp2+1
@t:     ldy     #0
        lda     (sp1),y
        sta     ev
        and     #$3F
        tay
        lda     (sp2),y
        pha
        txa
        asl     a
        tay
        pla
        sta     sh,y            ; AUDFn
        lda     ev
        bpl     @nolen
        ldy     #1              ; a new length
        lda     (sp1),y
        sta     vdur,x
        lda     #2
        jsr     advance
        jmp     @rest
@nolen: lda     #1
        jsr     advance
@rest:  bit     ev
        lda     #0
        bvs     @start          ; a rest
        lda     _mus_desc+D_BASE,x
@start: sta     vcur,x
        lda     _mus_desc+D_HOLD,x
        sta     vhold,x
        lda     _mus_desc+D_DECAY,x
        sta     vdec,x
        lda     vdur,x
        sta     vdurc,x
        lda     #1
        sta     vact,x
@env:   ; the envelope: hold, then decay (or cut)
        txa
        asl     a
        tay
        lda     vcur,x
        sta     sh+1,y
        lda     vhold,x
        beq     @dcy
        dec     vhold,x
        jmp     @len
@dcy:   lda     vcur,x
        and     #$0F
        beq     @len
        lda     _mus_desc+D_DECAY,x
        bne     @slow
        lda     _mus_desc+D_BASE,x
        and     #$F0
        sta     sh+1,y
        jmp     @len
@slow:  lda     vdec,x
        beq     @step
        dec     vdec,x
        jmp     @len
@step:  lda     _mus_desc+D_DECAY,x
        sta     vdec,x
        dec     vcur,x
        lda     vcur,x
        sta     sh+1,y
@len:   lda     vdurc,x
        cmp     #1
        bne     @more
        lda     #0
        sta     vact,x
        beq     @nextv
@more:  dec     vdurc,x
@nextv: inx
        cpx     #4
        bcs     @done
        jmp     @voice
@done:  rts

; _mus_desc,x (a pointer) += 2
add2:   lda     _mus_desc,x
        clc
        adc     #2
        sta     _mus_desc,x
        bcc     @r
        inc     _mus_desc+1,x
@r:     rts

; voice x's pointer += A (x kept)
advance:
        pha
        txa
        asl     a
        tay
        pla
        clc
        adc     vp,y
        sta     vp,y
        bcc     @r
        lda     vp+1,y
        adc     #0
        sta     vp+1,y
@r:     rts

; ---- the script sequencer -------------------------------------------------
;   $00          end: the registers it set go quiet
;   $01-$7F      wait that many frames
;   $80+r v      register r (0-7 AUDF1..AUDC4, 8 AUDCTL) = v
;   $90 n        loop n times (0: 256) ...
;   $91          ... to here (two deep)
;   $A0+r d      register r += d
;   $B0+r m o    register r = RANDOM & m | o
;   $C0+r v      register r = v one time in four, else 0
;   $D0 m        wait RANDOM & m + 1 frames
seq:    lda     sq_wait
        beq     @go
        dec     sq_wait
        rts
@go:    lda     _seq_ptr
        sta     sp1
        lda     _seq_ptr+1
        sta     sp1+1
@op:    ldy     #0
        lda     (sp1),y
        bne     @nz
        sta     _seq_ptr+1
        jmp     seq_quiet
@nz:    bmi     @cmd
        ; wait n (this frame is the first)
        sec
        sbc     #1
        sta     sq_wait
        lda     #1
        jsr     sqadv
        jmp     sqsave
@cmd:   cmp     #$90
        bcc     @set
        bne     @n90
        jmp     @loop
@n90:   cmp     #$91
        bne     @n91
        jmp     @next
@n91:   cmp     #$D0
        beq     @wrnd
        pha
        and     #$0F
        tax
        pla
        and     #$F0
        cmp     #$A0
        beq     @add
        cmp     #$B0
        beq     @rnd
        ; $C0: one time in four
        jsr     mark
        RND
        and     #3
        bne     @c0
        iny
        lda     (sp1),y
        .byte   $2C
@c0:    lda     #0
        sta     sh,x
        lda     #2
        jsr     sqadv
        jmp     @op
@set:   and     #$0F
        tax
        jsr     mark
        iny
        lda     (sp1),y
        sta     sh,x
        lda     #2
        jsr     sqadv
        jmp     @op
@add:   jsr     mark
        iny
        lda     (sp1),y
        clc
        adc     sh,x
        sta     sh,x
        lda     #2
        jsr     sqadv
        jmp     @op
@rnd:   jsr     mark
        iny
        RND
        and     (sp1),y
        iny
        ora     (sp1),y
        sta     sh,x
        lda     #3
        jsr     sqadv
        jmp     @op
@wrnd:  iny
        RND
        and     (sp1),y
        sta     sq_wait
        lda     #2
        jsr     sqadv
        jmp     sqsave
@loop:  ldx     sq_dep
        iny
        lda     (sp1),y
        sta     sq_lc,x
        lda     #2
        jsr     sqadv
        txa
        asl     a
        tax
        lda     sp1
        sta     sq_lp,x
        lda     sp1+1
        sta     sq_lp+1,x
        inc     sq_dep
        jmp     @op
@next:  ldx     sq_dep
        dex
        dec     sq_lc,x
        beq     @out
        txa
        asl     a
        tax
        lda     sq_lp,x
        sta     sp1
        lda     sq_lp+1,x
        sta     sp1+1
        jmp     @op
@out:   stx     sq_dep
        lda     #1
        jsr     sqadv
        jmp     @op

sqadv:  clc
        adc     sp1
        sta     sp1
        bcc     @r
        inc     sp1+1
@r:     rts

sqsave: lda     sp1
        sta     _seq_ptr
        lda     sp1+1
        sta     _seq_ptr+1
        rts

; register x: the script has set it
mark:   lda     bitlo,x
        ora     sq_mask
        sta     sq_mask
        lda     bithi,x
        ora     sq_mask+1
        sta     sq_mask+1
        rts

; the script's registers go quiet
seq_quiet:
        ldx     #7
@z:     lda     bitlo,x
        and     sq_mask
        beq     @n
        lda     #0
        sta     sh,x
@n:     dex
        bpl     @z
        lda     sq_mask+1
        beq     @c
        lda     #0
        sta     sh+8
@c:     lda     #0
        sta     sq_mask
        sta     sq_mask+1
        rts

; ---- the time bar's tick: voices 1 and 2 -----------------------------------
timer:  dec     tm_lease
        bne     @go
        lda     #0              ; nobody told us for a while: stop
        jmp     _tm_set
@go:    lda     tm_ph
        beq     @cnt
        dec     tm_ph
        beq     @last
        lda     #$A1            ; the tick's second frame
        sta     sh+1
        lda     #0
        sta     sh+3
        rts
@last:  lda     #$A0
        sta     sh+1
        rts
@cnt:   dec     tm_cnt
        bne     @r
        lda     tm_per
        sta     tm_cnt
        lda     #2
        sta     tm_ph
        lda     tm_tog
        eor     #1
        sta     tm_tog
        lda     #$0F
        ldx     tm_tog
        beq     @f
        lda     #$0A
@f:     sta     sh
        sta     sh+2
        lda     #$A2
        sta     sh+1
        lda     #$81
        sta     sh+3
@r:     rts

; ---- a footstep: voice 3, one frame ---------------------------------------
step:   lda     st_n
        beq     @off
        lda     st_f
        sta     sh+4
        lda     st_c
        sta     sh+5
        lda     #0
        sta     st_n
        lda     #1
        sta     st_on
        rts
@off:   lda     st_on
        beq     @r
        lda     #0
        sta     st_on
        sta     sh+5
@r:     rts

        .segment LOWBSS
st_on:  .res 1
        .code

; ---- the voice-4 effects ($5E3D in the original) ---------------------------
; _v4_tab: 7 pitches, 7 lengths, 7 volumes (gen/atari/snd.s, from the disk)
v4:     lda     v4_gap
        beq     @run
        dec     v4_gap
        rts
@run:   ldx     v4_id
        lda     v4_c8
        bne     @on
        lda     #0              ; start
        sta     sh+8
        lda     _v4_tab,x
        sta     sh+6
        lda     _v4_tab+14,x
        sta     sh+7
        sta     v4_c9
        lda     _v4_tab+7,x
        sta     v4_c8
        rts
@on:    cpx     #6
        bne     @beep
        lsr     v4_c8           ; the production chirp: a falling noise
        lda     v4_c8
        sta     sh+6
        lda     v4_c9
        clc
        adc     #2
        sta     v4_c9
        sta     sh+7
        cmp     #$0C
        bcs     @stop
        rts
@beep:  dec     v4_c8
        beq     @stop
        lda     v4_c8
        cmp     #14
        bcs     @r
        dec     v4_c9
        lda     v4_c9
        sta     sh+7
@r:     rts
@stop:  lda     #0
        sta     sh+6
        sta     sh+7
        sta     v4_c8
        lda     v4_rep
        beq     @end
        dec     v4_rep          ; again, after a frame's rest
        lda     #1
        sta     v4_gap
        rts
@end:   lda     #$80
        sta     v4_id
        rts
