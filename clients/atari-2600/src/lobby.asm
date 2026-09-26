; lobby.asm -- bank 1: the lobby (phase 0).
;
; The lobby: the four seats, each a pair of rows in its seat's colour -- the
; name and a tick when ready, then the species -- and the countdown. The
; stick changes your species, FIRE readies (and un-readies), RESET held
; leaves.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     1
MHASDEC EQU     1
MHASCMP EQU     0

        INCLUDE "../build/tail.inc"

; The phase area.
LSPEC   EQU     PV+3            ; my species, as last sent or shown
LRDY    EQU     PV+4            ; my ready flag, as last shown
LCOUNT  EQU     PV+5            ; the countdown's seconds, $FF = waiting
LBELL   EQU     PV+6            ; the countdown bell has rung

RSEAT   EQU     2               ; the lobby's first seat row (2 per seat, 3
                                ;   apart)

        ORG     $1000

LENTRY: lda     MENT
        cmp     #ENRUN
        bne     LE2
        jmp     SLOOP
LE2:    cmp     #ENSTATE
        bne     LE3
        jmp     LSTATE
LE3:    jmp     LENTER

; LSCREEN -- a fresh text screen: the kernel's TIA setup, every row blank,
; the title. Between frames, in the overscan.
LSCREEN: jsr    DINIT
        jsr     FNCLS
        lda     #0
        jsr     FNROWA
        ldx     #(STITLE)&$FF
        ldy     #(STITLE)>>8
        jsr     FNSTRXY
        jmp     FNENDW

; ---------------------------------------------------------------------------
; The lobby: ENENTER sets the screen up, ENSTATE redraws what changed. The
; rows are composed a few a frame from the vblank (LCOMPOSE), and HFNEW is
; cleared when the last is done.
LENTER: lda     #BKLOBBY
        sta     MSCR
        jsr     LSCREEN
        lda     #$FF
        sta     MBGMSK
        lda     #0
        sta     SNOPOLL
        sta     LBELL
        lda     #$FF
        sta     LSPEC
        sta     LRDY
        lda     #$FF
        sta     MBAR
        lda     #17
        jsr     FNROWA
        ldx     #(SSTICK)&$FF
        ldy     #(SSTICK)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        lda     #19
        jsr     FNROWA
        ldx     #(SLEAVE)&$FF
        ldy     #(SLEAVE)>>8
        jsr     FNSTRXY
        jsr     FNENDW
LSTATE: lda     #1
        sta     MCOMP
        lda     MENT            ; a state inside a frame's overscan: no fresh
        cmp     #ENSTATE        ;   start (SLOOPF re-arms the timer)
        beq     LST1
        jmp     SLOOPF
LST1:   jmp     SLOOP

; LCOMPOSE -- one row of the lobby's redraw a frame, from the vblank:
; steps 1-8 are the seats' two rows, then the rest.
LCOMPOSE: lda   MCOMP
        bne     LC0
        rts
LC0:    inc     MCOMP
        cmp     #9
        bcc     LC0A
        jmp     LC5
LC0A:   sec
        sbc     #1
        lsr     a
        sta     MIDX            ; the seat
        bcc     LC0B
        jmp     LCSPEC
LC0B:   tax
        lda     SEATROW,x
        jsr     FNROWA
        ldx     MIDX
        lda     PLOFS,x
        tax
        ldy     #8              ; the name, to its NUL
LC1:    lda     FNRPLY+SPLAYER+PLNAME,x
        beq     LC2
        jsr     FNCHR
        inx
        dey
        bne     LC1
LC2:    tya                     ; pad to 9
        clc
        adc     #1
        jsr     FNSPC
        ldx     MIDX
        lda     PLOFS,x
        tax
        lda     FNRPLY+SPLAYER+PLFLAGS,x
        and     #PF_READY
        beq     LC3
        lda     #'O'
        jsr     FNCHR
        lda     #'K'
        jsr     FNCHR
LC3:    jmp     FNENDW
LCSPEC: ldx     MIDX
        lda     SEATROW,x
        clc
        adc     #1
        jsr     FNROWA
        lda     #' '
        jsr     FNCHR
        ldx     MIDX
        lda     PLOFS,x
        tax
        lda     FNRPLY+SPLAYER+PLSPEC,x
        and     #7
        asl     a
        asl     a
        asl     a
        tax
        ldy     #8
LC4:    lda     SPNAME,x
        jsr     FNCHR
        inx
        dey
        bne     LC4
        lda     MIDX
        cmp     HSEAT
        bne     LC4A
        lda     #' '
        jsr     FNCHR
        lda     #'*'            ; you
        jsr     FNCHR
LC4A:   jmp     FNENDW
; step 5: the countdown, the FIRE line, my own flags; then done.
LC5:    ldx     HSEAT
        cpx     #4
        bcs     LC6
        lda     PLOFS,x
        tax
        lda     FNRPLY+SPLAYER+PLSPEC,x
        and     #7
        sta     LSPEC
        lda     FNRPLY+SPLAYER+PLFLAGS,x
        and     #PF_READY
        sta     LRDY
LC6:    lda     #18
        jsr     FNROWA
        ldx     #(SREADY)&$FF
        ldy     #(SREADY)>>8
        lda     LRDY
        beq     LC7
        ldx     #(SUNRDY)&$FF
        ldy     #(SUNRDY)>>8
LC7:    jsr     FNSTRXY
        jsr     FNENDW
        lda     #14
        jsr     FNROWA
        lda     FNRPLY+SHCOUNT+1
        bmi     LC8             ; -1: waiting for the others
        ldx     #(SSTART)&$FF
        ldy     #(SSTART)>>8
        jsr     FNSTRXY
; jiffies / 60, near enough: (hi*256 + lo) / 64 * 16/15 -- no: seconds by
; repeated subtraction, the count is under 30 s.
        lda     FNRPLY+SHCOUNT
        sta     MNUM
        lda     FNRPLY+SHCOUNT+1
        sta     MNUMH
        ldx     #0
LC9:    lda     MNUM
        sec
        sbc     #60
        tay
        lda     MNUMH
        sbc     #0
        bcc     LC10
        sta     MNUMH
        sty     MNUM
        inx
        bne     LC9
LC10:   txa
        pha
        ldx     #2
        jsr     MDEC8
        pla
        bne     LC11
        jmp     LC12
LC11:   lda     LBELL           ; the countdown has begun: ring once
        bne     LC12
        inc     LBELL
        lda     #SNDBELL
        jsr     SNDPLAY
        jmp     LC12
LC8:    ldx     #(SWAIT)&$FF
        ldy     #(SWAIT)>>8
        jsr     FNSTRXY
LC12:   jsr     FNENDW
        lda     #0
        sta     MCOMP
        lda     HFLAGS
        and     #~HFNEW&$FF
        sta     HFLAGS
        rts

; ---------------------------------------------------------------------------
; APPVBL: sound, input, a step of the redraw.
APPVBL: jsr     SNDRUN
        jsr     INREPT
LAPP:   jsr     LCOMPOSE
        jsr     RSTCHK
        lda     HSEAT
        cmp     #4
        bcs     LA9             ; a spectator only watches
        lda     MINP
        and     #IN_LEFT|IN_RIGHT
        beq     LA7
        and     #IN_LEFT
        beq     LA5
        lda     LSPEC
        sec
        sbc     #1
        jmp     LA6
LA5:    lda     LSPEC
        clc
        adc     #1
LA6:    and     #7
        sta     LSPEC
        tax
        lda     #RQSPEC
        ldy     #0
        jsr     QACT
LA7:    lda     MINP
        and     #IN_FIRE
        beq     LA9
        lda     #RQREADY
        ldx     LRDY
        beq     LA8
        lda     #RQUNRDY
LA8:    ldx     #0
        ldy     #0
        jsr     QACT
LA9:    rts

; STICK: nothing between frames.
STICK:  rts

PLOFS:  DB      0,PLSIZE,2*PLSIZE,3*PLSIZE
SEATROW: DB     RSEAT,RSEAT+3,RSEAT+6,RSEAT+9
SPNAME: DB      "MECHTRONGOLLUMERPACKER  BONZOID SPHEROIDFLAPPER LEGGITE HUMANOID"

STITLE: DB      "  M.U.L.E.",0
SSTICK: DB      "STICK: RACE",0
SLEAVE: DB      "RESET: LEAVE",0
SREADY: DB      "FIRE: READY",0
SUNRDY: DB      "FIRE: WAIT",0
SSTART: DB      "STARTS IN ",0
SWAIT:  DB      "  WAITING",0

; The rows: the seats' pairs in their colours.
MBGT:   DB      CBLACK,CBLACK
        DB      CPLAY0,CPLAY0,CBLACK
        DB      CPLAY1,CPLAY1,CBLACK
        DB      CPLAY2,CPLAY2,CBLACK
        DB      CPLAY3,CPLAY3,CBLACK
        DB      CBLACK,CBLACK,CBLACK,CBLACK,CBLACK,CBLACK,CBLACK
        DB      CBLACK

        INCLUDE "mlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "scrloop.inc"
        INCLUDE "disptext.inc"

        END
