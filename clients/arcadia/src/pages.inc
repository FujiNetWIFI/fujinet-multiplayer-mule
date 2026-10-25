; pages.inc -- which page holds what (mule.asm includes each page's file).
PG_BOOT  EQU    0               ; title, name, table list, joining, leaving
PG_LOBBY EQU    1
PG_SUM   EQU    2               ; status summary, game over
PG_GRANT EQU    3
PG_AUC   EQU    4               ; land auction's plot, goods status, declaring
PG_ARENA EQU    5               ; the auctions' arena
PG_DEV   EQU    6               ; development: the map
PG_TOWN  EQU    7
PG_DEVT  EQU    8               ; development: prompts, events, the computers
PG_PROD  EQU    9               ; production and the colony events
PG_FX    EQU    10              ; the sprite scripts: ships, events, walks
PG_MAP   EQU    11              ; the map and the time bar (the main loop calls it)
PG_ART   EQU    12              ; pictures: figures, ships, glyph sets
PG_TEST  EQU    13              ; the offline test mode, autoplay
PG_CORE  EQU    14              ; the URL, the clock's sync, the poll rate (fixed code far-calls it)
STRPG    EQU    15              ; strings: pages 15.. (build/str.inc)
NSTRPG   EQU    3
PG_ARB   EQU    18              ; the arena's labels, money, clock, messages
PG_ARC   EQU    19              ; the arena's screen, stages, input
PG_AUTO  EQU    20              ; AUTOPLAY builds: playing by itself
PG_TITLE EQU    21              ; the title and its parade (the test mode's handshake)
NPAGES   EQU    22

; the core page's entries
E_BLDURL EQU    E_EXTRA
E_CLKSYN EQU    E_EXTRA+3
E_POLLRT EQU    E_EXTRA+6
E_SPECTG EQU    E_EXTRA+9
; the boot page's
E_JOIN   EQU    E_EXTRA         ; "JOINING..."
E_RECHO  EQU    E_EXTRA+6       ; another table
E_LEAVE  EQU    E_EXTRA+9       ; Option in a game: ask, maybe leave

; the sprite scripts (p_fx.inc)
FX_LAND  EQU    0               ; the transport ship lands (round 0, the ship's return)
FX_DEPART EQU   1               ; and leaves (round 1's grant)
FX_PIRATE EQU   2
FX_PEST  EQU    3
FX_ACID  EQU    4
FX_SUN   EQU    5
FX_RADIAT EQU   6
FX_METEOR EQU   7
FX_FIRE  EQU    8
FX_N     EQU    10              ; (the fx page's scripts)
FX_STOP  EQU    $FE             ; FXSTART: whatever runs, stopped
FX_AIWALK EQU   9               ; a computer walks out to its plot (develop)
