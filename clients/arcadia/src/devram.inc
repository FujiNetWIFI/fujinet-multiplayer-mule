; devram.inc -- the development's bookkeeping, shared by its pages: the map
; (p_dev, the phase's page), the town (p_town, which takes the phase over
; while we walk in it) and the helpers they far-call (p_devt).

        ZONE    UNIONL,UNIONLE
        VAR     WX,1            ; our figure: pixel column, screen line (its top)
        VAR     WY,1
        VAR     WTOWN,1         ; 1: walking in the town
        VAR     WCNT,1          ; frames to the next step
        VAR     WSTEP,1         ; 1: it stepped last time (else standing)
        VAR     WFRAME,1        ; the walking frame (0, 1)
        VAR     WFACE,1         ; the figure's frame (ART_FIG: right 0-1, left
                                ;  2-3, standing 4-5, down 6-7, up 8-9)
        VAR     WDOOR,1           ; going through a door: 1 down, 2 up (0 not)
        VAR     WMULE,1         ; 1: leading a M.U.L.E.
        VAR     WTX,4           ; the last four steps (the M.U.L.E. follows)
        VAR     WTY,4
        VAR     WTH,1           ; the oldest of them
        VAR     WBLDG,1         ; the building we walk into (B_*, a good)
        VAR     WHOLD,1         ; 1: standing still (in a door, a screen change)
        VAR     WSP,1           ; our species, our ink (sprite colour)
        VAR     WCOL,1
        VAR     DVSEAT,1        ; the turn's seat, flags, ours?
        VAR     DVFLAGS,1
        VAR     DVMINE,1
        VAR     DVLEFTH,1       ; its time left, elapsed (at the state)
        VAR     DVLEFTL,1
        VAR     DVELH,1
        VAR     DVELL,1
        VAR     DVPTU,1         ; jiffies a time unit
        VAR     DVSAMPLE,1      ; the plot sampled (255 none)
        VAR     DVOFFICE,1      ; 1: the land office waits for a plot
        VAR     DVBWAIT,1       ; 1: in a doorway, waiting for the server
        VAR     DVBT0H,1        ; since when
        VAR     DVBT0L,1
        VAR     DVWP,1          ; the wampus showing (255 none), where
        VAR     WPX,1
        VAR     WPY,1
        VAR     DVAI,1          ; a computer's turn: 1 its land to outline, 2 outlined
        VAR     DVAIT,1         ; since (frame, low byte)
        VAR     DVAIP,1         ; the plot it walks to
        VAR     DVDO,1          ; DO_*: what the frame must do (far calls)
        VAR     DVBARU,1        ; the time bar as drawn (255 none)

        ZONE    UNIONH,UNIONHE
        VAR     DVEVSH,1        ; our turn's event: 1 said, 2 cleared (0 not yet)
        VAR     DVMSGS,1        ; whose turn the message says (255 none), and how
        VAR     DVMSGK,1
        VAR     WFIGS,1         ; the pictures in our figure's glyph and its
        VAR     WMULS,1         ;  M.U.L.E.'s (255 none)
        VAR     DVPMULE,NUM_PLOTS ; the plots' M.U.L.E.s at the last state
        VAR     DVAIS0,52       ; a computer's walk: its figure's script ...
        VAR     DVAIS1,28       ;  ... and its M.U.L.E.'s (p_fx.inc runs them;
                                ;  CPTR/CPUT reach them, PRAMH doesn't)
DVPMULEO EQU    DVPMULE-$2900
DVEVSHO  EQU    DVEVSH-$2900
WFIGSO   EQU    WFIGS-$2900
DVMSGSO  EQU    DVMSGS-$2900
DVMSGKO  EQU    DVMSGK-$2900
WMULSO   EQU    WMULS-$2900
        IF      DVPMULEO+NUM_PLOTS-1 > $FF
        ERROR   "the development's RAM-H past PRAMH's reach"
        ENDIF

; DVDO: done by the frame (far calls are kept out of the states' depth)
DO_AIWALK EQU   1               ; a computer's turn: its walk and marks
DO_TURN  EQU    2               ; "<name> takes a turn"
DO_EVENT EQU    4               ; our turn's player event

; the buildings: the outfitters are the goods (food, energy, smithore,
; crystite), then
B_ASSAY  EQU    4
B_LAND   EQU    5
B_PUB    EQU    6
B_CORRAL EQU    7

; slots
SL_PL    EQU    0
SL_MULE  EQU    1
SL_WAMP  EQU    2
