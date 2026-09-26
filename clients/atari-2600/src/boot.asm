; boot.asm -- bank 0: the cold start, the title, who you are, where to play.
;
; Bank 0 because the cold stub in the fixed tail sends the console here.
;
; The name is the FujiNet lobby's shared username (appkey 0); without one a
; keyboard types it (and writes it back). The server comes from appkey 10,
; the lobby's room URL "https://host/?table=id" (MekkoGX net.c appkey_load):
; its host is the endpoint and its table is joined straight away. Without it
; the endpoint is the build's and the table list is shown. None of these
; strings touch console RAM: they are cartridge path buffers 0-2.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     1
MHASDEC EQU     0
MHASCMP EQU     0

        INCLUDE "../build/tail.inc"

; The keyboard (bsname.asm's): three rows of twelve, A-Z and 0-9, then OK,
; DEL.
KSEL    EQU     PV+0
KGRIDW  EQU     12
KCELLS  EQU     36
KOK     EQU     KCELLS
KDEL    EQU     KCELLS+1
KLAST   EQU     KDEL
RNNAME  EQU     4
RNGRID  EQU     7
RNCTRL  EQU     11
RNPICK  EQU     14

        ORG     $1000

BENTRY: lda     #2
        sta     VBLANK
        lda     MENT
        cmp     #ENNAME
        bne     BE1
        jmp     KBEGIN
BE1:    jmp     BCOLD

; ---------------------------------------------------------------------------
; The cold start. RAM and every TIA register cleared (RESMP1's power-on state
; is undefined on silicon); the cartridge's four path buffers emptied, since
; they survive a console reset.
BCOLD:  ldx     #$FF
        txs
        lda     #0
BCL1:   sta     $00,x
        dex
        bne     BCL1
        sta     $00
        lda     #2
        sta     VBLANK
        ldx     #FP_SEL3
BCL2:   txa
        sta     FNRSEL+FH_PATHO
        lda     #FP_RST
        sta     FNRSEL+FH_PATHO
        dex
        cpx     #FP_SEL0-1
        bne     BCL2
        lda     #7
        sta     SNDALT
        lda     #$FF
        sta     HSCRB
        sta     HSEAT
        sta     MBAR
        jsr     FNARM
        jsr     FNCHK
        beq     BTITLE
; No cartridge, no character generator: a red screen is the whole message.
        lda     #CRED
        sta     COLUBK
        lda     #0
        sta     VBLANK
BHALT:  jmp     BHALT

; ---------------------------------------------------------------------------
BTITLE: jsr     DINIT
        jsr     FNCLS
        lda     #4
        ldx     #(STITL1)&$FF
        ldy     #(STITL1)>>8
        jsr     BROW
        lda     #6
        ldx     #(STITL2)&$FF
        ldy     #(STITL2)>>8
        jsr     BROW
        lda     #10
        ldx     #(STITL3)&$FF
        ldy     #(STITL3)>>8
        jsr     BROW
        lda     #18
        ldx     #(STITL4)&$FF
        ldy     #(STITL4)>>8
        jsr     BROW
        lda     #0
        sta     VBLANK
        jmp     DFRESH

; BROW -- text row A = the string at X/Y.
BROW:   jsr     FNROWA
        jsr     FNSTRXY
        jmp     FNENDW

APPVBL: jsr     SNDRUN
        lda     KMODE
        beq     APT
        jmp     KAPP
APT:    jsr     INSCAN
        and     #IN_FIRE
        beq     APV1
        jmp     BSTART
APV1:   rts

KMODE   EQU     PV+1            ; 0 the title, 1 the keyboard
NMLEN   EQU     PV+2            ; the name's length (the keyboard)
EPLEN   EQU     PV+3            ; the endpoint's length (the room appkey)
TBLEN   EQU     PV+4            ; the table id's length

; ---------------------------------------------------------------------------
; BSTART -- FIRE on the title: the appkeys, blind, then on.
BSTART: lda     #2
        sta     VBLANK
        jsr     BENDPT
        jsr     BNAME
        beq     BGO
        jmp     KBEGIN
BGO:    lda     #ENLIST
        sta     MENT
        lda     #BKTABLE
        jmp     MGOTO

; BNAME -- appkey 0 into path buffer 2, A-Z and 0-9 only (the URL carries it
; unescaped), lower case raised. 0 = a name.
BNAME:  lda     #PBNAME
        jsr     FNWSEL
        jsr     FNWRST
        lda     #0
        sta     NMLEN
        ldx     #AKNAME
        jsr     AKREAD
        bne     BNX
        lda     FNRPLY+1
        bne     BN0
        lda     FNRPLY
        cmp     #NAMEMAX+1
        bcc     BN1
BN0:    lda     #NAMEMAX
BN1:    sta     MTMP
        ldx     #0
BN2:    cpx     MTMP
        beq     BN5
        lda     FNRPLY+2,x
        beq     BN5
        cmp     #'a'
        bcc     BN3
        cmp     #'z'+1
        bcs     BNBAD
        and     #$DF
BN3:    cmp     #'0'
        bcc     BNBAD
        cmp     #'9'+1
        bcc     BN4
        cmp     #'A'
        bcc     BNBAD
        cmp     #'Z'+1
        bcs     BNBAD
BN4:    sta     FNRSEL+FH_PATHC
        inx
        bne     BN2
BN5:    stx     NMLEN
        txa
        beq     BNX
        lda     #0
        rts
BNBAD:  jsr     FNWRST
        lda     #0
        sta     NMLEN
BNX:    lda     #$FF
        rts

; BENDPT -- appkey 10: "https://host/?table=id". The host into path buffer
; 0 (no trailing '/'), the id into path buffer 1. No key, or not a URL: the
; build's endpoint and no table.
BENDPT: lda     #PBEND
        jsr     FNWSEL
        jsr     FNWRST
        lda     #PBTABLE
        jsr     FNWSEL
        jsr     FNWRST
        lda     #0
        sta     EPLEN
        sta     TBLEN
        ldx     #AKROOM
        jsr     AKREAD
        bne     BEDEFJ
        lda     FNRPLY+1
        bne     BEDEFJ          ; longer than any URL we would write
        lda     FNRPLY
        cmp     #5
        bcs     BE1A
BEDEFJ: jmp     BEDEF
BE1A:
        sta     MTMP            ; its length
        lda     FNRPLY+2
        cmp     #'h'
        bne     BEDEFJ
; The host: up to '?' or the end; then a trailing '/' dropped.
        lda     #PBEND
        jsr     FNWSEL
        ldx     #0
BE2:    cpx     MTMP
        beq     BE3B
        lda     FNRPLY+2,x
        beq     BE3B
        cmp     #'?'
        beq     BE3B
        sta     FNRSEL+FH_PATHC
        sta     MTMP2           ; the last character
        inc     EPLEN
        inx
        bne     BE2
BE3B:   lda     MTMP2
        cmp     #'/'
        bne     BE3C
        lda     #FP_POPCH
        sta     FNRSEL+FH_PATHO
        dec     EPLEN
BE3C:   cpx     MTMP
        beq     BE9
        lda     FNRPLY+2,x
        cmp     #'?'
        bne     BE9
; The table: "table=" and then letters and digits.
BE4:    inx
        ldy     #0
BE5:    lda     FNRPLY+2,x
        cmp     STABEQ,y
        bne     BE9
        inx
        iny
        cpy     #6
        bne     BE5
        lda     #PBTABLE
        jsr     FNWSEL
BE6:    cpx     MTMP
        beq     BE8
        lda     FNRPLY+2,x
        cmp     #'0'
        bcc     BE8
        sta     FNRSEL+FH_PATHC
        inc     TBLEN
        inx
        lda     TBLEN
        cmp     #8
        bne     BE6
BE8:    lda     TBLEN
        beq     BE9
        lda     HFLAGS
        ora     #HFTBL
        sta     HFLAGS
BE9:    lda     EPLEN
        bne     BEX
; BEDEF -- the build's endpoint, out of ROM into path buffer 0.
BEDEF:  lda     #PBEND
        jsr     FNWSEL
        jsr     FNWRST
        ldx     #0
BED1:   lda     UENDPT,x
        beq     BED2
        sta     FNRSEL+FH_PATHC
        inx
        bne     BED1
BED2:   stx     EPLEN
BEX:    rts

STABEQ: DB      "table="

; ---------------------------------------------------------------------------
; The keyboard.
KBEGIN: jsr     DINIT
        jsr     FNCLS
        lda     #1
        sta     KMODE
        lda     #0
        sta     KSEL
        sta     NMLEN
        lda     #PBNAME
        jsr     FNWSEL
        jsr     FNWRST
        lda     #1
        ldx     #(SNAME)&$FF
        ldy     #(SNAME)>>8
        jsr     BROW
        ldx     #0
KB1:    stx     MIDX
        txa
        clc
        adc     #RNGRID
        jsr     FNROWA
        lda     MIDX
        asl     a
        asl     a
        sta     MTMP
        asl     a
        adc     MTMP            ; row * 12
        tax
        ldy     #KGRIDW
KB2:    lda     KCHARS,x
        jsr     FNCHR
        inx
        dey
        bne     KB2
        jsr     FNENDW
        ldx     MIDX
        inx
        cpx     #3
        bne     KB1
        lda     #RNCTRL
        ldx     #(SCTRL)&$FF
        ldy     #(SCTRL)>>8
        jsr     BROW
        lda     #18
        ldx     #(SKFOOT)&$FF
        ldy     #(SKFOOT)>>8
        jsr     BROW
        jsr     KDRAW
        lda     #0
        sta     VBLANK
        jmp     DFRESH

KAPP:   jsr     INREPT
        lda     MINP
        and     #IN_LEFT
        beq     KA1
        lda     KSEL
        beq     KA1
        dec     KSEL
        jsr     KDRAW
KA1:    lda     MINP
        and     #IN_RIGHT
        beq     KA2
        lda     KSEL
        cmp     #KLAST
        bcs     KA2
        inc     KSEL
        jsr     KDRAW
KA2:    lda     MINP
        and     #IN_UP
        beq     KA3
        lda     KSEL
        sec
        sbc     #KGRIDW
        bcc     KA3
        sta     KSEL
        jsr     KDRAW
KA3:    lda     MINP
        and     #IN_DOWN
        beq     KA4
        lda     KSEL
        cmp     #KCELLS
        bcs     KA4
        clc
        adc     #KGRIDW
        cmp     #KCELLS
        bcc     KA3A
        lda     #KOK
KA3A:   sta     KSEL
        jsr     KDRAW
KA4:    lda     MINP
        and     #IN_FIRE
        beq     KA9
        lda     KSEL
        cmp     #KOK
        beq     KOKAY
        cmp     #KDEL
        beq     KBS
        ldx     NMLEN
        cpx     #NAMEMAX
        bcs     KA9
        tax
        lda     KCHARS,x
        sta     FNRSEL+FH_PATHC
        inc     NMLEN
        jmp     KDRAW
KBS:    lda     NMLEN
        beq     KA9
        lda     #FP_POPCH
        sta     FNRSEL+FH_PATHO
        dec     NMLEN
        jmp     KDRAW
KA9:    rts

; OK: write the name back for the next FujiNet game, then the tables.
KOKAY:  lda     NMLEN
        beq     KA9
        lda     #2
        sta     VBLANK
        lda     NMLEN
        ldx     #AKNAME
        jsr     AKWBEG
        bne     KOK1
        lda     #PBNAME
        jsr     FNWSEL
        jsr     FNWRAW
        jsr     FNGO
KOK1:   lda     #ENLIST
        sta     MENT
        lda     #BKTABLE
        jmp     MGOTO

; KDRAW -- the name so far (the cart renders its own buffer: the console
; cannot read it back), the cursor's cell, the bar.
KDRAW:  lda     #PBNAME
        jsr     FNWSEL
        lda     #0
        sta     FNRSEL+FH_BSL
        sta     FNRSEL+FH_BSH
        sta     FNRSEL+FH_BDH
        lda     #RNNAME
        sta     FNRSEL+FH_BDL
        lda     #FNTCOL
        sta     FNRSEL+FH_BCNT
        lda     #FB_PATH
        jsr     FNBLIT
        lda     #RNPICK
        jsr     FNROWA
        lda     #'>'
        jsr     FNCHR
        lda     KSEL
        cmp     #KCELLS
        bcs     KD1
        tax
        lda     KCHARS,x
        jsr     FNCHR
        lda     #'<'
        jsr     FNCHR
        jmp     KD2
KD1:    ldx     #(SOK)&$FF
        ldy     #(SOK)>>8
        cmp     #KOK
        beq     KD1A
        ldx     #(SDEL)&$FF
        ldy     #(SDEL)>>8
KD1A:   jsr     FNSTRXY
KD2:    jsr     FNENDW
        lda     KSEL
        cmp     #KCELLS
        bcc     KD3
        lda     #RNCTRL
        sta     MBAR
        rts
KD3:    ldx     #RNGRID-1
KD4:    inx
        sec
        sbc     #KGRIDW
        bcs     KD4
        stx     MBAR
        rts

STITL1: DB      "  M.U.L.E.",0
STITL2: DB      " TOURNAMENT",0
STITL3: DB      " ATARI 2600",0
STITL4: DB      "FIRE: START",0
SNAME:  DB      " YOUR NAME",0
SCTRL:  DB      " OK   DEL",0
SKFOOT: DB      "FIRE PICKS",0
SOK:    DB      "OK<",0
SDEL:   DB      "DEL<",0
KCHARS: DB      "ABCDEFGHIJKL"
        DB      "MNOPQRSTUVWX"
        DB      "YZ0123456789"

MBGT:   DB      0

        INCLUDE "../build/endpoint.inc"
        INCLUDE "mlib.inc"
        INCLUDE "appkey.inc"
        INCLUDE "sound.inc"
        INCLUDE "disptext.inc"

        END
