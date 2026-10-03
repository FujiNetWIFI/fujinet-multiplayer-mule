; goods.asm -- bank 9: a good's status and declaring (phase 5).
;
; MekkoGX auction.c's per-good status (stage SHOW, 561 jiffies) and the
; declaring (stage DECLARE), as text. Each seat is a band of two rows in its
; colour: its name and a number, then a bar of that many units ('#', one a
; unit, eleven at most). The Atari's bars step through four amounts, each
; named on the message row as it comes -- the previous amount, after usage,
; after spoilage, after production (which is now) -- and then the store's
; stock, with what each seat will need next round and how short (-) or over
; (+) it is.
;
; Declaring: the stick up is SELL, down is BUY, the button sits out. The
; bell says it is time.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     1
MHASDEC EQU     1
MHASCMP EQU     0

        INCLUDE "../build/tail.inc"

GSTG    EQU     PV+0            ; the auction's stage, as drawn
GK      EQU     PV+1            ; the status step shown (0-4), $FF none
GGOOD   EQU     PV+2
GBELL   EQU     PV+3
GEL0    EQU     PV+4            ; the phase's elapsed jiffies at the arrival
GEL1    EQU     PV+5
GROLE   EQU     PV+6            ; my role as last declared
GVAL    EQU     PV+7            ; [4] each seat's number for the step
GNEED   EQU     PV+11           ; [4] each seat's need next round
GDECL   EQU     PV+15           ; I have declared this stage

RMSG    EQU     15              ; the message row
RFOOT   EQU     18

        ORG     $1000

GENTRY: lda     MENT
        cmp     #ENRUN
        bne     GE1
        jmp     SLOOP
GE1:    cmp     #ENSTATE
        bne     GSETUP
        jmp     GSTATE

GSETUP: lda     #BKGOODS
        sta     MSCR
        lda     #2
        sta     VBLANK
        jsr     DINIT
        jsr     FNCLS
        lda     #$FF
        sta     MBGMSK
        sta     MBAR
        sta     GSTG
        sta     GK
        lda     #0
        sta     SNOPOLL
        sta     GBELL
        jsr     GREAD
        jmp     SLOOPF

GSTATE: jsr     GREAD
        jmp     SLOOP

; GREAD -- a state has arrived: what the screen will draw.
GREAD:  lda     HELAP
        sta     GEL0
        lda     HELAPH
        sta     GEL1
        lda     FNRPLY+SAGOOD
        and     #3
        sta     GGOOD
        lda     FNRPLY+SASTAGE
        cmp     GSTG
        beq     GR1
        sta     GSTG            ; a new stage: the whole screen again
        lda     #$FF
        sta     GK
        lda     #0
        sta     GDECL
        lda     GSTG
        cmp     #STGDECL
        bne     GR1
        lda     #SNDBELL        ; declaring opens: the bell
        jsr     SNDPLAY
; The step: taken here, at an arrival, and not from the vblank when its time
; comes -- the rows read the reply window, which is only whole between an
; arrival and the next request. A poll every 6 frames is fine enough.
GR1:    jsr     GSTEP
        sta     GK
        lda     #1
        sta     MCOMP
        rts

; GSTEP -- the status step the elapsed time says (127/250/373/496).
GSTEP:  jsr     GELAPSED
        ldx     #0
GS1:    lda     MTMP2
        cmp     GSTH,x
        bcc     GS2
        bne     GS1A
        lda     MTMP
        cmp     GSTL,x
        bcc     GS2
GS1A:   inx
        cpx     #4
        bne     GS1
GS2:    txa
        rts
GSTL:   DB      (127)&$FF,(250)&$FF,(373)&$FF,(496)&$FF
GSTH:   DB      (127)>>8,(250)>>8,(373)>>8,(496)>>8

; GELAPSED -- MTMP/MTMP2 = the phase's elapsed jiffies now.
GELAPSED: lda   MFRAME
        sec
        sbc     HRXF
        sta     MTMP
        lda     MFRAMEH
        sbc     HRXFH
        sta     MTMP2
        lda     MTMP
        clc
        adc     GEL0
        sta     MTMP
        lda     MTMP2
        adc     GEL1
        sta     MTMP2
        rts

; ---------------------------------------------------------------------------
; GVAL1 -- seat X's number for step GK (GSTG SHOW), into GVAL,x and A.
; now = goods, made = prod, rot = spoil, use = the need this round:
;   0 previous  = now + rot + use - made
;   1 used      = previous - use
;   2 spoiled   = now - made
;   3 produced  = now (and 4, the store's step)
GVAL1:  stx     MTMP2
        lda     PLOFS,x
        clc
        adc     GGOOD
        tay                     ; Y: this seat's record + the good
        lda     GK
        cmp     #3
        bcc     GV2
        lda     FNRPLY+SPLAYER+PLGOODS,y        ; 3, 4: now
        jmp     GV9
GV2:    cmp     #2
        bne     GV3
        lda     FNRPLY+SPLAYER+PLGOODS,y        ; 2: now - made
        sec
        sbc     FNRPLY+SPLAYER+PLPROD,y
        bcs     GV9
        lda     #0
        beq     GV9
GV3:    lda     HROUND          ; 0, 1: the need this round
        jsr     GNEEDOF
        sta     MTMP
        ldx     MTMP2
        lda     PLOFS,x
        clc
        adc     GGOOD
        tay
        lda     FNRPLY+SPLAYER+PLGOODS,y
        clc
        adc     FNRPLY+SPLAYER+PLSPOIL,y
        bcs     GV8
        clc
        adc     MTMP
        bcs     GV8
        sec
        sbc     FNRPLY+SPLAYER+PLPROD,y
        bcs     GV4
        lda     #0
GV4:    ldx     GK
        beq     GV9             ; previous
        sec                     ; used: previous - use
        sbc     MTMP
        bcs     GV9
        lda     #0
        beq     GV9
GV8:    lda     #255
GV9:    ldx     MTMP2
        sta     GVAL,x
        rts

; GNEEDOF -- A = what seat X needs of the good in round A: food 3 (4 from
; round 4, 5 from round 8, none after 12); energy one for each of its
; M.U.L.E.s making something else; nothing else is needed.
GNEEDOF: ldy    GGOOD
        bne     GN4
        cmp     #13
        bcs     GN0
        cmp     #8
        bcs     GN5
        cmp     #4
        bcs     GN3A
        lda     #3
        rts
GN3A:   lda     #4
        rts
GN5:    lda     #5
        rts
GN0:    lda     #0
        rts
GN4:    cpy     #1
        bne     GN0
        stx     MTMP
        lda     #0
        sta     MNUM
        ldy     #NPLOTS-1
GN6:    lda     FNRPLY+SMOWNER,y
        cmp     MTMP
        bne     GN7
        lda     FNRPLY+SMMULE,y
        bmi     GN7
        cmp     #1
        beq     GN7
        inc     MNUM
GN7:    dey
        bpl     GN6
        lda     MNUM
        rts

; ---------------------------------------------------------------------------
; GCOMPOSE -- one row a frame from the vblank. 1: the title, 2: the good,
; 3-10: the seats' rows, 11: the message, 12: the footer; then HFNEW off.
GCOMPOSE: lda   MCOMP
        bne     GC0
        rts
GC0:    inc     MCOMP
        cmp     #1
        bne     GC2
        lda     #0              ; STATUS #n, or DECLARE
        jsr     FNROWA
        ldx     #(SSTAT)&$FF
        ldy     #(SSTAT)>>8
        jsr     FNSTRXY
        lda     HROUND
        ldx     #2
        jsr     MDEC8
        jmp     FNENDW
GC2:    cmp     #2
        bne     GC3
        lda     #1
        jsr     FNROWA
        lda     #' '
        jsr     FNCHR
        lda     #' '
        jsr     FNCHR
        ldx     GGOOD
        lda     GNAMEL,x
        ldy     GNAMEH,x
        tax
        jsr     FNSTRXY
        jmp     FNENDW
GC3:    cmp     #11
        JCS     GC11
; 3-10: seat (A-3)/2, row (A-3)&1
        sec
        sbc     #3
        lsr     a
        sta     MIDX
        bcs     GCBAR
        tax
        lda     SEATROW,x
        jsr     FNROWA
        ldx     MIDX
        lda     PLOFS,x
        tax
        ldy     #8
GC4:    lda     FNRPLY+SPLAYER+PLNAME,x
        beq     GC5
        jsr     FNCHR
        inx
        dey
        bne     GC4
GC5:    tya
        clc
        adc     #1
        jsr     FNSPC
        lda     GSTG
        bne     GC6
; the status: the step's number (worked out here, a seat a frame)
        ldx     MIDX
        jsr     GVAL1
        ldx     #3
        jsr     MDEC8
        jmp     FNENDW
; declaring (and after): the role
GC6:    ldx     MIDX
        lda     FNRPLY+SAROLE,x
        tax
        lda     ROLEL,x
        ldy     ROLEH,x
        tax
        jsr     FNSTRXY
        jmp     FNENDW
GCBAR:  tax
        lda     SEATROW,x
        clc
        adc     #1
        jsr     FNROWA
        lda     GSTG
        bne     GCB9
        lda     GK
        cmp     #4
        beq     GCNEED
        ldx     MIDX
        lda     GVAL,x
        cmp     #11
        bcc     GCB1
        lda     #11
GCB1:   tax
        beq     GCB9
GCB2:   lda     #'#'
        jsr     FNCHR
        dex
        bne     GCB2
GCB9:   jmp     FNENDW
; the store's step: next round's need, and how short or over
GCNEED: ldx     MIDX
        lda     HROUND
        clc
        adc     #1
        jsr     GNEEDOF
        ldx     MIDX
        sta     GNEED,x
        tay                     ; (a store sets no flags)
        beq     GCB9
        ldx     #(SNEED)&$FF
        ldy     #(SNEED)>>8
        jsr     FNSTRXY
        ldx     MIDX
        lda     GNEED,x
        ldx     #2
        jsr     MDEC8
        lda     #' '
        jsr     FNCHR
        ldx     MIDX
        lda     GVAL,x          ; now
        sec
        sbc     GNEED,x
        bcs     GCN1
        eor     #$FF
        adc     #1
        pha
        lda     #'-'
        bne     GCN2
GCN1:   pha
        lda     #'+'
GCN2:   jsr     FNCHR
        pla
        ldx     #3
        jsr     MDEC8
        jmp     FNENDW

GC11:   bne     GC12
        lda     #RMSG           ; the message
        jsr     FNROWA
        lda     GSTG
        bne     GC11D
        ldx     GK
        cpx     #4
        beq     GC11S
        lda     STEPL,x
        ldy     STEPH,x
        tax
        jsr     FNSTRXY
        jmp     FNENDW
GC11S:  ldx     #(SSTHAS)&$FF
        ldy     #(SSTHAS)>>8
        jsr     FNSTRXY
        ldx     GGOOD
        lda     FNRPLY+SSSTOCK,x
        ldx     #3
        jsr     MDEC8
        jmp     FNENDW
GC11D:  cmp     #STGDECL
        bne     GC11E
        ldx     HSEAT
        cpx     #4
        bcs     GC11E
        ldx     #(SDECL)&$FF
        ldy     #(SDECL)>>8
        jsr     FNSTRXY
GC11E:  jmp     FNENDW
GC12:   lda     #RFOOT          ; the footer, then done
        jsr     FNROWA
        ldx     HSEAT           ; no seat: SPECTATOR
        cpx     #4
        bcc     GC12B
        ldx     #(SSPECT)&$FF
        ldy     #(SSPECT)>>8
        jsr     FNSTRXY
        jmp     GC12A
GC12B:  lda     GSTG
        cmp     #STGDECL
        bne     GC12A
        ldx     #(SFOOT)&$FF
        ldy     #(SFOOT)>>8
        jsr     FNSTRXY
GC12A:  jsr     FNENDW
        lda     #0
        sta     MCOMP
        lda     HFLAGS
        and     #~HFNEW&$FF
        sta     HFLAGS
        rts

; ---------------------------------------------------------------------------
APPVBL: jsr     SNDRUN
        jsr     GCOMPOSE
        jsr     INSCAN
        jsr     RSTCHK
        lda     GSTG
        bne     AP5
        rts
; declaring
AP5:    cmp     #STGDECL
        bne     AP9
        lda     HSEAT
        cmp     #4
        bcs     AP9
        lda     GDECL
        bne     AP9
        lda     MINP
        and     #IN_UP|IN_DOWN|IN_FIRE
        beq     AP9
        ldx     #2              ; up: sell
        and     #IN_UP
        bne     AP6
        ldx     #1              ; down: buy
        lda     MINP
        and     #IN_DOWN
        bne     AP6
        ldx     #0              ; the button: sit out
AP6:    inc     GDECL
        lda     #RQDECL
        ldy     #0
        jmp     QACT
AP9:    rts

STICK:  rts

PLOFS:  DB      0,PLSIZE,2*PLSIZE,3*PLSIZE
SEATROW: DB     3,6,9,12

SSTAT:  DB      "STATUS #",0
GN0S:   DB      "FOOD",0
GN1S:   DB      "ENERGY",0
GN2S:   DB      "SMITHORE",0
GN3S:   DB      "CRYSTITE",0
SNEED:  DB      "NEED",0
SSTHAS: DB      "STORE HAS",0
SDECL:  DB      "UP SELL DN BUY",0
SFOOT:  DB      "FIRE:SIT OUT",0
SSPECT: DB      " SPECTATOR",0
ST0:    DB      "PREVIOUS",0
ST1:    DB      "USAGE",0
ST2:    DB      "SPOILAGE",0
ST3:    DB      "PRODUCTION",0
RL0:    DB      "---",0
RL1:    DB      "BUY",0
RL2:    DB      "SELL",0

MBGT:   DB      CBLACK,CBLACK,CBLACK
        DB      CPLAY0,CPLAY0,CBLACK
        DB      CPLAY1,CPLAY1,CBLACK
        DB      CPLAY2,CPLAY2,CBLACK
        DB      CPLAY3,CPLAY3,CBLACK
        DB      CBLACK,CBLACK,CBLACK,CBLACK,CBLACK,CBLACK,CBLACK

        INCLUDE "mlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "scrloop.inc"
        INCLUDE "disptext.inc"

; ---------------- data: tables, not code (tools/dataranges.py) ----------
DSTART:
GNAMEL: DB      (GN0S)&$FF,(GN1S)&$FF,(GN2S)&$FF,(GN3S)&$FF
GNAMEH: DB      (GN0S)>>8,(GN1S)>>8,(GN2S)>>8,(GN3S)>>8
STEPL:  DB      (ST0)&$FF,(ST1)&$FF,(ST2)&$FF,(ST3)&$FF
STEPH:  DB      (ST0)>>8,(ST1)>>8,(ST2)>>8,(ST3)>>8
ROLEL:  DB      (RL0)&$FF,(RL1)&$FF,(RL2)&$FF
ROLEH:  DB      (RL0)>>8,(RL1)>>8,(RL2)>>8

        END
