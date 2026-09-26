; devaddr.inc -- where the development screens keep their pictures: fixed
; addresses, because bank 11 (the walker's logic) points the kernels of
; banks 5 (the map) and 6 (the town) at them, and cannot see their labels.
; Each bank ORGs its data here and checks its code stops short.

; bank 5, the map
DVCOL   EQU     $1500           ; the map's colour a entry (80)
DVFIGS  EQU     $1600           ; 8 species x 7 frames x 8 rows (448)
DVMULE  EQU     $17C0           ; the M.U.L.E., 2 frames x 8
DVWAMP  EQU     $17D0           ; the wampus, 8
DVDATA  EQU     DVCOL           ; the code stops before this

; bank 6, the town (72-entry tables, none crossing a page; the M.U.L.E.
; first so that its shapes -- drawn on the kernel's tight second line --
; never cross one either)
TWT1    EQU     $1500           ; the playfield: PF1 left (72)
TWT2    EQU     $1548           ;   PF2 left
TWT3    EQU     $1590           ;   PF0 right
TWT4    EQU     $1600           ;   PF1 right
TWCOL   EQU     $1648           ; the colour a entry
TWZERO  EQU     $1690           ; 72 zeros: PF0 left and PF2 right
TWMULE  EQU     $16D8           ; the M.U.L.E., 2 frames x 12
TWFIGS  EQU     $16F0           ; 8 species x 3 frames x 11 rows (264)
TWDATA  EQU     TWT1
