; ---------------------------------------------------------------------------
; text.inc -- characters, strings and numbers on the screen.
;
; The family's 3x5 font in a 4x6 cell, drawn opaque (the gap column and row
; are ground), so text needs no clearing first. A text cell tx is at pixel
; column TX0 + 4 * tx: TCOLS cells across the visible area. Rows are pixel
; rows: every screen lays its own lines out (the Atari's 24 rows do not fit
; 58 lines of 6).
;
; Lower case prints as upper case.
; ---------------------------------------------------------------------------

TX0     EQU 6                   ; pixel column of text cell 0
TCOLS   EQU 24                  ; 6..101
TITLEY  EQU 4                   ; the title's row

; ---------------------------------------------------------------------------
; DGLYPH -- character r0 at pixel (r1, r2), colour r6.  Leaf.
;   Leaves r1, r6 and r2 + 6.  Clobbers A, r0, r3-r5, r7-r9, H, Q, DC0.
; ---------------------------------------------------------------------------
DGLYPH: LR A,0
        AI 0E0H                 ; - $20
        CI 03FH
        BC DGL1                 ; $20-$5F
        AI 0E0H                 ; lower case: - $20 again
        CI 03FH
        BC DGL1
        LI 01FH                 ; '?'
DGL1:   DCI FNTR0
        ADC
        LR Q,DC                 ; this glyph's row 0
        LIS 6
        LR 8,A                  ; rows to go
DGROW:  DCI ROWKEY              ; the colour's value on this row
        LR A,2
        ADC
        LM
        AS 6
        DCI COLTAB
        ADC
        LM
        LR 7,A
        LR A,8                  ; the sixth row is the gap
        CI 1
        BZ DGGAP
        LR DC,Q
        LM
        SL 4
        SL 1                    ; pixels in bits 7, 6, 5
        LR 4,A
        LI 63                   ; the next row table (LM moved one already)
        ADC
        LR Q,DC
        BR DGPIX
DGGAP:  CLR
        LR 4,A
DGPIX:  PORTS
        MARKNZ
        LIS 4
        LR 5,A
DGPX:   LR A,4
        NI 080H
        BZ DGBG
        LR A,7
        BR DGV
DGBG:   CLR
DGV:    LR 3,A                  ; the value wanted
        LR A,4
        SL 1
        LR 4,A
        LM
        XS 3
        BZ DGSK
        LI 0FFH
        ADC
        LR A,3
        ST
        SL 4
        SL 1
        SL 1
        COM
        OUTS 1
        LR A,9
        OUTS 4
        LI 060H
        OUTS 0
        LI 050H
        OUTS 0
        SETTLE
DGSK:   DS 9
        DS 5
        BNZ DGPX
        LR A,2
        INC
        LR 2,A
        DS 8
        BNZ DGROW
        POP

; ---------------------------------------------------------------------------
; DTEXT -- the NUL-terminated string at DC0, from pixel (r1, r2), colour r6.
;   Leaves r1 after the last character.  Clobbers A, r0, r3-r9, H, Q, DC0, DC1.
;   (STRLEN keeps nothing but r1-r4 and Q needed here.)
; ---------------------------------------------------------------------------
DTEXT:  ENTER
        LR Q,DC                 ; a sprite under it is drawn again after
        PI STRLEN
        SL 1
        SL 1
        LR 3,A
        LIS 6
        LR 4,A
        PI SPRTOUCH
        LR DC,Q
        XDC                     ; DC1 = the string
        LR A,2                  ; a row below 58 would run the shadow past
        CI 58                   ; $9FFF, into the strings: not drawn
        BNC DTXX
DTXL:   LR A,1                  ; past the right edge: stop
        CI TX0+4*(TCOLS-1)
        BNC DTXX
        XDC
        LM
        XDC
        NI 0FFH
        BZ DTXX
        LR 0,A
        PI DGLYPH
        LR A,2
        AI 0FAH                 ; back up the 6 rows
        LR 2,A
        LR A,1
        AI 4
        LR 1,A
        BR DTXL
DTXX:   LEAVE

; ---------------------------------------------------------------------------
; DTEXTN -- at most r4 characters of the string at DC0 (a NUL ends it
;   sooner): copied to LINBUF, then DTEXT.  Clobbers as DTEXT, and r4.
; ---------------------------------------------------------------------------
DTEXTN: XDC                     ; DC1 = the string
        DCI LINBUF
        LR A,4
        NI 0FFH
        BZ DTNX
DTNL:   XDC
        LM
        XDC
        ST
        NI 0FFH
        BZ DTNG
        DS 4
        BNZ DTNL
DTNX:   CLR
        ST
DTNG:   DCI LINBUF
        JMP DTEXT               ; (a leaf until here: DTEXT returns for us)

; ---------------------------------------------------------------------------
; DCLRT -- r3 text cells cleared from pixel (r1, r2): the ground.  r3 >= 1.
;   Clobbers A, r2-r9, H, DC0.
; ---------------------------------------------------------------------------
DCLRT:  ENTER
        LR A,3
        SL 1
        SL 1
        LR 3,A
        LIS 6
        LR 4,A
        CLR
        LR 0,A
        PI DFILL
        LEAVE

; ---------------------------------------------------------------------------
; FMTNUM -- r0:r1 in decimal at DC0, NUL-terminated; A = its length.
;   Clobbers r0-r6, DC0, DC1.
; ---------------------------------------------------------------------------
FMTNUM: ENTER
        XDC                     ; DC1 = where the digits go
        CLR
        LR 6,A                  ; digits so far
        DCI POW10
        LIS 4
        LR 5,A
FNP:    LM
        LR 3,A
        LM
        LR 2,A                  ; r2:r3 = the power
        LI '0'
        LR 4,A
FND:    PI GE16
        BNC FNE
        PI SUB16
        LR A,4
        INC
        LR 4,A
        BR FND
FNE:    LR A,4                  ; no leading zeros
        CI '0'
        BNZ FNW
        LR A,6
        NI 0FFH
        BZ FNS
FNW:    XDC
        LR A,4
        ST
        XDC
        LR A,6
        INC
        LR 6,A
FNS:    DS 5
        BNZ FNP
        LR A,0                  ; the units
        AI '0'
        XDC
        ST
        CLR
        ST
        LR A,6
        INC
        LEAVE

POW10:  DW 10000, 1000, 100, 10

; ---------------------------------------------------------------------------
; PUTNUM -- r0:r1 right-aligned in r4 cells: DTMP+0 = the first text cell,
;   r2 = pixel row, r6 = colour.  Spaces in front; a number wider than the
;   field keeps its last r4 digits.
;   Clobbers A, r0-r9, H, Q, DC0, DC1.
; ---------------------------------------------------------------------------
PUTNUM: ENTER
        LR A,4
        STB DTMP+1              ; the width
        LR A,2
        STB DTMP+2              ; the row
        LR A,6
        STB DTMP+3              ; the colour
        DCI LINBUF+24
        PI FMTNUM               ; A = digits
        LR 5,A
        LDB DTMP+1
        LR 4,A                  ; width
        ; skip = digits > width ? digits - width : 0
        LR A,4
        COM
        AS 5                    ; carries when digits > width
        BNC PNFIT
        INC                     ; digits - width
        LR 3,A                  ; digits to skip
        LR A,4
        LR 5,A                  ; digits shown = width
        CLR
        BR PNPAD
PNFIT:  CLR
        LR 3,A
        LR A,5                  ; pad = width - digits
        COM
        INC
        AS 4
PNPAD:  LR 8,A                  ; pad
        DCI LINBUF
        LR A,8
        NI 0FFH
        BZ PNCOPY
        LI ' '
PNSP:   ST
        DS 8
        BNZ PNSP
PNCOPY: XDC                     ; DC1 = after the padding
        DCI LINBUF+24
        LR A,3
        ADC
PNCL:   LM
        XDC
        ST
        XDC
        NI 0FFH
        BNZ PNCL
        LDB DTMP+0              ; the cell
        SL 1
        SL 1
        AI TX0
        LR 1,A
        LDB DTMP+2
        LR 2,A
        LDB DTMP+3
        LR 6,A
        DCI LINBUF
        PI DTEXT
        LEAVE

; ---------------------------------------------------------------------------
; STRP -- DC0 = string r0 of the pointer table at DC0 (gen/str.inc: STT
;   titles, STM messages, STW words, STG goods, STS species, STP player
;   events, STR ratings).  Leaf.  Clobbers A, H.
; ---------------------------------------------------------------------------
STRP:   LR A,0
        SL 1
        ADC
        LM
        LR 10,A
        LM
        LR 11,A
        LR DC,H
        POP

; STRLEN -- A = the length of the string at DC0.  Leaf.  Clobbers A, r5, DC0.
STRLEN: CLR
        LR 5,A
STRL1:  LM
        NI 0FFH
        BZ STRLX
        LR A,5
        INC
        LR 5,A
        BR STRL1
STRLX:  LR A,5
        POP

; ---------------------------------------------------------------------------
; TXTEXP -- the string at DC0 copied to DC1, each '%' replaced by a number:
;   EXPV1, then EXPV2 (text.c text_expand).  A = the length (at most 120).
;   Clobbers r0-r6, Q, DC0, DC1.
; ---------------------------------------------------------------------------
TXTEXP: ENTER
        LR Q,DC
        CLR
        STB EXPN
        LR DC,Q
TXEL:   LM
        NI 0FFH
        BZ TXEX
        CI '%'
        BZ TXEP
        XDC
        ST
        XDC
        LR Q,DC
        INCB EXPN
        LR DC,Q
        CI 119
        BC TXEL                 ; up to 120
        BR TXEX
TXEP:   LR Q,DC                 ; (the source, after the '%')
        LDW 0, 1, EXPV1
        LDW 2, 3, EXPV2
        STW 2, 3, EXPV1
        XDC
        PI FMTNUM               ; DC0 past the NUL
        LR 5,A
        LI 0FFH
        ADC
        XDC                     ; DC1: the NUL, the next character's place
        DCI EXPN
        LM
        AS 5
        DCI EXPN
        ST
        LR DC,Q
        BR TXEL
TXEX:   XDC
        CLR
        ST
        LDB EXPN
        LEAVE

; ---------------------------------------------------------------------------
; TCENTER -- the string at DC0 centred on pixel row r2 in colour r6, the
;   row cleared first; a line wider than the screen is cut at a space and
;   goes on centred on the next row (text.c text_center).
;   Clobbers A, r0-r9, H, Q, DC0, DC1.
; ---------------------------------------------------------------------------
TCENTER: ENTER
TCEN0:  LR H,DC                 ; the string first: STB moves DC0
        STW 10, 11, TCPTR
        LR A,2
        STB TCROW
        LR A,6
        STB TCCOL
        LI TX0                  ; the row cleared
        LR 1,A
        LI TCOLS
        LR 3,A
        PI DCLRT
        LDW 10, 11, TCPTR
        LR DC,H
        PI STRLEN
        STB TCLEN
        CI TCOLS
        BC TCFIT
        LI TCOLS                ; too wide: the last space at or before TCOLS
        LR 4,A
TCSP:   LDW 10, 11, TCPTR
        LR DC,H
        LR A,4
        ADC
        LM
        CI ' '
        BZ TCCUT1
        DS 4
        BNZ TCSP
        BR TCFIT                ; no space: as it comes (the edge cuts it)
TCCUT1: LR A,4
        STB TCCUT
TCSB:   LR A,4                  ; n = cut, less the spaces before it
        NI 0FFH
        BZ TCSB2
        LDW 10, 11, TCPTR
        LR DC,H
        LR A,4
        AI 0FFH
        ADC
        LM
        CI ' '
        BNZ TCSB2
        DS 4
        BR TCSB
TCSB2:  LR A,4                  ; centred: x = TX0 + 4 * ((TCOLS - n) / 2)
        COM
        INC
        AI TCOLS
        SR 1
        SL 1
        SL 1
        AI TX0
        LR 1,A
        LDB TCROW
        LR 2,A
        LDB TCCOL
        LR 6,A
        LDW 10, 11, TCPTR
        LR DC,H
        PI DTEXTN
        LDB TCCUT               ; the rest, past the spaces, on the next row
        LR 4,A
        LDW 10, 11, TCPTR
        LR DC,H
        LR A,4
        ADC
TCSK:   LM
        CI ' '
        BZ TCSK
        LI 0FFH
        ADC
        LR H,DC
        LDB TCROW
        AI 6
        LR 2,A
        LDB TCCOL
        LR 6,A
        LR DC,H
        JMP TCEN0
TCFIT:  LDB TCLEN               ; x = TX0 + 4 * ((TCOLS - len) / 2), or TX0
        CI TCOLS
        LI 0
        BNC TCF1
        LDB TCLEN
        COM
        INC
        AI TCOLS
        SR 1
TCF1:   SL 1
        SL 1
        AI TX0
        LR 1,A
        LDB TCROW
        LR 2,A
        LDB TCCOL
        LR 6,A
        LDW 10, 11, TCPTR
        LR DC,H
        PI DTEXT
        LEAVE

; ---------------------------------------------------------------------------
; The message line (text.c): two text rows from MSGROW; a message longer
; than both scrolls in from the right along the second.
; ---------------------------------------------------------------------------

; MSGCLR -- both rows cleared, the scroll stopped.
MSGCLR: ENTER
        CLR
        STB MSGSCR
        LDB MSGONE              ; one row: the first is someone else's
        NI 0FFH
        BNZ MSCL1
        LDB MSGROW
        LR 2,A
        LI TX0
        LR 1,A
        LI TCOLS
        LR 3,A
        PI DCLRT
MSCL1:  LDB MSGROW
        AI 6
        LR 2,A
        LI TX0
        LR 1,A
        LI TCOLS
        LR 3,A
        PI DCLRT
        LEAVE

; MSGSHOW -- message r0 with the numbers EXPV1, EXPV2 (text.c msg_show).
MSGSHOW: ENTER
        DCI STM
        PI STRP
        XDC
        DCI TXT
        XDC
        PI TXTEXP
        LR 0,A
        TAIL MSGSTRT

; MSGSTRT -- the message already in TXT, r0 characters long (msg_start):
;   centred on one row, word-wrapped onto two, or scrolling.
MSGSTRT: ENTER
        LR A,0
        STB MSGLEN
        PI MSGCLR
        LDB MSGONE              ; one row (MSGONE): longer than it scrolls
        NI 0FFH
        BZ MSST0
        LDB MSGLEN
        CI TCOLS
        BNC MSSCR
        LDB MSGROW
        AI 6
        BR MSST2
MSST0:  LDB MSGLEN
        CI 2*TCOLS
        BC MSST1
MSSCR:  LIS 1                   ; too long for two rows: scroll it
        STB MSGSCR
        CLR
        STB MSGPOS
        LDW 0, 1, FRAMES
        STW 0, 1, MSGLAST
        LEAVE
MSST1:  LDB MSGROW              ; (one or two rows: text_center's cut)
MSST2:  LR 2,A
        LDB MSGCOL
        LR 6,A
        DCI TXT
        PI TCENTER
        LEAVE

; MSGTICK -- every frame: a scrolling message moves a character every 4.
MSGTICK: ENTER
        LDB MSGSCR
        NI 0FFH
        JZ MSTKX
        LDW 0, 1, FRAMES        ; 4 frames since the last step?
        LDW 2, 3, MSGLAST
        PI SUB16
        LR A,1
        NI 0FFH
        BNZ MSTK1
        LR A,0
        CI 3
        JC MSTKX
MSTK1:  LDW 0, 1, FRAMES
        STW 0, 1, MSGLAST
        ; LINBUF[i] = txt[pos + i - TCOLS] while that is in the message
        LDB MSGPOS
        LR 4,A                  ; k = pos + i, from pos
        LI TCOLS
        LR 5,A
        DCI LINBUF
        XDC
MSTKL:  LR A,4                  ; k >= TCOLS and k < TCOLS + len?
        CI TCOLS-1
        BC MSTKS
        LR A,4
        AI 256-TCOLS
        LR 3,A                  ; k - TCOLS
        LDB MSGLEN
        AI 0FFH
        COM
        AS 3                    ; carries when k - TCOLS > len - 1
        BC MSTKS
        DCI TXT
        LR A,3
        ADCU
        LM
        BR MSTKC
MSTKS:  LI ' '
MSTKC:  XDC
        ST
        XDC
        LR A,4
        INC
        LR 4,A
        DS 5
        BNZ MSTKL
        XDC
        CLR
        ST
        LDB MSGROW
        AI 6
        LR 2,A
        LI TX0
        LR 1,A
        LDB MSGCOL
        LR 6,A
        DCI LINBUF
        PI DTEXT
        LDB MSGLEN              ; on, and round again after TCOLS + 4 more
        AI TCOLS+4
        LR 3,A
        INCB MSGPOS
        COM
        AS 3                    ; carries when len + TCOLS + 4 > pos
        BC MSTKX
        CLR
        STB MSGPOS
MSTKX:  LEAVE

; TITLE -- title r0 (STT) with the number r1, centred on the top row
;   (text.c title_show).
TITLE:  ENTER
        LR A,1
        STB EXPV1
        CLR
        ST
        STB EXPV2
        ST
        DCI STT
        PI STRP
        XDC
        DCI LINBUF
        XDC
        PI TXTEXP
        LI TITLEY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI LINBUF
        PI TCENTER
        LEAVE

; STRCPY -- the string at DC0 copied to DC1, NUL and all; DC1 is left on
;   the NUL (to append to), DC0 after the source's.  Leaf.  Clobbers A.
STRCPY: LM
        XDC
        ST
        XDC
        NI 0FFH
        BNZ STRCPY
        XDC
        LI 0FFH
        ADC
        XDC
        POP

; SPACES -- r0 spaces and a NUL at DC0.  Leaf.  Clobbers A, r0, DC0.
SPACES: LR A,0
        NI 0FFH
        BZ SPCX
        LI ' '
SPC1:   ST
        DS 0
        BNZ SPC1
SPCX:   CLR
        ST
        POP
