; arena.inc -- what the arena's three pages share (p_arena, p_arb, p_arc):
; its rows, its glyphs, its scratch; after auram.inc.

ARF_SCR  EQU    1               ; ARFLAGS: a new screen,
ARF_BEGIN EQU   2               ;  a stage begins,
ARF_DIRTY EQU   4               ;  a state to draw

        ZONE    AUMSIG+1,UNIONHE
        VAR     ARFIG,4         ; each figure's picture in its glyph (255 none)
        VAR     AUTBU,1         ; the clock as drawn (255 none)
ARFIGO  EQU     ARFIG-$2900
AUTBUO  EQU     AUTBU-$2900

ARLROW  EQU     17              ; each player's BUY / SELL or price
ARMROW  EQU     18              ; rows 18-21: a player each: name, money, units
ARMSG   EQU     22              ; the message, 3 rows
ARMSGN  EQU     3
ARBROW  EQU     25
GLNS    EQU     GUDC+4          ; the sellers' line, the buyers', both
GLNB    EQU     GUDC+5
GLN2    EQU     GUDC+6

; scratch (one frame)
ARL1V   EQU     UNIONB+0        ; the new sellers' line, buyers' (255 none)
ARL2V   EQU     UNIONB+1
ARDN    EQU     UNIONB+5        ; ARDRAW's count
ARK     EQU     UNIONB+6        ; our released line's seat (255 none)
ARJ     EQU     UNIONB+7        ; steps the state missed
ARC     EQU     UNIONB+8        ; the critical seller (255 none)
ARWH    EQU     UNIONB+9        ; a price
ARWL    EQU     UNIONB+10
ARN     EQU     UNIONB+11       ; a count

