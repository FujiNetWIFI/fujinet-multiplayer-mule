; p_title.inc -- the title (mekkogx main.c): "M.U.L.E." in block graphics,
; TOURNAMENT, and the eight colonists walking across under it, right to
; left, about three seconds (FIRE goes on at once). MAININIT shows it
; before the boot page.
;
; The harness may start the offline test mode meanwhile: it finds 3C C3 at
; TESTHK and pokes 5A, the phase + 1, the argument and the player event;
; we answer A5 (TESTM 1: MAININIT runs the test page).
;
; The parade: colonist j (species j) starts 17j frames after the first, 2
; pixels a frame from column 136, so it shows while d = t - 17j is 5-71;
; sprite j & 3 carries it (j and j + 4 are never both on the screen).

        BCTA,UN TIENTER         ; E_ENTER
        RETC,UN
        NOP
        NOP
        RETC,UN
        NOP
        NOP

TESTHK  EQU     UNIONB          ; the harness's handshake (4 bytes)
TISHOW  EQU     UNIONB+4        ; each sprite's picture (4)
TIT     EQU     UNIONB+8        ; frames gone by
TIEND   EQU     192
TIFEET  EQU     176             ; the parade's line (rows 22-23)

TIENTER: BSTA,UN TIDRAW
        LODI,R0 $3C             ; the handshake offered
        STRA,R0 TESTHK
        LODI,R0 $C3
        STRA,R0 TESTHK+1
        LODI,R0 $FF             ; no pictures yet
        STRA,R0 TISHOW
        STRA,R0 TISHOW+1
        STRA,R0 TISHOW+2
        STRA,R0 TISHOW+3
        EORZ    R0
        STRA,R0 TIT
TI1:    BSTA,UN VBSYNC
        BSTA,UN TICKRD
        BSTA,UN KEYDEC
        LODA,R0 TESTHK          ; the harness: the offline test mode
        COMI,R0 $5A
        BCTR,EQ TITEST
        LODA,R0 EVBTN           ; FIRE: on
        BCFR,EQ TI9
        BSTR,UN TIPARADE
        LODA,R0 TIT
        ADDA,R0 DT
        STRA,R0 TIT
        COMI,R0 TIEND
        BCTR,LT TI1
TI9:    RETC,UN
TITEST: LODI,R0 $A5
        STRA,R0 TESTHK
        LODI,R0 1
        CPUT    TESTM
        RETC,UN

; TIPARADE: each sprite on the colonist it carries now, or off.
TIPARADE: LODI,R0 0
TIP1:   STRA,R0 T3              ; T3: the sprite (k), T2: d for walker k or k + 4
        LODA,R0 TIT             ; walker k: d = t - 17k
        STRA,R0 T2
        LODA,R3 T3
        LODA,R0 TI17,R3
        STRA,R0 T1              ; (17k)
        LODA,R0 T2
        SUBA,R0 T1
        BCTA,LT TIP4            ; not started (nor k + 4)
        STRA,R0 T2
        COMI,R0 72
        BCTR,LT TIP2
        SUBI,R0 68              ; walker k + 4: d - 68
        BCTA,LT TIP4
        STRA,R0 T2
        LODA,R0 T3
        ADDI,R0 4
        BCTR,UN TIP3
TIP2:   LODA,R0 T3
TIP3:   STRA,R0 T1              ; T1: the species
        LODA,R0 T2
        COMI,R0 5
        BCTA,LT TIP4
        COMI,R0 72
        BCFA,LT TIP4
        LODA,R0 T1              ; its picture: walking left, 2-3, each its
        STRZ    R1              ;  own step
        RRL,R0
        RRL,R0
        ADDZ    R1
        RRL,R0
        STRA,R0 T0
        LODA,R0 T2
        RRR,R0
        RRR,R0
        ANDI,R0 1
        ADDI,R0 2
        ADDA,R0 T0
        STRA,R0 T0              ; T0: species * 10 + 2 + (d / 4) & 1
        LODA,R3 T3
        LODA,R0 TISHOW,R3
        COMA,R0 T0
        BCTR,EQ TIP3A
        LODA,R0 UDCQN
        COMI,R0 3
        BCFR,LT TIP3A
        LODA,R0 T0
        STRA,R0 TISHOW,R3
        BSTR,UN TIFIG
TIP3A:  LODA,R1 T3              ; its colour, its place: 136 - 2d
        LODA,R3 T1
        LODA,R0 TICOL,R3
        STRZ    R2
        ZBSR    *ZSPRCOL
        LODA,R0 T2
        RRL,R0
        STRZ    R1
        LODI,R0 136
        SUBZ    R1
        STRZ    R3
        LODA,R1 T3
        LODI,R2 TIFEET
        ZBSR    *ZSPRSET
        BCTR,UN TIP5
TIP4:   LODA,R1 T3
        ZBSR    *ZSPRHID
TIP5:   LODA,R0 T3
        ADDI,R0 1
        COMI,R0 4
        BCFA,EQ TIP1
        RETC,UN
TI17:   DB      0,17,34,51
TICOL:  DB      SYELLOW|$80,SCYAN|$80,SGREEN|$80,SMAGEN|$80
        DB      SRED|$80,SWHITE|$80,SYELLOW|$80,SCYAN|$80

; TIFIG: picture T0 into sprite T3's glyph. Keeps T1, T2, T3.
TIFIG:  LODA,R0 T1
        STRA,R0 TIT+1
        LODA,R0 T2
        STRA,R0 TIT+2
        LODA,R2 T0              ; ART_FIG + picture * 8
        LODI,R1 8
        ZBSR    *ZMUL8
        LODZ    R2
        ADDI,R0 ART_FIG&$FF
        STRZ    R2
        PPSL    PSLWC
        LODZ    R1
        ADDI,R0 ART_FIG>>8
        CPSL    PSLWC
        STRZ    R3
        LODZ    R2
        STRZ    R1
        LODZ    R3
        LODA,R2 T3
        LODI,R3 PG_ART
        ZBSR    *ZUDCQ
        LODA,R0 TIT+1
        STRA,R0 T1
        LODA,R0 TIT+2
        STRA,R0 T2
        RETC,UN

; TIDRAW: block-graphics rows from the top, text rows starting with the
; switch back: the logo on rows 4-7, TOURNAMENT, the FujiNet line.
TIDRAW: BSTA,UN SCRG26
        LODI,R0 GFX26|$80       ; every row starts in block graphics
        STRA,R0 GFXMOD
        LODI,R1 0               ; every row a switch to text first
TD1:    ZBSR    *ZDPOS
        LODI,R2 $FF
        LODI,R0 GTEXT
        STRA,R0 *DROWH,R2,+
        ADDI,R1 1
        COMI,R1 26
        BCFR,EQ TD1
        LODI,R1 4               ; the logo
        LODI,R3 0
TD2:    ZBSR    *ZDPOS
        LODI,R2 $FF
TD3:    LODA,R0 LOGO,R3
        STRA,R0 *DROWH,R2,+
        ADDI,R3 1
        COMI,R2 15
        BCFR,EQ TD3
        ADDI,R1 1
        COMI,R1 4+LOGON
        BCFR,EQ TD2
        DCOL    CBLUE
        DAT     10,3
        TXTAT   STOURN
        DCOL    CWHITE
        DAT     13,1
        TXTAT   SFUJI
        RETC,UN

STOURN: DB      "TOURNAMENT",$FF
SFUJI:  DB      "ARCADIA FUJINET",$FF    ; (15: column 0 is the row's switch)
        INCLUDE "build/logo.inc"
