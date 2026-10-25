; p_fx.inc -- the sprite scripts (mekkogx fx.c and the set pieces of
; events.c): the transport ship landing and leaving, the pirates, and the
; colony events' actors. The main loop runs it while FXON (fixb.inc
; SERVICES); a phase starts a script with FXSTART (mapscr.inc), which
; leaves its id in FXREQ for the next frame here.
;
; Two actors, each stepping a script once a frame (DT times when frames
; went by), so the pieces keep the Atari's speed. Script bytes:
;   FO_POS x, y            jump there (x: pixel + 64, y: line + 48)
;   FO_MOVE dx, dy, per, n n steps of (dx, dy), signed, one every per frames
;   FO_WAIT n              n frames (under 128)
;   FO_LOOK kind, slot, art, frame, d2, col
;                          FK_SINGLE: one sprite, art + frame (+ d2 every
;                          other step of a move); FK_SHIP: the four sprites
;                          side by side, art's four slices; col as SPRCOL's
;   FO_FRAME f / FO_HIDE / FO_COL c / FO_SIG v (FXFLAG = v)
;   FO_BG c                the screen's ground colour (a flash)
;   FO_LOOP n, back        the `back` bytes before this, n more times
;   FO_END
; A sprite's picture is its user glyph: a frame change queues the rewrite
; (UDCQUE: three a frame, so a ship's four slices show as they load).
;
; An actor is 18 bytes of RAM-H (read through PRAMH, written through the
; mailbox, about 100 clocks a byte): it changes a few times a step.

        RETC,UN                 ; E_ENTER
        NOP
        NOP
        RETC,UN                 ; E_STATE
        NOP
        NOP
        BCTA,UN FXTICK          ; E_TICK

        INCLUDE "src/fxops.inc"
        INCLUDE "src/devram.inc"        ; (a computer's walk: DVAIS0, DVAIS1)

AF_ST    EQU    0               ; 1: running
AF_X     EQU    1               ; pixel + 64
AF_Y     EQU    2               ; line + 48
AF_DUE   EQU    3               ; the frame (low byte) of its next step
AF_N     EQU    4               ; move steps left
AF_PER   EQU    5
AF_DX    EQU    6
AF_DY    EQU    7
AF_PCH   EQU    8               ; its script
AF_PCL   EQU    9
AF_KIND  EQU    10
AF_SLOT  EQU    11
AF_ART   EQU    12              ; its pictures: ART_* - $0800, / 8
AF_FRAME EQU    13
AF_D2    EQU    14              ; b7: this step shows frame + d2
AF_REP   EQU    15              ; FO_LOOP's count
AF_SHOWN EQU    16              ; the picture in its glyph ($FF none); a ship: slices loaded
AFSZ     EQU    18
FXOFF0   EQU    FXA-$2900
FXOFF1   EQU    FXA-$2900+AFSZ

FXIX     EQU    UNIONB          ; (console scratch, this frame only) the script's index
FXPASS   EQU    UNIONB+1        ; passes left this frame
FXPCOL   EQU    UNIONB+2        ; a colour, a picture, an offset
FXPS     EQU    UNIONB+3        ; a sprite
FXPT0    EQU    UNIONB+4        ; a pair kept
FXPT1    EQU    UNIONB+5
FXPLD    EQU    UNIONB+6        ; a ship's slices loaded


; ---- a frame -------------------------------------------------------------------
FXTICK: LODA,R0 FXREQ           ; a script asked for
        COMI,R0 $FF
        BCTR,EQ FXT2
        BSTA,UN FXBEGIN
        LODI,R0 $FF
        LPUT    FXREQ
FXT2:   LODA,R0 FXON            ; (a stop leaves it off)
        RETC,EQ
        LODA,R0 DT
        STRA,R0 FXPASS
FXT3:   LODI,R0 FXOFF0
        STRA,R0 T3
        BSTA,UN FXSTEP
        LODI,R0 FXOFF1
        STRA,R0 T3
        BSTA,UN FXSTEP
        LODA,R0 FXPASS
        SUBI,R0 1
        STRA,R0 FXPASS
        BCFR,EQ FXT3
        LODI,R3 FXOFF0+AF_ST    ; neither running: off
        LODA,R0 *PRAMH,R3
        LODI,R3 FXOFF1+AF_ST
        IORA,R0 *PRAMH,R3
        RETC,GT
        LPUT    FXON
        RETC,UN

; AGET: R0 = field R3 of the actor at T3. Leaf. Clobbers R3.
AGET:   ADDA,R3 T3
        LODA,R0 *PRAMH,R3
        RETC,UN
; APUT: its field R3 = R0. Clobbers R0-R3.
APUT:   ADDA,R3 T3
        LODI,R1 FRRAMP2
        BCTA,UN RAMPUT
; AADD: field R1 += field R2. Clobbers R0-R3, T0.
AADD:   LODZ    R2
        STRZ    R3
        BSTR,UN AGET
        STRA,R0 T0
        LODZ    R1
        STRZ    R3
        BSTR,UN AGET
        ADDA,R0 T0
        STRZ    R2
        LODZ    R1
        STRZ    R3
        LODZ    R2
        BCTR,UN APUT

; FXSTEP: one frame of the actor at T3 (fx.c step): waiting, a step of
; its move, or its script read on until something takes time.
FXSTEP: LODI,R3 AF_ST
        BSTR,UN AGET
        RETC,EQ
        LODI,R3 AF_DUE          ; due? (frame - due >= 0)
        BSTR,UN AGET
        STRA,R0 T0
        LODA,R0 FRML
        SUBA,R0 T0
        RETC,LT
        LODI,R3 AF_N
        BSTR,UN AGET
        BCTA,EQ FXFETCH
        SUBI,R0 1               ; a step of the move
        LODI,R3 AF_N
        BSTR,UN APUT
        LODI,R1 AF_X
        LODI,R2 AF_DX
        BSTR,UN AADD
        LODI,R1 AF_Y
        LODI,R2 AF_DY
        BSTR,UN AADD
        LODI,R1 AF_DUE
        LODI,R2 AF_PER
        BSTA,UN AADD
        LODI,R3 AF_D2           ; the other frame, every other step
        BSTA,UN AGET
        EORI,R0 $80
        LODI,R3 AF_D2
        BSTA,UN APUT
        BCTA,UN FXDRAW

; ---- the script ------------------------------------------------------------------
; FXBYTE: R0 = its next byte (CC by it). Clobbers R3.
FXBYTE: LODA,R3 FXIX
        LODA,R0 *T0,R3,+
        STRA,R3 FXIX
        RETC,UN
; FXARG: the next byte into field R1. Clobbers R0-R3.
FXARG:  BSTR,UN FXBYTE
        STRZ    R2
        LODZ    R1
        STRZ    R3
        LODZ    R2
        BCTA,UN APUT

FXFETCH: LODI,R3 AF_PCH         ; T0:T1 = the script, read from FXIX + 1
        BSTA,UN AGET
        STRA,R0 T0
        LODI,R3 AF_PCL
        BSTA,UN AGET
        STRA,R0 T1
        LODI,R0 $FF
        STRA,R0 FXIX
FXOP:   BSTR,UN FXBYTE
        COMI,R0 FO_N
        BCFR,LT FXOEND          ; (a bad byte ends it)
        STRZ    R3              ; * 3: the table's BCTA
        ADDZ    R3
        ADDZ    R3
        STRZ    R3
        BXA     FXOTAB,R3
FXOTAB: BCTA,UN FXOEND
        BCTA,UN FXOPOS
        BCTA,UN FXOMOVE
        BCTA,UN FXOWAIT
        BCTA,UN FXOLOOK
        BCTA,UN FXOFRAME
        BCTA,UN FXOBG
        BCTA,UN FXOHIDE
        BCTA,UN FXOSIG
        BCTA,UN FXOLOOP
        BCTA,UN FXOCOL

FXOEND: EORZ    R0
        LODI,R3 AF_ST
        BCTA,UN APUT
FXOPOS: LODI,R1 AF_X
        BSTA,UN FXARG
        LODI,R1 AF_Y
        BSTA,UN FXARG
FXSHOW: LODI,R3 AF_KIND         ; shown again
        BSTA,UN AGET
        ANDI,R0 $7F
        LODI,R3 AF_KIND
        BSTA,UN APUT
        BCTA,UN FXOP
FXOMOVE: LODI,R1 AF_DX
        BSTA,UN FXARG
        LODI,R1 AF_DY
        BSTA,UN FXARG
        LODI,R1 AF_PER
        BSTA,UN FXARG
        LODI,R1 AF_N
        BSTA,UN FXARG
        LODI,R3 AF_PER          ; its first step: per frames on
        BSTA,UN AGET
        BCTR,UN FXOW1
FXOWAIT: BSTA,UN FXBYTE
FXOW1:  STRA,R0 FXPCOL          ; due = due + n (late steps catch up)
        LODI,R3 AF_DUE
        BSTA,UN AGET
        ADDA,R0 FXPCOL
        LODI,R3 AF_DUE
        BSTA,UN APUT
; the script waits here: its place kept (T0:T1 + FXIX + 1), then drawn
        LODA,R0 FXIX
        ADDI,R0 1
        ADDA,R0 T1
        STRA,R0 T1              ; (C: the carry into the page)
        PPSL    PSLWC
        LODA,R0 T0
        ADDI,R0 0
        CPSL    PSLWC
        LODI,R3 AF_PCH
        BSTA,UN APUT
        LODA,R0 T1
        LODI,R3 AF_PCL
        BSTA,UN APUT
        BCTA,UN FXDRAW
FXOLOOK: LODI,R1 AF_KIND
        BSTA,UN FXARG
        LODI,R1 AF_SLOT
        BSTA,UN FXARG
        LODI,R1 AF_ART
        BSTA,UN FXARG
        LODI,R1 AF_FRAME
        BSTA,UN FXARG
        LODI,R1 AF_D2
        BSTA,UN FXARG
        LODI,R0 $FF             ; nothing in its glyph yet
        LODI,R3 AF_SHOWN
        BSTA,UN APUT
        BSTA,UN FXBYTE          ; its colour (a ship: each slice)
        STRA,R0 FXPCOL
        BSTA,UN FXCOLS
        BCTA,UN FXOP
FXOFRAME: LODI,R1 AF_FRAME
        BSTA,UN FXARG
        BCTA,UN FXSHOW
FXOBG:  BSTA,UN FXBYTE          ; the ground: BGCB's colour bits
        STRZ    R1
        LODA,R0 BGCB
        ANDI,R0 $F8
        IORZ    R1
        STRA,R0 BGCB
        BCTA,UN FXOP
FXOHIDE: BSTA,UN FXHIDE
        LODI,R3 AF_KIND
        BSTA,UN AGET
        IORI,R0 FKHID
        LODI,R3 AF_KIND
        BSTA,UN APUT
        BCTA,UN FXOP
FXOSIG: BSTA,UN FXBYTE
        LPUT    FXFLAG
        BCTA,UN FXOP
FXOCOL: BSTA,UN FXBYTE
        STRA,R0 FXPCOL
        BSTA,UN FXCOLS
        BCTA,UN FXOP
; FO_LOOP n, back: REP counts down from n + 1; while it lasts, the script
; goes back to the loop's first byte (T0:T1 moved there: it may lie before
; where this fetch began)
FXOLOOP: LODI,R3 AF_REP
        BSTA,UN AGET
        STRA,R0 FXPCOL          ; (scratch: the count)
        BSTA,UN FXBYTE          ; n
        STRZ    R1
        LODA,R0 FXPCOL
        BCFR,EQ FXL1
        LODZ    R1
        ADDI,R0 1
FXL1:   SUBI,R0 1
        STRA,R0 FXPCOL
        LODI,R3 AF_REP
        BSTA,UN APUT
        BSTA,UN FXBYTE          ; back
        STRZ    R1
        LODA,R0 FXPCOL
        BCTA,EQ FXOP            ; done: on past it
        LODZ    R1              ; T0:T1 += FXIX - (back + 2)
        ADDI,R0 2
        STRZ    R1
        LODA,R0 FXIX
        SUBZ    R1              ; (a borrow: C clear)
        STRZ    R2
        PPSL    PSLWC
        LODI,R0 0
        SUBI,R0 0
        CPSL    PSLWC
        STRZ    R3              ; R3:R2 = the signed offset
        LODA,R0 T1
        ADDZ    R2
        STRA,R0 T1
        PPSL    PSLWC
        LODA,R0 T0
        ADDZ    R3
        CPSL    PSLWC
        STRA,R0 T0
        LODI,R0 $FF
        STRA,R0 FXIX
        BCTA,UN FXOP

; FXCOLS: the actor's sprite (a ship: all four) in colour FXPCOL. Clobbers
; R0-R3, T0, T1 (kept: the script's place).
FXCOLS: LODA,R0 T0
        STRA,R0 FXPT0
        LODA,R0 T1
        STRA,R0 FXPT1
        LODI,R3 AF_SLOT
        BSTA,UN AGET
        STRA,R0 FXPS
        LODI,R3 AF_KIND
        BSTA,UN AGET
        ANDI,R0 $7F
        COMI,R0 FK_SHIP
        BCFR,EQ FXC2
        LODI,R0 3
        STRA,R0 FXPS
FXC1:   LODA,R1 FXPS
        LODA,R2 FXPCOL
        ZBSR    *ZSPRCOL
        LODA,R0 FXPS
        SUBI,R0 1
        STRA,R0 FXPS
        BCTR,GT FXC1
        BCTR,EQ FXC1
        BCTR,UN FXC3
FXC2:   LODA,R1 FXPS
        LODA,R2 FXPCOL
        ZBSR    *ZSPRCOL
FXC3:   LODA,R0 FXPT0
        STRA,R0 T0
        LODA,R0 FXPT1
        STRA,R0 T1
        RETC,UN

; FXHIDE: the actor's sprite (a ship: all four) off. Clobbers R0-R3.
FXHIDE: LODI,R3 AF_KIND
        BSTA,UN AGET
        ANDI,R0 $7F
        COMI,R0 FK_SHIP
        BCTR,EQ FXHALL
        LODI,R3 AF_SLOT
        BSTA,UN AGET
        STRZ    R1
        ZBRR    *ZSPRHID
FXHALL: LODI,R1 0
        ZBSR    *ZSPRHID
        LODI,R1 1
        ZBSR    *ZSPRHID
        LODI,R1 2
        ZBSR    *ZSPRHID
        LODI,R1 3
        ZBRR    *ZSPRHID

; ---- drawing (fx.c draw) -------------------------------------------------------------
; FXDRAW: the actor at T3 as it is now: its picture into its glyph when it
; changed, its sprite placed, or off when it is off the screen.
FXDRAW: LODI,R3 AF_KIND
        BSTA,UN AGET
        TMI,R0  FKHID
        RETC,EQ                 ; hidden
        COMI,R0 FK_SHIP
        BCTA,EQ FXDSHIP
        LODI,R3 AF_D2           ; its picture: art + frame (+ d2)
        BSTA,UN AGET
        STRZ    R1
        EORZ    R0
        TMI,R1  $80
        BCFR,EQ FXD1
        LODZ    R1
        ANDI,R0 $7F
FXD1:   STRA,R0 FXPCOL          ; (scratch)
        LODI,R3 AF_FRAME
        BSTA,UN AGET
        ADDA,R0 FXPCOL
        STRA,R0 FXPCOL
        LODI,R3 AF_ART
        BSTA,UN AGET
        ADDA,R0 FXPCOL
        STRA,R0 FXPCOL          ; FXPCOL: the picture
        LODI,R3 AF_SHOWN
        BSTA,UN AGET
        COMA,R0 FXPCOL
        BCTR,EQ FXD2
        LODA,R0 UDCQN           ; (the queue full: next step)
        COMI,R0 3
        BCFR,LT FXD2
        LODI,R3 AF_SLOT
        BSTA,UN AGET
        STRZ    R2
        LODA,R0 FXPCOL
        BSTR,UN FXQUE
        LODA,R0 FXPCOL
        LODI,R3 AF_SHOWN
        BSTA,UN APUT
FXD2:   LODI,R3 AF_SLOT
        BSTA,UN AGET
        STRA,R0 FXPS
        EORZ    R0
        STRA,R0 FXPCOL          ; FXPCOL: the x offset (a ship's slices)
; FXPLACE: sprite FXPS at the actor's place + FXPCOL pixels, or off.
FXPLACE: LODI,R3 AF_Y           ; line: Y - 48, 1-207
        BSTA,UN AGET
        SUBI,R0 48
        BCTR,EQ FXPOFF
        COMI,R0 208
        BCFR,LT FXPOFF
        STRZ    R2
        LODI,R3 AF_X            ; pixel: X - 64, -7 to 127
        BSTA,UN AGET
        ADDA,R0 FXPCOL
        COMI,R0 57
        BCTR,LT FXPOFF
        COMI,R0 192
        BCFR,LT FXPOFF
        SUBI,R0 64
        STRZ    R3
        LODA,R1 FXPS
        ZBRR    *ZSPRSET
FXPOFF: LODA,R1 FXPS
        ZBRR    *ZSPRHID

; FXQUE: picture R0 (an art index) into glyph R2. Clobbers R0-R3, T0, T1.
FXQUE:  STRZ    R1              ; $0800 + index * 8
        RRL,R0
        RRL,R0
        RRL,R0
        ANDI,R0 $07
        IORI,R0 $08
        STRA,R0 FXPT0
        LODZ    R1
        RRL,R0
        RRL,R0
        RRL,R0
        ANDI,R0 $F8
        STRZ    R1
        LODA,R0 FXPT0
        LODI,R3 PG_ART
        ZBRR    *ZUDCQ

; FXDSHIP: four slices side by side, each shown once its glyph has loaded.
FXDSHIP: LODI,R3 AF_SHOWN
        BSTA,UN AGET
        COMI,R0 4
        BCTR,LT FXS1
        EORZ    R0              ; ($FF: none yet)
FXS1:   STRA,R0 FXPS
FXS2:   LODA,R0 FXPS            ; load what the queue takes
        COMI,R0 4
        BCTR,EQ FXS3
        LODA,R0 UDCQN
        COMI,R0 3
        BCFR,LT FXS3
        LODA,R0 FXPS            ; slice k: glyph k
        STRZ    R2
        LODI,R3 AF_ART
        BSTA,UN AGET
        ADDA,R0 FXPS
        BSTA,UN FXQUE
        LODA,R0 FXPS
        ADDI,R0 1
        STRA,R0 FXPS
        BCTR,UN FXS2
FXS3:   LODA,R0 FXPS
        STRA,R0 FXPLD
        LODI,R3 AF_SHOWN
        BSTA,UN APUT
        LODI,R0 0               ; place them: FXPS the slice, FXPCOL its x
FXS4:   STRA,R0 FXPS
        RRL,R0
        RRL,R0
        RRL,R0
        STRA,R0 FXPCOL
        LODA,R0 FXPS
        COMA,R0 FXPLD
        BCFR,LT FXS5
        BSTA,UN FXPLACE
        BCTR,UN FXS6
FXS5:   LODA,R1 FXPS            ; not loaded yet: off
        ZBSR    *ZSPRHID
FXS6:   LODA,R0 FXPS
        ADDI,R0 1
        COMI,R0 4
        BCFR,EQ FXS4
        RETC,UN

; ---- starting (events.c pr_event_start) ---------------------------------------------
; FXBEGIN: script set R0 (FX_*), or FX_STOP: what runs is stopped first.
FXBEGIN: STRA,R0 FXPCOL
        LODI,R0 FXOFF0
        STRA,R0 T3
        BSTR,UN FXQUIT
        LODI,R0 FXOFF1
        STRA,R0 T3
        BSTR,UN FXQUIT
        LODA,R0 FXPCOL
        COMI,R0 FX_N
        BCFR,LT FXB9            ; FX_STOP: off
        STRZ    R3              ; * 3: the table's BCTA
        ADDZ    R3
        ADDZ    R3
        STRZ    R3
        LODI,R0 FXOFF0          ; (actor 0, as most are)
        STRA,R0 T3
        BXA     FXBTAB,R3
FXB9:   EORZ    R0
        LPUT    FXON
        RETC,UN
FXQUIT: LODI,R3 AF_ST
        BSTA,UN AGET
        RETC,EQ
        BSTA,UN FXHIDE
        EORZ    R0
        LODI,R3 AF_ST
        BCTA,UN APUT

FXBTAB: BCTA,UN FXBLAND
        BCTA,UN FXBDEP
        BCTA,UN FXBPIR
        BCTA,UN FXBPEST
        BCTA,UN FXBACID
        BCTA,UN FXBSUN
        BCTA,UN FXBRAD
        BCTA,UN FXBMET
        BCTA,UN FXBFIRE
        BCTA,UN FXBAI

FXBLAND: LODI,R0 SCLAND>>8
        LODI,R1 SCLAND&$FF
        BCTA,UN FXGO
FXBDEP: LODI,R0 SCDEP>>8
        LODI,R1 SCDEP&$FF
        BCTA,UN FXGO
FXBPIR: LODI,R0 SCPIR>>8
        LODI,R1 SCPIR&$FF
        BCTA,UN FXGO
FXBPEST: LODI,R0 0              ; at its plot
        LODI,R1 0
        BSTA,UN FXAT
        LODI,R0 SCPEST>>8
        LODI,R1 SCPEST&$FF
        BCTA,UN FXGO
FXBACID: LODI,R0 16+64          ; the cloud: from the map's left, over its row
        LODI,R1 $F0             ; (-16)
        BSTA,UN FXATY
        LODI,R0 SCCLOUD>>8
        LODI,R1 SCCLOUD&$FF
        BSTA,UN FXGO
        LODI,R0 FXOFF1          ; its lightning under it
        STRA,R0 T3
        LODI,R0 16+64
        LODI,R1 0
        BSTA,UN FXATY
        LODI,R0 SCBOLT>>8
        LODI,R1 SCBOLT&$FF
        BCTA,UN FXGO
FXBSUN: LODI,R0 SCSUN>>8
        LODI,R1 SCSUN&$FF
        BSTA,UN FXGO
        LODI,R0 FXOFF1
        STRA,R0 T3
        LODI,R0 SCSUN2>>8
        LODI,R1 SCSUN2&$FF
        BCTR,UN FXGO
FXBRAD: LODI,R0 0               ; its M.U.L.E.
        LODI,R1 0
        BSTA,UN FXAT
        LODI,R0 SCRAD>>8
        LODI,R1 SCRAD&$FF
        BCTR,UN FXGO
FXBMET: LODI,R0 48              ; from up and to the right of its plot
        LODI,R1 $D0             ; (-48)
        BSTA,UN FXAT
        LODI,R0 SCMET>>8
        LODI,R1 SCMET&$FF
        BCTR,UN FXGO
FXBFIRE: LODI,R0 SCFIRE>>8
        LODI,R1 SCFIRE&$FF
        BSTR,UN FXGO
        LODI,R0 FXOFF1
        STRA,R0 T3
        LODI,R0 SCFIRE2>>8
        LODI,R1 SCFIRE2&$FF
        BCTR,UN FXGO

FXBAI:  LODI,R0 DVAIS0>>8        ; a computer's walk (p_devt.inc wrote it)
        LODI,R1 DVAIS0&$FF
        BSTR,UN FXGO
        LODI,R0 FXOFF1
        STRA,R0 T3
        LODI,R0 DVAIS1>>8
        LODI,R1 DVAIS1&$FF
        BCTR,UN FXGO

; FXGO: the actor at T3 runs the script at R0:R1, its first step now.
FXGO:   STRA,R0 FXPT0
        STRA,R1 FXPT1
        LODI,R3 AF_PCH          ; (one burst: ST is set last, below)
        ADDA,R3 T3
        LODI,R1 FRRAMP2
        BSTA,UN RAMSET
        LODA,R0 FXPT0
        BSTA,UN RAMPUT1         ; PCH, PCL
        LODA,R0 FXPT1
        BSTA,UN RAMPUT1
        LODI,R0 FKHID|FK_SINGLE ; KIND: hidden till it looks like something
        BSTA,UN RAMPUT1
        BSTA,UN RAMPEND
        EORZ    R0
        LODI,R3 AF_N
        BSTA,UN APUT
        EORZ    R0
        LODI,R3 AF_REP
        BSTA,UN APUT
        EORZ    R0
        LODI,R3 AF_D2
        BSTA,UN APUT
        LODA,R0 FRML
        LODI,R3 AF_DUE
        BSTA,UN APUT
        LODI,R0 1
        LODI,R3 AF_ST
        BCTA,UN APUT

; FXAT: the actor at T3 placed at the colony event's plot + (R0, R1)
; pixels and lines. FXATY: at pixel R0 - 64 (as a script's x), the plot's
; line + R1.
FXAT:   STRA,R1 FXPT1
        STRA,R0 FXPT0
        LODA,R2 C_PLOT
        COMI,R2 NUM_PLOTS
        BCTR,LT FXA1
        LODI,R2 0
FXA1:   LODA,R0 PLCOL,R2
        ADDI,R0 64
        ADDA,R0 FXPT0
        STRA,R0 FXPT0
        BCTR,UN FXA2
FXATY:  STRA,R1 FXPT1
        STRA,R0 FXPT0
FXA2:   LODA,R2 C_PLOT
        COMI,R2 NUM_PLOTS
        BCTR,LT FXA3
        LODI,R2 0
FXA3:   LODA,R0 PLROW,R2
        ADDI,R0 48
        ADDA,R0 FXPT1
        STRA,R0 FXPT1
        LODA,R0 FXPT0
        LODI,R3 AF_X
        BSTA,UN APUT
        LODA,R0 FXPT1
        LODI,R3 AF_Y
        BCTA,UN APUT

; the plot tables (mapscr.inc's)
PLROW:  DB      16,16,16,16,16,16,16,16,16
        DB      32,32,32,32,32,32,32,32,32
        DB      48,48,48,48,48,48,48,48,48
        DB      64,64,64,64,64,64,64,64,64
        DB      80,80,80,80,80,80,80,80,80
PLCOL:  DB      24,32,40,48,56,64,72,80,88
        DB      24,32,40,48,56,64,72,80,88
        DB      24,32,40,48,56,64,72,80,88
        DB      24,32,40,48,56,64,72,80,88
        DB      24,32,40,48,56,64,72,80,88

; ---- the scripts ---------------------------------------------------------------------
; The store is plot 22: pixel 56, lines 48-63. A ship is 32 x 16 and sits
; on the store's row with its nose (the left) at pixel 48.
SHIPL   EQU     48
SHIPX   EQU     48
DOCKX   EQU     56

; the transport ship lands (round 1's arrival, "the ship is back")
SCLAND: DB      FO_LOOK,FK_SHIP,0
        AIX     ART_TSHIP
        DB      0,0,SWHITE|$80
        DB      FO_POS
        XB      136
        YB      SHIPL
        DB      FO_WAIT,36
        DB      FO_MOVE,-1&$FF,0,1,176  ; a fast pass right to left
        DB      FO_HIDE,FO_WAIT,18
        DB      FO_POS
        XB      -32
        YB      SHIPL
        DB      FO_MOVE,1,0,6,62        ; a slow return
        DB      FO_MOVE,2,0,6,9         ; settling onto the store
        DB      FO_HIDE
SCDOCK: DB      FO_LOOK,FK_SINGLE,0     ; sinking into it
        AIX     ART_DOCK
        DB      0,0,SCYAN|$80
        DB      FO_POS
        XB      DOCKX
        YB      SHIPL
        DB      FO_WAIT,8
        DB      FO_FRAME,1,FO_WAIT,8,FO_FRAME,2,FO_WAIT,8
        DB      FO_FRAME,3,FO_WAIT,8,FO_HIDE
        DB      FO_SIG,1
        DB      FO_END

; the transport ship leaves (round 1's grant: "back in 12 months")
SCDEP:  DB      FO_WAIT,45
        DB      FO_LOOK,FK_SINGLE,0     ; rising out of the store
        AIX     ART_DOCK
        DB      3,0,SCYAN|$80
        DB      FO_POS
        XB      DOCKX
        YB      SHIPL
        DB      FO_WAIT,8
        DB      FO_FRAME,2,FO_WAIT,8,FO_FRAME,1,FO_WAIT,8
        DB      FO_FRAME,0,FO_WAIT,8,FO_HIDE
        DB      FO_LOOK,FK_SHIP,0
        AIX     ART_TSHIP
        DB      0,0,SWHITE|$80
        DB      FO_POS
        XB      SHIPX
        YB      SHIPL
        DB      FO_MOVE,-2&$FF,0,8,9
        DB      FO_MOVE,-1&$FF,0,8,90   ; off to the left
        DB      FO_HIDE
        DB      FO_SIG,1
        DB      FO_END

; pirates: a fast pass, back to the store, the raid, away
SCPIR:  DB      FO_LOOK,FK_SHIP,0
        AIX     ART_PSHIP
        DB      0,0,SRED|$80
        DB      FO_POS
        XB      136
        YB      SHIPL
        DB      FO_WAIT,36
        DB      FO_MOVE,-1&$FF,0,1,176
        DB      FO_HIDE,FO_WAIT,18
        DB      FO_POS
        XB      -32
        YB      SHIPL
        DB      FO_MOVE,1,0,4,62
        DB      FO_MOVE,2,0,6,9
        DB      FO_SIG,2                ; landed: "pirates take all crystite"
        DB      FO_WAIT,75,FO_WAIT,75
        DB      FO_MOVE,-2&$FF,0,8,9
        DB      FO_MOVE,-1&$FF,0,4,90
        DB      FO_HIDE
        DB      FO_SIG,1
        DB      FO_END

; the pest crawls round its plot, pausing at each turn
SCPEST: DB      FO_LOOK,FK_SINGLE,2
        AIX     ART_PEST
        DB      0,1,SWHITE
        DB      FO_FRAME,0
SCPST1: DB      FO_MOVE,1,0,5,3,FO_WAIT,11
        DB      FO_MOVE,0,2,5,4,FO_WAIT,11
        DB      FO_MOVE,-1&$FF,0,5,3,FO_WAIT,11
        DB      FO_MOVE,0,-2&$FF,5,4,FO_WAIT,11
        DB      FO_LOOP,6,$-SCPST1
        DB      FO_HIDE
        DB      FO_SIG,1
        DB      FO_END

; acid rain: the cloud drifts along its row, its colour cycling, its
; lightning flickering under it
SCCLOUD: DB     FO_LOOK,FK_SINGLE,2
        AIX     ART_CLOUD
        DB      0,0,SWHITE|$80
SCCL1:  DB      FO_FRAME,0,FO_COL,SWHITE|$80,FO_MOVE,1,0,7,3
        DB      FO_FRAME,1,FO_COL,SBLUE|$80,FO_MOVE,1,0,7,3
        DB      FO_FRAME,2,FO_COL,SWHITE|$80,FO_MOVE,1,0,7,3
        DB      FO_FRAME,3,FO_COL,SCYAN|$80,FO_MOVE,1,0,7,3
        DB      FO_LOOP,7,$-SCCL1
        DB      FO_HIDE
        DB      FO_SIG,1
        DB      FO_END
SCBOLT: DB      FO_LOOK,FK_SINGLE,3
        AIX     ART_BOLT
        DB      0,0,SYELLOW|$80
SCBL1:  DB      FO_FRAME,0,FO_MOVE,1,0,7,3
        DB      FO_HIDE,FO_MOVE,1,0,7,3
        DB      FO_FRAME,0,FO_MOVE,1,0,7,3
        DB      FO_HIDE,FO_MOVE,1,0,7,3
        DB      FO_LOOP,7,$-SCBL1
        DB      FO_HIDE
        DB      FO_END

; sunspot: the sky flashes, sparks flicker over the map
SCSUN:  DB      FO_BG,SYELLOW,FO_WAIT,3,FO_BG,SBLACK,FO_WAIT,6
        DB      FO_BG,SYELLOW,FO_WAIT,3,FO_BG,SBLACK
        DB      FO_LOOK,FK_SINGLE,2
        AIX     ART_SPARK
        DB      0,0,SWHITE
SCSN1:  DB      FO_POS
        XB      28
        YB      22
        DB      FO_WAIT,6,FO_POS
        XB      80
        YB      70
        DB      FO_WAIT,6,FO_POS
        XB      44
        YB      30
        DB      FO_WAIT,6,FO_POS
        XB      92
        YB      52
        DB      FO_WAIT,6,FO_POS
        XB      26
        YB      86
        DB      FO_WAIT,6,FO_POS
        XB      66
        YB      34
        DB      FO_WAIT,6,FO_POS
        XB      40
        YB      66
        DB      FO_WAIT,6,FO_POS
        XB      84
        YB      24
        DB      FO_WAIT,6
        DB      FO_LOOP,7,$-SCSN1
        DB      FO_HIDE
        DB      FO_SIG,1
        DB      FO_END
SCSUN2: DB      FO_LOOK,FK_SINGLE,3
        AIX     ART_SPARK
        DB      0,0,SYELLOW
        DB      FO_WAIT,3+6+3+6
SCSN2:  DB      FO_POS
        XB      56
        YB      80
        DB      FO_WAIT,6,FO_POS
        XB      30
        YB      44
        DB      FO_WAIT,6,FO_POS
        XB      90
        YB      90
        DB      FO_WAIT,6,FO_POS
        XB      70
        YB      18
        DB      FO_WAIT,6,FO_POS
        XB      34
        YB      96
        DB      FO_WAIT,6,FO_POS
        XB      94
        YB      38
        DB      FO_WAIT,6,FO_POS
        XB      24
        YB      60
        DB      FO_WAIT,6,FO_POS
        XB      60
        YB      56
        DB      FO_WAIT,6
        DB      FO_LOOP,7,$-SCSN2
        DB      FO_HIDE
        DB      FO_END

; radiation: the plot's M.U.L.E. flashes, then bolts off to the right
SCRAD:  DB      FO_LOOK,FK_SINGLE,1
        AIX     ART_MULE
        DB      0,1,SWHITE|$80
SCRD1:  DB      FO_FRAME,0,FO_WAIT,6,FO_HIDE,FO_WAIT,6
        DB      FO_LOOP,5,$-SCRD1
        DB      FO_FRAME,0
        DB      FO_MOVE,2,0,1,60
        DB      FO_HIDE
        DB      FO_SIG,1
        DB      FO_END

; a meteorite falls onto its plot: a flash, the crater a while
SCMET:  DB      FO_LOOK,FK_SINGLE,2
        AIX     ART_METEOR
        DB      0,0,SYELLOW
        DB      FO_FRAME,0
        DB      FO_MOVE,-1&$FF,1,2,48
        DB      FO_BG,SWHITE,FO_WAIT,2,FO_BG,SYELLOW,FO_WAIT,2,FO_BG,SBLACK
        DB      FO_LOOK,FK_SINGLE,2
        AIX     ART_CRATER
        DB      0,0,SRED|$80
        DB      FO_FRAME,0,FO_WAIT,90
        DB      FO_HIDE
        DB      FO_SIG,1
        DB      FO_END

; the store burns: two flames over it, flickering red and yellow (640
; frames, as long as the Atari's)
SCFIRE: DB      FO_LOOK,FK_SINGLE,2
        AIX     ART_FIRE
        DB      0,0,SRED
        DB      FO_POS
        XB      DOCKX
        YB      SHIPL
SCFR1:  DB      FO_FRAME,0,FO_COL,SRED,FO_WAIT,4
        DB      FO_FRAME,1,FO_COL,SYELLOW,FO_WAIT,4
        DB      FO_LOOP,79,$-SCFR1
        DB      FO_HIDE
        DB      FO_SIG,1
        DB      FO_END
SCFIRE2: DB     FO_LOOK,FK_SINGLE,3
        AIX     ART_FIRE
        DB      1,0,SYELLOW
        DB      FO_POS
        XB      DOCKX
        YB      SHIPL+8
SCFR2:  DB      FO_FRAME,1,FO_COL,SYELLOW,FO_WAIT,4
        DB      FO_FRAME,0,FO_COL,SRED,FO_WAIT,4
        DB      FO_LOOP,79,$-SCFR2
        DB      FO_HIDE
        DB      FO_END
