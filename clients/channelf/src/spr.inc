; ---------------------------------------------------------------------------
; spr.inc -- sprites (gfx.h spr_set / spr_hide / spr_flush) on a console that
; has none.
;
; A slot holds what the game wants (position, frame, colour) and what is on
; the screen, each as a list of the frame's horizontal RUNS of lit pixels
; ({dy * 32 + dx, length}), made from the frame's bitmap when the frame
; changes. SPRFLUSH, once a frame, puts a changed slot up in two passes:
;   1. the new runs are written, and tagged in the shadow with this flush's
;      generation (bits 7-2: the background value is bits 1-0);
;   2. the old runs' pixels not tagged with it get the background back (and
;      lose any old tag).
; A pixel in both images goes straight from the old colour to the new, so
; nothing flickers, and the shadow (the background, never the sprites) is
; how a moved sprite gets back what it covered. A tag left under a sprite on
; the screen only makes the background's diff rewrite that pixel.
;
; A background drawing under a sprite (disp.inc and text.inc call SPRTOUCH)
; marks it dirty: it is drawn again at the next flush. So is a sprite a
; neighbour's pass 2 went across.
;
; Positions come in the Atari's units (gfx.h) and are converted here:
;   x = 6 + (x - 8) * 5 / 8      y = 10 + (y - 16) / 4 (+ the shape's yoff)
; and kept biased by 32 so a sprite may hang off the left or the top. Only
; the visible area (columns 4-105, rows 4-61) is ever written.
; ---------------------------------------------------------------------------

SPRSLOTS EQU 10
S_WFL   EQU 0                   ; wanted: 1 visible
S_WX    EQU 1                   ;   x + 32
S_WY    EQU 2                   ;   y + 32
S_WC    EQU 3                   ;   colour (C_*)
S_WN    EQU 4                   ;   runs in S_WL
S_WW    EQU 5                   ;   width, height
S_WH    EQU 6
S_WSH   EQU 7                   ;   shape, frame, flip: the list's source
S_WFR   EQU 8
S_WFP   EQU 9
S_DFL   EQU 10                  ; on the screen: the same seven as S_WFL..
S_DX    EQU 11
S_DY    EQU 12
S_DC    EQU 13
S_DN    EQU 14
S_DW    EQU 15
S_DH    EQU 16
S_CHG   EQU 17                  ; 1 changed, 2 dirty (draw it again),
                                ; 4 the list changed
S_WL    EQU 18                  ; 27 runs: {dy * 32 + dx, length}
S_DL    EQU 73                  ; the runs on the screen
SPRMAXR EQU 27

SPR_FLIPX EQU 1
BIAS    EQU 32

SH_SPECIES EQU 0
SH_FIGURE EQU 1
SH_MULE EQU 2
SH_WAMPUS EQU 3
SH_CURSOR EQU 4
SH_NOSE EQU 5
SH_HULLT EQU 6
SH_HULLB EQU 7
SH_DOCK EQU 8
SH_PEST EQU 9
SH_CLOUD EQU 10
SH_BOLT EQU 11
SH_SPARK EQU 12
SH_METEOR EQU 13

SF_SLOT EQU SPT+0               ; the flush's slot
SF_PASS EQU SPT+1
SF_X    EQU SPT+2               ; the list being walked: at (x, y), colour,
SF_Y    EQU SPT+3               ; length (these four follow each other)
SF_C    EQU SPT+4
SF_N    EQU SPT+5
SF_GEN  EQU SPT+6               ; this flush's tag (1-63)
SP_X    EQU SPT+8               ; SPRSET: the new place
SP_Y    EQU SPT+9
SP_W    EQU SPT+10              ; SPRLIST: the frame's size
SP_H    EQU SPT+11
ST_SAVE EQU SPT+12              ; 5: SPRTOUCH keeps r0, r5-r8 here

; SLOTDC -- DC0 = slot r0's record (SPRS + 128 * r0).  Clobbers A, H.
SLOTDC  MACRO
        LR A,0
        SR 1
        AI SPRS>>8
        LR 10,A
        LR A,0
        SL 4
        SL 1
        SL 1
        SL 1
        LR 11,A
        LR DC,H
        ENDM

; SLOTF -- DC0 = field f of slot r0.  Clobbers A, H.
SLOTF   MACRO f
        SLOTDC
        LI f
        ADC
        ENDM

; ROWBASE -- H = SHADOW + 128 * r (the real row in A).  Clobbers A, r3.
ROWBASE MACRO
        LR 3,A
        SR 1
        AI SHADOW>>8
        LR 10,A
        LR A,3
        SL 4
        SL 1
        SL 1
        SL 1
        LR 11,A
        ENDM

; ---------------------------------------------------------------------------
; XCONVB / YCONVB -- A = the Atari position r0:r1 (signed) as a biased
;   Channel F one, or 0 when it is off any screen.  Clobber r0-r4.
; ---------------------------------------------------------------------------
XCONVB: ENTER
        LIS 8                   ; 6 + (x - 8) * 5 / 8
        LR 2,A
        CLR
        LR 3,A
        PI SUB16
        LR A,0
        LR 2,A
        LR A,1
        LR 3,A
        PI DBL16
        PI DBL16
        PI ADD16
        PI ASR16
        PI ASR16
        PI ASR16
        LI 6+BIAS
        LR 2,A
        CLR
        LR 3,A
        PI ADD16
        TAIL CONVOK

YCONVB: ENTER
        LI 16                   ; 10 + (y - 16) / 4
        LR 2,A
        CLR
        LR 3,A
        PI SUB16
        PI ASR16
        PI ASR16
        LI 10+BIAS
        LR 2,A
        CLR
        LR 3,A
        PI ADD16
        TAIL CONVOK

; CONVOK -- A = r0 if r0:r1 is 1..255, else 0.  Leaf.
CONVOK: LR A,1
        NI 0FFH
        BNZ CNVOFF
        LR A,0
        POP
CNVOFF: CLR
        POP

; ---------------------------------------------------------------------------
; SPRSET -- slot SPA_SL shows frame SPA_FR of shape SPA_SH at (SPA_X,
;   SPA_Y), colour SPA_CO, flipped if SPA_FL says so (gfx.h spr_set).
;   Clobbers A, r0-r9, H, Q, DC0, DC1.
; ---------------------------------------------------------------------------
SPRSET: ENTER
        LDW 0, 1, SPA_XL
        PI XCONVB
        LR 5,A
        NI 0FFH
        JZ SPSHID
        LR A,5
        STB SP_X
        LDW 0, 1, SPA_YL
        PI YCONVB
        LR 5,A
        NI 0FFH
        JZ SPSHID
        LR A,5
        STB SP_Y                ; (before the shape's yoff)
        LDB SPA_SH              ; its entry: SHTAB + 7 * shape
        LR 0,A
        SL 1
        SL 1
        SL 1
        LR 1,A
        LR A,0
        COM
        INC
        AS 1
        DCI SHTAB
        ADC
        LM
        STB SP_W                ; (DC0 moves: the entry again)
        DCI SHTAB
        LR A,0
        SL 1
        SL 1
        SL 1
        LR 1,A
        LR A,0
        COM
        INC
        AS 1
        ADC
        LM
        LM
        STB SP_H
        DCI SHTAB
        LR A,0
        SL 1
        SL 1
        SL 1
        LR 1,A
        LR A,0
        COM
        INC
        AS 1
        AI 3
        ADC
        LM                      ; yoff
        LR 3,A
        LDB SP_Y
        AS 3
        STB SP_Y
        ; a new list if the shape, frame or flip are not the slot's own
        LDB SPA_SL
        LR 0,A
        SLOTF S_WSH
        LM
        LR 1,A
        LM
        LR 2,A
        LM
        LR 3,A
        LDB SPA_SH
        XS 1
        BNZ SPSNEW
        LDB SPA_FR
        XS 2
        BNZ SPSNEW
        LDB SPA_FL
        NI SPR_FLIPX
        XS 3
        BNZ SPSNEW
        SLOTF S_WN              ; (and one made already)
        LM
        NI 0FFH
        BNZ SPSPOS
SPSNEW: PI SPRLIST
SPSPOS: LDB SPA_SL              ; place and colour: changed if not as wanted
        LR 0,A
        SLOTDC
        LM                      ; visible
        LR 1,A
        LM                      ; x
        LR 2,A
        LM                      ; y
        LR 3,A
        LM                      ; colour
        LR 4,A
        LDB SP_X
        LR 5,A
        LDB SP_Y
        LR 6,A
        LDB SPA_CO
        LR 7,A
        SLOTDC
        LIS 1
        ST
        LR A,5
        ST
        LR A,6
        ST
        LR A,7
        ST
        LR A,1
        CI 1
        BNZ SPSCH
        LR A,2
        XS 5
        BNZ SPSCH
        LR A,3
        XS 6
        BNZ SPSCH
        LR A,4
        XS 7
        BZ SPSX
SPSCH:  SLOTF S_CHG
        LM
        OI 1
        LR 1,A
        LI 0FFH
        ADC
        LR A,1
        ST
SPSX:   LEAVE
SPSHID: LDB SPA_SL              ; off any screen: hidden
        LR 0,A
        PI SPRHIDE
        LEAVE

; ---------------------------------------------------------------------------
; SPRLIST -- slot SPA_SL's wanted list, made from frame SPA_FR of shape
;   SPA_SH (mirrored if SPA_FL says so); its size and source noted, and the
;   slot marked changed.  Clobbers A, r0-r9, Q, H, DC0, DC1.
; ---------------------------------------------------------------------------
SPRLIST: ENTER
        LDB SPA_SH
        LR 7,A
        LDB SPA_FR
        LR 8,A
        LDB SPA_FL
        NI SPR_FLIPX
        LR 9,A
        LR A,7                  ; the entry: SHTAB + 7 * shape
        SL 1
        SL 1
        SL 1
        LR 1,A
        LR A,7
        COM
        INC
        AS 1
        DCI SHTAB
        ADC
        LM
        LR 4,A                  ; width
        LM
        LR 5,A                  ; rows
        LM
        LR 6,A                  ; bytes a row
        LM
        LM
        LM                      ; the frames
        LR QU,A
        LM
        LR QL,A
        LDB SPA_SL              ; the source and size, into the slot
        LR 0,A
        SLOTF S_WW
        LR A,4
        ST
        LR A,5
        ST
        LR A,7
        ST
        LR A,8
        ST
        LR A,9
        ST
        LR A,4
        STB SP_W
        LR A,5
        ST                      ; SP_H
        CLR                     ; a frame is rows * bytes a row
        LR 2,A
SPLZ:   LR A,2
        AS 6
        LR 2,A
        DS 5
        BNZ SPLZ
        LR A,8
        LR 0,A
        CLR
        LR 1,A
        PI MUL8                 ; r0:r1 = frame * its size
        LR A,QL
        AS 0
        LR QL,A
        LR A,QU
        LNK
        AS 1
        LR QU,A
        LR DC,Q
        XDC                     ; DC1 = the frame's rows
        LDB SPA_SL
        LR 0,A
        SLOTF S_WL              ; DC0 = the list
        LDB SP_W                ; (both DCI-free from here)
        LR 4,A
        LDB SP_H
        LR 6,A
        LDB SPA_SL
        LR 0,A
        SLOTF S_WL
        CLR
        LR 0,A                  ; row
        LR 1,A                  ; runs
SPLR:   CLR
        LR 2,A                  ; column
        LR 5,A                  ; bits left in r3
        LI 0FFH
        LR 7,A                  ; the run under way: its first column
SPLC:   LR A,5
        NI 0FFH
        BNZ SPLC1
        XDC
        LM
        XDC
        LR 3,A
        LIS 8
        LR 5,A
SPLC1:  LR A,3
        NI 080H
        BZ SPLC2
        LR A,7                  ; lit: a run starts or goes on
        INC
        BNZ SPLC1A
        LR A,2
        LR 7,A
        CLR
        LR 8,A
SPLC1A: LR A,8
        INC
        LR 8,A
        BR SPLC3
SPLC2:  LR A,7                  ; dark: a run under way ends
        INC
        BZ SPLC3
        PI SPLEMIT
SPLC3:  LR A,3
        SL 1
        LR 3,A
        DS 5
        LR A,2
        INC
        LR 2,A
        XS 4
        BNZ SPLC
        LR A,7                  ; the row's end ends a run
        INC
        BZ SPLR1
        PI SPLEMIT
SPLR1:  LR A,0                  ; the next row
        INC
        LR 0,A
        XS 6
        BNZ SPLR
        LDB SPA_SL              ; how many, and the slot changed
        LR 0,A
        SLOTF S_WN
        LR A,1
        ST
        SLOTF S_CHG
        LM
        OI 5
        LR 1,A
        LI 0FFH
        ADC
        LR A,1
        ST
        LEAVE

; SPLEMIT -- the run from column r7, r8 long, of row r0 into the list at
;   DC0 (mirrored if r9; while there is room); r7 = none.  Leaf.
;   Clobbers A, r10; r1 counts.
SPLEMIT: LR A,1
        CI SPRMAXR-1
        BNC SPLEX
        LR A,9                  ; mirrored: it starts at w - (r7 + r8)
        NI 0FFH
        LR A,7
        BZ SPLE1
        AS 8
        COM
        INC
        AS 4
SPLE1:  LR 10,A                 ; dx (r10: free in SPRLIST's loop)
        LR A,0
        SL 4
        SL 1
        AS 10
        ST                      ; dy * 32 + dx
        LR A,8
        ST                      ; length
        LR A,1
        INC
        LR 1,A
SPLEX:  LI 0FFH
        LR 7,A
        POP

; SPRHIDE -- slot r0 hidden (gfx.h spr_hide).  Leaf.  Clobbers A, r8, H, DC0.
SPRHIDE: SLOTDC
        LM
        NI 1
        BZ SPHX
        LI 0FFH
        ADC
        CLR
        ST
        LI S_CHG-1
        ADC
        LM
        OI 1
        LR 8,A
        LI 0FFH
        ADC
        LR A,8
        ST
SPHX:   POP

; SPRHALL -- every slot hidden.  Clobbers A, r0, r8, H, DC0.
SPRHALL: ENTER
        CLR
        LR 0,A
SPHA1:  PI SPRHIDE
        LR A,0
        INC
        LR 0,A
        CI SPRSLOTS-1
        BC SPHA1
        LEAVE

; SPROFF -- every slot hidden, and off the screen now.
SPROFF: ENTER
        PI SPRHALL
        PI SPRFLUSH
        LEAVE

; ---------------------------------------------------------------------------
; SPRTOUCH -- the background is being drawn over the rectangle at real
;   (r1, r2), r3 x r4: a sprite on the screen there is drawn again at the
;   next flush.  Leaf.  Keeps r0-r8 (the draws' arguments: DFILL's
;   colour is r0, DTEXT's r6).  Clobbers A, H, DC0.
; ---------------------------------------------------------------------------
SPRTOUCH: LDB SPANY
        NI 0FFH
        BNZ STOU0
        POP
STOU0:  DCI ST_SAVE               ; r0, r5-r8 kept
        LR A,0
        ST
        LR A,5
        ST
        LR A,6
        ST
        LR A,7
        ST
        LR A,8
        ST
        CLR
        LR 0,A
STOU1:  SLOTF S_DFL
        LM                      ; on the screen?
        NI 1
        JZ STOUN
        LM                      ; its x, y (biased)
        LR 5,A
        LM
        LR 6,A
        LM
        LM
        LM                      ; w, h
        LR 7,A
        LM
        LR 8,A
        ; apart if sx >= x + 32 + w, or x + 32 >= sx + sw (and so for y)
        LR A,1
        AS 3
        AI BIAS-1
        COM
        AS 5                    ; carries when sx > x + 32 + w - 1
        JC STOUN
        LR A,5
        AS 7
        AI 0FFH
        COM
        LR 7,A
        LR A,1
        AI BIAS
        AS 7                    ; carries when x + 32 > sx + sw - 1
        JC STOUN
        LR A,2
        AS 4
        AI BIAS-1
        COM
        AS 6
        JC STOUN
        LR A,6
        AS 8
        AI 0FFH
        COM
        LR 8,A
        LR A,2
        AI BIAS
        AS 8
        JC STOUN
        SLOTF S_CHG             ; it overlaps: dirty
        LM
        OI 2
        LR 5,A
        LI 0FFH
        ADC
        LR A,5
        ST
STOUN:  LR A,0
        INC
        LR 0,A
        CI SPRSLOTS-1
        JC STOU1
        DCI ST_SAVE
        LM
        LR 0,A
        LM
        LR 5,A
        LM
        LR 6,A
        LM
        LR 7,A
        LM
        LR 8,A
        POP

; ---------------------------------------------------------------------------
; SPRFLUSH -- put what changed on the screen (gfx.h spr_flush): once a frame.
;   Twice round: a sprite a neighbour's erasing crossed is dirty after the
;   first.
; ---------------------------------------------------------------------------
SPRFLUSH: ENTER
        LIS 2
        STB SF_PASS
SPFP:   CLR
        STB SF_SLOT
SPF1:   LDB SF_SLOT
        LR 0,A
        SLOTF S_CHG
        LM
        NI 0FFH
        BZ SPF2
        PI SPRDO
SPF2:   INCB SF_SLOT
        CI SPRSLOTS
        BNZ SPF1
        DECB SF_PASS
        BNZ SPFP
        CLR                     ; how many are on the screen now
        LR 4,A
        LR 0,A
SPF3:   SLOTF S_DFL
        LM
        NI 1
        BZ SPF4
        LR A,4
        INC
        LR 4,A
SPF4:   LR A,0
        INC
        LR 0,A
        CI SPRSLOTS-1
        BC SPF3
        LR A,4
        STB SPANY
        LEAVE

; ---------------------------------------------------------------------------
; SPRDO -- slot SF_SLOT's changes onto the screen.
; ---------------------------------------------------------------------------
SPRDO:  ENTER
        LDB SF_GEN              ; this flush's tag: the next one, 1-63
        INC
        NI 03FH
        BNZ SPDG
        LIS 1
SPDG:   STB SF_GEN
        PI SPRWANT              ; 1. the new image on, tagged
        NI 0FFH
        BZ SPD2
        PI SPRP1
SPD2:   LDB SF_SLOT             ; 2. the old image's untagged pixels off
        LR 0,A
        SLOTF S_DFL
        LM
        NI 1
        BZ SPD3
        LM
        STB SF_X
        LDB SF_SLOT
        LR 0,A
        SLOTF S_DY
        LM
        STB SF_Y
        LDB SF_SLOT
        LR 0,A
        SLOTF S_DN
        LM
        STB SF_N
        LDB SF_SLOT
        LR 0,A
        SLOTF S_DL
        XDC
        PI SPRP2
        PI SPRNEIGH             ; (it erased: a neighbour there is dirty)

SPD3:
SPD4:   LDB SF_SLOT             ; drawn = wanted
        LR 0,A
        SLOTF S_DFL
        XDC
        LDB SF_SLOT
        LR 0,A
        SLOTDC
        LIS 7
        LR 0,A
        CLR
        LR 1,A
        PI MEMCPY
        LDB SF_SLOT             ; the list too, if it changed
        LR 0,A
        SLOTF S_CHG
        LM
        NI 4
        BZ SPD5
        LDB SF_SLOT
        LR 0,A
        SLOTF S_DL
        XDC
        LDB SF_SLOT
        LR 0,A
        SLOTF S_WL
        LI 2*SPRMAXR
        LR 0,A
        CLR
        LR 1,A
        PI MEMCPY
SPD5:   LDB SF_SLOT
        LR 0,A
        SLOTF S_CHG
        CLR
        ST
        LEAVE

; SPRWANT -- if slot SF_SLOT is wanted on the screen: SF_X, SF_Y, SF_C,
;   SF_N its new place, colour and length, DC1 its list, A = 1; else A = 0.
SPRWANT: ENTER
        LDB SF_SLOT
        LR 0,A
        SLOTDC
        LM
        NI 1
        BZ SPWX
        LM
        STB SF_X                ; (SF_X, Y, C, N follow each other, as
        LDB SF_SLOT             ; S_WX, Y, C, N do)
        LR 0,A
        SLOTF S_WY
        LM
        LR 5,A
        LM
        LR 6,A
        LM
        LR 7,A
        DCI SF_Y
        LR A,5
        ST
        LR A,6
        ST
        LR A,7
        ST
        LDB SF_SLOT
        LR 0,A
        SLOTF S_WL
        XDC
        LIS 1
        LEAVE
SPWX:   CLR
        LEAVE

; ---------------------------------------------------------------------------
; The passes: the runs at DC1, SF_N of them, at (SF_X, SF_Y) biased.  A run
; is clipped to the visible area (both passes clip alike).  Leaves.
; Clobber A, r0-r9, H, DC0, DC1.
; ---------------------------------------------------------------------------

; RUNSET -- the next run from DC1: r2 = its row (biased), r5 = its first
;   column (biased), r4 = its length, both clipped; C set if any of it is
;   visible.  (r6 = SF_X, r7 = SF_Y.)  Clobbers A, r3.
RUNSET  MACRO
        XDC
        LM
        LR 3,A
        LM
        LR 4,A
        XDC
        LR A,3
        SR 4
        SR 1
        AS 7
        LR 2,A                  ; the row: 36..93
        CI VY0+BIAS-1
        BC m_no
        CI VY1+BIAS
        BNC m_no
        LR A,3
        NI 31
        AS 6
        LR 5,A                  ; the first column
        CI VX0+BIAS-1
        BNC m_l
        COM                     ; left of 36: d = 36 - x off the front
        INC
        AI VX0+BIAS
        LR 3,A
        LR A,4
        COM
        INC
        AS 3                    ; d + (256 - length) carries: d >= length,
        BC m_no                 ; none of it (never a 0-length run: 256 pixels)
        LR A,3
        COM
        INC
        AS 4
        LR 4,A
        LI VX0+BIAS
        LR 5,A
m_l:    LR A,5                  ; past 137: none
        CI VX1+BIAS
        BNC m_no
        AS 4                    ; the last column past 137: cut it there
        AI 0FFH
        CI VX1+BIAS
        BC m_ok
        LR A,5
        COM
        INC
        AI VX1+BIAS+1
        LR 4,A
m_ok:   CI 0FFH                 ; C set
        BR m_end
m_no:   NI 0                    ; C clear
m_end:
        ENDM

; RUNROW -- r8 = row r2, port 5 on it (the sound bits kept); r9 = it, real.
;   Clobbers A, r3.
RUNROW  MACRO
        LR A,2
        LR 8,A
        AI 256-BIAS
        LR 9,A
        COM
        NI 03FH
        LR 3,A
        INS 5
        NI 0C0H
        AS 3
        OUTS 5
        ENDM

; RUNAT -- DC0 = the shadow at the run's start, r9 = its column, complemented
;   (for port 4).  Clobbers A, r3, H.
RUNAT   MACRO
        LR A,2
        AI 256-BIAS
        ROWBASE
        LR DC,H
        LR A,5
        AI 256-BIAS
        ADC
        COM
        LR 9,A
        ENDM

; SPRP1 -- pass 1: the runs in colour SF_C, tagged SF_GEN.
SPRP1:  LDB SF_X
        LR 6,A
        LDB SF_Y
        LR 7,A
        LDB SF_GEN
        SL 1
        SL 1
        LR 0,A
        LDB SF_N
        LR 1,A
        NI 0FFH
        JZ SP1X
        LI 0FFH
        LR 8,A                  ; the row the ports are on: none
SP1R:   RUNSET
        JNC SP1N
        LR A,2
        XS 8
        BZ SP1S
        RUNROW                  ; a new row: the colour's value on it
        DCI ROWKEY
        LR A,9
        ADC
        LM
        LR 3,A
        LDB SF_C
        AS 3
        DCI COLTAB
        ADC
        LM
        SL 4
        SL 1
        SL 1
        COM
        OUTS 1
SP1S:   RUNAT
SP1P:   LM                      ; tag it, keeping the background value
        NI 3
        AS 0
        LR 3,A
        LI 0FFH
        ADC
        LR A,3
        ST
        LR A,9
        OUTS 4
        LI 060H
        OUTS 0
        LI 050H
        OUTS 0
        SETTLE
        DS 9
        DS 4
        BNZ SP1P
SP1N:   DS 1
        JNZ SP1R
SP1X:   POP

; SPRP2 -- pass 2: each pixel of the runs not tagged SF_GEN gets the
;   background back, untagged.
SPRP2:  LDB SF_X
        LR 6,A
        LDB SF_Y
        LR 7,A
        LDB SF_GEN
        LR 0,A
        LDB SF_N
        LR 1,A
        NI 0FFH
        JZ SP2X
        LI 0FFH
        LR 8,A
SP2R:   RUNSET
        JNC SP2N
        LR A,2
        XS 8
        BZ SP2S
        RUNROW
SP2S:   RUNAT
SP2P:   LM
        LR 3,A
        SR 1
        SR 1
        XS 0                    ; the new image's: it stays
        BZ SP2K
        LR A,3
        NI 3
        LR 3,A
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
SP2K:   DS 9
        DS 4
        BNZ SP2P
SP2N:   DS 1
        JNZ SP2R
SP2X:   POP

; ---------------------------------------------------------------------------
; SPRNEIGH -- slot SF_SLOT's old image was erased: any slot on the screen
;   whose image it crossed is dirty (drawn again). The slot itself is marked
;   too, harmlessly: SPRDO clears its flags next.
;   Clobbers A, r0-r8, H, DC0.
; ---------------------------------------------------------------------------
SPRNEIGH: ENTER
        LDB SF_SLOT
        LR 0,A
        SLOTF S_DX
        LM
        AI 256-BIAS
        LR 1,A                  ; x, y (real), w, h
        LM
        AI 256-BIAS
        LR 2,A
        LM
        LM
        LM
        LR 3,A
        LM
        LR 4,A
        PI SPRTOUCH
        LEAVE
