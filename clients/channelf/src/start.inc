; ---------------------------------------------------------------------------
; start.inc -- power-up, the pages, the clock.
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; START -- the BIOS jumps here ($0802) at power-up and on RESET. The BIOS has
; zeroed the scratchpad and set r59 = 40 (the K-stack). RAM is the cart's and
; outlives a RESET, so everything is set here.
; ---------------------------------------------------------------------------
START:  DI
        LISU 7                  ; r59 = KSTK0: the K-stack empty
        LISL 3
        LI KSTK0
        LR S,A
        DCI VARS                ; the globals, the buffers, the tables: 0
        LI (RAMEND-VARS)>>8
        LR 1,A
        CLR
        LR 0,A
STZL:   CLR
        ST
        DS 0
        BNZ STZL
        DS 1
        BNZ STZL
        LIS PGDATA              ; the strings, scripts and art into RAM
        LR 9,A
        PI SELPG
        DCI PGWIN
        XDC
        DCI DATA
        XDC
        LI (DATALEN+255)>>8
        LR 1,A
        CLR
        LR 0,A
STCPL:  LM
        XDC
        ST
        XDC
        DS 0
        BNZ STCPL
        DS 1
        BNZ STCPL
        LIS PGBOOT
        LR 9,A
        PI SELPG
        PI DINIT
        PI FNCHK
        BZ STOK1
        JMP NOCART
STOK1:  LDB FNPVER
        CI 1
        BNC STOK2               ; protocol 2 or later
        JMP OLDCART
STOK2:
        IF GALTEST=1
        JMP SPRGAL
        ENDIF
        IF GALTEST=2
        JMP FXGAL
        ENDIF
        JMP MAIN

; No FujiNet cart answering, or one too old to page: say so and stop.
NOCART: PRTXT 4, 28, S_NOCART, C_WHITE
HALT:   BR HALT
OLDCART: PRTXT 1, 28, S_OLDCART, C_WHITE
        BR HALT
S_NOCART: DB "NO FUJINET FOUND",0
S_OLDCART: DB "CART FIRMWARE TOO OLD",0

; FNCHK -- Z set if the cart's 'F','N' magic is there.  Leaf; clobbers A, DC0.
FNCHK:  DCI FNMAG0
        LM
        CI 'F'
        BNZ FNCHKX
        LM
        CI 'N'
FNCHKX: POP

; ---------------------------------------------------------------------------
; SELPG -- map page r9 at $4800.  Leaf; keeps DC0.  Clobbers A, DC1.
; With PGPUSH, the only stores to FNBANK (tools/checkcalls.py).
; ---------------------------------------------------------------------------
SELPG:  XDC
        LR A,9
        DCI CURPG
        ST
        DCI FNBANK
        ADC
        ST                      ; any value: the page is mapped from here on
        XDC
        POP

; PGPUSH -- remember the page mapped now, then map page r9.  Leaf.
;   Clobbers A, r8, DC0.
PGPUSH: DCI CURPG
        LM
        LR 8,A                  ; the page now
        LM                      ; (CURPG, PGSP, PGSTK follow each other)
        ADC                     ; DC0 = PGSTK + depth
        LR A,8
        ST
        DCI PGSP
        LM
        INC
        DCI PGSP
        ST
        LR A,9
        DCI CURPG
        ST
        DCI FNBANK
        ADC
        ST
        POP

; PGPOP -- map the page PGPUSH remembered.  Leaf.  Clobbers A, r8, r9, DC0, DC1.
PGPOP:  DCI PGSP
        LM
        AI 0FFH
        LR 8,A
        DCI PGSP
        ST                      ; DC0 = PGSTK
        LR A,8
        ADC
        LM
        LR 9,A
        JMP SELPG               ; (its POP returns for us)

; FAR -- call ent, which is in page pg, from anywhere (ENTER first).  Results
; come back in r0-r6; A, r7-r9, DC0 and DC1 do not survive.
FAR     MACRO pg, ent
        LIS pg
        LR 9,A
        PI PGPUSH
        PI ent
        PI PGPOP
        ENDM

STUB:   POP

; ---------------------------------------------------------------------------
; TICKRD -- r0:r1 = the cart's tick (60 Hz).  The bytes change together but
;   are read one at a time: HI, LO, HI, again if HI moved.  Leaf.
;   Clobbers A, DC0.
; ---------------------------------------------------------------------------
TICKRD: DCI FNTICK+1
        LM
        LR 1,A
        DCI FNTICK
        LM
        LR 0,A
        LM
        XS 1
        BNZ TICKRD
        POP

; FRWAIT -- wait for the tick to move on from FRAMES; FRAMES = the tick.
;   Leaf.  Clobbers A, r0, r1, DC0.
FRWAIT: DCI FNTICK+1
        LM
        LR 1,A
        DCI FNTICK
        LM
        LR 0,A
        LM
        XS 1
        BNZ FRWAIT
        DCI FRAMES
        LM
        XS 0
        BNZ FRWNEW
        LM
        XS 1
        BZ FRWAIT
FRWNEW: DCI FRAMES
        LR A,0
        ST
        LR A,1
        ST
        POP

        IF GALTEST=1
; ---------------------------------------------------------------------------
; SPRGAL -- the sprites' check (GALTEST=1 builds): walkers crossing over
; coloured boxes, a cloud, a ship hanging off the right edge; then all of
; them hidden (the harness then checks VRAM against the shadow).
; ---------------------------------------------------------------------------
GT_T    EQU GT+20
SPRGAL: PI SNDINIT
        PI BANDS
        LI 30                   ; a box of each player colour
        LR 1,A
        LI 20
        LR 2,A
        LIS 8
        LR 3,A
        LIS 8
        LR 4,A
        LIS C_BLUE
        LR 0,A
        PI DFILL
        LI 50
        LR 1,A
        LI 20
        LR 2,A
        LIS 8
        LR 3,A
        LIS 8
        LR 4,A
        LIS C_P3
        LR 0,A
        PI DFILL
        PRTXT 0, 36, S_SPRT, C_PURPLE
        CLR
        STB GT_T
SGL:    ; slot 0: species 0 walking right, slot 1: species 3 left
        CLR
        STB SPA_SL
        STB SPA_SH
        STB SPA_FL
        LDB GT_T
        SR 1
        SR 1
        SR 1
        NI 1
        AI 2
        STB SPA_FR
        LDB GT_T
        STB SPA_XL
        CLR
        STB SPA_XH
        LI 90
        STB SPA_YL
        CLR
        STB SPA_YH
        LIS C_BLUE
        STB SPA_CO
        PI SPRSET
        LIS 1
        STB SPA_SL
        LI 33+2
        STB SPA_FR
        LI 150                  ; x = 150 - t
        LR 4,A
        LDB GT_T
        COM
        INC
        AS 4
        STB SPA_XL
        CLR
        STB SPA_XH
        LIS C_P3
        STB SPA_CO
        PI SPRSET
        LIS 2                   ; slot 2: the cloud, x = t / 2, y = 40
        STB SPA_SL
        LI SH_CLOUD
        STB SPA_SH
        CLR
        STB SPA_FR
        LDB GT_T
        SR 1
        STB SPA_XL
        LI 40
        STB SPA_YL
        LIS C_GREEN
        STB SPA_CO
        PI SPRSET
        LIS 3                   ; slot 3: a hull, x = 140 + t / 4 (off right)
        STB SPA_SL
        LI SH_HULLT
        STB SPA_SH
        LDB GT_T
        SR 1
        SR 1
        AI 140
        STB SPA_XL
        LI 110
        STB SPA_YL
        LIS C_PURPLE
        STB SPA_CO
        PI SPRSET
        PI SPRFLUSH
        PI FRWAIT
        INCB GT_T
        CI 160
        JNZ SGL
        PI SPRHALL              ; all of them off
        PI SPRFLUSH
        LIS 1
        STB GT_T+1
SGH:    BR SGH
S_SPRT: DB "SPRITES",0
        ENDIF

        IF GALTEST=2
; ---------------------------------------------------------------------------
; FXGAL -- the set pieces' check (GALTEST=2 builds): each fx script over the
; test map in turn, its length in ticks at PV+16 + 2n; then everything off
; (the harness checks VRAM against the shadow). PV+1 = the piece running.
; ---------------------------------------------------------------------------
FG_N    EQU PV+1
FG_T0   EQU PV+2
FG_P    EQU PV+4
FG_LEN  EQU PV+16
FXGAL:  CLR
        STB TESTHK+1
        STB TESTHK+2
        STB TESTHK+3
        PI TESTST
        PI SNDINIT
        PI BANDS
        PI MAPENTER
        LI FGFIRST
        STB FG_N
FGL:    LDB FG_N                ; actor 0, then actor 1
        SL 1
        SL 1
        SL 1
        DCI FGTAB
        ADC
        CLR
        LR 0,A
        PI FGACT
        LDB FG_N
        SL 1
        SL 1
        SL 1
        AI 4
        DCI FGTAB
        ADC
        LIS 1
        LR 0,A
        PI FGACT
        LDW 0, 1, FRAMES
        STW 0, 1, FG_T0
        STW 0, 1, DONE
FGR:    PI SPRFLUSH             ; as MAINLP: the steps the frames gone by owe
        PI FRWAIT
        PI CATCHN
FGC:    LDB NCATCH
        NI 0FFH
        BZ FGD
        AI 0FFH
        STB NCATCH
        PI FXRUN
        BR FGC
FGD:    DCI FXA+AC_ST
        LM
        DCI FXA+FXSZ+AC_ST
        OM
        BNZ FGR
        LDW 0, 1, FRAMES        ; its length in ticks
        LDW 2, 3, FG_T0
        PI SUB16
        LDB FG_N
        SL 1
        DCI FG_LEN
        ADC
        LR A,0
        ST
        LR A,1
        ST
        LI 30
        STB FG_P
FGP:    PI SPRFLUSH
        PI FRWAIT
        DECB FG_P
        BNZ FGP
        INCB FG_N
        CI FGFIRST+FGCOUNT
        JNZ FGL
        PI FXSTOP
        PI SPRFLUSH
        LI 0FFH
        STB FG_N
FGH:    BR FGH

; FGACT -- actor r0 at DC0's {DW script (0: none), DB x, y (XB bytes)}.
FGACT:  ENTER
        LM
        LR 10,A
        LM
        LR 11,A
        LM
        LR 4,A
        LM
        LR 6,A
        LR A,10                 ; (no script lives in page zero)
        NI 0FFH
        BZ FGAX
        LR A,4                  ; x, y = byte - 64
        AI 0C0H
        LR 4,A
        LIS 0
        BC FGA1
        LI 0FFH
FGA1:   LR 5,A
        LR A,6
        AI 0C0H
        LR 6,A
        LIS 0
        BC FGA2
        LI 0FFH
FGA2:   LR 7,A
        PI FXAT
        LIS AC_XL
        ADC
        LR A,4
        ST
        LR A,5
        ST
        LR A,6
        ST
        LR A,7
        ST
        PI FXSTART
FGAX:   LEAVE

; {actor 0, actor 1}: land, depart, pirate, a pest on plot 10, acid rain
; along row 1, sunspots, radiation on plot 12, a meteorite onto plot 20
FGTAB:  DW FXLAND
        DB 0, 0
        DW 0
        DB 0, 0
        DW FXDEPART
        DB 0, 0
        DW 0
        DB 0, 0
        DW FXPIRATE
        DB 0, 0
        DW 0
        DB 0, 0
        DW FXPEST
        DB XB(24), XB(48)
        DW 0
        DB 0, 0
        DW FXCLOUD
        DB XB(0), XB(48)
        DW FXBOLT
        DB XB(0), XB(72)
        DW FXSPARKS
        DB 0, 0
        DW FXSPARK2
        DB 0, 0
        DW FXRADIA
        DB XB(64), XB(56)
        DW 0
        DB 0, 0
        DW FXMETEOR
        DB XB(160), XB(-40)
        DW 0
        DB 0, 0
        ENDIF

S_GAL:  DB "M.U.L.E. CHANNEL F",0
S_ABC:  DB "ABCDEFGHIJKLMNOPQRSTUVWX",0
S_DIG:  DB "YZ 0123456789 $%#!?.,:-+",0
S_PUN:  DB "*/()<>=@[]^_'",22H,"&;",0
S_PG:   DB "PAGES",0
