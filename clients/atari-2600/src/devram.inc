; devram.inc -- the walker, shared by bank 5 (the map, your turn) and bank 6
; (the town). After the map kernel's cells (dispmap.inc: PV+0-20, MKEND);
; the town kernel keeps its own cells in the same first twenty-one bytes.
;
; Positions are the MekkoGX client's (develop.c), which are the
; Intellivision's MOB units: x = colour clock + 8, y = scanline / 2 + 8 --
; so a map entry is y - 16 and a town entry is y - 16 as well (both boxes
; start 16 scanlines down the Atari's screen).

WX      EQU     PV+21           ; the figure's left, x units
WY      EQU     PV+22           ; its top, y units
WFACE   EQU     PV+23           ; 0 right, 1 left, 2 down, 3 up, 4 standing
WFRM    EQU     PV+24           ; the walk's toggle
WCNT    EQU     PV+25           ; frames to the next step
WTX     EQU     PV+26           ; where it was a step ago: the M.U.L.E.
WTY     EQU     PV+27
DMSG    EQU     PV+28           ; the message on row 1 (MSG*)
DVLIST  EQU     PV+29           ; the plot listed at the land office, $FF
DVBTN   EQU     PV+30           ; the button was pressed (for bank 11)
WMULE   EQU     PV+31           ; leading a M.U.L.E.
DVST    EQU     PV+32           ; DV*
DVSMP   EQU     PV+33           ; the plot sampled for the assay, $FF none
DVOFF   EQU     PV+34           ; the land office waits for a plot
DVBW    EQU     PV+35           ; in a doorway, waiting for the server
DVPTU   EQU     PV+36           ; jiffies a time-bar step
DVACT   EQU     PV+37           ; the action sent, for its answer
DVLEFT  EQU     $E8             ; turn time left at the arrival, 16 bits
DVLEFTH EQU     $E9
DVEL    EQU     $EA             ; turn time elapsed at the arrival, 16 bits
DVELH   EQU     $EB
WSP     EQU     $EC             ; my species
WCOL    EQU     $ED             ; my colour
WPPLOT  EQU     $EE             ; the wampus: the plot, $FF none
WPMNT   EQU     $EF             ;   its mountain
WPFROM  EQU     $F0             ;   when it shows (turn jiffies), 16 bits
WPFROMH EQU     $F1
WPTO    EQU     $F2             ;   and until
WPTOH   EQU     $F3
DVTK    EQU     $F4             ; frames to the timer's next tick
WTOWN   EQU     $F5             ; in the town
; $F5 is the last: the stack is $F6-$FF.

; The messages (MSG*): bank 10 has their words.
MSGNONE EQU     0
MSGSTART EQU    1
MSGRAN  EQU     2
MSGINST EQU     3
MSGCAUGHT EQU   4
MSGOFFICE EQU   5
MSGSMP  EQU     6
MSGNO   EQU     7
MSGMONEY EQU    8
MSGNOMULE EQU   9
MSGNOSMP EQU    10
MSGSALE EQU     11
MSGUNSALE EQU   12
MSGASSAY EQU    13              ; 13-17: none, low, medium, high, very high
MSGPUB  EQU     18

; DVST
DVPROMPT EQU    1               ; your turn, not begun: the button starts it
DVWALK  EQU     2               ; walking
DVOVER  EQU     3               ; done

; The town's geometry (develop.c), x and y units.
TWYTOP  EQU     37              ; the street: the figure's top 37..41
TWYBOT  EQU     41
TWYINDN EQU     57              ; walked into a lower building
TWYINUP EQU     21              ; walked into an outfitter
TWXLEFT EQU     36              ; one more step left leaves town
TWXRGHT EQU     132
TWXSTRT EQU     84

; Buildings: the outfitters are the goods (0-3); then these.
BASSAY  EQU     4
BLAND   EQU     5
BPUB    EQU     6
BCORRAL EQU     7
