; phase.asm -- a placeholder screen for the phases not yet drawn: the phase
; and round as text, polling, RESET held to leave.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     1
MHASDEC EQU     1
MHASCMP EQU     0

        INCLUDE "../build/tail.inc"

        ORG     $1000

PENTRY: lda     MENT
        cmp     #ENRUN
        bne     PE1
        jmp     SLOOP
PE1:    cmp     #ENSTATE
        beq     PSTATE
        lda     FNBBNK          ; the live bank: this is it
        and     #$0F
        sta     MSCR
        lda     #2
        sta     VBLANK
        jsr     DINIT
        jsr     FNCLS
        lda     #0
        sta     SNOPOLL
        sta     MBGMSK
PSTATE: lda     #4
        jsr     FNROWA
        ldx     #(SPH)&$FF
        ldy     #(SPH)>>8
        jsr     FNSTRXY
        lda     HPHASE
        ldx     #2
        jsr     MDEC8
        jsr     FNENDW
        lda     #6
        jsr     FNROWA
        ldx     #(SRD)&$FF
        ldy     #(SRD)>>8
        jsr     FNSTRXY
        lda     HROUND
        ldx     #2
        jsr     MDEC8
        jsr     FNENDW
        lda     HFLAGS
        and     #~HFNEW&$FF
        sta     HFLAGS
        lda     #OSTIM
        sta     TIM64T
        lda     #0
        sta     VBLANK
        jmp     SLOOP

APPVBL: jsr     SNDRUN
        jsr     INSCAN
        jsr     RSTCHK
; development, my turn (a stand-in): FIRE begins it, SELECT ends it
        lda     HPHASE
        cmp     #PHDEV
        bne     PA9
        lda     MINP
        and     #IN_FIRE
        beq     PA1
        lda     #RQBEGIN
        bne     PA2
PA1:    lda     MINP
        and     #IN_SEL
        beq     PA9
        lda     #RQEND
PA2:    ldx     #0
        ldy     #0
        jmp     QACT
PA9:    rts
STICK:  rts

SPH:    DB      "PHASE ",0
SRD:    DB      "ROUND ",0
MBGT:   DB      0

        INCLUDE "mlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "scrloop.inc"
        INCLUDE "disptext.inc"

        END
