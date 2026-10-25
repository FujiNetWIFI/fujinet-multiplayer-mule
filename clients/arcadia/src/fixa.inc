; fixa.inc -- fixed A, $0000-$05FF: the same in every page's assembly and
; always mapped (2650 page 0, with the screen, console RAM and RAM-L).
;
; The start-up, the vblank's work, the input, the sound player and the
; display primitives every page calls. Page code calls these freely; only
; fixed code may switch pages (BORROW, SELPG), since the next fetch from
; the window comes from the new page.
;
; Register conventions: R0 data, R1-R3 arguments; a routine says what it
; clobbers. PSL is PSLBASE (logical compares, no carry-in) everywhere but
; inside 16-bit arithmetic.

        ORG     $0000
        BCTA,UN START
        DB      $17             ; RETC,UN: the interrupt guard at $0003

; ---- the zero-page vectors: ZBSR *Zx is a 2-byte call from either page ----
        ORG     $0004
ZDPOS:  DWBE    DPOS
ZDPUTC: DWBE    DPUTC
ZDTXT:  DWBE    DTXT
ZDNUM:  DWBE    DNUM
ZDNUM16: DWBE   DNUM16
ZSNDQ:  DWBE    SNDQUE
ZSAY:   DWBE    SAY
ZSAYBX: DWBE    SAYBX
ZLPUT:  DWBE    LPUTR
ZRAMPUT: DWBE   RAMPUT
ZSPRSET: DWBE   SPRSET
ZSPRHID: DWBE   SPRHIDE
ZDFILL: DWBE    DFILL
ZDCLR:  DWBE    DCLR
ZUDCQ:  DWBE    UDCQUE
ZMUL8:  DWBE    MUL8
ZDIV8:  DWBE    DIV8
ZNOW:   DWBE    FRNOW
ZSINCE: DWBE    SINCE
ZACT:   DWBE    ACTSEND
ZSPRCOL: DWBE   SPRCOL
ZDGLY:  DWBE    DGLYPH
ZTRACE: DWBE    TRACE           ; (spare vectors are zero)
ZDNAME: DWBE    DNAME
ZSAYV:  DWBE    SAYV
ZDNAME6: DWBE   DNAME6

        ORG     $0040
; ---- pointer words: page-0 code's way into page 1 ------------------------
PBANK:  DWBE    FNBANK          ; *PBANK,Rn selects page Rn
PRSEL:  DWBE    FNRSEL
PRDAT:  DWBE    FNRDAT
PRDATA: DWBE    FNRDATA
PTXD:   DWBE    FNDATA          ; *PTXD,Rn appends Rn to the TX stream
PREPLY: DWBE    FNREPLY
PMAGF:  DWBE    FNMAGF
PMAGN:  DWBE    FNMAGN
PPVER:  DWBE    FNPVER
PTICKH: DWBE    FNTICKH
PTICKL: DWBE    FNTICKL
PRAMH:  DWBE    $2900           ; RAM-H, *PRAMH,Rn reads $2900+Rn
PRAMH2: DWBE    $2A00
PMUTE:  DWBE    MUTE            ; (RAM-H: Select's)
PSNDT:  DWBE    SNDTAB          ; tables in fixed B (fixed A is the tight one)
PSNDT1: DWBE    SNDTAB+1
PSNDT2: DWBE    SNDTAB+2
PSNDT3: DWBE    SNDTAB+3
PSNDT4: DWBE    SNDTAB+4
PKT0:   DWBE    KT0
PKT1:   DWBE    KT1
PKT2:   DWBE    KT2
PKTP:   DWBE    KTP

; ---- start-up ----------------------------------------------------------------
; RESET restarts here but never resets the cart: the page and cart RAM are
; as they were. So everything is set up afresh.
START:  LODI,R0 $20             ; interrupts inhibited (none are wired anyway)
        LPSU
        LODI,R0 PSLBASE
        LPSL
        LODI,R1 0
        STRA,R1 CURPG
        LODA,R0 *PBANK,R1       ; page 0 into the window
        EORZ    R0
        STRA,R0 PITCH
        STRA,R0 VOLUME
        LODI,R3 8               ; sprites parked (Y 0: off)
ST1:    STRA,R0 SPRPOS-1,R3
        BDRR,R3 ST1
        LODI,R3 $F0-$D0         ; zone A cleared
ST2:    STRA,R0 $18D0-1,R3
        BDRR,R3 ST2
        LODI,R3 $30             ; zone B
ST3:    STRA,R0 $1AD0-1,R3
        BDRR,R3 ST3
        STRA,R0 ACT
        STRA,R0 POLLW
        LODI,R0 $FF
        STRA,R0 SNDID
        STRA,R0 SNDQ
        STRA,R0 KRAW
        STRA,R0 VPOS            ; rows from the top of the screen
        BSTA,UN SCRG26          ; the game's screen, cleared
        LODA,R0 *PMAGF          ; the cart: 'F','N', protocol 2 or later
        COMI,R0 $46
        BCFR,EQ STBAD
        LODA,R0 *PMAGN
        COMI,R0 $4E
        BCFR,EQ STBAD
        LODA,R0 *PPVER
        COMI,R0 2
        BCTR,LT STBAD
        BCTA,UN MAININIT        ; fixb.inc
STBAD:  DAT     11,0
        TXTAT   SNEEDFW
        DAT     12,0
        TXTAT   SNEEDF2
STHALT: BCTR,UN STHALT
SNEEDFW: DB     " NEEDS A FUJINET",$FF
SNEEDF2: DB     " CART, PROTO 2.",$FF

; SCRG26: the game's screen: 26 lines of 8x8 cells, even inks on black,
; text mode, cleared, white ink; sprites 8 lines and off, no sprite
; scripts, no map kept, no shake. Clobbers R0-R3.
SCRG26: EORZ    R0
        STRA,R0 DCOLOR          ; (white)
        STRA,R0 SPRPOS
        STRA,R0 SPRPOS+2
        STRA,R0 SPRPOS+4
        STRA,R0 SPRPOS+6
        STRA,R0 SHAKE
        LPUT    FXON
        EORZ    R0
        LPUT    MAPON
        LODI,R0 GFX26
        STRA,R0 GFXMOD
        LODI,R0 BGBASE
        STRA,R0 BGCB
        STRA,R0 BGCOL
        LODI,R0 $C0
        STRA,R0 SPRC01
        STRA,R0 SC01SH
        STRA,R0 SPRC23
        STRA,R0 SC23SH
        LODI,R1 0
        LODI,R3 26
        BCTA,UN DCLR            ; tail call

; ---- the vblank ----------------------------------------------------------------
; VBSYNC: wait for the next vertical blank, then do what must happen in it:
; the disc (the 2637 digitises it only then), the queued user-glyph
; rewrites (so no row is drawn half old, half new), the sound. About 2,000
; of its 2,620 clocks at worst. Clobbers R0-R3, T0, T1.
VBSYNC: TPSU    $80
        BCTR,EQ VBSYNC          ; still in the last one
VBS1:   TPSU    $80
        BCFR,EQ VBS1
; the disc: one axis a frame, each kept, the mux set for the other
        LODA,R0 POT
        LODA,R1 AXSEL
        BCFR,EQ VBS2
        STRA,R0 AXV
        LODA,R0 BGCB
        IORI,R0 MUXH
        STRA,R0 BGCOL
        LODI,R0 1
        BCTR,UN VBS3
VBS2:   STRA,R0 AXH
        LODA,R0 BGCB
        STRA,R0 BGCOL
        EORZ    R0
VBS3:   STRA,R0 AXSEL
; the user glyphs waiting: each entry slot, page ($FF: fixed), source
        LODA,R0 UDCQN
        BCTR,EQ VBS9
        LODI,R1 0               ; entry offset
VBS4:   LODA,R0 UDCQ+2,R1       ; its source, where the copy can name it
        STRA,R0 T0
        LODA,R0 UDCQ+3,R1
        STRA,R0 T1
        LODA,R0 UDCQ+1,R1       ; its page
        STRZ    R3
        COMI,R3 $FF
        BCTR,EQ VBS5
        LODA,R0 *PBANK,R3       ; borrowed (this is fixed code)
VBS5:   LODA,R0 UDCQ,R1         ; slot * 8 + 8: the copy runs down
        RRL,R0
        RRL,R0
        RRL,R0
        ADDI,R0 8
        STRZ    R2
        LODI,R3 8
VBS6:   LODA,R0 *T0,R3,-
        STRA,R0 UDC,R2,-
        BRNR,R3 VBS6
        ADDI,R1 4
        LODA,R0 UDCQN
        SUBI,R0 1
        STRA,R0 UDCQN
        BCFR,EQ VBS4
        LODA,R3 CURPG           ; give the window back
        LODA,R0 *PBANK,R3
VBS9:
; the sound: a queued one starts, a playing one counts down; VOLUME's
; top bits are the screen's shift (a planetquake shakes it)
        LODA,R1 SNDQ
        COMI,R1 $FF
        BCTR,EQ VBSN1
        LODI,R0 $FF
        STRA,R0 SNDQ
        STRA,R1 SNDID
        BSTA,UN SNDROW
        LODA,R0 *PSNDT,R1       ; pitch
        STRA,R0 PITCH
        LODA,R0 *PSNDT1,R1      ; frames
        STRA,R0 SNDCNT
        BCTR,UN VBSN7
VBSN1:  LODA,R0 SNDCNT
        BCTR,EQ VBSN7
        SUBI,R0 1
        STRA,R0 SNDCNT
        BCFR,EQ VBSN7
        LODA,R1 SNDID           ; the note is over: a second, or quiet
        BSTA,UN SNDROW
        LODA,R0 *PSNDT3,R1      ; the second note's frames
        BCTR,EQ VBSN8
        STRA,R0 SNDCNT
        LODA,R0 *PSNDT2,R1
        STRA,R0 PITCH
        LODI,R0 SND2ND          ; the second note: loud, and no third
        STRA,R0 SNDID
        BCTR,UN VBSN7
VBSN8:  EORZ    R0
        STRA,R0 PITCH
        LODI,R0 $FF
        STRA,R0 SNDID
VBSN7:  LODA,R1 SNDID           ; VOLUME = the sound's | the shift
        EORZ    R0
        COMI,R1 $FF
        BCTR,EQ VBSN6
        BSTA,UN SNDROW
        LODA,R0 *PSNDT4,R1
VBSN6:  IORA,R0 SHAKE
        STRA,R0 VOLUME
        RETC,UN
; R1 = the table row of sound R1 (id * 5). Leaf. Clobbers R0.
SNDROW: LODZ    R1
        ADDZ    R1
        ADDZ    R1
        ADDZ    R1
        ADDZ    R1
        STRZ    R1
        RETC,UN

; SNDQUE: queue sound R0 for the next vblank (none while muted). A soft
; one (SND_STEP to SND_TOCK) never cuts another, only another soft one.
; Leaf. Clobbers R1.
SNDQUE: STRZ    R1              ; muted: none
        LODA,R0 *PMUTE
        BCTR,EQ SNQ0
        RETC,UN
SNQ0:   LODZ    R1
        COMI,R0 SND_STEP
        BCTR,LT SNQ1            ; loud: always
        LODA,R1 SNDQ
        BSTR,UN SNQOK
        BCFR,EQ SNQ9
        LODA,R1 SNDID
        BSTR,UN SNQOK
        BCFR,EQ SNQ9
SNQ1:   STRA,R0 SNDQ
SNQ9:   RETC,UN
; CC EQ when sound R1 is none or soft (SND2ND, the alert's second note,
; is loud). Leaf.
SNQOK:  COMI,R1 $FF
        RETC,EQ
        COMI,R1 SND_STEP
        BCTR,LT SNQN
        COMI,R1 SND_TOCK+1
        BCTR,LT SNQY
SNQN:   COMI,R1 $FF             ; (not $FF here: not EQ)
        RETC,UN
SNQY:   TMI,R1  0               ; no bits to test: EQ
        RETC,UN

; ---- input ----------------------------------------------------------------------
; KEYDEC: this frame's controls from the disc (as last digitised), the
; keypad and the panel. INDIR: directions held (diagonals too). INBTN:
; FIRE held, EVBTN: it went down. EVKEY: a key that went down (K_*,
; digits; FIRE is keypad 2's wire, so 2 is never a digit). Leaf.
; Clobbers R0-R2.
KEYDEC: LODI,R2 0
        LODA,R0 AXV
        COMI,R0 JOYLO
        BCFR,LT KD1
        IORI,R2 D_UP
KD1:    COMI,R0 JOYHI
        BCFR,GT KD2
        IORI,R2 D_DOWN
KD2:    LODA,R0 AXH
        COMI,R0 JOYLO
        BCFR,LT KD3
        IORI,R2 D_LEFT
KD3:    COMI,R0 JOYHI
        BCFR,GT KD4
        IORI,R2 D_RIGHT
KD4:    STRA,R2 INDIR
        LODA,R0 KEYPAD+1        ; FIRE
        ANDI,R0 $08
        BCTR,EQ KD5
        LODA,R0 INBTN           ; down: an event if it was up
        EORI,R0 1
        STRA,R0 EVBTN
        LODI,R0 1
        STRA,R0 INBTN
        BCTR,UN KD6
KD5:    STRA,R0 EVBTN
        STRA,R0 INBTN
; a key: the first found, edge-detected
KD6:    LODI,R1 K_NONE
        LODA,R0 KEYPAD          ; b3 1, b2 4, b1 7, b0 Clear
        ANDI,R0 $0F
        BCTR,EQ KD10
        STRZ    R2
        LODA,R0 *PKT0,R2
        BCTR,UN KD12
KD10:   LODA,R0 KEYPAD+1        ; b2 5, b1 8, b0 0 (b3 is FIRE)
        ANDI,R0 $07
        BCTR,EQ KD11
        STRZ    R2
        LODA,R0 *PKT1,R2
        BCTR,UN KD12
KD11:   LODA,R0 KEYPAD+2        ; b3 3, b2 6, b1 9, b0 Enter
        ANDI,R0 $0F
        BCTR,EQ KD13
        STRZ    R2
        LODA,R0 *PKT2,R2
        BCTR,UN KD12
KD13:   LODA,R0 PANEL           ; b0 Start, b1 Option, b2 Select
        ANDI,R0 $07
        BCTR,EQ KD14
        STRZ    R2
        LODA,R0 *PKTP,R2
KD12:   STRZ    R1
KD14:   LODZ    R1
        COMA,R0 KRAW
        STRA,R1 KRAW
        BCFR,EQ KD15
        LODI,R1 K_NONE          ; held: no event
KD15:   STRA,R1 EVKEY
        RETC,UN

; ---- the display ------------------------------------------------------------------
; DPOS: point the display at row R1 (0-25); R2 is left alone (the cell
; before the first one written: see DAT). Leaf. Clobbers R0.
DPOS:   LODZ    R1
        COMI,R0 13
        BCTR,LT DPO1
        SUBI,R0 13
        RRL,R0
        RRL,R0
        RRL,R0
        RRL,R0
        STRA,R0 DROWL
        LODI,R0 SCR1>>8
        STRA,R0 DROWH
        RETC,UN
DPO1:   RRL,R0
        RRL,R0
        RRL,R0
        RRL,R0
        STRA,R0 DROWL
        LODI,R0 SCR0>>8
        STRA,R0 DROWH
        RETC,UN

; DPUTC: glyph R0 (0-63) in DCOLOR at the next cell (R2 pre-incremented).
; DGLYPH: R0 already carries its colour bits. An exact $C0 (a blue
; space) would start block graphics: it is written as $00, which looks
; the same. Leaf. Clobbers R0, advances R2.
DPUTC:  IORA,R0 DCOLOR
DGLYPH: COMI,R0 GMOSAIC
        BCFR,EQ DPC1
        EORZ    R0
DPC1:   STRA,R0 *DROWH,R2,+
        RETC,UN

; DTXT: the glyph string at R0:R1 (glyph codes, $FF ends it; DB strings
; are glyphs, see GLYPHS). DTXTS: the one at DSTRH:DSTRL. Calls one
; down. Clobbers R0, R3; advances R2.
DTXT:   STRA,R0 DSTRH
        STRA,R1 DSTRL
DTXTS:  LODI,R3 $FF
DTX1:   LODA,R0 *DSTRH,R3,+
        COMI,R0 $FF
        BCTR,EQ DTX2
        BSTR,UN DPUTC
        BCTR,UN DTX1
DTX2:   RETC,UN

; DNAME: seat R1's name (ASCII, A-Z and 0-9 shown) in DCOLOR, 8 cells,
; padded with spaces (DNAME6: its first 6). Calls one down. Clobbers R0, R1, R3; advances R2.
DNAME:  LODA,R0 DNMREC,R1
        STRZ    R3
        LODI,R1 8
        BCTR,UN DNA1
DNAME6: LODA,R0 DNMREC,R1       ; (6 cells: the arena's lines)
        STRZ    R3
        LODI,R1 6
DNA1:   LODA,R0 H_PLAYERS+PL_NAME,R3
        BCTR,EQ DNA5            ; its end: spaces
        ANDI,R0 $5F             ; (lower case up)
        COMI,R0 $41
        BCTR,LT DNA2
        SUBI,R0 $41-GALPHA
        BCTR,UN DNA4
DNA2:   ANDI,R0 $1F             ; ($30-$39 came through as $10-$19)
        COMI,R0 $10
        BCTR,LT DNA3
        COMI,R0 $19
        BCTR,LT DNA4
        BCTR,EQ DNA4
DNA3:   EORZ    R0
DNA4:   BSTA,UN DPUTC
        ADDI,R3 1
        BDRR,R1 DNA1
        RETC,UN
DNA5:   BSTA,UN DPUTC
        BDRR,R1 DNA5
        RETC,UN
DNMREC: DB      0,29,58,87      ; a seat's record

; DFILL: R3 cells of R0 (glyph and colour bits) at the next cells.
; Leaf. Clobbers R0, R3; advances R2.
DFILL:  STRA,R0 *DROWH,R2,+
        BDRR,R3 DFILL
        RETC,UN

; DCLR: R3 rows of spaces from row R1. Calls one down. Clobbers R0-R3.
DCLR:   BSTA,UN DPOS
        LODI,R2 $FF
        LODI,R0 0
DCL1:   STRA,R0 *DROWH,R2,+
        COMI,R2 15
        BCFR,EQ DCL1
        ADDI,R1 1
        BDRR,R3 DCLR
        RETC,UN

; DNUM: NUMH:NUMM:NUML in decimal, right-aligned in R1 cells (a number
; with more digits shows its last R1), in DCOLOR at the next cells.
; DNUM16 takes R0:R1 as the value and the width in R3. Calls two down.
; Clobbers R0, R1, R3, NUM*, T0, T1; advances R2.
DNUM16: STRA,R0 NUMM
        STRA,R1 NUML
        EORZ    R0
        STRA,R0 NUMH
        LODZ    R3
        STRZ    R1
DNUM:   STRA,R1 T1              ; the width
        BSTR,UN DNCNT           ; R1 = the digits
        LODA,R0 T1              ; spaces: width - digits
        SUBZ    R1
        BCTR,LT DNM4
        BCTR,EQ DNM6
        STRZ    R3
DNM3:   EORZ    R0
        BSTA,UN DPUTC
        BDRR,R3 DNM3
        BCTR,UN DNM6
DNM4:   STRA,R0 T0              ; more digits than cells: drop the top ones
        LODI,R0 0
        SUBA,R0 T0
        STRA,R0 T0              ; how many
        BSTR,UN DNIDX
DNM5:   BSTA,UN DNCMP
        BCTR,LT DNM7
        BSTA,UN DNSUB
        BCTR,UN DNM5
DNM7:   ADDI,R3 3
        LODA,R0 T0
        SUBI,R0 1
        STRA,R0 T0
        BCFR,EQ DNM5
        LODA,R1 T1
DNM6:   BCTA,UN DNPRT           ; tail call: R1 digits

; DNCNT: R1 = the digits of NUM (1-8). Leaf (DNCMP inline: it is at the
; bottom of SAY's chain). Clobbers R0, R3.
DNCNT:  LODI,R1 1
        LODI,R3 18              ; 10^1, then up to 10^7
DNCT1:  LODA,R0 NUMH
        COMA,R0 POW10,R3
        BCFR,EQ DNCT3
        LODA,R0 NUMM
        COMA,R0 POW10+1,R3
        BCFR,EQ DNCT3
        LODA,R0 NUML
        COMA,R0 POW10+2,R3
DNCT3:  BCTR,LT DNCT2
        ADDI,R1 1
        SUBI,R3 3
        BCFR,LT DNCT1           ; (below 0: all eight)
DNCT2:  RETC,UN

; DNIDX: R3 = the table index of 10^(R1-1): 24 - 3 * R1. Leaf. Clobbers R0.
DNIDX:  LODZ    R1
        ADDZ    R1
        ADDZ    R1
        STRZ    R3
        LODI,R0 24
        SUBZ    R3
        STRZ    R3
        RETC,UN

; DNPRT: NUM's R1 lowest digits (the higher ones must be zero), no spaces,
; at the next cells. Calls one down. Clobbers R0, R1, R3, NUM*; advances R2.
DNPRT:  BSTR,UN DNIDX
DNP1:   COMI,R3 21
        BCTR,EQ DNP3
        LODI,R1 0
DNP2:   BSTR,UN DNCMP
        BCTR,LT DNPD
        BSTR,UN DNSUB
        ADDI,R1 1
        BCTR,UN DNP2
DNPD:   LODZ    R1
        IORI,R0 GDIGIT
        BSTA,UN DPUTC
        ADDI,R3 3
        BCTR,UN DNP1
DNP3:   LODA,R0 NUML
        IORI,R0 GDIGIT
        BCTA,UN DPUTC           ; tail call
; NUM against power R3 (POW10 + R3): CC LT if under it. Leaf.
DNCMP:  LODA,R0 NUMH
        COMA,R0 POW10,R3
        BCFR,EQ DNC9
        LODA,R0 NUMM
        COMA,R0 POW10+1,R3
        BCFR,EQ DNC9
        LODA,R0 NUML
        COMA,R0 POW10+2,R3
DNC9:   RETC,UN
; NUM -= power R3. Leaf.
DNSUB:  LODA,R0 NUML
        SUBA,R0 POW10+2,R3      ; (C: no borrow)
        STRA,R0 NUML
        PPSL    PSLWC
        LODA,R0 NUMM
        SUBA,R0 POW10+1,R3
        STRA,R0 NUMM
        LODA,R0 NUMH
        SUBA,R0 POW10,R3
        STRA,R0 NUMH
        CPSL    PSLWC
        RETC,UN
POW10:  DB      $98,$96,$80     ; 10^7
        DB      $0F,$42,$40
        DB      $01,$86,$A0
        DB      $00,$27,$10
        DB      $00,$03,$E8
        DB      $00,$00,$64
        DB      $00,$00,$0A     ; 10^1

; ---- sprites and user glyphs -------------------------------------------------------
; SPRSET: sprite R1 (0-3) at screen line R2 (1-208: 0 hides it), column
; R3 (pixels from the screen's left edge). Leaf. Clobbers R0, R1.
SPRSET: LODZ    R1
        ADDZ    R1
        STRZ    R1
        EORZ    R0
        SUBZ    R2              ; Y = 256 - line
        STRA,R0 SPRPOS,R1
        LODZ    R3
        ADDI,R0 44              ; (MAME: a sprite at x lines up with cell column x / 8)
        STRA,R0 SPRPOS+1,R1
        RETC,UN
; SPRHIDE: sprite R1 off. Leaf. Clobbers R0, R1.
SPRHIDE: LODZ   R1
        ADDZ    R1
        STRZ    R1
        EORZ    R0
        STRA,R0 SPRPOS,R1
        RETC,UN
; ---- time ------------------------------------------------------------------------
; FRNOW: R1:R2 = our frame clock. Leaf.
FRNOW:  LODA,R1 FRMH
        LODA,R2 FRML
        RETC,UN
; SINCE: R1:R2 = frames since the frame R1:R2. Leaf. Clobbers R0.
SINCE:  LODA,R0 FRML
        SUBZ    R2
        STRZ    R2
        PPSL    PSLWC
        LODA,R0 FRMH
        SUBZ    R1
        STRZ    R1
        CPSL    PSLWC
        RETC,UN

; TRACE: R0 to the trace byte the test harness watches (DEBUG builds).
TRACE:  STRA,R0 UNIONBE-1
        RETC,UN

        INCLUDE "src/say.inc"

        IF      $ > $0600
        ERROR   "fixed A runs into RAM-L"
        ENDIF
