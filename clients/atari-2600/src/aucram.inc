; aucram.inc -- the auction screen's cells, shared by bank 7 (the arena)
; and bank 10 (its text).

; After the arena kernel's cells (dispauc.inc: PV+0-15).
AKIND   EQU     PV+16
AGOOD   EQU     PV+17
ASTG    EQU     PV+18           ; the stage, as drawn ($FF: none yet)
ASTEPV  EQU     PV+19           ; dollars a step
ABASE   EQU     PV+20           ; the base line's price, 16 bits
ABASEH  EQU     PV+21
APOSE   EQU     PV+22           ; [4] where each line is, entries
ATGTE   EQU     PV+26           ; [4] where it is going
AROLE   EQU     PV+30           ; [4] ROLE*
APUSH   EQU     PV+34           ; my stick as sent: 0 held, 1 up, 2 down
ATRADED EQU     PV+35           ; units traded, as shown
ATICK   EQU     PV+36           ; frames to the next line step
ABLINK  EQU     PV+37           ; seats blinking (a bit a seat): dealing
ASPEC   EQU     $E8             ; [4] each seat's species ($E8-$EB)
ATKS    EQU     $EC             ; the clock's ticks, as last seen
AHIE    EQU     $ED             ; the scale's top and bottom, entries
ALOE    EQU     $EE
ASSELE  EQU     $EF             ; the store's selling price, an entry
APEND   EQU     $F0             ; a state waits to be read (next overscan)
AHELD   EQU     $F1             ; MFRAME when my goods line was let go
ATKF    EQU     $F2             ; frames to the clock's next tick

