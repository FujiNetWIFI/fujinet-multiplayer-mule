; dev.asm -- bank 5: your turn in development, on the map: the picture.
;
; MekkoGX develop.c, in three banks. This one draws: the map kernel, the
; time bar, the figures' shapes at fixed addresses (devaddr.inc). Bank 11
; thinks -- a state's arrival, the walk, the button, the wampus, where the
; figures stand and which shape they show -- once a frame, from this bank's
; overscan (STICK), and comes back here to carry on (ENPOST). The town is
; bank 6, drawn by the same kernel and walked by the same bank 11. The text
; is bank 10's.

        CPU     6502UNDOC
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     1
MHASDEC EQU     0
MHASCMP EQU     1
MHASMSG EQU     1
MHASTXT EQU     0               ; bank 10 draws the text
TICKROW EQU     1

        INCLUDE "../build/tail.inc"
        INCLUDE "devram.inc"
        INCLUDE "devaddr.inc"

        ORG     $1000

DENTRY: lda     MENT
        cmp     #ENRUN
        bne     DE1
        jmp     SLOOP
DE1:    cmp     #ENPOST
        bne     DE2
        jmp     SLOOP1
DE2:    cmp     #ENSTATE
        beq     DHOP            ; a state: bank 11 reads it
        cmp     #ENTOWN
        beq     DFROMT
; ENENTER: my turn has come. The screen, then bank 11 sets the turn up.
; (The kernel shows text rows 0, 1 and 17 only, and bank 10 redraws them;
; the rest of the planes are the map, which the first state repaints.)
        jsr     DSCREEN
        jmp     DHOP
; ENTOWN: out of the town: the map's tables and text are still in the
; planes, so only the kernel's setup; bank 11 puts the figure by the store.
DFROMT: jsr     DSCREEN
DHOP:   lda     #BKDEVX
        jmp     MGOTO

; DSCREEN -- the map screen's setup, and a fresh overscan.
DSCREEN: lda    #BKDEV
        sta     MSCR
        lda     #2
        sta     VBLANK
        jsr     DINIT
        lda     #0
        sta     SNOPOLL
        sta     FIGNUS
        sta     MULREF
        sta     BARPF1
        sta     BARPF2
        sta     BARPF1R
        sta     BARPF2R
        lda     #CWHITE
        sta     TXCOL
        sta     MULCOL
        lda     #8
        sta     P0HV
        lda     #200
        sta     P0TOP
        sta     P1TOP
        lda     #(DVFIGS)&$FF
        sta     P0PTR
        sta     P1PTR
        lda     #(DVFIGS)>>8
        sta     P0PTRH
        sta     P1PTRH
        lda     #OSTIM
        sta     TIM64T
        lda     #0
        sta     VBLANK
        rts

; ---------------------------------------------------------------------------
APPVBL: jsr     SNDRUN
        jsr     INSCAN
        jsr     RSTCHK
        jsr     TICKER
        jmp     DBAR

; STICK -- bank 11, once a frame.
STICK:  lda     #ENRUN
        sta     MENT
        lda     #BKDEVX
        jmp     MGOTO

        INCLUDE "devbar.inc"
        INCLUDE "ticker.inc"

MBGT:   DB      0

MT0     EQU     FNTEXT+$000+MAP0
MT1     EQU     FNTEXT+$080+MAP0
MT2     EQU     FNTEXT+$100+MAP0
MT3     EQU     FNTEXT+$180+MAP0
MT4     EQU     FNTEXT+$200+MAP0
MT5     EQU     FNTEXT+$280+MAP0

        INCLUDE "mlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "scrloop.inc"
        INCLUDE "dispmap.inc"

; ---------------- data at fixed addresses (devaddr.inc) ----------------
        IF      * > DVDATA
        ERROR   "dev.asm: the code runs into the data at DVDATA"
        ENDIF
DSTART:
        ORG     DVCOL
        INCLUDE "../build/mapcol.inc"
        ORG     DVFIGS
        INCLUDE "../build/gen/mapspr.inc"

        END
