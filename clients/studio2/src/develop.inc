; develop.inc -- the development phase (mekkogx develop.c): turns, walking,
; the town. The server keeps no player position: we walk here and send what
; we do -- buy or return a M.U.L.E. at the corral, outfit it at a shop,
; install it on a plot, assay, list land, catch the wampus, gamble at the pub.
; A turn starts in the middle of the town.
;
; The keypad walks: 2 4 6 8, and 1 3 7 9 the diagonals; 5 is the button.
; Positions are the Studio II's: x pixels, y lines, the figure's top left.

SLOTPL  EQU     0                       ; sprite slots
SLOTMU  EQU     1
SLOTWP  EQU     2
WE_TOWNL EQU    1                       ; walk events for develop_tick
WE_TOWNR EQU    2
WE_STOREL EQU   3
WE_STORER EQU   4
WE_BLDG EQU     5
B_ASSAY EQU     4                       ; buildings: the outfitters are the goods
B_LAND  EQU     5
B_PUB   EQU     6
B_CORRAL EQU    7
TY_TOP  EQU     44                      ; town: the street, two steps deep
TY_BOT  EQU     50
TY_INUP EQU     30                      ; walked into an outfitter
TY_INDN EQU     66                      ; into a lower building
TX_LEFT EQU     1                       ; one step more leaves the town
TX_RIGHT EQU    57
TX_START EQU    29

DV_SEAT EQU     PH+0
DV_FLAGS EQU    PH+1
DV_MINE EQU     PH+2
DV_PTU  EQU     PH+3
DV_EVSH EQU     PH+4                    ; the player event shown
DV_SAMP EQU     PH+5                    ; the plot we took a sample from
DV_OFFC EQU     PH+6                    ; the land office waits for a plot
DV_MSEAT EQU    PH+7                    ; whose turn the message says,
DV_MKIND EQU    PH+8                    ;  and what kind
DV_LEFTL EQU    PH+9                    ; turn time left, elapsed at the state
DV_LEFTH EQU    PH+10
DV_ELL  EQU     PH+11
DV_ELH  EQU     PH+12
DV_WP   EQU     PH+13                   ; the wampus showing (255 none)
DV_WPX  EQU     PH+14
DV_WPY  EQU     PH+15
DV_BWAIT EQU    PH+16                   ; in a doorway, the server asked
DV_BT0L EQU     PH+17
DV_BT0H EQU     PH+18
DV_AI   EQU     PH+19                   ; a computer's land marks: 1 to show, 2 shown
DV_AIT  EQU     PH+20
DV_AIP  EQU     PH+21                   ; the plot it works
DV_PX   EQU     PH+22                   ; the time bar: units left
DV_PXT  EQU     PH+23                   ; frames to the next unit
DV_PXF  EQU     PH+24                   ; the frame counted to
WK_X     EQU     PH+25
WK_Y     EQU     PH+26
WK_TOWN  EQU     PH+27
WK_HOLD  EQU     PH+28
WK_EVENT EQU     PH+29
WK_BLDG  EQU     PH+30
WK_CNT   EQU     PH+31
WK_STEP  EQU     PH+32
WK_FRAME EQU     PH+33
WK_LEFT  EQU     PH+34                   ; facing left (the M.U.L.E. too)
WK_SP    EQU     PH+35
WK_IN    EQU     PH+36                   ; walking in: 1 down, 2 up
WK_MULE  EQU     PH+37
WK_TH    EQU     PH+38
WK_TX    EQU     PH+39                   ; 4: the last steps, for the M.U.L.E.
WK_TY    EQU     PH+43                   ; 4
WK_IX    EQU     PH+47                   ; the push: -1, 0, 1
WK_IY    EQU     PH+48
DV_K    EQU     PH+49

; develop_enter (mekkogx develop_enter)
        RFIT    49
develop_enter:
        SETV    DVSTATE, 0FFH
        SETV    DV_MSEAT, 0FFH
        SETV    WK_MULE, 0
        SETV    MAPDOTS, 0
        SETV    DV_AI, 0
        SETV    DV_BWAIT, 0
        SETV    SCRNOW, SCR_NONE
        SETV    DV_WP, 0FFH
        FRET

; develop_state: a new state (mekkogx develop_state).
        RFIT    222
develop_state:
        SLD     TN_SEAT
        PUTV    DV_SEAT
        SLD     TN_FLAGS
        PUTV    DV_FLAGS
        SLD     TN_LEFT
        PUTV    DV_LEFTL
        INC     R8
        LDN     R8
        PUTV    DV_LEFTH
        SLD     TN_ELAPSED
        PUTV    DV_ELL
        INC     R8
        LDN     R8
        PUTV    DV_ELH
        SLD     TN_PTU
        BNZ     ds0
        LDI     28
ds0:    PUTV    DV_PTU
        GETV    DV_SEAT
        STR     R2
        GETV    MYSEAT
        XOR
        BZ      ds1
        LDI     1
ds1:    XRI     1
        PUTV    DV_MINE
        FCALL   dv_barset               ; the bar from this state's time
        GETV    DV_MINE                 ; our turn, under way: we walk
        BZ      ds5
        GETV    DV_FLAGS
        ANI     TF_STARTED|TF_ENDED
        XRI     TF_STARTED
        BNZ     ds5
        GETV    DVSTATE
        XRI     DV_WALK
        BZ      ds2
        FCALL   dv_startwalk
ds2:    GETV    DV_FLAGS                ; the server's view of what we lead wins
        ANI     TF_HASMULE
        BZ      ds3
        LDI     1
ds3:    PUTV    WK_MULE
        GETV    DV_BWAIT                ; a building answered
        BZ      ds4
        FCALL   dv_bldact
        BZ      ds4
        FCALL   dv_bldresult
        FCALL   dv_street
ds4:    GETV    SCRNOW
        XRI     SCR_MAP
        BNZ     ds9
        FCALL   map_update
        FJMP    dv_mapresult
ds9:    FRET
ds5:    FJMP    dv_notwalk

; dv_bldact: D != 0 if the action answered was a building's.
dv_bldact:
        GETV    ACTSENT
        SMI     ACT_BUYMU
        BNF     dba0
        SMI     ACT_PUB-ACT_BUYMU+1
        BDF     dba0
        GETV    ACTSENT
        XRI     ACT_INST
        BZ      dba9
        XRI     ACT_INST!ACT_SELL
        BZ      dba9
        XRI     ACT_SELL!ACT_UNSELL
        BZ      dba9
        XRI     ACT_UNSELL!ACT_WAMPUS
dba9:   FRET
dba0:   LDI     0
        FRET

; dv_notwalk: anything else happens on the map.
        RFIT    72
dv_notwalk:
        GETV    DVSTATE
        XRI     DV_WALK
        BNZ     dn1
        FCALL   dv_stopwalk
dn1:    GETV    SCRNOW
        XRI     SCR_MAP
        BZ      dn2
        FCALL   dv_mapenter
dn2:    SETV    MSGROW, 8
        GETV    DV_MINE                 ; ours, not begun: the prompt
        BZ      dn4
        GETV    DV_FLAGS
        ANI     TF_STARTED
        BNZ     dn4
        GETV    DVSTATE
        XRI     DV_PROMPT
        BZ      dn3
        SETV    DVSTATE, DV_PROMPT
        LDI     M_START
        FCALL   msg_show
        LDI     SND_ALERT
        FCALL   snd_play
dn3:    FJMP    dv_barnow
dn4:    FJMP    dv_others

; dv_others: someone's turn: said once a turn, and once more when it ends;
; a computer's plays out until the next seat's.
        RFIT    212
dv_others:
        GETV    DV_SEAT                 ; someone's turn: once a turn, once
        SMI     4                       ; more when it ends; a computer's
        BDF     dn9                     ; plays out until the next seat's
        LDI     0
        PLO     RC
        LDR     R9, S_PLAYERS+PL_FLAGS
        GETV    DV_SEAT
        FCALL   pl_byte
        ANI     PF_AI
        BZ      dn5
        LDI     1
        PLO     RC
        BR      dn6
dn5:    GETV    DV_FLAGS
        ANI     TF_ENDED
        BZ      dn6
        LDI     2
        PLO     RC
dn6:    GETV    DV_MSEAT
        STR     R2
        GETV    DV_SEAT
        XOR
        BNZ     dn7
        GETV    DV_MKIND
        STR     R2
        GLO     RC
        XOR
        BZ      dn9
dn7:    GETV    DV_SEAT
        PUTV    DV_MSEAT
        GLO     RC
        PUTV    DV_MKIND
        XRI     1
        BZ      dn_ai
        XRI     1!2
        BZ      dn_over
        SETV    DVSTATE, DV_WAIT        ; a player's turn
        FCALL   dv_sayturn
dn9:    FJMP    dv_barnow
dn_ai:  SETV    DVSTATE, DV_WAIT        ; a computer's
        LDI     M_COMPUTER
        FCALL   msg_show
        FCALL   dv_aiwalk
        FJMP    dv_barnow
dn_over:
        SETV    DVSTATE, DV_OVER        ; it ended
        GETV    ACTSENT
        XRI     ACT_PUB
        BNZ     dno1
        GETV    DV_MINE
        BZ      dno1
        SPTR    H_RESULT                ; won $% gambling
        LDA     R8
        PUTV    V1L
        LDN     R8
        PUTV    V1H
        LDI     M_GAMBLE
        BR      dno3
dno1:   GETV    DV_LEFTL
        BNZ     dno2
        GETV    DV_LEFTH
        BNZ     dno2
        LDI     M_TIMEOUT
dno3:   FCALL   msg_show
        FJMP    dv_barnow
dno2:   FCALL   msg_clear
        FJMP    dv_barnow

; dv_barnow: the bar for whoever plays, the map's changes.
dv_barnow:
        GETV    DV_FLAGS
        ANI     TF_STARTED|TF_ENDED
        XRI     TF_STARTED
        BNZ     dbn1
        FCALL   dv_bar
        BR      dbn2
dbn1:   LDI     0
        FCALL   bar_draw
dbn2:   FJMP    map_update

; dv_sayturn: "<NAME>'S TURN".
        RFIT    48
dv_sayturn:
        LDR     R9, S_PLAYERS+PL_NAME
        GETV    DV_SEAT
        FCALL   pl_byte
        LDR     R9, TXT
        LDI     8
        PLO     RC
dst1:   LDA     R8
        BZ      dst2
        STR     R9
        INC     R9
        DEC     RC
        GLO     RC
        BNZ     dst1
dst2:   GLO     R9
        SMI     LO(TXT)
        PUTV    TXTLEN
        LDI     M_TURN
        FCALL   str_cat
        FJMP    msg_start

; ---- the time bar (mekkogx dv_bar): one unit a PTU of turn time left ---------
; dv_barset: at a state: units = (left - rx_age) / PTU, and the frames to
; the next unit; counted down from the state's frame.
        RFIT    75
dv_barset:
        GETV    RXAGE                   ; left - rx_age
        STR     R2
        GETV    DV_LEFTL
        SM
        PLO     R9
        GETV    DV_LEFTH
        SMBI    0
        PHI     R9
        BDF     dbs1
        LDI     0
        PLO     R9
        PHI     R9
dbs1:   LDI     0                       ; / PTU
        PLO     RC
        GETV    DV_PTU
        STR     R2
dbs2:   GLO     R9
        SM
        PLO     RB
        GHI     R9
        SMBI    0
        BNF     dbs3
        PHI     R9
        GLO     RB
        PLO     R9
        INC     RC
        GLO     RC
        BNZ     dbs2
        DEC     RC                      ; (255 units at most)
dbs3:   GLO     RC
        PUTV    DV_PX
        GLO     R9                      ; the remainder: frames to the next
        ADI     1
        PUTV    DV_PXT
        GETV    RXFRL
        PUTV    DV_PXF
        FRET

; dv_bar: every frame of a turn: the bar run down, ticking (mekkogx dv_bar).
        RFIT    64
dv_bar: GETV    DV_PXF                  ; the frames since we last counted
        STR     R2
        GETV    FCL
        SM
        BZ      dvb3
        PLO     RC
        GETV    FCL
        PUTV    DV_PXF
dvb1:   GETV    DV_PXT                  ; each frame: a unit gone every PTU
        SMI     1
        STR     RA
        BNZ     dvb2
        GETV    DV_PTU
        STR     RA
        GETV    DV_PX
        BZ      dvb2
        SMI     1
        STR     RA
dvb2:   DEC     RC
        GLO     RC
        BNZ     dvb1
dvb3:   GETV    DV_PX
        FCALL   bar_tick
        GETV    DV_PX
        FJMP    bar_draw

; ---- walking (mekkogx walk_isr): every frame, more to catch up ------------------
        RFIT    143
walk_isr:
        GETV    WK_HOLD
        BNZ     wi9
        GETV    WK_CNT
        BZ      wi1
        SMI     1
        STR     RA
wi9:    FRET
wi1:    GETV    WK_IN
        BZ      wi2
        FJMP    walk_bldg
wi2:    GETV    KEY                     ; the keypad's push
        SMI     10
        BDF     wi3
        GETV    KEY
        ADI     LO(keydir)
        PLO     R8
        LDI     HI(keydir)
        ADCI    0
        PHI     R8
        LDN     R8
        BR      wi4
wi3:    LDI     0
wi4:    PLO     RC                      ; bits 0-1 x (1 left, 2 right), 2-3 y
        ANI     1
        BZ      wx1
        LDI     0FFH
        BR      wx3
wx1:    GLO     RC
        ANI     2
        BZ      wx3
        LDI     1
wx3:    PUTV    WK_IX
        GLO     RC
        ANI     4
        BZ      wy1
        LDI     0FFH
        BR      wy3
wy1:    GLO     RC
        ANI     8
        BZ      wy3
        LDI     1
wy3:    PUTV    WK_IY
        GLO     RC
        BNZ     wi10
        GETV    WK_STEP                  ; let go: it stands
        BZ      wi9
        SETV    WK_STEP, 0
        FJMP    walk_draw
wi10:   GETV    WK_TOWN
        BZ      wi11
        FCALL   walk_town
        BR      wi12
wi11:   FCALL   walk_map
wi12:   GETV    WK_HOLD
        BNZ     wi9
        FJMP    walk_stepped
; keydir: key -> bits 0-1 x (1 left, 2 right), 2-3 y (4 up, 8 down)
keydir: DB      0, 1+4, 4, 2+4, 1, 0, 2, 1+8, 8, 2+8

; walk_stepped: a step made: the frame, the facing, the M.U.L.E.'s trail.
        RFIT    73
walk_stepped:
        SETV    WK_STEP, 1
        GETV    WK_FRAME
        XRI     1
        STR     RA
        GETV    WK_IX                    ; facing: left or right, if across
        BZ      wst1
        ANI     80H
        BZ      wst0
        LDI     1
wst0:   PUTV    WK_LEFT
wst1:   LDI     SND_STEP
        FCALL   snd_play
        GETV    WK_X                     ; remember where we were
        PLO     R9
        GETV    WK_Y
        PHI     R9
        GETV    WK_TH
        ADI     LO(WK_TX)
        PLO     RA
        GLO     R9
        STR     RA
        GLO     RA
        ADI     4
        PLO     RA
        GHI     R9
        STR     RA
        GETV    WK_TH
        ADI     1
        ANI     3
        STR     RA
        FJMP    walk_draw

; walk_town: a step along the street, or through a door (mekkogx walk_town).
        RFIT    222
walk_town:
        SETV    WK_CNT, 5
        GETV    WK_IX
        BZ      wt3
        ANI     80H
        BZ      wt2
        GETV    WK_X                     ; left
        SMI     TX_LEFT+1
        BDF     wt1
        LDI     WE_TOWNL
        BR      wtev
wt1:    GETV    WK_X
        SMI     2
        STR     RA
        BR      wt3
wt2:    GETV    WK_X                     ; right
        SMI     TX_RIGHT
        BNF     wt2a
        LDI     WE_TOWNR
wtev:   PUTV    WK_EVENT
        SETV    WK_HOLD, 1
        FRET
wt2a:   GETV    WK_X
        ADI     2
        STR     RA
wt3:    GETV    WK_IY
        BZ      wt9
        ANI     80H
        BNZ     wt6
        GETV    WK_Y                     ; down: deeper into the street, or
        SMI     TY_BOT                  ; through a lower door
        BDF     wt4
        GETV    WK_Y
        ADI     3
        STR     RA
        FRET
wt4:    LDI     B_ASSAY
        PLO     RC
wt5:    GLO     RC                      ; door ranges: lo, hi
        SHL
        ADI     LO(doorlo-8)
        PLO     R8
        LDI     HI(doorlo-8)
        ADCI    0
        PHI     R8
        GETV    WK_X
        SEX     R8
        SM
        SEX     R2
        BNF     wt5a
        INC     R8
        LDN     R8
        STR     R2
        GETV    WK_X
        SD
        BNF     wt5a
        SETV    WK_IN, 1
        GLO     RC
        PUTV    WK_BLDG
        FRET
wt5a:   INC     RC
        GLO     RC
        XRI     8
        BNZ     wt5
        FRET
wt6:    GETV    WK_Y                     ; up: or through an outfitter's door
        SMI     TY_TOP+1                ; (with a M.U.L.E. only)
        BNF     wt7
        GETV    WK_Y
        SMI     3
        STR     RA
        FRET
wt7:    GETV    WK_MULE
        BZ      wt9
        LDI     0
        PLO     RC
wt8:    GLO     RC
        ADI     LO(doorup)
        PLO     R8
        LDI     HI(doorup)
        ADCI    0
        PHI     R8
        LDN     R8
        STR     R2
        GETV    WK_X
        XOR
        BNZ     wt8a
        SETV    WK_IN, 2
        GLO     RC
        PUTV    WK_BLDG
        FRET
wt8a:   INC     RC
        GLO     RC
        XRI     4
        BNZ     wt8
wt9:    FRET
; the outfitters' doors, by good: food, energy, smithore, crystite (from the
; left they stand crystite, smithore, energy, food); the lower doors' x
; ranges: assay, land, pub, corral
doorup: DB      53, 37, 21, 5
doorlo: DB      1, 11, 17, 27, 33, 43, 49, 59

; walk_bldg: through a door, a step every 6 frames, until inside.
        RFIT    66
walk_bldg:
        SETV    WK_CNT, 5
        GETV    WK_FRAME
        XRI     1
        STR     RA
        GETV    WK_IN
        XRI     1
        BNZ     wb2
        GETV    WK_Y                     ; down
        ADI     3
        STR     RA
        SMI     TY_INDN
        BNF     wb3
        BR      wb4
wb2:    GETV    WK_Y                     ; up
        SMI     3
        STR     RA
        SMI     TY_INUP+1
        BDF     wb3
wb4:    SETV    WK_IN, 0
        SETV    WK_EVENT, WE_BLDG
        SETV    WK_HOLD, 1
wb3:    FJMP    walk_draw

; walk_map: a step on the map: slower in the mountains and the river; into
; the store, back to the town (mekkogx walk_map, walk_terrain).
        RFIT    189
walk_map:
        SETV    WK_CNT, 5
        GETV    WK_IX
        BZ      wm2
        ANI     80H
        BZ      wm1
        GETV    WK_X
        BZ      wm2
        SMI     1
        STR     RA
        BR      wm2
wm1:    GETV    WK_X
        SMI     58
        BDF     wm2
        GETV    WK_X
        ADI     1
        STR     RA
wm2:    GETV    WK_IY
        BZ      wm4
        ANI     80H
        BZ      wm3
        GETV    WK_Y
        SMI     MAPY0+1
        BNF     wm4
        GETV    WK_Y
        SMI     1
        STR     RA
        BR      wm4
wm3:    GETV    WK_Y
        SMI     MAPY0+73
        BDF     wm4
        GETV    WK_Y
        ADI     1
        STR     RA
wm4:    FCALL   w_plot                  ; the plot under it: its terrain
        SMI     NUM_PLOTS
        BDF     wm9
        GETV    DV_K
        FCALL   mp_terrain
        PLO     RC
        ANI     3
        XRI     TERRAIN_RIVER
        BNZ     wm5
        GETV    WK_X                     ; the river's band: x 29-33
        ADI     2
        SMI     29
        BNF     wm7
        SMI     5
        BDF     wm7
        GETV    WK_CNT
        ADI     8
        STR     RA
        BR      wm7
wm5:    GLO     RC                      ; mountains: in the plot's upper part
        ANI     0CH
        BZ      wm7
        GETV    WK_Y
        ADI     3-MAPY0
        ANI     0FH
        SMI     11
        BDF     wm7
        GETV    WK_CNT
        ADI     15
        STR     RA
wm7:    GETV    DV_K                    ; into the store?
        XRI     STORE_PLOT
        BNZ     wm9
        GETV    WK_X
        ADI     2
        SMI     31
        BDF     wm8
        LDI     WE_STOREL
        BR      wm8a
wm8:    LDI     WE_STORER
wm8a:   PUTV    WK_EVENT
        SETV    WK_HOLD, 1
wm9:    FRET

; w_plot: D and DV_K = the plot under the figure's middle (45: none).
        RFIT    58
w_plot: GETV    WK_Y                     ; row: (y + 3 - 13) / 16
        ADI     3
        SMI     MAPY0
        BNF     wp9
        SHR
        SHR
        SHR
        SHR
        SMI     5
        BDF     wp9
        ADI     5
        PLO     RC                      ; * 9
        SHL
        SHL
        SHL
        STR     R2
        GLO     RC
        ADD
        PLO     RC
        GETV    WK_X                     ; column: (x + 2) / 7
        ADI     2
wp1:    SMI     7
        BNF     wp2
        INC     RC
        BR      wp1
wp2:    GLO     RC
        PUTV    DV_K
        FRET
wp9:    LDI     NUM_PLOTS
        PUTV    DV_K
        FRET

; walk_draw: the walker, and the M.U.L.E. at the oldest step remembered.
        RFIT    255
walk_draw:
        PARS
        P       SLOTPL
        GETV    WK_TOWN
        BZ      wd2
        GETV    WK_STEP                  ; town: the species, standing or walking
        BNZ     wd0
        LDI     IM_STAND_MECHTRON
        BR      wd1
wd0:    GETV    WK_FRAME
        BZ      wd0a
        LDI     IM_WALKB_MECHTRON
        BR      wd1
wd0a:   LDI     IM_WALKA_MECHTRON
wd1:    STR     R2
        GETV    WK_SP
        ADD
        BR      wd3
wd2:    GETV    WK_FRAME                 ; map: small
        BZ      wd2a
        LDI     IM_MWB_MECHTRON
        BR      wd2b
wd2a:   LDI     IM_MWA_MECHTRON
wd2b:   STR     R2
        GETV    WK_SP
        ADD
wd3:    PD
        GETV    WK_X
        PD
        GETV    WK_Y
        PD
        P       TM_OR
        GOP     TOP_SPRITE
        GETV    WK_MULE                  ; the M.U.L.E., behind
        BZ      wd9
        GETV    WK_TH
        ADI     LO(WK_TX)
        PLO     RA
        LDN     RA
        BNZ     wd4
        SETV    DV_K, LO(WK_TX)          ; (nothing that old yet: the first)
        BR      wd5
wd4:    GETV    WK_TH
        ADI     LO(WK_TX)
        PUTV    DV_K
wd5:    GETV    DV_K
        PLO     RA
        LDN     RA
        BZ      wd9
        PARS
        P       SLOTMU
        GETV    WK_TOWN                  ; the picture: its size, its way, its step
        BZ      wd6
        LDI     IM_MULEA
        BR      wd7
wd6:    LDI     IM_SMULEA
wd7:    STR     R2
        GETV    WK_FRAME
        ADD
        STR     R2
        GETV    WK_LEFT
        BZ      wd8
        GETV    WK_TOWN
        BZ      wd7a
        LDI     IM_MULEAL-IM_MULEA
        BR      wd7b
wd7a:   LDI     IM_SMULEAL-IM_SMULEA
wd7b:   ADD
        STR     R2
wd8:    LDN     R2
        PD
        GETV    DV_K                    ; its place
        PLO     RA
        LDN     RA
        PD
        GETV    DV_K
        ADI     4
        PLO     RA
        LDN     RA
        STR     R2
        GETV    WK_TOWN
        BZ      wd8a
        LDI     6                       ; town: beside the walker's legs
        BR      wd8b
wd8a:   LDI     3
wd8b:   ADD
        PD
        P       TM_OR
        GOP     TOP_SPRITE
        FRET
wd9:    PARS
        P       SLOTMU
        P       100
        P       0
        GOP     TOP_SPRMOVE
        FRET

; ---- starting and stopping (mekkogx dv_start_walk, dv_stop_walk) ----------------
        RFIT    198
dv_startwalk:
        SETV    DVSTATE, DV_WALK
        SETV    DV_SAMP, 0FFH
        SETV    DV_OFFC, 0
        SETV    DV_EVSH, 0
        SETV    DV_BWAIT, 0
        LDR     R9, S_PLAYERS+PL_SPECIES
        GETV    MYSEAT
        FCALL   pl_byte
        ANI     7
        PUTV    WK_SP
        FCALL   w_clear
        SETV    WK_X, TX_START
        SETV    WK_Y, TY_TOP+3
        SETV    WK_IN, 0
        SETV    WK_EVENT, 0
        SETV    WK_CNT, 0
        SETV    WK_LEFT, 0
        FCALL   town_enter
        LDI     SND_SELECT
        FJMP    snd_play

; w_clear: no steps remembered.
w_clear:
        LDR     R8, WK_TX
        LDI     8
        PLO     RC
wc1:    LDI     0
        STR     R8
        INC     R8
        DEC     RC
        GLO     RC
        BNZ     wc1
        FRET

dv_stopwalk:
        SETV    ANIM, AN_NONE
        SETV    WK_HOLD, 1
        SETV    DVSTATE, DV_OVER
        FCALL   spr_hide
        GETV    SCRNOW
        XRI     SCR_MAP
        BZ      dsw9
        FJMP    dv_mapenter
dsw9:   FRET

; dv_resume: walking again.
dv_resume:
        SETV    WK_HOLD, 0
        SETV    WK_STEP, 0
        FCALL   walk_draw
        SETV    ANIM, AN_WALK
        FRET

; dv_mapenter: the map, for the development.
dv_mapenter:
        FCALL   map_enter
        GETV    RND
        PUTV    V1L
        SETV    V1H, 0
        LDI     T_DEVELOP
        FCALL   title_show
        FJMP    bar_map

; dv_blank: the screen blanks for about 22 frames between the town and the map.
        RFIT    38
dv_blank:
        SETV    ANIM, AN_NONE
        FCALL   scr_clear
        LDI     22
        PUTV    DV_K
dbl1:   FCALL   frame_wait
        FCALL   snd_tick
        GETV    DV_K
        SMI     1
        STR     RA
        BNZ     dbl1
        FCALL   done_now
        FJMP    w_clear

; develop_tick: the phase's frame (mekkogx develop_tick).
        RFIT    138
develop_tick:
        FCALL   msg_tick
        GETV    DVSTATE
        XRI     DV_PROMPT
        BNZ     dt2
        GETV    KEDGE                   ; the prompt: 5 begins
        XRI     K_FIRE
        BNZ     dt9
        LDI     ACT_BEGIN
        FCALL   act_send
        LDI     SND_SELECT
        FJMP    snd_play
dt2:    GETV    DVSTATE
        XRI     DV_WALK
        BZ      dt4
        GETV    DVSTATE                 ; watching: the marks, the bar
        XRI     DV_WAIT
        BNZ     dt3
        FCALL   dv_aimarks
dt3:    GETV    DV_FLAGS
        ANI     TF_STARTED|TF_ENDED
        XRI     TF_STARTED
        BNZ     dt9
        FJMP    dv_bar
dt4:    FCALL   dv_bar                  ; our own turn
        FCALL   dv_eventmsg
        GETV    DV_BWAIT
        BZ      dt5
        GETV    DV_BT0L                 ; no answer for 600: out again
        STR     R2
        GETV    FCL
        SM
        PLO     RC
        GETV    DV_BT0H
        STR     R2
        GETV    FCH
        SMB
        SMI     HI(600)
        BNF     dt9
        BNZ     dt4a
        GLO     RC
        SMI     LO(600)
        BNF     dt9
dt4a:   FJMP    dv_street
dt5:    GETV    WK_EVENT
        BZ      dt6
        FJMP    dv_wevent
dt6:    GETV    WK_TOWN
        BNZ     dt9
        FCALL   dv_wampus
        GETV    KEDGE
        XRI     K_FIRE
        BNZ     dt9
        FJMP    dv_button
dt9:    FRET

; dv_wevent: what the walk ran into (mekkogx develop_tick's switch).
        RFIT    117
dv_wevent:
        GETV    WK_EVENT
        XRI     WE_BLDG
        BNZ     dw1
        SETV    WK_EVENT, 0
        FJMP    dv_building
dw1:    XRI     WE_BLDG!WE_STOREL
        BZ      dw3
        XRI     WE_STOREL!WE_STORER
        BZ      dw3
        GETV    WK_EVENT                 ; out of the town: onto the map
        XRI     WE_TOWNL
        BNZ     dw2
        LDI     22
        BR      dw2a
dw2:    LDI     36
dw2a:   PUTV    WK_X
        SETV    WK_Y, MAPY0+35
        SETV    WK_EVENT, 0
        SETV    WK_TOWN, 0
        FCALL   dv_blank
        FCALL   dv_mapenter
        FCALL   dv_resume
        SETV    WK_CNT, 20               ; the first step on the map waits 20
        FRET
dw3:    GETV    WK_EVENT                 ; into the store: the town again
        XRI     WE_STOREL
        BNZ     dw4
        LDI     TX_LEFT
        BR      dw5
dw4:    LDI     TX_RIGHT
dw5:    PUTV    WK_X
        SETV    WK_Y, TY_TOP+3
        SETV    WK_EVENT, 0
        FCALL   dv_blank
        FJMP    town_enter
