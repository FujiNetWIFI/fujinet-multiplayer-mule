; gfx.inc -- the drawing the pages share (gfx.h), as resident wrappers
; around the sprite page (sprpg.inc) and the map page (mappg.inc).

; shapes (gfx.h SH_*: the order of SHTAB)
SH_SPEC EQU     0               ; frame = species * 11 + f
SH_FIG  EQU     1               ; frame = species * 10 + f
SH_MULE EQU     2
SH_WAMP EQU     3
SH_CURS EQU     4
SH_NOSE EQU     5
SH_HTOP EQU     6
SH_HBOT EQU     7
SH_DOCK EQU     8
SH_PEST EQU     9
SH_CLOUD EQU    10
SH_BOLT EQU     11
SH_SPARK EQU    12
SH_METEOR EQU   13

; SPRSET: slot A as SPA_* (shape, frame, x, Atari y, colour, flags) say.
; (The sprite page works through IX: the wrappers keep the caller's.)
SPRSET: PUSH    IX
        CALL    FARCALL
        DB      PGSPR
        DW      XSPRSET
        POP     IX
        RET

; SPRSETP: slot A from the 8 bytes at HL (shape, frame, x lo/hi, y lo/hi,
; colour, flags) -- the caller's page may hold them.
SPRSETP: PUSH   BC
        PUSH    DE
        LD      DE,SPA_SH
        LD      BC,8
        LDIR
        POP     DE
        POP     BC
        JR      SPRSET

; SPRHIDE: slot A off the screen.
SPRHIDE: PUSH   IX
        CALL    FARCALL
        DB      PGSPR
        DW      XSPRHIDE
        POP     IX
        RET

; SPROFF: every slot off (gfx.h spr_hide_all).
SPROFF: PUSH    IX
        CALL    FARCALL
        DB      PGSPR
        DW      XSPRALL
        POP     IX
        RET

; SPRLIFT / SPRDROP: every sprite off, to draw the background under them,
; and back on.
SPRLIFT: PUSH   IX
        CALL    FARCALL
        DB      PGSPR
        DW      XSPRLIFT
        POP     IX
        RET
SPRDROP: PUSH   IX
        CALL    FARCALL
        DB      PGSPR
        DW      XSPRDROP
        POP     IX
        RET

; PLOT: plot A as PLK_* say (gfx.h gfx_plot).
PLOT:   CALL    FARCALL
        DB      PGMAP
        DW      XPLOT
        RET

; TOWN: the town's picture (gfx.h gfx_town).
TOWN:   CALL    FARCALL
        DB      PGMAP
        DW      XTOWN
        RET

; FILL: byte column B, line C, D bytes wide, E lines, colour A.
GFILL:  CALL    FARCALL
        DB      PGMAP
        DW      XFILL
        RET

; PIC: picture A at byte column B, line C, colour E.
PIC:    CALL    FARCALL
        DB      PGMAP
        DW      XPIC
        RET

; ---- the map (the map page) ---------------------------------------------------
SCR_NONE EQU    0
SCR_TEXT EQU    1
SCR_MAP EQU     2
SCR_TOWN EQU    3
SCR_AUC EQU     4
SCR_SUMM EQU    5
SCR_FOUND EQU   6

AN_NONE EQU     0
AN_WALK EQU     1
AN_GRANT EQU    2
AN_AUC  EQU     3

; (Plots are drawn under the sprites: lifted first, or XOR leaves ghosts.)
MAPPLOT: PUSH   AF              ; plot A from the state
        CALL    SPRLIFT
        POP     AF
        CALL    FARCALL
        DB      PGMAP
        DW      XMAPPLOT
        JP      SPRDROP
MAPALL: CALL    SPRLIFT
        CALL    FARCALL
        DB      PGMAP
        DW      XMAPALL
        JP      SPRDROP
MAPUPD: CALL    SPRLIFT
        CALL    FARCALL
        DB      PGMAP
        DW      XMAPUPD
        JP      SPRDROP
MAPENTER: CALL  FARCALL
        DB      PGMAP
        DW      XMAPENTER
        RET
BARDRAW: CALL   FARCALL         ; the time bar at A (Atari lines)
        DB      PGMAP
        DW      XBARDRAW
        RET

; HDOTS: a dotted line at byte column B, Atari line C, D bytes, colour A.
HDOTS:  CALL    FARCALL
        DB      PGMAP
        DW      XHDOTS
        RET
; AFILL: x B (colour clocks), Atari lines C..C+E, D bytes wide, colour A.
AFILL:  CALL    FARCALL
        DB      PGMAP
        DW      XAFILL
        RET
; APIC: picture A at x B (colour clocks), Atari line C, colour E.
APIC:   CALL    FARCALL
        DB      PGMAP
        DW      XAPIC
        RET
