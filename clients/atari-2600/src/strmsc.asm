; strmsc.asm -- bank 14: the messages that are not player events: the
; landing, the grant, whose turn it is, time, the colony events, the bids,
; the colony's rating. MNUM is the number a '%' prints.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     0
MHASDEC EQU     0
MHASTXT EQU     0
MHASSRUN EQU    0

        INCLUDE "../build/tail.inc"

; Scratch: the request machine's (see strpev.asm).
SNUMS   EQU     NPRV0
SY      EQU     NPRV1
SNUM2   EQU     NARG
SNUM2H  EQU     NARGH
SCNTX   EQU     NAVL1

        ORG     $1000

        INCLUDE "strcode.inc"

; STRTAB -- message X. Player events 20-22 (27-29) are here too: two
; cost 4 x 25 x (round / 4 + 1), the third nothing.
STRTAB: lda     SMSCL,x
        sta     FNPTRL
        lda     SMSCH,x
        sta     FNPTRH
        stx     SEVID
        cpx     #MSEV20
        bcc     STR9
        lda     #0
        sta     MNUMH
        lda     HROUND
        lsr     a
        lsr     a
        tay
        lda     #0
        clc
ST1:    adc     #100            ; 4 x 25
        bcc     ST2
        inc     MNUMH
ST2:    dey
        bpl     ST1
        sta     MNUM
STR9:   rts

; STRPRE -- "*** " before a player event.
STRPRE: lda     SEVID
        cmp     #MSEV20
        bcc     SP9
        ldx     #3
        lda     #'*'
SP1:    sta     FNRSEL+FH_PATHC
        dex
        bne     SP1
        lda     #' '
        sta     FNRSEL+FH_PATHC
SP9:    rts
SEVID   EQU     SCNTX

        INCLUDE "mlib.inc"

        INCLUDE "../build/gen/strmsc.inc"
; ---------------- data: tables, not code (tools/dataranges.py) ----------
        INCLUDE "../build/gen/strmscp.inc"

        END
