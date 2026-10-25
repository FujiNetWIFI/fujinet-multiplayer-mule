; say.inc -- SAY: a message from the strings pages, word-wrapped and centred
; in the message box (MSGROW, MSGN rows), the box cleared first (mekkogx
; msg_show; the Arcadia's 16 columns have room for the Atari's whole
; wording, a line at a time, where its 40 scrolled).
;
; Strings are glyph codes (tools/mkstr.py): $00 a space, $FE the first
; number (SAYV1), $FD the second (SAYV2), $FF the end. No word is wider
; than a line (mkstr.py refuses one). A string's page is STRPG + id / 64;
; the page begins with its strings' addresses.

; SAYV: SAY with SAYV1 = R2:R3 first. Calls three down. Clobbers R0-R3,
; T0-T3, DSTR*, NUM*.
SAYV:   STRA,R1 T2
        STRA,R3 T3
        LODZ    R2
        CPUT    SAYV1H
        LODA,R0 T3
        CPUT    SAYV1L
        LODA,R1 T2
; SAY: message R1 in the box. SAYBX: in R3 rows from row R2 instead.
; Calls two down. Clobbers as SAYV.
SAY:    LODA,R2 MSGROW
        LODA,R3 MSGN
SAYBX:  STRA,R2 SAYROW
        STRA,R3 SAYLFT
        LODZ    R1              ; its page, borrowed (this is fixed code)
        RRL,R0
        RRL,R0
        ANDI,R0 3
        ADDI,R0 STRPG
        STRZ    R3
        LODA,R0 *PBANK,R3
        LODZ    R1
        ANDI,R0 $3F
        RRL,R0
        STRZ    R3
        LODA,R0 WIN,R3          ; where it is
        STRA,R0 DSTRH
        LODA,R0 WIN+1,R3
        STRA,R0 DSTRL
        LODA,R0 SAYV1H          ; the numbers' widths
        LODA,R1 SAYV1L
        BSTA,UN SAYCNT
        RRL,R0
        RRL,R0
        RRL,R0
        RRL,R0
        STRA,R0 SAYD
        LODA,R0 SAYV2H
        LODA,R1 SAYV2L
        BSTA,UN SAYCNT
        IORA,R0 SAYD
        STRA,R0 SAYD
        LODA,R1 SAYROW          ; the box, cleared
        LODA,R3 SAYLFT
        BSTA,UN DCLR
        LODI,R0 $FF             ; T0: the character before the line
        STRA,R0 T0
; a line: as many words as fit in 16
SAYLN:  LODA,R3 T0
        LODI,R1 0               ; R1: the line's width so far
        STRA,R3 T1              ; T1: its last character (none yet)
SAYWD:  LODI,R2 0               ; R2: the next word's width
SAYW1:  LODA,R0 *DSTRH,R3,+
        COMI,R0 $FF
        BCTR,EQ SAYWE
        COMI,R0 0
        BCTR,EQ SAYWE
        COMI,R0 $FE
        BCTR,EQ SAYW2
        COMI,R0 $FD
        BCTR,EQ SAYW3
        ADDI,R2 1
        BCTR,UN SAYW1
SAYW2:  LODA,R0 SAYD            ; the first number's digits
        RRR,R0
        RRR,R0
        RRR,R0
        RRR,R0
        BCTR,UN SAYW4
SAYW3:  LODA,R0 SAYD
SAYW4:  ANDI,R0 $0F
        ADDZ    R2
        STRZ    R2
        BCTR,UN SAYW1
SAYWE:  LODZ    R1              ; does it fit? width + a space + the word
        BCTR,EQ SAYW5
        ADDI,R0 1
SAYW5:  ADDZ    R2
        COMI,R0 16
        BCTR,GT SAYBR           ; no: the line ends before it
        STRZ    R1
        STRA,R3 T1              ; T1: the terminator...
        LODA,R0 T1
        SUBI,R0 1
        STRA,R0 T1              ; ...less one: the word's last character
        LODA,R0 *DSTRH,R3       ; the end of the string: the last line
        COMI,R0 $FF
        BCFA,EQ SAYWD           ; a space: on to the next word
        BSTR,UN SAYPR
        BCTR,UN SAYDN
SAYBR:  LODZ    R1
        BCTR,EQ SAYDN           ; a word wider than a line (mkstr.py refuses them)
        BSTR,UN SAYPR
        LODA,R0 T1              ; the next line starts after the space
        ADDI,R0 1
        STRA,R0 T0
        LODA,R0 SAYLFT
        BCFA,EQ SAYLN
SAYDN:  LODA,R3 CURPG           ; the window back
        LODA,R0 *PBANK,R3
        RETC,UN

; print the line T0+1..T1, R1 wide, centred on SAYROW; then the next row.
; Calls three down. Clobbers R0-R3, T2, NUM*.
SAYPR:  LODA,R0 SAYLFT
        RETC,EQ                 ; the box is full: the rest is dropped
        SUBI,R0 1
        STRA,R0 SAYLFT
        LODI,R0 16              ; the first cell: (16 - width) / 2
        SUBZ    R1
        ANDI,R0 $FE
        RRR,R0
        SUBI,R0 1
        STRZ    R2
        LODA,R1 SAYROW
        BSTA,UN DPOS
        ADDI,R1 1
        STRA,R1 SAYROW
        LODA,R0 T0
        STRA,R0 T2              ; T2: where we are
SAYP1:  LODA,R3 T2
        COMA,R3 T1
        RETC,EQ                 ; (R3 is the character before: at T1, done)
        LODA,R0 *DSTRH,R3,+
        STRA,R3 T2
        COMI,R0 $FE
        BCTR,EQ SAYP2
        COMI,R0 $FD
        BCTR,EQ SAYP3
        BSTA,UN DPUTC
        BCTR,UN SAYP1
SAYP2:  LODA,R0 SAYV1H
        LODA,R1 SAYV1L
        BCTR,UN SAYP4
SAYP3:  LODA,R0 SAYV2H
        LODA,R1 SAYV2L
SAYP4:  BSTR,UN SAYNUM
        BCTR,UN SAYP1
; NUM = R0:R1, R1 = its digits (and R0 too). Calls two down. Clobbers R3.
SAYNUM: STRA,R0 NUMM
        STRA,R1 NUML
        EORZ    R0
        STRA,R0 NUMH
        BSTA,UN DNCNT
        BCTA,UN DNPRT           ; (SAYP4's call: print them)
SAYCNT: STRA,R0 NUMM            ; (SAY's: just count)
        STRA,R1 NUML
        EORZ    R0
        STRA,R0 NUMH
        BSTA,UN DNCNT
        LODZ    R1
        RETC,UN
