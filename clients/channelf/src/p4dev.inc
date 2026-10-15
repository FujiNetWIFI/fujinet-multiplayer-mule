; ---------------------------------------------------------------------------
; p4dev.inc -- page 4: the development phase (mekkogx develop.c): turns,
; walking on the map and in town, the buildings, the wampus, a computer's
; turn played out.
;
; The server keeps no player position: we walk here and send actions. The
; walking positions are the Intellivision client's MOB units, so every
; constant is the one measured there: x = colour clock + 8, y = line / 2 + 8;
; the sprites get (x - 8, (y - 8) * 2) and SPRSET puts that on our screen.
;
; The town (TOWNDRAW, 60 x 36 at (21, 10)): the title above, the money on
; y50, messages on y56 (one row); the corral's price and M.U.L.E.s left at
; its right, past the time bar (x 86). On the map: the map's own layout.
; ---------------------------------------------------------------------------

DV_SEAT EQU PV+0                ; whose turn
DV_FLAGS EQU PV+1
DV_MINE EQU PV+2
DV_PTU  EQU PV+3                ; jiffies a bar unit
DV_EVSH EQU PV+4                ; the player event shown
DV_SAMP EQU PV+5                ; the plot of our assay sample (255 none)
DV_OFFICE EQU PV+6              ; the land office waits for a plot
DV_MSEAT EQU PV+7               ; the turn message said: seat, kind
DV_MKIND EQU PV+8
DV_LEFT EQU PV+9                ; 2: turn time left at the last state
DV_EL   EQU PV+11               ; 2: and gone
DV_WP   EQU PV+13               ; the wampus showing (255 none)
WP_X    EQU PV+14
WP_Y    EQU PV+15
DV_BWAIT EQU PV+16              ; in a doorway, waiting for the server
DV_BT0  EQU PV+17               ; 2: since when
DV_AI   EQU PV+19               ; a computer's land marks: 1 to show, 2 shown
DV_AIT  EQU PV+20
DV_AIP  EQU PV+21               ; the plot it works
W_X     EQU PV+22               ; walking
W_Y     EQU PV+23
W_TOWN  EQU PV+24
W_HOLD  EQU PV+25
W_EVENT EQU PV+26
W_BLDG  EQU PV+27
W_CNT   EQU PV+28
W_STEP  EQU PV+29
W_FRAME EQU PV+30
W_FACE  EQU PV+31
W_SP    EQU PV+32
W_COL   EQU PV+33
W_IN    EQU PV+34
W_MULE  EQU PV+35
W_TX    EQU PV+36               ; 4: the last steps, for the M.U.L.E. to follow
W_TY    EQU PV+40               ; 4
W_TH    EQU PV+44
DV_I    EQU PV+45               ; scratch
DV_J    EQU PV+46
DV_K    EQU PV+47
DV_N    EQU PV+48
DV_V    EQU PV+49               ; 2
DV_V2   EQU PV+51               ; 2
DV_X    EQU PV+53
DV_Y    EQU PV+54
DV_HC   EQU PV+55
DV_HIH  EQU PV+56
DV_A    EQU PV+57
DV_IX   EQU PV+58               ; the stick: -1, 0, 1
DV_IY   EQU PV+59
DV_COL  EQU PV+60
DV_SPTR EQU PV+61               ; 2: a script's start
AISCR   EQU PV+64               ; 2 x 40: a computer's walk scripts

SLOT_PL EQU 0
SLOT_MULE EQU 1
SLOT_WAMP EQU 2

WE_TOWNL EQU 1                  ; walk events
WE_TOWNR EQU 2
WE_STOREL EQU 3
WE_STORER EQU 4
WE_BLDG EQU 5

B_ASSAY EQU 4                   ; buildings: the outfitters are the goods
B_LAND  EQU 5
B_PUB   EQU 6
B_CORRAL EQU 7

TOWN_Y_TOP EQU 37               ; the street: the player's top 37..41
TOWN_Y_BOT EQU 41
TOWN_Y_IN_DN EQU 57             ; walked into a lower building
TOWN_Y_IN_UP EQU 21             ; into an outfitter
TOWN_X_LEFT EQU 36
TOWN_X_RIGHT EQU 132
TOWN_X_START EQU 84

CORCELL EQU 21                  ; the corral's numbers, right of the bar
CORY1   EQU 22
CORY2   EQU 30

; the doors: the four outfitters (by good), assay, land, pub, corral
DOORLO: DB 120, 96, 72, 48, 48, 68, 88, 108
DOORHI: DB 120, 96, 72, 48, 48, 68, 88, 124
; a player event's amount (text.c pevent_x): x 25 (round / 4 + 1), or per
; M.U.L.E. (100 food, 101 mining, 102 energy)
PEVX:   DB 0, 0, 2, 4, 100, 4, 8, 2, 3, 6, 4, 2, 0, 0, 3, 101, 102, 6, 4, 4, 4, 0

; AIPUT -- byte v into the script at DC1.  Clobbers A.
AIPUT   MACRO v
        XDC
        LI v
        ST
        XDC
        ENDM

; ---------------------------------------------------------------------------

; DEVE -- develop_enter.
DEVE:   ENTER
        LI 0FFH
        STB DVSTATE
        STB DV_MSEAT
        CLR
        STB W_MULE
        STB MAPDOTS
        STB DV_AI
        STB DV_BWAIT
        STB SCRNOW
        LEAVE

; DVMSG -- message r0, no numbers.
DVMSG:  ENTER
        CLR
        STB EXPV1
        ST
        STB EXPV2
        ST
        PI MSGSHOW
        LEAVE

; ---------------------------------------------------------------------------
; Walking, every frame (WALKISR): a step every 6 frames (on the map 14 on
; the river, 21 in the mountains); 4 x / 2 y in town, 2 / 1 on the map. On
; the map the player is the small figure, in town the species.
; ---------------------------------------------------------------------------

; WALKDRAW -- the player and the M.U.L.E. behind (walk_draw).
WALKDRAW: ENTER
        CLR
        STB SPA_SL
        STB SPA_FL
        LDB W_SP
        LR 4,A
        SL 1
        LR 5,A                  ; 2 sp
        SL 1
        SL 1
        AS 5                    ; 10 sp
        LR 5,A
        LDB W_TOWN
        NI 0FFH
        BZ WD1
        LIS SH_SPECIES
        STB SPA_SH
        LR A,5
        AS 4                    ; 11 sp
        BR WD2
WD1:    LIS SH_FIGURE
        STB SPA_SH
        LR A,5
WD2:    LR 5,A
        LDB W_FACE
        AS 5
        STB SPA_FR
        LDB W_X                 ; (x - 8, (y - 8) * 2)
        AI 0F8H
        STB SPA_XL
        CLR
        ST
        LDB W_Y
        AI 0F8H
        SL 1
        STB SPA_YL
        CLR
        ST
        LDB W_COL
        STB SPA_CO
        PI SPRSET
        LDB W_MULE
        NI 0FFH
        BNZ WD3
        LIS SLOT_MULE
        LR 0,A
        PI SPRHIDE
        LEAVE
WD3:    LDB W_TH                ; the oldest step remembered
        LR 5,A
        DCI W_TX
        ADC
        LM
        NI 0FFH
        BNZ WD4
        CLR
        LR 5,A
WD4:    DCI W_TX
        LR A,5
        ADC
        LM
        NI 0FFH
        JZ WDX
        LR 6,A
        DCI W_TY
        LR A,5
        ADC
        LM
        LR 7,A
        LIS SLOT_MULE
        STB SPA_SL
        LIS SH_MULE
        STB SPA_SH
        LDB W_FACE              ; facing left (2, 3): flipped
        AI 0FEH
        CI 1
        LIS 0
        BNC WD5
        LIS SPR_FLIPX
WD5:    STB SPA_FL
        LDB W_TOWN
        NI 0FFH
        LDB W_FRAME
        BNZ WD6
        AI 2
WD6:    STB SPA_FR
        LR A,6
        AI 0F8H
        STB SPA_XL
        CLR
        ST
        LDB W_TOWN              ; in town 3 lower
        NI 0FFH
        LR A,7
        BZ WD7
        AI 3
WD7:    AI 0F8H
        SL 1
        STB SPA_YL
        CLR
        ST
        LIS C_WHITE
        STB SPA_CO
        PI SPRSET
WDX:    LEAVE

; WTERR -- how long a map step takes (walk_terrain): the figure's middle on
;   the river's band, or by a plot's peaks (its upper left).
WTERR:  ENTER
        LDB W_X                 ; cx = x - 6, cy = 2 y - 14
        AI 0FAH
        STB DV_X
        LDB W_Y
        SL 1
        AI 0F2H
        STB DV_Y
        LDB DV_X
        CI 7
        JC WTX
        LDB DV_Y
        CI 15
        JC WTX
        LDB DV_X                ; px = (cx - 8) / 16, py = (cy - 16) / 32
        AI 0F8H
        SR 4
        LR 4,A
        LDB DV_Y
        AI 0F0H
        SR 4
        SR 1
        LR 5,A
        LR A,4
        CI 8
        JNC WTX
        LR A,5
        CI 4
        JNC WTX
        SL 1                    ; p = 9 py + px
        SL 1
        SL 1
        AS 5
        AS 4
        STB DV_A
        DCI ST+M_TERR
        ADC
        LM
        LR 6,A
        NI 3
        CI T_RIVER
        BNZ WT2
        LDB DV_X                ; the river: cx + 4 >= 76 and cx <= 83
        AI 4
        CI 75
        JC WTX
        LDB DV_X
        CI 83
        JNC WTX
        LDB W_CNT
        AI 8
        STB W_CNT
        JMP WTX
WT2:    LR A,6                  ; peaks?
        SR 1
        SR 1
        NI 3
        JZ WTX
        LDB DV_A
        LR 0,A
        PI PLATXY               ; r4 = PLOT_X, r6 = PLOT_Y
        LDB DV_X                ; cx + 4 > PX + 1
        AI 4
        LR 5,A
        LR A,4
        INC
        COM
        AS 5
        JNC WTX
        LR A,4                  ; cx < PX + 9
        AI 9
        LR 5,A
        LDB DV_X
        COM
        AS 5
        JNC WTX
        LDB DV_Y                ; cy + 6 > PY + 2
        AI 6
        LR 5,A
        LR A,6
        AI 2
        COM
        AS 5
        JNC WTX
        LR A,6                  ; cy < PY + 20
        AI 20
        LR 5,A
        LDB DV_Y
        COM
        AS 5
        JNC WTX
        LDB W_CNT
        AI 15
        STB W_CNT
WTX:    LEAVE

; WTOWN -- a step in town (walk_town): along the street 4 a step, out of
;   either end back to the map; the street is two steps deep, and pushing on
;   at a door walks you in (the outfitters only with a M.U.L.E.).
WTOWN:  ENTER
        LIS 5
        STB W_CNT
        LDB DV_IX
        NI 080H
        BZ WTW2
        LDB W_X
        CI TOWN_X_LEFT
        BNC WTW1
        LIS WE_TOWNL
        STB W_EVENT
        LIS 1
        STB W_HOLD
        LEAVE
WTW1:   AI 0FCH
        STB W_X
        BR WTW4
WTW2:   LDB DV_IX
        NI 0FFH
        BZ WTW4
        LDB W_X
        CI TOWN_X_RIGHT-1
        BC WTW3
        LIS WE_TOWNR
        STB W_EVENT
        LIS 1
        STB W_HOLD
        LEAVE
WTW3:   AI 4
        STB W_X
WTW4:   LDB DV_IY
        NI 080H
        BNZ WTW7
        LDB DV_IY
        NI 0FFH
        JZ WTWX
        LDB W_Y                 ; down
        CI TOWN_Y_BOT-1
        BNC WTW5
        AI 2
        STB W_Y
        JMP WTWX
WTW5:   LIS 4                   ; at a lower door: in
        LR 5,A
WTW6:   DCI DOORLO
        LR A,5
        ADC
        LM
        LR 6,A
        DCI DOORHI
        LR A,5
        ADC
        LM
        LR 7,A
        LDB W_X
        LR 4,A
        COM
        AS 6                    ; lo + ~x carries: lo > x
        BC WTW6N
        LR A,7
        COM
        AS 4                    ; x + ~hi carries: x > hi
        BC WTW6N
        LIS 1
        STB W_IN
        LR A,5
        STB W_BLDG
WTW6N:  LR A,5
        INC
        LR 5,A
        CI 7
        BC WTW6
        JMP WTWX
WTW7:   LDB W_Y                 ; up
        CI TOWN_Y_TOP
        BC WTW8
        AI 0FEH
        STB W_Y
        JMP WTWX
WTW8:   LDB W_MULE
        NI 0FFH
        JZ WTWX
        CLR
        LR 5,A
WTW9:   DCI DOORLO
        LR A,5
        ADC
        LM
        LR 6,A
        LDB W_X
        XS 6
        BNZ WTW9N
        LIS 2
        STB W_IN
        LR A,5
        STB W_BLDG
WTW9N:  LR A,5
        INC
        LR 5,A
        CI 3
        BC WTW9
WTWX:   LEAVE

; WBLDG -- through a door (walk_building): 2 a step until inside; then
;   DEVT runs the building and puts us back on the street.
WBLDG:  ENTER
        LIS 5
        STB W_CNT
        LDB W_FRAME
        XI 1
        STB W_FRAME
        LR 4,A
        LDB W_IN
        CI 1
        BNZ WB2
        LDB W_Y
        AI 2
        STB W_Y
        LR A,4
        AI 6
        STB W_FACE
        LDB W_Y
        CI TOWN_Y_IN_DN-1
        BC WB3
WB1:    CLR
        STB W_IN
        LIS WE_BLDG
        STB W_EVENT
        LIS 1
        STB W_HOLD
        BR WB3
WB2:    LDB W_Y
        AI 0FEH
        STB W_Y
        LR A,4
        AI 8
        STB W_FACE
        LDB W_Y
        CI TOWN_Y_IN_UP
        BC WB1
WB3:    PI WALKDRAW
        LEAVE

; WMAP -- a step on the map (walk_map): the figure kept to x 15..157, y
;   16..92; into the store (plot 22: x 80..95, y 48..63) takes us to town.
WMAP:   ENTER
        LIS 5
        STB W_CNT
        LDB DV_IX
        NI 080H
        BZ WM1
        LDB W_X
        CI 15
        BC WM2
        AI 0FEH
        STB W_X
        BR WM2
WM1:    LDB DV_IX
        NI 0FFH
        BZ WM2
        LDB W_X
        CI 156
        BNC WM2
        AI 2
        STB W_X
WM2:    LDB DV_IY
        NI 080H
        BZ WM3
        LDB W_Y
        CI 16
        BC WM4
        AI 0FFH
        STB W_Y
        BR WM4
WM3:    LDB DV_IY
        NI 0FFH
        BZ WM4
        LDB W_Y
        CI 91
        BNC WM4
        INC
        STB W_Y
WM4:    PI WTERR
        LDB W_X
        AI 5
        CI 79
        BC WMX
        LDB W_X
        AI 2
        CI 95
        BNC WMX
        LDB W_Y
        AI 3
        CI 47
        BC WMX
        LDB W_Y
        CI 63
        BNC WMX
        LDB W_X
        CI 87
        LIS WE_STOREL
        BC WM5
        LIS WE_STORER
WM5:    STB W_EVENT
        LIS 1
        STB W_HOLD
WMX:    LEAVE

; WALKISR -- every frame while we walk (walk_isr).
WALKISR: ENTER
        LDB W_HOLD
        NI 0FFH
        JNZ WIX
        LDB W_CNT
        NI 0FFH
        BZ WI1
        AI 0FFH
        STB W_CNT
        JMP WIX
WI1:    LDB W_IN
        NI 0FFH
        BZ WI2
        PI WBLDG
        JMP WIX
WI2:    CLR
        STB DV_IX
        STB DV_IY
        LDB INPDIR
        NI DIR_LEFT
        BZ WI3
        LI 0FFH
        STB DV_IX
WI3:    LDB INPDIR
        NI DIR_RIGHT
        BZ WI4
        LIS 1
        STB DV_IX
WI4:    LDB INPDIR
        NI DIR_UP
        BZ WI5
        LI 0FFH
        STB DV_IY
WI5:    LDB INPDIR
        NI DIR_DOWN
        BZ WI6
        LIS 1
        STB DV_IY
WI6:    LDB DV_IX
        NI 0FFH
        BNZ WI8
        LDB DV_IY
        NI 0FFH
        BNZ WI8
        LDB W_STEP              ; standing still: facing us
        NI 0FFH
        JZ WIX
        CLR
        STB W_STEP
        LIS 5
        STB W_FACE
        PI WALKDRAW
        JMP WIX
WI8:    LDB W_TOWN
        NI 0FFH
        BZ WI9
        PI WTOWN
        BR WI10
WI9:    PI WMAP
WI10:   LDB W_HOLD
        NI 0FFH
        JNZ WIX
        LIS 1
        STB W_STEP
        LDB W_FRAME
        XI 1
        STB W_FRAME
        LR 4,A
        LDB DV_IX
        NI 0FFH
        BZ WI12
        NI 080H
        BNZ WI11
        LR A,4                  ; right
        STB W_FACE
        BR WI14
WI11:   LR A,4                  ; left
        AI 2
        STB W_FACE
        BR WI14
WI12:   LDB DV_IY
        NI 080H
        BNZ WI13
        LR A,4                  ; down
        AI 6
        STB W_FACE
        BR WI14
WI13:   LR A,4                  ; up
        AI 8
        STB W_FACE
WI14:   LIS SND_STEP
        LR 0,A
        PI SNDPLAY
        LDB W_X                 ; remembered, for the M.U.L.E. to follow
        LR 5,A
        LDB W_Y
        LR 6,A
        LDB W_TH
        LR 7,A
        DCI W_TX
        ADC
        LR A,5
        ST
        DCI W_TY
        LR A,7
        ADC
        LR A,6
        ST
        LR A,7
        INC
        NI 3
        STB W_TH
        PI WALKDRAW
WIX:    LEAVE

; ---------------------------------------------------------------------------
; The screens
; ---------------------------------------------------------------------------

; TMONEY -- our money under the town, the corral's price and stock beside
;   it (town_money).
TMONEY: ENTER
        LDB SCRNOW
        CI SCR_TOWN
        JNZ TMX
        LDB MYSEAT
        CI 3
        JNC TMX
        LR 0,A
        PI PLDC
        LI P_MONEY
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        STW 0, 1, EXPV1
        CLR
        STB EXPV2
        ST
        LI MS_MONEY
        LR 0,A
        DCI STM
        PI STRP
        XDC
        DCI LINBUF
        XDC
        PI TXTEXP
        LI MSGY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI LINBUF
        PI TCENTER
        LI CORCELL
        STB DTMP+0
        LDW 0, 1, ST+S_MPRIC
        LIS 3
        LR 4,A
        LI CORY1
        LR 2,A
        LIS C_WHITE
        LR 6,A
        PI PUTNUM
        LI CORCELL+1
        STB DTMP+0
        LDB ST+S_MULES
        LR 0,A
        CLR
        LR 1,A
        LIS 2
        LR 4,A
        LI CORY2
        LR 2,A
        LIS C_WHITE
        LR 6,A
        PI PUTNUM
TMX:    LEAVE

; DVRESUME -- walking again (dv_resume).
DVRESUME: ENTER
        CLR
        STB W_HOLD
        LIS 5
        STB W_FACE
        PI WALKDRAW
        LIS AN_WALK
        STB ANIM
        LEAVE

; TOWNENT -- the town screen (town_enter).
TOWNENT: ENTER
        CLR
        STB ANIM
        PI FXSTOP
        PI DCLS
        PI BANDS
        LIS SCR_TOWN
        STB SCRNOW
        PI TOWNDRAW
        LI TI_DEVELOPMENT
        LR 0,A
        LDB RND
        LR 1,A
        PI TITLE
        LI MSGY
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        LIS 1
        STB MSGONE
        PI TMONEY
        LI 86
        STB BARX
        LI 10
        STB BARTOP
        LI 36
        STB BARH
        LI 0FFH
        STB BARLAST
        LIS 1
        STB W_TOWN
        PI DVRESUME
        LEAVE

; DMAPENT -- the map, for development (dev_map_enter).
DMAPENT: ENTER
        PI MAPENTER
        CLR
        STB MSGONE
        LI MSGY
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        LI TI_DEVELOPMENT
        LR 0,A
        LDB RND
        LR 1,A
        PI TITLE
        LEAVE

; DVBLANK -- the screen blank about 22 frames between town and map.
DVBLANK: ENTER
        CLR
        STB ANIM
        PI SPRHALL
        PI DCLS
        LI 22
        STB DV_K
DBL1:   PI FRWAIT
        PI SNDTICK
        DECB DV_K
        BNZ DBL1
        CLR
        DCI W_TX
        ST
        ST
        ST
        ST
        LEAVE

; DVSTART -- our turn begins, in the middle of town (dv_start_walk).
DVSTART: ENTER
        LIS DV_WALK
        STB DVSTATE
        LI 0FFH
        STB DV_SAMP
        CLR
        STB DV_OFFICE
        STB DV_EVSH
        STB DV_BWAIT
        LDB MYSEAT
        LR 0,A
        PI PLDC
        LI P_SPEC
        ADC
        LM
        NI 7
        STB W_SP
        LDB MYSEAT
        LR 0,A
        PI PLCOLR
        STB W_COL
        STB BARCOL
        CLR
        DCI W_TX                ; (W_TY follows)
        ST
        ST
        ST
        ST
        ST
        ST
        ST
        ST
        LI TOWN_X_START
        STB W_X
        LI TOWN_Y_TOP+2
        STB W_Y
        CLR
        STB W_IN
        STB W_EVENT
        STB W_CNT
        PI TOWNENT
        LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
        LEAVE

; DVSTOP -- our turn is over (dv_stop_walk).
DVSTOP: ENTER
        CLR
        STB ANIM
        LIS 1
        STB W_HOLD
        LIS DV_OVER
        STB DVSTATE
        LIS SLOT_PL
        LR 0,A
        PI SPRHIDE
        LIS SLOT_MULE
        LR 0,A
        PI SPRHIDE
        LIS SLOT_WAMP
        LR 0,A
        PI SPRHIDE
        LDB SCRNOW
        CI SCR_MAP
        BZ DST1
        PI DMAPENT
DST1:   LEAVE

; ---------------------------------------------------------------------------
; The buildings: the action goes to the server, its answer puts us out.
; ---------------------------------------------------------------------------

; BACKST -- back on the street (back_to_street).
BACKST: ENTER
        CLR
        STB DV_BWAIT
        LDB DVSTATE
        CI DV_WALK
        BNZ BSX
        LI TOWN_Y_TOP+2
        STB W_Y
        LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
        PI DVRESUME
BSX:    LEAVE

; DVBLD -- inside a building (dv_building).
DVBLD:  ENTER
        CLR
        STB DV_A                ; the action
        STB DV_V                ; its argument
        ST
        LDB W_BLDG
        CI B_CORRAL
        BNZ DB1
        LDB W_MULE              ; the corral: buy one, or give it back
        NI 0FFH
        LI ACT_RETN
        BNZ DB0
        LI ACT_BUYM
DB0:    STB DV_A
        BR DB9
DB1:    CI GOOD_CRYS            ; an outfitter
        BNC DB2
        STB DV_V
        LI ACT_OUTF
        STB DV_A
        BR DB9
DB2:    CI B_ASSAY
        BNZ DB3
        LDB DV_SAMP
        CI NPLOTS-1
        BNC DB2A
        STB DV_V
        LI ACT_ASSAY
        STB DV_A
        BR DB9
DB2A:   LI MS_NOSAMPLE
        LR 0,A
        PI DVMSG
        BR DB9
DB3:    CI B_LAND
        BNZ DB4
        LIS 1
        STB DV_OFFICE
        LI MS_LANDOFFICE
        LR 0,A
        PI DVMSG
        BR DB9
DB4:    CI B_PUB
        BNZ DB9
        LI ACT_PUB
        STB DV_A
DB9:    LDB DV_A                ; stand in the door until the server answers
        NI 0FFH
        BZ DB10
        LR 0,A
        LDW 1, 2, DV_V
        PI ACTSEND
        LIS 1
        STB DV_BWAIT
        LDW 0, 1, FRAMES
        STW 0, 1, DV_BT0
        LEAVE
DB10:   PI BACKST
        LEAVE

; DVBRES -- what the server said about a building (dv_building_result).
DVBRES: ENTER
        LDB ST+H_ERR
        CI ERR_MONEY
        BNZ DR1
        LI MS_NOMONEY
        LR 0,A
        PI DVMSG
        BR DR9
DR1:    CI ERR_NOSTK
        BNZ DR2
        LI MS_NOMULES
        LR 0,A
        PI DVMSG
        BR DR9
DR2:    NI 0FFH
        BZ DR3
        LIS SND_ERROR
        LR 0,A
        PI SNDPLAY
        BR DR9
DR3:    LDB ACTSENT
        CI ACT_ASSAY
        BNZ DR4
        LDB ST+H_RES
        NI 7
        AI MS_ASSAY
        LR 0,A
        PI DVMSG
        LI 0FFH
        STB DV_SAMP
DR4:    LDB ACTSENT
        CI ACT_BUYM
        BZ DR5
        CI ACT_OUTF
        BNZ DR9
DR5:    LIS SND_BUY
        LR 0,A
        PI SNDPLAY
DR9:    PI TMONEY
        LEAVE

; DVMRES -- what the server said about a map action (dv_map_result).
DVMRES: ENTER
        LDB ACTSENT             ; install .. wampus
        AI 256-ACT_INST
        CI ACT_WAMP-ACT_INST
        JNC DMX
        LIS C_WHITE
        STB MSGCOL
        LDB ST+H_ERR
        NI 0FFH
        BZ DM1
        LIS SND_ERROR
        LR 0,A
        PI SNDPLAY
        CLR
        STB ACTSENT
        JMP DMX
DM1:    CLR
        STB DV_K
        LDB ACTSENT
        CI ACT_INST
        BNZ DM2
        LDB ST+H_RES
        CI 1
        BNZ DM1A
        LI MS_RANAWAY
        STB DV_K
        LIS SND_ERROR
        LR 0,A
        PI SNDPLAY
        BR DM5
DM1A:   LIS SND_BUY
        LR 0,A
        PI SNDPLAY
        BR DM5
DM2:    CI ACT_WAMP
        BNZ DM3
        LI MS_CAUGHT
        STB DV_K
        LIS SND_FOUND
        LR 0,A
        PI SNDPLAY
        BR DM5
DM3:    CI ACT_SELL
        BNZ DM4
        LI MS_FORSALE
        STB DV_K
        BR DM5
DM4:    CI ACT_UNSEL
        BNZ DM5
        LI MS_NOTFORSALE
        STB DV_K
DM5:    LDB DV_K
        NI 0FFH
        BZ DM6
        LR 0,A
        LDW 4, 5, ST+H_RES
        STW 4, 5, EXPV1
        CLR
        STB EXPV2
        ST
        PI MSGSHOW
DM6:    CLR
        STB ACTSENT
DMX:    LEAVE

; DVBTN -- the button on the map (dv_button): catch the wampus, install
;   the M.U.L.E. we lead, list land after the land office, or take a sample.
DVBTN:  ENTER
        LI 0FFH                 ; the plot under the figure's middle
        STB DV_K
        LDB W_X
        CI 11
        BC DBT1
        LDB W_Y
        CI 13
        BC DBT1
        LDB W_X
        AI 0F4H
        SR 4
        LR 4,A
        LDB W_Y
        AI 0F2H
        SR 4
        LR 5,A
        LR A,4
        CI 8
        BNC DBT1
        LR A,5
        CI 4
        BNC DBT1
        SL 1
        SL 1
        SL 1
        AS 5
        AS 4
        STB DV_K
DBT1:   LDB DV_WP               ; a wampus showing, nothing led: within 8
        CI 11
        JNC DBT2
        LDB W_MULE
        NI 0FFH
        JNZ DBT2
        LDB WP_X                ; x + 8 >= wp_x
        LR 4,A
        LDB W_X
        AI 8
        COM
        AS 4
        JC DBT2
        LDB WP_X                ; x <= wp_x + 8
        AI 8
        COM
        LR 4,A
        LDB W_X
        AS 4
        JC DBT2
        LDB WP_Y                ; y + 4 >= wp_y
        LR 4,A
        LDB W_Y
        AI 4
        COM
        AS 4
        JC DBT2
        LDB WP_Y                ; y <= wp_y + 8
        AI 8
        COM
        LR 4,A
        LDB W_Y
        AS 4
        JC DBT2
        LI ACT_WAMP
        LR 0,A
        CLR
        LR 1,A
        LR 2,A
        PI ACTSEND
        LEAVE
DBT2:   LDB DV_K
        CI NPLOTS-1
        JNC DBTX
        LDB W_MULE
        NI 0FFH
        BZ DBT3
        LI ACT_INST
        LR 0,A
        LDB DV_K
        LR 1,A
        CLR
        LR 2,A
        PI ACTSEND
        LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
        LEAVE
DBT3:   LDB DV_OFFICE
        NI 0FFH
        BZ DBT4
        LDB DV_K                ; ours: list it, or take it off
        DCI ST+M_OWNER
        ADC
        LM
        LR 4,A
        LDB MYSEAT
        XS 4
        JNZ DBTX
        LDB DV_K
        DCI ST+M_PROD
        ADC
        LM
        NI 64
        LI ACT_SELL
        BZ DBT3A
        LI ACT_UNSEL
DBT3A:  LR 0,A
        LDB DV_K
        LR 1,A
        CLR
        LR 2,A
        PI ACTSEND
        CLR
        STB DV_OFFICE
        LEAVE
DBT4:   LDB DV_K
        STB DV_SAMP
        LIS SND_SAMPLE
        LR 0,A
        PI SNDPLAY
DBTX:   LEAVE

; DVWAMP -- the wampus (dv_wampus): the turn lists when (turn jiffies) and
;   where it shows; it flashes, 8 frames on and 8 off.
DVWAMP: ENTER
        LI 0FFH
        STB DV_WP
        LDW 0, 1, FRAMES        ; now, in the turn's jiffies
        LDW 2, 3, RXFRAME
        PI SUB16
        LDW 2, 3, DV_EL
        PI ADD16
        STW 0, 1, DV_V
        LDB ST+T_NWAMP
        CI 12
        BC DW0
        LI 12
DW0:    STB DV_N
        CLR
        STB DV_I
DW1:    LDB DV_I
        LR 4,A
        LDB DV_N
        XS 4
        BZ DW3
        LDB DV_I                ; {plot, mount, from, to} a 6-byte entry
        SL 1
        LR 4,A
        SL 1
        AS 4
        DCI ST+T_WAMP+2
        ADC
        LM
        LR 2,A
        LM
        LR 3,A
        LM
        LR 6,A
        LM
        LR 7,A
        LDW 0, 1, DV_V
        PI GE16
        BNC DW2
        LR A,6
        LR 2,A
        LR A,7
        LR 3,A
        PI GE16
        BC DW2
        LDB DV_I
        STB DV_WP
DW2:    INCB DV_I
        BR DW1
DW3:    LDB DV_WP
        CI 11
        BC DW4
        LIS SLOT_WAMP
        LR 0,A
        PI SPRHIDE
        LEAVE
DW4:    SL 1                    ; on its plot's mountains
        LR 4,A
        SL 1
        AS 4
        DCI ST+T_WAMP
        ADC
        LM
        LR 5,A
        LM
        STB DV_A
        LR A,5
        LR 0,A
        PI PLATXY
        LDB DV_A
        SL 1
        LR 8,A
        LR A,4                  ; (col * 16) + 17 + 2 c = PLOT_X + 9 + 2 c
        AI 9
        AS 8
        STB WP_X
        LR A,6                  ; (row * 16) + 17 + 2 c
        AI 0F0H
        SR 1
        AI 17
        AS 8
        STB WP_Y
        LDB FRAMES
        NI 8
        BNZ DW5
        LIS SLOT_WAMP
        LR 0,A
        PI SPRHIDE
        LEAVE
DW5:    LIS SLOT_WAMP
        STB SPA_SL
        LIS SH_WAMPUS
        STB SPA_SH
        CLR
        STB SPA_FR
        STB SPA_FL
        LDB WP_X
        AI 0F8H
        STB SPA_XL
        CLR
        ST
        LDB WP_Y
        AI 0F8H
        SL 1
        STB SPA_YL
        CLR
        ST
        LIS C_WHITE
        STB SPA_CO
        PI SPRSET
        LEAVE

; DVEVENT -- the player event at the start of our turn (dv_event_msg).
DVEVENT: ENTER
        LDB DV_EVSH
        NI 0FFH
        JNZ DEX
        LIS 1
        STB DV_EVSH
        LDB ST+T_EVENT
        STB DV_K
        NI 0FFH
        JZ DEX
        CI 22
        JNC DEX
        LDB RND                 ; v = 25 (round / 4 + 1)
        SR 1
        SR 1
        INC
        LR 2,A
        LI 25
        LR 0,A
        CLR
        LR 1,A
        PI MUL8
        STW 0, 1, DV_V
        CLR
        STB DV_V2
        ST
        STB DV_N
        LDB DV_K
        AI 0FFH
        DCI PEVX
        ADC
        LM
        STB DV_J
        CI 99
        JC DE5
        CLR                     ; per M.U.L.E.: count ours
        STB DV_I
DE1:    LDB DV_I
        DCI ST+M_OWNER
        ADC
        LM
        LR 4,A
        LDB MYSEAT
        XS 4
        BNZ DE2
        LDB DV_I
        DCI ST+M_MULE
        ADC
        LM
        LR 5,A
        LDB DV_J
        CI 100
        BNZ DE1A
        LR A,5
        CI GOOD_FOOD
        BZ DE1N
        BR DE2
DE1A:   CI 101
        BNZ DE1B
        LR A,5
        CI GOOD_SMITH
        BZ DE1N
        CI GOOD_CRYS
        BZ DE1N
        BR DE2
DE1B:   LR A,5
        CI GOOD_ENERGY
        BNZ DE2
DE1N:   INCB DV_N
DE2:    INCB DV_I
        CI NPLOTS-1
        BC DE1
        LDB DV_J
        CI 102
        BNZ DE3
        LDW 0, 1, DV_V          ; energy: v * n
        LDB DV_N
        LR 2,A
        PI MUL8
        STW 0, 1, DV_V
        BR DE6
DE3:    LDW 0, 1, DV_V          ; else v2 = 2 v n, v = 2 v; mining: v = v2
        PI DBL16
        STW 0, 1, DV_V
        LDB DV_N
        LR 2,A
        PI MUL8
        STW 0, 1, DV_V2
        LDB DV_J
        CI 101
        BNZ DE6
        LDW 0, 1, DV_V2
        STW 0, 1, DV_V
        BR DE6
DE5:    LDW 0, 1, DV_V          ; v * x
        LDB DV_J
        LR 2,A
        PI MUL8
        STW 0, 1, DV_V
DE6:    DCI TXT                 ; "*** " and the event
        LI '*'
        ST
        ST
        ST
        LI ' '
        ST
        XDC
        LDW 0, 1, DV_V
        STW 0, 1, EXPV1
        LDW 0, 1, DV_V2
        STW 0, 1, EXPV2
        LDB DV_K
        AI 0FFH
        LR 0,A
        DCI STP
        PI STRP
        PI TXTEXP
        AI 4
        LR 0,A
        LIS C_WHITE
        STB MSGCOL
        PI MSGSTRT
DEX:    LEAVE

; ---------------------------------------------------------------------------
; A computer's turn: the server plays it at once, so the map already shows
; what it did. Here its figure walks out of the store to the plot that
; gained a M.U.L.E. (or one of its plots), the M.U.L.E. behind, which stays
; there; then back into the store to wait (dv_ai_walk).
; ---------------------------------------------------------------------------

; AIMV -- FX_MOVE r4, r5 (signed), every 6 frames, r6 times, at DC1.  Leaf.
AIMV:   XDC
        LIS FX_MOVE
        ST
        LR A,4
        ST
        LR A,5
        ST
        LIS 6
        ST
        LR A,6
        ST
        XDC
        POP

; DVAILAND -- the computer's land outlined in white (r0 = 1), or plain.
DVAILAND: ENTER
        LR A,0
        STB DV_A
        CLR
        STB DV_I
DL1:    LDB DV_I
        DCI ST+M_OWNER
        ADC
        LM
        LR 4,A
        LDB DV_SEAT
        XS 4
        BNZ DL2
        LDB DV_A
        NI 0FFH
        BZ DL1A
        LDB DV_I
        STB MAPBLNK
        LIS 1
        STB MAPBON
DL1A:   LDB DV_I
        LR 0,A
        PI MAPPLOT
        LI NOBODY
        STB MAPBLNK
DL2:    INCB DV_I
        CI NPLOTS-1
        BC DL1
        LEAVE

; DVAIWALK -- dv_ai_walk.
DVAIWALK: ENTER
        PI FXSTOP
        LI 0FFH
        STB DV_J
        CLR
        STB DV_N
        STB DV_I
DA1:    LDB DV_I                ; its plots: the one that gained a M.U.L.E.
        DCI ST+M_OWNER          ; this turn, else its first
        ADC
        LM
        LR 4,A
        LDB DV_SEAT
        XS 4
        BNZ DA3
        LDB DV_I
        CI STOREPL
        BZ DA3
        LDB DV_J
        CI 0FFH
        BNZ DA2
        LDB DV_I
        STB DV_J
DA2:    LDB DV_I
        DCI ST+M_MULE
        ADC
        LM
        CI 3
        BNC DA3
        LR 4,A
        LDB DV_I
        DCI PREV+M_MULE
        ADC
        LM
        XS 4
        BZ DA3
        LDB DV_I
        STB DV_J
        LIS 1
        STB DV_N
DA3:    INCB DV_I
        CI NPLOTS-1
        BC DA1
        LDB DV_J                ; no land: out and back
        CI NPLOTS-1
        BC DA4
        LI 13
        STB DV_J
DA4:    LDB DV_J                ; x = col 16 + 20, y = row 16 + 22
        LR 0,A
        PI PLATXY
        LR A,4
        AI 12
        STB DV_X
        LR A,6
        AI 0F0H
        SR 1
        AI 22
        STB DV_Y
        LDB DV_SEAT
        LR 0,A
        PI PLDC
        LI P_SPEC
        ADC
        LM
        NI 7
        STB DV_A
        LDB DV_SEAT
        LR 0,A
        PI PLCOLR
        STB DV_COL
        LDB DV_AI               ; the last one's land plain again
        CI 2
        BNZ DA5
        PI MAPALL
DA5:    LDB DV_J
        STB DV_AIP
        LIS 1
        STB DV_AI
        LDB DV_X                ; hc = |x - 83| / 2, hi = |y - 52|
        CI 82
        BC DA6
        AI 256-83
        SR 1
        STB DV_HC
        BR DA7
DA6:    COM
        INC
        AI 83
        SR 1
        STB DV_HC
DA7:    LDB DV_Y
        CI 51
        BC DA8
        AI 256-52
        STB DV_HIH
        BR DA9
DA8:    COM
        INC
        AI 52
        STB DV_HIH
DA9:    CLR                     ; actor 0 the figure, 1 its M.U.L.E.
        STB DV_K
DA10:   LDB DV_K
        NI 0FFH
        BZ DA11
        LDB DV_N
        NI 0FFH
        JZ DAX
DA11:   LDB DV_K
        LR 0,A
        PI FXAT
        LIS AC_XL
        ADC
        LI 75                   ; in the store: (83, 52) as (75, 88)
        ST
        CLR
        ST
        LI 88
        ST
        CLR
        ST
        ST                      ; wait, n, per, dx, dy
        ST
        ST
        ST
        ST
        LIS FK_SINGLE
        ST
        LR A,0                  ; slot: the actor
        ST
        LR Q,DC
        LR A,0
        NI 0FFH
        BZ DA12
        LIS SH_MULE             ; shape, frame, colour, fbase
        LR 4,A
        LIS 2
        LR 5,A
        LIS C_WHITE
        LR 6,A
        CLR
        LR 7,A
        BR DA13
DA12:   LIS SH_FIGURE
        LR 4,A
        CLR
        LR 5,A
        LDB DV_COL
        LR 6,A
        LDB DV_A                ; species * 10
        SL 1
        LR 7,A
        SL 1
        SL 1
        AS 7
        LR 7,A
DA13:   LR DC,Q
        LR A,4
        ST
        LR A,5
        ST
        LIS 1                   ; d2
        ST
        LR A,6
        ST
        CLR
        ST                      ; col2
        ST                      ; flags
        LR A,7
        ST                      ; fbase
        LDB DV_K                ; its script: AISCR + 40 a
        NI 0FFH
        LIS 0
        BZ DA13A
        LI 40
DA13A:  DCI AISCR
        ADC
        LR H,DC
        STW 10, 11, DV_SPTR
        LR DC,H
        XDC
        LDB DV_K                ; the M.U.L.E. waits 24 frames first
        NI 0FFH
        BZ DA14
        AIPUT FX_WAIT
        AIPUT 24
DA14:   AIPUT FX_FLAGS          ; facing the way it goes
        LDB DV_X
        CI 82
        LIS 0
        BNC DA15
        LIS SPR_FLIPX
DA15:   XDC
        ST
        XDC
        LDB DV_X                ; across, 2 every 6 frames
        CI 82
        LIS 2
        BNC DA16
        LI 0FEH
DA16:   LR 4,A
        CLR
        LR 5,A
        LDB DV_HC
        INC
        LR 6,A
        PI AIMV
        CLR                     ; down or up, 1 (2 lines) every 6
        LR 4,A
        LDB DV_Y
        CI 51
        LIS 2
        BNC DA17
        LI 0FEH
DA17:   LR 5,A
        LDB DV_HIH
        INC
        LR 6,A
        PI AIMV
        LDB DV_K
        NI 0FFH
        BZ DA18
        AIPUT FX_WAIT           ; the M.U.L.E. is installed: no further
        AIPUT 20
        AIPUT FX_HIDE
        AIPUT FX_END
        JMP DA19
DA18:   AIPUT FX_SIG            ; 90 frames on the plot (outlined: SIG 3 /
        AIPUT 3                 ; 4), then the same way back, facing home,
        AIPUT FX_WAIT           ; and a long wait in the store
        AIPUT 90
        AIPUT FX_SIG
        AIPUT 4
        CLR
        LR 4,A
        LDB DV_Y
        CI 51
        LI 0FEH
        BNC DA18A
        LIS 2
DA18A:  LR 5,A
        LDB DV_HIH
        INC
        LR 6,A
        PI AIMV
        AIPUT FX_FLAGS
        LDB DV_X
        CI 82
        LIS SPR_FLIPX
        BNC DA18B
        LIS 0
DA18B:  XDC
        ST
        XDC
        LDB DV_X
        CI 82
        LI 0FEH
        BNC DA18C
        LIS 2
DA18C:  LR 4,A
        CLR
        LR 5,A
        LDB DV_HC
        INC
        LR 6,A
        PI AIMV
        AIPUT FX_WAIT
        AIPUT 255
        AIPUT FX_LOOP
        AIPUT 200
        AIPUT 2
DA19:   LDW 10, 11, DV_SPTR     ; start it, shown at once
        LDB DV_K
        LR 0,A
        PI FXSTART
        LDB DV_K
        STB FXI
        PI FXLOAD
        PI FXDRAW
        INCB DV_K
        CI 1
        JC DA10
DAX:    LEAVE

; DVAIMARK -- per frame in a computer's turn (dv_ai_marks): its land
;   outlined 40 frames as the turn starts; the plot it works while there.
DVAIMARK: ENTER
        LDB DV_AI
        CI 1
        BNZ DAM2
        LIS 1
        LR 0,A
        PI DVAILAND
        LIS 2
        STB DV_AI
        LDB FRAMES
        STB DV_AIT
        BR DAM3
DAM2:   CI 2
        BNZ DAM3
        LDB DV_AIT
        COM
        INC
        LR 4,A
        LDB FRAMES
        AS 4
        CI 39
        BC DAM3
        CLR
        LR 0,A
        PI DVAILAND
        CLR
        STB DV_AI
DAM3:   LDB FXFLAG
        CI 3
        BZ DAM4
        CI 4
        BNZ DAMX
DAM4:   CI 3
        BNZ DAM5
        LDB DV_AIP
        STB MAPBLNK
        LIS 1
        STB MAPBON
DAM5:   LDB DV_AIP
        CI NPLOTS-1
        BNC DAM6
        LR 0,A
        PI MAPPLOT
DAM6:   LI NOBODY
        STB MAPBLNK
        CLR
        STB FXFLAG
DAMX:   LEAVE

; ---------------------------------------------------------------------------
; A new state, and every frame
; ---------------------------------------------------------------------------

; SAYTURN -- "NAME'S TURN" (say_turn).
SAYTURN: ENTER
        LDB DV_SEAT
        LR 0,A
        PI PLDC
        XDC
        DCI TXT
        XDC
        LIS 8
        LR 4,A
        CLR
        LR 5,A
SAY1:   LM
        NI 0FFH
        BZ SAY2
        XDC
        ST
        XDC
        LR A,5
        INC
        LR 5,A
        DS 4
        BNZ SAY1
SAY2:   LR A,5
        STB DV_N
        LI MS_TURN
        LR 0,A
        DCI STM
        PI STRP
        PI STRCPY
        LDB DV_N
        AI 7
        LR 0,A
        PI MSGSTRT
        LEAVE

; DEVS -- develop_state.
DEVS:   ENTER
        LDB ST+T_SEAT
        STB DV_SEAT
        LDB ST+T_FLAGS
        STB DV_FLAGS
        LDW 0, 1, ST+T_LEFT
        STW 0, 1, DV_LEFT
        LDW 0, 1, ST+T_ELAP
        STW 0, 1, DV_EL
        LDB ST+T_PTU
        NI 0FFH
        BNZ DS1
        LI 28
DS1:    STB DV_PTU
        LDB DV_SEAT
        LR 4,A
        LDB MYSEAT
        XS 4
        LIS 0
        BNZ DS2
        LIS 1
DS2:    STB DV_MINE
        NI 0FFH                 ; our turn, under way: we walk
        JZ DS10
        LDB DV_FLAGS
        NI TF_START
        JZ DS10
        LDB DV_FLAGS
        NI TF_ENDED
        JNZ DS10
        LDB DVSTATE
        CI DV_WALK
        BZ DS3
        PI DVSTART
DS3:    LDB DV_FLAGS            ; the server's view of what we lead wins
        NI TF_MULE
        LIS 0
        BZ DS4
        LIS 1
DS4:    STB W_MULE
        LDB DV_BWAIT            ; a building's answer
        NI 0FFH
        BZ DS6
        LDB ACTSENT
        CI ACT_BUYM
        BZ DS5
        CI ACT_OUTF
        BZ DS5
        CI ACT_RETN
        BZ DS5
        CI ACT_ASSAY
        BZ DS5
        CI ACT_PUB
        BNZ DS6
DS5:    PI DVBRES
        PI BACKST
DS6:    LDB SCRNOW
        CI SCR_MAP
        JNZ DSX
        PI MAPUPD
        PI DVMRES
        JMP DSX
DS10:   LDB DVSTATE             ; anything else happens on the map
        CI DV_WALK
        BNZ DS11
        PI DVSTOP
DS11:   LDB SCRNOW
        CI SCR_MAP
        BZ DS12
        PI DMAPENT
DS12:   LI MSGY
        STB MSGROW
        CLR
        STB MSGONE
        LDB DV_MINE
        NI 0FFH
        BZ DS14
        LDB DV_FLAGS
        NI TF_START
        BNZ DS14
        LDB DVSTATE             ; ours, not begun: press to start
        CI DV_PROMPT
        JZ DS20
        LIS DV_PROMPT
        STB DVSTATE
        LIS C_WHITE
        STB MSGCOL
        LIS MS_START
        LR 0,A
        PI DVMSG
        LIS SND_ALERT
        LR 0,A
        PI SNDPLAY
        JMP DS20
DS14:   LDB DV_SEAT             ; someone's: said once a turn, and once at
        CI 3                    ; its end (a computer's stays "taking his
        JNC DS20                ; turn" until the next seat)
        CLR
        STB DV_K
        LDB DV_SEAT
        LR 0,A
        PI PLDC
        LI P_FLAGS
        ADC
        LM
        NI PF_AI
        BZ DS15
        LIS 1
        STB DV_K
        BR DS16
DS15:   LDB DV_FLAGS
        NI TF_ENDED
        BZ DS16
        LIS 2
        STB DV_K
DS16:   LDB DV_SEAT
        LR 4,A
        LDB DV_MSEAT
        XS 4
        BNZ DS17
        LDB DV_K
        LR 4,A
        LDB DV_MKIND
        XS 4
        JZ DS20
DS17:   LDB DV_SEAT
        STB DV_MSEAT
        LDB DV_K
        STB DV_MKIND
        LIS C_WHITE
        STB MSGCOL
        LDB DV_K
        CI 1
        BNZ DS18
        LIS DV_WAIT
        STB DVSTATE
        LIS MS_COMPUTER
        LR 0,A
        PI DVMSG
        PI DVAIWALK
        BR DS20
DS18:   CI 2
        BNZ DS19
        LIS DV_OVER
        STB DVSTATE
        LDB ACTSENT
        CI ACT_PUB
        BNZ DS18A
        LDB DV_MINE
        NI 0FFH
        BZ DS18A
        LDW 0, 1, ST+H_RES
        STW 0, 1, EXPV1
        CLR
        STB EXPV2
        ST
        LIS MS_GAMBLE
        LR 0,A
        PI MSGSHOW
        BR DS20
DS18A:  LDW 0, 1, DV_LEFT
        LR A,0
        NI 0FFH
        BNZ DS18B
        LR A,1
        NI 0FFH
        BNZ DS18B
        LIS MS_TIMEOUT
        LR 0,A
        PI DVMSG
        BR DS20
DS18B:  PI MSGCLR
        BR DS20
DS19:   LIS DV_WAIT
        STB DVSTATE
        PI SAYTURN
DS20:   LDB DV_SEAT             ; the bar for whoever plays, their colour
        CI 3
        BNC DS21
        LR 0,A
        PI PLCOLR
        STB BARCOL
DS21:   LDB DV_FLAGS
        NI TF_START
        BZ DS22
        LDB DV_FLAGS
        NI TF_ENDED
        BNZ DS22
        PI DVBAR
        BR DS23
DS22:   CLR
        LR 0,A
        PI BARDRAW
DS23:   PI MAPUPD
DSX:    LEAVE

; DVBAR -- the turn's time bar, run down from the last state; a row more
;   than it shows is a later reply's jitter, not time given back (dv_bar).
DVBAR:  ENTER
        LDW 0, 1, FRAMES
        LDW 2, 3, RXFRAME
        PI SUB16
        LR A,0
        LR 2,A
        LR A,1
        LR 3,A
        LDW 0, 1, DV_LEFT
        PI GE16
        BNC DVB0
        PI SUB16
        LDB DV_PTU
        LR 2,A
        PI DIV8
        LR A,1
        NI 0FFH
        LR A,0
        BZ DVB2
        LI 0FFH
        BR DVB2
DVB0:   CLR
DVB2:   STB DV_K
        LR 0,A
        PI BARTICK
        LDB BARLAST
        CI 0FFH
        BZ DVB3
        INC
        LR 4,A
        LDB DV_K
        SR 1
        SR 1
        XS 4
        BZ DVB4
DVB3:   LDB DV_K
        LR 0,A
        PI BARDRAW
DVB4:   LEAVE

; DEVT -- develop_tick.
DEVT:   ENTER
        PI MSGTICK
        LDB DVSTATE
        CI DV_PROMPT
        BNZ DT1
        LDB EVBTN
        NI 0FFH
        JZ DTX
        LI ACT_BEGIN
        LR 0,A
        CLR
        LR 1,A
        LR 2,A
        PI ACTSEND
        LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
        JMP DTX
DT1:    CI DV_WALK
        BZ DT3
        CI DV_WAIT
        BNZ DT2
        PI DVAIMARK
DT2:    LDB DV_FLAGS            ; watching: the bar runs down between polls
        NI TF_START
        JZ DTX
        LDB DV_FLAGS
        NI TF_ENDED
        JNZ DTX
        PI DVBAR
        JMP DTX
DT3:    PI DVBAR                ; our own turn
        PI DVEVENT
        LDB DV_BWAIT
        NI 0FFH
        BZ DT5
        LDW 0, 1, FRAMES        ; no answer in 10 s: stop waiting
        LDW 2, 3, DV_BT0
        PI SUB16
        LI 600&0FFH
        LR 2,A
        LI 600>>8
        LR 3,A
        PI GE16
        JNC DTX
        PI BACKST
        JMP DTX
DT5:    LDB W_EVENT
        NI 0FFH
        JZ DT10
        CI WE_TOWNL
        BZ DT6
        CI WE_TOWNR
        BNZ DT7
DT6:    CI WE_TOWNL             ; out of town: the map, by the store
        LI 68
        BZ DT6A
        LI 99
DT6A:   STB W_X
        LI 52
        STB W_Y
        CLR
        STB W_EVENT
        STB W_TOWN
        PI DVBLANK
        PI DMAPENT
        LI 0FFH
        STB BARLAST
        PI DVRESUME
        LI 20                   ; the first map step waits 20 frames
        STB W_CNT
        JMP DTX
DT7:    CI WE_STOREL
        BZ DT8
        CI WE_STORER
        BNZ DT9
DT8:    CI WE_STOREL            ; into the store: town, at that end
        LI TOWN_X_LEFT
        BZ DT8A
        LI TOWN_X_RIGHT
DT8A:   STB W_X
        LI TOWN_Y_TOP+2
        STB W_Y
        CLR
        STB W_EVENT
        PI DVBLANK
        PI TOWNENT
        JMP DTX
DT9:    CI WE_BLDG
        JNZ DTX
        CLR
        STB W_EVENT
        PI DVBLD
        JMP DTX
DT10:   LDB W_TOWN
        NI 0FFH
        BNZ DT11
        PI DVWAMP
        LDB EVBTN
        NI 0FFH
        BZ DT11
        PI DVBTN
DT11:   LDB EVKEY               ; START ends the turn
        CI KEY_END
        BNZ DTX
        LI ACT_END
        LR 0,A
        CLR
        LR 1,A
        LR 2,A
        PI ACTSEND
DTX:    LEAVE
