; ---------------------------------------------------------------------------
; game.inc -- small helpers the pages share (pages never call each other):
; a seat's record and colour, a species standing, a compare.
; ---------------------------------------------------------------------------

PLOFS:  DB 0, PLSIZE, 2*PLSIZE, 3*PLSIZE

; PLDC -- DC0 = seat r0's record in ST.  Leaf.  Clobbers A.
PLDC:   DCI PLOFS
        LR A,0
        ADC
        LM
        DCI ST+PLAYERS
        ADC
        POP

; PLCOLR -- A = seat r0's colour (mule.h PLCOL).  Leaf.  Clobbers DC0.
PLCOLR: DCI PLOFS
        LR A,0
        ADC
        LM
        AI P_COL
        DCI ST+PLAYERS
        ADC
        LM
        NI 3
        DCI PLCOLT
        ADC
        LM
        POP

; SPSPEC -- slot r0 shows seat r5's species, frame species * 11 + r6, at the
;   Atari's (r1, r2), in the seat's colour, or SPCOL's if not 0.
SPSPEC: ENTER
        LR A,0
        STB SPA_SL
        LIS SH_SPECIES
        STB SPA_SH
        LR A,1
        STB SPA_XL
        CLR
        ST
        LR A,2
        STB SPA_YL
        CLR
        ST
        CLR
        STB SPA_FL
        LDB SPCOL               ; (0: the seat's own)
        NI 0FFH
        BNZ SPS1
        LR A,5
        LR 0,A
        PI PLCOLR
SPS1:   STB SPA_CO
        LR A,5
        LR 0,A
        PI PLDC
        LI P_SPEC
        ADC
        LM
        NI 7
        LR 4,A
        SL 1
        SL 1
        SL 1
        AS 4
        AS 4
        AS 4
        AS 6
        STB SPA_FR
        PI SPRSET
        LEAVE

; MEMCMP -- A = 0 if the r0 bytes at DC0 and DC1 are the same.  Leaf.
;   Clobbers r0, r1, DC0, DC1.
MEMCMP: LM
        LR 1,A
        XDC
        LM
        XDC
        XS 1
        BNZ MCX
        DS 0
        BNZ MEMCMP
        CLR
MCX:    POP


; ---------------------------------------------------------------------------
; SPECTAG -- watching (no seat): "WATCH" in the top left corner (text.c
;   spec_tag, every 32 frames). Only where the corner is blank: a repaint
;   takes it off, a long title keeps it off, and once drawn it stays.
;   (The C client's "SPECTATOR" leaves the titles no room on 24 columns;
;   the lobby says WATCHING GAME itself.)
; ---------------------------------------------------------------------------
SPECTAG: ENTER
        LDB SCRNOW
        NI 0FFH
        JZ SPTX
        LIS 4                   ; rows 4..9, x 6..29 all ground (the tag
                                ; and a cell's gap before any title)?
        LR 2,A
SPT1:   LR A,2
        SR 1
        AI SHADOW>>8
        LR 10,A
        LR A,2
        SL 4
        SL 1
        SL 1
        SL 1
        AI 6
        LR 11,A
        LR DC,H
        LI 24
        LR 3,A
SPT2:   LM
        NI 3
        JNZ SPTX
        DS 3
        BNZ SPT2
        LR A,2
        INC
        LR 2,A
        CI 9
        BC SPT1
        LIS 6
        LR 1,A
        LIS 4
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI S_WATCH
        PI DTEXT
SPTX:   LEAVE
S_WATCH: DB "WATCH",0
