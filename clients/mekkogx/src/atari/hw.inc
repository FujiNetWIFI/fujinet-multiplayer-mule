;
; hw.inc -- the Atari port's chips and graphics memory, for the assembly
; (atari.h has the same for C). The 5200 (src/atari5200) assembles these
; files too: the same ANTIC, GTIA and POKEY, GTIA and POKEY at other
; addresses, and the graphics at the top of its 16K, laid out as here
; ($8000 lower).
;
;   DYNCH   the drawing charset (mode 4 cells made on the fly)
;   FONT    the original's font, its 64 characters (modes 2 and 6)
;   MAPCH   the original's map characters, 64 (mode 6)
;   TOWNCH  the original's town characters (mode 4)
;   SCREEN  24 rows x 40 bytes (a mode 6 row uses the first 20)
;   DLIST   PMBASE: the display list and DLI tables (PMGFREE), then the
;           missiles and players 0-3
;
.ifdef __ATARI5200__
GTIA    = $C000
POKEY   = $E800
GFX     = $2800
.else
GTIA    = $D000
POKEY   = $D200
GFX     = $A800
.endif
ANTIC   = $D400

DYNCH   = GFX
FONT    = GFX + $0400
MAPCH   = GFX + $0600
TOWNCH  = GFX + $0800
SCREEN  = GFX + $0C00
DLIST   = GFX + $1000
MISSILES = DLIST + $0300
PLAYERS = DLIST + $0400

HPOSP0  = GTIA + $00
HPOSM0  = GTIA + $04
SIZEP0  = GTIA + $08
SIZEM   = GTIA + $0C
COLPM0  = GTIA + $12
COLPF0  = GTIA + $16
COLBK   = GTIA + $1A
PRIOR   = GTIA + $1B
GRACTL  = GTIA + $1D

DMACTL  = ANTIC + $00
PMBASE  = ANTIC + $07
CHBASE  = ANTIC + $09
WSYNC   = ANTIC + $0A
NMIEN   = ANTIC + $0E

AUDF1   = POKEY + $00
AUDC1   = POKEY + $01
