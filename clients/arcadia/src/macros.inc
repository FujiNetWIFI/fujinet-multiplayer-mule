; macros.inc -- the macros every module uses. Included first: AS resolves
; forward labels, not forward macros.
;
; Two 2650 rules most of these exist for:
;   - non-branch memory instructions reach only their own 8K page: page-0
;     code (fixed A and the window) names the mailbox and RAM-H through DWBE
;     pointer words, page-1 code (fixed B) names console RAM and RAM-L the
;     same way. Branches carry 15 bits and need no help.
;   - a store into cart space is a bus fight on real hardware (the cart
;     drives D0-D7 whenever A12 is low): cart RAM is written only through
;     the mailbox, by RAMPUT (fixb.inc). tools/checkstore.py fails the build
;     on a STRA into $0000-$0FFF or $2000-$2FFF.

; Big-endian word: what the 2650's indirection reads. DW is little-endian
; on this target, so never use DW for anything the CPU dereferences.
DWBE    MACRO   val
        DB      ((val)>>8)&$FF, (val)&$FF
        ENDM

; Strings in DB are glyph codes, not ASCII: the 2637's ROM font, space $00,
; digits $10, letters $1A, then . , + $, and / as the whole-cell diagonal.
; CHARSET remaps character constants too ('F' becomes $1F), so code that
; compares ASCII (the network's, the appkeys') writes the numbers. ASCII
; turns the mapping off for a stretch, GLYPHS back on.
GLYPHS  MACRO
        CHARSET ' ',0
        CHARSET '0','9',$10
        CHARSET 'A','Z',$1A
        CHARSET '.',$34
        CHARSET ',',$35
        CHARSET '+',$36
        CHARSET '$',$37
        CHARSET '/',$01
        ENDM
ASCII   MACRO
        CHARSET
        ENDM

; PSL: logical compares (COM) always; WC (carry into ADD/SUB and the
; rotates) only inside 16-bit arithmetic, which turns it off again.
PSLBASE EQU     $02
PSLWC   EQU     $08
PSLC    EQU     $01

; A cart RAM byte = R0, by name (RAM-L $0600-$07FF or RAM-H $2900-$2AFF).
; Calls RAMPUT (fixed B): clobbers R0-R3. 9 bytes.
CPUT    MACRO   adr
        IF      (adr) >= $2900
        LODI,R1 FRRAMP2+(((adr)-$2900)>>8)
        LODI,R3 ((adr)-$2900)&$FF
        ELSE
        LODI,R1 FRRAMP0+(((adr)-$0600)>>8)
        LODI,R3 ((adr)-$0600)&$FF
        ENDIF
        BSTA,UN RAMPUT
        ENDM

; A RAM-L byte from $0700 up (the bookkeeping past the state's copy) = R0.
; 4 bytes. Clobbers R0-R3.
LPUT    MACRO   adr
        IF      ((adr) < $0700) || ((adr) > $07FF)
        ERROR   "LPUT is for RAM-L $0700-$07FF"
        ENDIF
        LODI,R3 (adr)&$FF
        ZBSR    *ZLPUT
        ENDM

; The cart RAM pointer at a name, then RAMPUT1 stores R0 there and steps
; on, a burst RAMPEND closes. (fixb.inc)
CPTR    MACRO   adr
        IF      (adr) >= $2900
        LODI,R1 FRRAMP2+(((adr)-$2900)>>8)
        LODI,R3 ((adr)-$2900)&$FF
        ELSE
        LODI,R1 FRRAMP0+(((adr)-$0600)>>8)
        LODI,R3 ((adr)-$0600)&$FF
        ENDIF
        BSTA,UN RAMSET
        ENDM

; Queue a sound. 4 bytes.
SND     MACRO   id
        LODI,R0 id
        ZBSR    *ZSNDQ
        ENDM

; Point the display at (row, col): DROW holds the row's base, R2 the cell
; before col (the pre-increment convention of every screen store).
DAT     MACRO   row,col
        LODI,R1 row
        LODI,R2 (col)-1
        ZBSR    *ZDPOS
        ENDM

DCOL    MACRO   attr
        LODI,R0 attr
        STRA,R0 DCOLOR
        ENDM

; DTXT a glyph string at a label: R0:R1 = its address.
TXTAT   MACRO   adr
        LODI,R0 (adr)>>8
        LODI,R1 (adr)&$FF
        ZBSR    *ZDTXT
        ENDM

; 16-bit add/subtract of constants on a register pair (hi, lo).
ADD16I  MACRO   rh,rl,v
        PPSL    PSLWC
        CPSL    PSLC
        ADDI,rl (v)&$FF
        ADDI,rh ((v)>>8)&$FF
        CPSL    PSLWC
        ENDM
