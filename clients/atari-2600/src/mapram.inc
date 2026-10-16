; mapram.inc -- the map screen's cells after the kernel's (dispmap.inc's
; MKEND, PV+21), shared by bank 4 and bank 12's map half (mapx.inc).

GCUR    EQU     PV+21           ; the box's plot (the grant cursor, the plot
                                ;   for sale, the colony event's), $FF none
GNEXT   EQU     PV+22           ; frames to the cursor's next step
GSTEP   EQU     PV+23           ; frames per step
GCLAIM  EQU     PV+24           ; I have claimed this grant (or asked to)
MLAST   EQU     PV+25           ; the message shown last: a key, so each is
                                ;   said once (phase * 16 + what)
GFRESH  EQU     PV+26           ; a state has come: bank 12 reads it
GNF     EQU     PV+27           ; the plot the cursor may step to, $FF: wait
GPH     EQU     PV+28           ; round * 16 + phase: a new one starts afresh
GSAID   EQU     PV+29           ; bit 0: the cursor's start said, bit 1: the ship's
GSHIPT  EQU     PV+30           ; the ship's ticks, 0: none
GEVT    EQU     PV+31           ; production: the colony event (CE_*), 0 none
GEPL    EQU     PV+32           ;   its plot, $FF none (acid rain: the row's first)

; Bank 4's pictures, where bank 12 can point the kernel at them.
MSHIP   EQU     $17E8           ; the transport ship, 8 entries (player 1)
MCURS   EQU     $17F0           ; the box, 16 entries (player 0, double width)
