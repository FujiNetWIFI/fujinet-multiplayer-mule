; text.inc -- numbers, the strings (build/str.inc, in banks), and the message
; rows (mekkogx text.c). A string is copied into TXT with its '%'s expanded
; (V1, then V2), then drawn: centred, word-wrapped onto a second row, or (a
; message longer than two rows) scrolling along the second.

; ---- numbers -----------------------------------------------------------------
; fmt_num: R9 in decimal at NUMBUF, NUL-ended; D = its length. Subtracts
; powers of ten (mekkogx fmt_num). Clobbers R8, R9, RB, RC.
        RFIT    79
fmt_num:
        LDR     RB, NUMBUF
        LDR     RC, pow10
        LDI     0
        PLO     R8                      ; nothing written yet
fn1:    LDI     '0'
        PHI     R8
fn2:    SEX     RC                      ; R9 - 10^k, if it goes
        GLO     R9
        SM
        STR     R2
        INC     RC
        GHI     R9
        SMB
        DEC     RC
        SEX     R2
        BNF     fn3
        PHI     R9
        LDN     R2
        PLO     R9
        GHI     R8
        ADI     1
        PHI     R8
        BR      fn2
fn3:    GHI     R8                      ; the digit, unless a leading zero
        XRI     '0'
        BNZ     fn4
        GLO     R8
        BZ      fn5
fn4:    GHI     R8
        STR     RB
        INC     RB
        LDI     1
        PLO     R8
fn5:    INC     RC
        INC     RC
        GLO     RC
        XRI     LO(pow10e)
        BNZ     fn1
        GLO     R9                      ; the units, always
        ADI     '0'
        STR     RB
        INC     RB
        LDI     0
        STR     RB
        GLO     RB
        SMI     LO(NUMBUF)
        FRET
pow10:  DB      LO(10000), HI(10000), LO(1000), HI(1000)    ; low byte first
        DB      LO(100), HI(100), LO(10), HI(10)
pow10e:

; put_num: R9 right-aligned in D columns at the cursor (blanks before).
        RFIT    43
put_num:
        PUTV    T5
        FCALL   fmt_num
        STR     R2
        GETV    T5                      ; blanks = width - length
        SM
        BNF     pn2                     ; too wide: as it is
        BZ      pn2
        PLO     R9
pn1:    LDI     ' '
        HOT     TX_PUTC
        DEC     R9
        GLO     R9
        BNZ     pn1
pn2:    LDR     R8, NUMBUF
        FJMP    d_puts

; pt_num: R9 in decimal, pixel text ending at x RC.0 (exclusive), line
; RC.1, mode D (TM_*, TM_SHORT).
        RFIT    74
pt_num: PUTV    T5
        GLO     RC
        PUTV    T2
        GHI     RC
        PUTV    T3
        FCALL   fmt_num
pt_buf: SHL                             ; x = right - 4 * length
        SHL
        STR     R2
        GETV    T2
        SM
        PLO     RC
        PARS
        GLO     RC
        PD
        GETV    T3
        PD
        GETV    T5
        PD
        LDR     R8, NUMBUF
pn3:    LDA     R8
        BZ      pn4
        PD
        BR      pn3
pn4:    GOP     TOP_PTEXT
        FRET

; pt_numk: as pt_num, in four characters at most: 10000 and up as 12K.
        RFIT    88
pt_numk:
        PUTV    T5
        GLO     RC
        PUTV    T2
        GHI     RC
        PUTV    T3
        GHI     R9                      ; under 10000: as it is
        SMI     HI(10000)
        BNF     pk2
        BNZ     pk0
        GLO     R9
        SMI     LO(10000)
        BNF     pk2
pk0:    LDI     0                       ; thousands
        PLO     RB
pk1:    GLO     R9
        SMI     LO(1000)
        PLO     R9
        GHI     R9
        SMBI    HI(1000)
        PHI     R9
        BNF     pk1a
        INC     RB
        BR      pk1
pk1a:   GLO     RB
        PLO     R9
        LDI     0
        PHI     R9
        FCALL   fmt_num
        PLO     RB                      ; NUMBUF += "K"
        ADI     LO(NUMBUF)
        PLO     R8
        LDI     HI(NUMBUF)
        PHI     R8
        LDI     'K'
        STR     R8
        INC     R8
        LDI     0
        STR     R8
        GLO     RB
        ADI     1
        FJMP    pt_buf
pk2:    GETV    T2
        PLO     RC
        GETV    T5
        FJMP    pt_num

; fmt32: the 32-bit number at R8 (LE) in decimal at NUMBUF; D = its length.
; Divides by 10 a digit at a time (bit by bit): the digits come backwards,
; then are turned round.
        RFIT    151
fmt32:  LDR     RB, NUM32               ; a copy, to divide
        LDI     4
        PLO     RC
f32a:   LDA     R8
        STR     RB
        INC     RB
        DEC     RC
        GLO     RC
        BNZ     f32a
        SETV    T4, 0                   ; digits made
f32b:   LDI     0                       ; remainder
        PHI     RC
        LDI     32
        PLO     RC
f32c:   LDR     RB, NUM32               ; the number left, into the remainder
        LDN     RB
        SHL
        STR     RB
        INC     RB
        LDN     RB
        SHLC
        STR     RB
        INC     RB
        LDN     RB
        SHLC
        STR     RB
        INC     RB
        LDN     RB
        SHLC
        STR     RB
        GHI     RC
        SHLC
        PHI     RC
        SMI     10                      ; it goes: a 1 into the quotient
        BNF     f32d
        PHI     RC
        LDR     RB, NUM32
        LDN     RB
        ORI     1
        STR     RB
f32d:   DEC     RC
        GLO     RC
        BNZ     f32c
        GETV    T4                      ; the digit
        ADI     LO(NUMBUF)
        PLO     R8
        LDI     HI(NUMBUF)
        PHI     R8
        GHI     RC
        ADI     '0'
        STR     R8
        GETV    T4
        ADI     1
        STR     RA
        LDR     RB, NUM32               ; until the quotient is 0
        LDA     RB
        SEX     RB
        OR
        INC     RB
        OR
        INC     RB
        OR
        SEX     R2
        BNZ     f32b
        GETV    T4                      ; NUL, then turned round
        ADI     LO(NUMBUF)
        PLO     RC
        LDI     HI(NUMBUF)
        PHI     RC
        LDI     0
        STR     RC
        DEC     RC
        LDR     RB, NUMBUF
f32e:   GLO     RB                      ; swap ends until they meet
        STR     R2
        GLO     RC
        SM
        BNF     f32f
        BZ      f32f
        LDN     RB
        PLO     R9
        LDN     RC
        STR     RB
        GLO     R9
        STR     RC
        INC     RB
        DEC     RC
        BR      f32e
f32f:   GETV    T4
        FRET

; ---- strings --------------------------------------------------------------------
; str_get: string D into TXT, its '%'s expanded (V1, then V2); D and TXTLEN =
; its length. Fixed code: it puts the strings' bank in and the caller's back.
        RFIT    225
str_get:
        PUTV    T4
        SETV    T2, LO(TXT)             ; T2 = where in TXT
        BR      sg0
; str_cat: the same, after what TXT holds.
str_cat:
        PUTV    T4
        GETV    TXTLEN
        ADI     LO(TXT)
        PUTV    T2
sg0:    GETV    CURBANK
        STXD                            ; the caller's bank
        GETV    T4
        SHR
        SHR
        SHR
        SHR
        SHR
        SHR
        ADI     STRBK
        FCALL   bksel
        GETV    T4                      ; its address, at $C400 + 2 * (id & 63)
        ANI     3FH
        SHL
        PLO     R8
        LDI     0C4H
        PHI     R8
        LDA     R8
        PUTV    T0                      ; T0:T1 = the source
        LDN     R8
        PUTV    T1
        SETV    T3, 0                   ; T3 = the numbers used
sg1:    GETV    T0
        PHI     R8
        GETV    T1
        PLO     R8
        LDA     R8
        PLO     R9
        GLO     R8
        PUTV    T1
        GHI     R8
        PUTV    T0
        GLO     R9
        BZ      sg9
        XRI     '%'
        BZ      sg2
        GLO     R9
        FCALL   sg_put
        BR      sg1
sg2:    GETV    T3                      ; a number: V1, then V2
        BNZ     sg3
        GETV    V1L
        PLO     R9
        GETV    V1H
        BR      sg4
sg3:    GETV    V2L
        PLO     R9
        GETV    V2H
sg4:    PHI     R9
        SETV    T3, 1
        FCALL   fmt_num
        LDR     R8, NUMBUF
sg5:    LDA     R8
        BZ      sg1
        FCALL   sg_put
        BR      sg5
sg9:    GETV    T2
        PLO     R8
        LDI     HI(TXT)
        PHI     R8
        LDI     0
        STR     R8
        GLO     R8
        SMI     LO(TXT)
        PUTV    TXTLEN
        IRX
        LDX
        FCALL   bksel                   ; the caller's bank back
        GETV    TXTLEN
        FRET
; sg_put: D into TXT at T2, unless it is full. Keeps R8.
sg_put: PLO     R9
        GETV    T2
        SMI     LO(TXT)+TXTMAX
        BDF     sgp9
        GETV    T2
        PLO     RB
        ADI     1
        STR     RA
        LDI     HI(TXT)
        PHI     RB
        GLO     R9
        STR     RB
sgp9:   FRET

; ---- drawing text -------------------------------------------------------------------
; putn: D characters from R8 at the cursor.
        RFIT    48
putn:   PLO     R9
        LDI     TX_PUTC
        PHI     RF
pu1:    GLO     R9
        BZ      pu2
        DEC     R9
        LDA     R8
        PLO     RF
        LDN     RF
        BR      pu1
pu2:    LDI     FN_PARK
        PHI     RF
        FRET

; clrrow: blank row D (0-9).
clrrow: SHL
        SHL
        SHL
        SHL
        HOT     TX_CURSOR
        OPN     TOP_CLREOL
        FRET

; say_center4: string D centred on row 4 (the boot's and the errors').
say_center4:
        FCALL   str_get
        LDI     4
        FJMP    text_center

; text_center: TXT centred on row D; a line wider than the screen is cut at
; a space and goes on, centred, on the next row (mekkogx text_center).
        RFIT    127
text_center:
        PUTV    T0                      ; T0 = the row
        FCALL   clrrow
        LDR     R8, TXT
tc0:    GLO     R8                      ; T1 = where the rest starts (low)
        PUTV    T1
        STR     R2                      ; n = what is left
        GETV    TXTLEN
        ADI     LO(TXT)
        SM
        BZ      tc9
        PLO     RB                      ; RB.0 = n
        SMI     TCOLS+1
        BNF     tc3                     ; it fits
        LDI     TCOLS                   ; cut at the last space in the row
        PLO     RC
tc1:    GETV    T1
        STR     R2
        GLO     RC
        ADD
        PLO     R8
        LDI     HI(TXT)
        PHI     R8
        LDN     R8
        XRI     ' '
        BZ      tc2
        DEC     RC
        GLO     RC
        BNZ     tc1
        LDI     TCOLS                   ; no space: cut it at the edge
        PLO     RC
tc2:    GLO     RC
        PLO     RB                      ; n = the cut
tc3:    GLO     RB                      ; column (16 - n) / 2
        SDI     TCOLS
        SHR
        STR     R2
        GETV    T0
        SHL
        SHL
        SHL
        SHL
        OR
        HOT     TX_CURSOR
        GETV    T1
        PLO     R8
        LDI     HI(TXT)
        PHI     R8
        GLO     RB
        FCALL   putn                    ; R8 at the cut
tc4:    LDN     R8                      ; skip the spaces
        BZ      tc9
        XRI     ' '
        BNZ     tc5
        INC     R8
        BR      tc4
tc5:    GETV    T0                      ; the rest on the next row
        ADI     1
        STR     RA
        SMI     10
        BDF     tc9
        LDN     RA
        FCALL   clrrow
        BR      tc0
tc9:    FRET

; ---- the message rows (mekkogx msg_*) ---------------------------------------------------
; msg_clear: both message rows blank, nothing scrolling.
        RFIT    89
msg_clear:
        SETV    MSGSCR, 0
        GETV    MSGROW
        FCALL   clrrow
        GETV    MSG1
        BNZ     mc9
        GETV    MSGROW
        ADI     1
        FJMP    clrrow
mc9:    FRET

; msg_show: message D (V1, V2 its numbers): on one row, two, or scrolling.
msg_show:
        FCALL   str_get
; msg_start: TXT (TXTLEN): two rows at most, else it scrolls along the
; second (mekkogx msg_start).
msg_start:
        FCALL   msg_clear
        GETV    MSG1                    ; one row: longer than it scrolls
        BZ      ms0
        GETV    TXTLEN
        SMI     TCOLS+1
        BDF     ms1
        BR      ms0a
ms0:    GETV    TXTLEN
        SMI     2*TCOLS+1
        BDF     ms1
ms0a:   GETV    MSGROW
        FJMP    text_center
ms1:    SETV    MSGSCR, 1
        SETV    MSGPOS, 0
        GETV    FCL
        PUTV    MSGT
        FRET

; msg_tick: a scrolling message moves a character every 4 frames: the
; second row shows TXT[pos-16 .. pos-1], blanks around it.
        RFIT    188
msg_tick:
        GETV    MSGSCR
        BZ      mt9
        GETV    MSGT
        STR     R2
        GETV    FCL
        SM
        SMI     4
        BNF     mt9
        GETV    FCL
        PUTV    MSGT
        GETV    MSG1                    ; the second row, or the one
        XRI     1
        STR     R2
        GETV    MSGROW
        ADD
        SHL
        SHL
        SHL
        SHL
        HOT     TX_CURSOR
        LDI     TX_PUTC
        PHI     RF
        LDI     0
        PLO     RC                      ; column
mt1:    GETV    MSGPOS                  ; k = pos + column - 16
        STR     R2
        GLO     RC
        ADD
        SMI     TCOLS
        BNF     mt2                     ; before the text
        PLO     R8
        STR     R2
        GETV    TXTLEN
        SM
        BNF     mt2                     ; after it
        BZ      mt2
        GLO     R8
        ADI     LO(TXT)
        PLO     R8
        LDI     HI(TXT)
        PHI     R8
        LDN     R8
        BR      mt3
mt2:    LDI     ' '
mt3:    PLO     RF
        LDN     RF
        INC     RC
        GLO     RC
        XRI     TCOLS
        BNZ     mt1
        LDI     FN_PARK
        PHI     RF
        GETV    MSGPOS                  ; on, round again 4 after the end
        ADI     1
        STR     RA
        STR     R2
        GETV    TXTLEN                  ; (mekkogx: pos >= len + 20)
        ADI     TCOLS+4
        SM
        BZ      mt8
        BDF     mt9
mt8:    SETV    MSGPOS, 0
mt9:    FRET
