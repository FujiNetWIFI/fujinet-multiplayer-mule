; strpev.asm -- bank 13: the player events, the message a turn starts with.
;
; turn.event k (1-22), in the Atari's words, "*** " before. The amount is
; x * 25 * (round / 4 + 1) (MekkoGX develop.c, text.c pevent_x); three
; events count M.U.L.E.s instead: 100 for each food M.U.L.E. (x 2, and the
; total), 101 for each mining one (x 2), 102 for each energy one.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     0
MHASDEC EQU     0
MHASTXT EQU     0
MHASSRUN EQU    0

        INCLUDE "../build/tail.inc"

; Scratch: the request machine's cells, idle whenever a message is made (a
; state has just arrived); the screens' own cells are not ours to touch.
SNUMS   EQU     NPRV0           ; strcode.inc's: which number is next
SY      EQU     NPRV1
SNUM2   EQU     NARG            ; the second number
SNUM2H  EQU     NARGH
SBASE   EQU     NAVL0           ; 25 * (round / 4 + 1)
SCNT    EQU     NAVL1           ; M.U.L.E.s counted

        ORG     $1000

        INCLUDE "strcode.inc"

; STRTAB -- event X (1-22): its words, and its amounts.
STRTAB: dex
        lda     SPEVL,x
        sta     FNPTRL
        lda     SPEVH,x
        sta     FNPTRH
        lda     HROUND          ; 25 * (round / 4 + 1)
        lsr     a
        lsr     a
        tay
        lda     #0
        clc
ST1:    adc     #25
        dey
        bpl     ST1
        sta     SBASE
        lda     PEVX,x
        cmp     #100
        bcs     ST5
        tay                     ; x times the base
        lda     #0
        sta     MNUM
        sta     MNUMH
        cpy     #0
        beq     ST9
ST2:    lda     MNUM
        clc
        adc     SBASE
        sta     MNUM
        bcc     ST3
        inc     MNUMH
ST3:    dey
        bne     ST2
        rts
; per M.U.L.E.: count mine of the kind
ST5:    sta     MTMP2           ; 100 food, 101 mining, 102 energy
        lda     #0
        sta     SCNT
        ldx     #NPLOTS-1
ST6:    lda     FNRPLY+SMOWNER,x
        cmp     HSEAT
        bne     ST8
        lda     FNRPLY+SMMULE,x
        bmi     ST8
        ldy     MTMP2
        cpy     #100
        bne     ST6A
        cmp     #0              ; food
        beq     ST7
        bne     ST8
ST6A:   cpy     #102
        bne     ST6B
        cmp     #1              ; energy
        beq     ST7
        bne     ST8
ST6B:   cmp     #2              ; smithore or crystite
        bcc     ST8
ST7:    inc     SCNT
ST8:    dex
        bpl     ST6
        lda     #0              ; base * count
        sta     MNUM
        sta     MNUMH
        ldy     SCNT
        beq     ST8B
ST8A:   lda     MNUM
        clc
        adc     SBASE
        sta     MNUM
        bcc     ST8C
        inc     MNUMH
ST8C:   dey
        bne     ST8A
ST8B:   lda     MTMP2
        cmp     #102
        beq     ST9             ; energy: base * count
        asl     MNUM            ; food and mining: twice that is the total
        rol     MNUMH
        lda     MNUM
        sta     SNUM2
        lda     MNUMH
        sta     SNUM2H
        lda     MTMP2
        cmp     #101
        beq     ST8D
        lda     SBASE           ; food: base * 2 each, the total second
        asl     a
        sta     MNUM
        lda     #0
        rol     a
        sta     MNUMH
        rts
ST8D:   rts                     ; mining: the total is the one number
ST9:    rts

; STRPRE -- "*** " before an event.
STRPRE: ldx     #3
        lda     #'*'
SP1:    sta     FNRSEL+FH_PATHC
        dex
        bne     SP1
        lda     #' '
        sta     FNRSEL+FH_PATHC
        rts

; the amount's multiplier, by event (text.c pevent_x)
PEVX:   DB      0,0,2,4,100,4,8,2,3,6,4,2,0,0,3,101,102,6,4,4,4,0

        INCLUDE "mlib.inc"

        INCLUDE "../build/gen/strpev.inc"
; ---------------- data: tables, not code (tools/dataranges.py) ----------
        INCLUDE "../build/gen/strpevp.inc"

        END
