;
; a78.inc -- the 7800 port's hardware and memory map (C: a78hw.h).
;
; MARIA (never TIA: a store to $00-$1F rewrites INPTCTRL, which the
; FujiNet cartridge keeps unlocked)
BACKGRND = $20
WSYNC    = $24          ; MARIA's
MSTAT    = $28          ; bit 7: vertical blank
DPPH     = $2C
DPPL     = $30
CHARBASE = $34
OFFSET   = $38
CTRL     = $3C
P4C1     = $31          ; palette n colour c: $20 + 4n + c

; The cart's POKEY and the console's RIOT
POKEY    = $0450
AUDCTL   = POKEY + 8
SKCTL    = POKEY + $0F
RANDOM   = POKEY + $0A
SWCHA    = $0280
CTLSWA   = $0281
SWCHB    = $0282
CTLSWB   = $0283
INPT0    = $08
INPT1    = $09
INPT4    = $0C

; SuperGame: a store anywhere in the window picks its bank
BANKSEL  = $8000

; The character blocks in the cart's RAM: 8 pages each, CHARBASE = the
; first; glyph line k (0 = top) of code c at block + (7 - k) * 256 + c
DYNB     = $4000        ; the drawing cells: 1-127, the same again at 128-255
TXTB     = $4800        ; the font (0-63), terrain and sprite pieces (64-255)
BIGB     = $5000        ; big text: 2-byte codes, value 1 at 0-127, 2 at 128-255
MTB      = $5800        ; the map's glyphs (3 values) or the town's characters
SCREEN   = $6000        ; 24 rows x 40 bytes, the Atari port's (see gfx.c)
CB_DYN   = >DYNB
CB_TXT   = >TXTB
CB_BIG   = >BIGB
CB_MT    = >MTB

; Columns of TXTB's pages (64-255) for direct objects
COL_MTN  = 64           ; 4 mountains x 2
COL_RIV  = 72           ; 4 river pieces x 2
COL_STORE = 80          ; the store: 4 rows x 4
COL_BAR  = 96           ; the time bar: a column per row (24)
COL_SPR  = 120          ; sprite pieces: 120-255
NCOL_SPR = 136

; CTRL: DMA on, BACKGRND border, 160A or 320A, 1- or 2-byte characters
CT_160   = $48
CT_320   = $4B
CT_CW2   = $10

; Row modes (gfx.c RM_*, draw.s)
RM_TEXT  = 0
RM_BIG   = 1
RM_DRAW  = 2
RM_MAP   = 3
RM_TOWN  = 4
ROWS     = 24
