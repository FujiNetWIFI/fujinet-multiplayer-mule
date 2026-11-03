; mapram.inc -- the map screen's cells after the kernel's (dispmap.inc's
; MKEND, PV+21), shared by bank 4 and bank 12's map half (mapx.inc).

GCUR    EQU     PV+21           ; the box's plot (the grant cursor, the plot
                                ;   for sale, the colony event's, a computer's
                                ;   assay), $FF none
GNEXT   EQU     PV+22           ; frames to the cursor's next step (a
                                ;   computer's assays: to the next one shown)
GSTEP   EQU     PV+23           ; frames per step
MLAST   EQU     PV+24           ; the message shown last: a key, so each is
                                ;   said once (phase * 16 + what)
GFRESH  EQU     PV+25           ; a state has come: bank 12 reads it
GPH     EQU     PV+26           ; round * 16 + phase: a new one starts afresh
; These six lie together: in development (a computer's turn watched) they are
; the assays still to show, AQ0.., an entry plot + 1 + 45 * level, 0 = none.
GNF     EQU     PV+27           ; the plot the cursor may step to, $FF: wait
GSAID   EQU     PV+28           ; bit 0: the cursor's start said, bit 1: the ship's
GSHIPT  EQU     PV+29           ; the ship's ticks, 0: none
GEVT    EQU     PV+30           ; production: the colony event (CE_*), 0 none
GEPL    EQU     PV+31           ;   its plot, $FF none (acid rain: the row's first)
GCLAIM  EQU     PV+32           ; I have claimed this grant (or asked to)
AQ0     EQU     PV+27
AQN     EQU     6
; The plots seen assayed (a bit each, the river's column left out: 40), in
; development: a state's new ones are a computer's, shown one by one. Kept
; from one state to the next while the screen stays; a screen entered afresh
; takes the map as it finds it. Bank 12 only, and other screens clobber it.
AMASK   EQU     PV+33
AMASKN  EQU     5

; Bank 4's pictures, where bank 12 can point the kernel at them.
MSHIP   EQU     $17E8           ; the transport ship, 8 entries (player 1)
MCURS   EQU     $17F0           ; the box, 16 entries (player 0, double width)
