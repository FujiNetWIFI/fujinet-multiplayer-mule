; appkey.inc -- the FujiNet lobby's appkeys: creator 1, app 1. Key 0 is the
; shared username every FujiNet game reads; key 10 is the room, a URL
; "https://host/?table=id" (MekkoGX net.c appkey_load/save_room).
;
; Blocking: FNGO spins with the screen held. Used at boot and at a join,
; which are screen changes anyway.

; AKOPEN -- OPEN_APPKEY: key X, mode A. 0 = taken. The payload is a SIX-byte
; packed struct (creator u16, app, key, mode, reserved) and the reserved byte
; is not optional: without it the adapter waits for it and times out.
AKOPEN: pha
        txa
        pha
        lda     #FNDEVF
        sta     FNDEV
        lda     #FCAKOPN
        sta     FNCMD
        lda     #0
        sta     FNNPR
        jsr     FNBEG
        lda     #AKCREAT
        sta     FNTX
        lda     #0
        sta     FNTX
        lda     #AKAPP
        sta     FNTX
        pla
        sta     FNTX            ; the key
        pla
        sta     FNTX            ; the mode
        lda     #0
        sta     FNTX
        jsr     FNGO
        bne     AKOX
        jmp     FNACK
AKOX:   rts

; AKREAD -- read key X into the reply window: [0-1] length, then the value.
; 0 = there is one, and it is not empty.
AKREAD: lda     #AKMRD
        jsr     AKOPEN
        bne     AKBAD
        lda     #FNDEVF
        sta     FNDEV
        lda     #FCAKRD
        sta     FNCMD
        lda     #0
        sta     FNNPR
        jsr     FNBEG
        jsr     FNGO
        bne     AKBAD
        jsr     FNACK
        bne     AKBAD
        lda     FNRPLY+0
        ora     FNRPLY+1
        beq     AKBAD
        lda     #0
        rts
AKBAD:  lda     #$FF
        rts

; AKWBEG -- open key X for writing and begin the WRITE of A bytes; the
; caller streams the value into FNTX and then calls FNGO.
AKWBEG: pha
        lda     #AKMWR
        jsr     AKOPEN
        bne     AKWX
        lda     #FNDEVF
        sta     FNDEV
        lda     #FCAKWR
        sta     FNCMD
        lda     #1
        sta     FNNPR
        jsr     FNBEG
        pla
        jsr     FNPB
        lda     #0
        rts
AKWX:   pla
        lda     #$FF
        rts
