; town.inc -- the development's town and what the walker does (mekkogx
; develop.c): the town's picture, the buildings and their answers, the map's
; button, the wampus, the player events, a computer's turn.
;
; The town: the outfitters across the top (from the left crystite,
; smithore, energy, food), the street, then assay, land office, pub and the
; corral; the time bar under them, across.

TOWNY0  EQU     14                      ; the outfitters
TOWNY1  EQU     67                      ; the lower buildings
BLDW    EQU     14
BLDH    EQU     27

; town_enter: the town (mekkogx town_enter).
        RFIT    158
town_enter:
        SETV    ANIM, AN_NONE
        FCALL   fx_stop_all
        FCALL   scr_clear
        SETV    SCRNOW, SCR_TOWN
        SETV    MSGROW, 8
        SETV    MSG1, 0
        HOLD1
        LDR     RC, townbld             ; each building: x, y, its sign
tn1:    LDA     RC
        XRI     0FFH
        BZ      tn2
        DEC     RC
        FCALL   tn_bldg
        BR      tn1
tn2:    GETV    RND
        PUTV    V1L
        SETV    V1H, 0
        LDI     T_DEVELOP
        FCALL   title_show
        FCALL   town_money
        HOLD0
        FCALL   bar_town
        SETV    WK_TOWN, 1
        FJMP    dv_resume
; x, y, sign picture, sign x offset, door: 0 below (outfitters), 1 above
townbld:
        DB      1, TOWNY0, IM_ICCRYS, 4, 0
        DB      17, TOWNY0, IM_ICSMITH, 4, 0
        DB      33, TOWNY0, IM_ICENRG, 4, 0
        DB      49, TOWNY0, IM_ICFOOD, 4, 0
        DB      1, TOWNY1, IM_FLASK, 5, 1
        DB      17, TOWNY1, IM_FLAG, 4, 1
        DB      33, TOWNY1, IM_MUG, 4, 1
        DB      49, TOWNY1, IM_SMULEA, 5, 1
        DB      0FFH

; tn_bldg: the building at RC (5 bytes, RC past them): walls, roof, door, sign.
        RFIT    180
tn_bldg:
        LDA     RC
        PUTV    T0                      ; x
        LDA     RC
        PUTV    T1                      ; y
        PARS                            ; the walls
        GETV    T0
        PD
        GETV    T1
        PD
        P       BLDW
        P       BLDH
        P       TM_SET
        GOP     TOP_RECT
        PARS                            ; inside
        GETV    T0
        ADI     1
        PD
        GETV    T1
        ADI     3
        PD
        P       BLDW-2
        P       BLDH-4
        P       TM_CLEAR
        GOP     TOP_RECT
        PARS                            ; the sign
        LDA     RC
        PD
        LDA     RC
        STR     R2
        GETV    T0
        ADD
        PD
        GETV    T1
        ADI     6
        PD
        P       TM_OR
        GOP     TOP_STAMP
        PARS                            ; the door: an opening, below or above
        GETV    T0
        ADI     4
        PD
        LDA     RC
        BNZ     tb1
        GETV    T1
        ADI     BLDH-8
        BR      tb2
tb1:    GETV    T1
tb2:    PD
        P       6
        P       8
        P       TM_CLEAR
        GOP     TOP_RECT
        FRET

; town_money: our money, small, at the street's corner; the corral's price
; and its M.U.L.E.s (mekkogx town_money).
        RFIT    183
town_money:
        GETV    SCRNOW
        XRI     SCR_TOWN
        BNZ     tm9
        GETV    MYSEAT
        SMI     4
        BDF     tm9
        PARS
        P       40
        P       59
        P       24
        P       6
        P       TM_CLEAR
        GOP     TOP_RECT
        LDR     R9, S_PLAYERS+PL_MONEY
        GETV    MYSEAT
        FCALL   pl_byte
        PLO     R9
        INC     R8
        LDN     R8
        PHI     R9
        LDI     59
        PHI     RC
        LDI     64
        PLO     RC
        LDI     TM_OR|TM_SHORT
        FCALL   pt_num
        PARS                            ; "$" before it
        P       40
        P       59
        P       TM_OR|TM_SHORT
        P       '$'
        GOP     TOP_PTEXT
        PARS                            ; the corral: price, then how many
        P       49
        P       TOWNY1+BLDH+1
        P       14
        P       5
        P       TM_CLEAR
        GOP     TOP_RECT
        SPTR    SO_MULEPRICE
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
        LDI     TOWNY1+18
        PHI     RC
        LDI     62
        PLO     RC
        LDI     TM_OR|TM_SHORT
        FCALL   pt_num
        SLD     SO_MULES
        PLO     R9
        LDI     0
        PHI     R9
        LDI     TOWNY1+11
        PHI     RC
        LDI     62
        PLO     RC
        LDI     TM_OR|TM_SHORT
        FJMP    pt_num
tm9:    FRET

; ---- the buildings (mekkogx dv_building): the action goes to the server,
; whose answer puts us back out --------------------------------------------------
        RFIT    161
dv_building:
        GETV    WK_BLDG
        XRI     B_CORRAL
        BNZ     db1
        GETV    WK_MULE                  ; the corral: buy, or give it back
        BZ      db0
        LDI     ACT_RETURN
        BR      dbsend
db0:    LDI     ACT_BUYMU
        BR      dbsend
db1:    GETV    WK_BLDG
        SMI     4
        BDF     db2
        GETV    WK_BLDG                  ; an outfitter
        PLO     R9
        LDI     ACT_OUTFIT
        BR      dbsend1
db2:    GETV    WK_BLDG
        XRI     B_ASSAY
        BNZ     db4
        GETV    DV_SAMP                 ; assay: the sample we took
        SMI     NUM_PLOTS
        BNF     db3
        LDI     M_NOSAMPLE
        FCALL   msg_show
        FJMP    dv_street
db3:    GETV    DV_SAMP
        PLO     R9
        LDI     ACT_ASSAY
        BR      dbsend1
db4:    XRI     B_ASSAY!B_LAND
        BNZ     db5
        SETV    DV_OFFC, 1              ; the land office: a plot, on the map
        LDI     M_LANDOFFICE
        FCALL   msg_show
        FJMP    dv_street
db5:    LDI     ACT_PUB
dbsend: PLO     RC
        LDI     0
        PLO     R9
        GLO     RC
dbsend1:
        PLO     RC                      ; stand in the door until it answers
        LDI     0
        PHI     R9
        GLO     RC
        FCALL   act_send
        SETV    DV_BWAIT, 1
        GETV    FCL
        PUTV    DV_BT0L
        GETV    FCH
        PUTV    DV_BT0H
        FRET

; dv_street: back out on the street (mekkogx back_to_street).
dv_street:
        SETV    DV_BWAIT, 0
        GETV    DVSTATE
        XRI     DV_WALK
        BNZ     dsr9
        SETV    WK_Y, TY_TOP+3
        LDI     SND_SELECT
        FCALL   snd_play
        FJMP    dv_resume
dsr9:   FRET

; dv_bldresult: what the building's answer says (mekkogx dv_building_result).
        RFIT    98
dv_bldresult:
        SLD     H_ERR
        XRI     ERR_MONEY
        BNZ     dbr1
        LDI     M_NOMONEY
        BR      dbr3
dbr1:   XRI     ERR_MONEY!ERR_NOSTOCK
        BNZ     dbr2
        LDI     M_NOMULES
dbr3:   FCALL   msg_show
        FJMP    town_money
dbr2:   SLD     H_ERR
        BZ      dbr4
        LDI     SND_ERROR
        FCALL   snd_play
        FJMP    town_money
dbr4:   GETV    ACTSENT
        XRI     ACT_ASSAY
        BNZ     dbr6
        SLD     H_RESULT                ; the assay's level, 0-4
        ANI     7
        SMI     5
        BNF     dbr5
        LDI     4-5
dbr5:   ADI     M_ASSAY+5
        FCALL   msg_show
        SETV    DV_SAMP, 0FFH
dbr6:   GETV    ACTSENT
        XRI     ACT_BUYMU
        BZ      dbr7
        XRI     ACT_BUYMU!ACT_OUTFIT
        BNZ     dbr8
dbr7:   LDI     SND_BUY
        FCALL   snd_play
dbr8:   FJMP    town_money

; dv_mapresult: what the server said about a map action (mekkogx dv_map_result).
        RFIT    126
dv_mapresult:
        GETV    ACTSENT
        SMI     ACT_INST
        BNF     dmr9
        SMI     ACT_WAMPUS-ACT_INST+1
        BDF     dmr9
        SLD     H_ERR
        BZ      dmr1
        LDI     SND_ERROR
        FCALL   snd_play
        SETV    ACTSENT, 0
dmr9:   FRET
dmr1:   SLD     H_RESULT
        PUTV    V1L
        INC     R8
        LDN     R8
        PUTV    V1H
        GETV    ACTSENT
        XRI     ACT_INST
        BNZ     dmr3
        SLD     H_RESULT
        XRI     1
        BNZ     dmr2
        LDI     SND_ERROR               ; it ran off
        FCALL   snd_play
        LDI     M_RANAWAY
        BR      dmr7
dmr2:   LDI     SND_BUY
        FCALL   snd_play
        BR      dmr8
dmr3:   XRI     ACT_INST!ACT_WAMPUS
        BNZ     dmr4
        LDI     SND_FOUND
        FCALL   snd_play
        LDI     M_CAUGHT
        BR      dmr7
dmr4:   XRI     ACT_WAMPUS!ACT_SELL
        BNZ     dmr5
        LDI     M_FORSALE
        BR      dmr7
dmr5:   XRI     ACT_SELL!ACT_UNSELL
        BNZ     dmr8
        LDI     M_NOTFORSALE
dmr7:   FCALL   msg_show
dmr8:   SETV    ACTSENT, 0
        FRET

; dv_button: 5 on the map: catch the wampus, install the M.U.L.E. we lead,
; list land after the land office, or take a sample (mekkogx dv_button).
        RFIT    159
dv_button:
        FCALL   w_plot
        GETV    DV_WP                   ; near a showing wampus, no M.U.L.E.
        SMI     12
        BDF     bn1
        GETV    WK_MULE
        BNZ     bn1
        GETV    DV_WPX                  ; |x - wx| <= 5, |y - wy| <= 8
        STR     R2
        GETV    WK_X
        SM
        ADI     5
        SMI     11
        BDF     bn1
        GETV    DV_WPY
        STR     R2
        GETV    WK_Y
        SM
        ADI     8
        SMI     17
        BDF     bn1
        LDI     ACT_WAMPUS
        FJMP    act_send
bn1:    GETV    DV_K
        SMI     NUM_PLOTS
        BDF     bn9
        GETV    DV_K
        PLO     R9
        LDI     0
        PHI     R9
        GETV    WK_MULE                  ; install it here
        BZ      bn2
        LDI     ACT_INST
        FCALL   act_send
        LDI     SND_SELECT
        FJMP    snd_play
bn2:    GETV    DV_OFFC                 ; the land office: ours, listed or not
        BZ      bn4
        GETV    DV_K
        FCALL   mp_owner
        STR     R2
        GETV    MYSEAT
        XOR
        BNZ     bn9
        GETV    DV_K
        FCALL   mp_prod
        ANI     40H
        BZ      bn3
        LDI     ACT_UNSELL
        BR      bn3a
bn3:    LDI     ACT_SELL
bn3a:   PLO     RC
        GETV    DV_K
        PLO     R9
        LDI     0
        PHI     R9
        GLO     RC
        FCALL   act_send
        SETV    DV_OFFC, 0
bn9:    FRET
bn4:    GETV    DV_K                    ; a sample
        PUTV    DV_SAMP
        LDI     SND_SAMPLE
        FJMP    snd_play

; dv_wampus: when and where it shows: turn.wampus, by turn jiffies; it
; flashes 8 frames on, 8 off (mekkogx dv_wampus).
        RFIT    123
dv_wampus:
        SETV    DV_WP, 0FFH
        GETV    RXFRL                   ; w = elapsed + since the state
        STR     R2
        GETV    FCL
        SM
        PLO     R9
        GETV    RXFRH
        STR     R2
        GETV    FCH
        SMB
        PHI     R9
        GETV    DV_ELL
        STR     R2
        GLO     R9
        ADD
        PLO     R9
        GETV    DV_ELH
        STR     R2
        GHI     R9
        ADC
        PHI     R9
        SLD     TN_NWAMPUS
        SMI     MAX_WAMPUS+1
        BNF     dwp0
        LDI     MAX_WAMPUS
        BR      dwp0a
dwp0:   ADI     MAX_WAMPUS+1
dwp0a:  PLO     RC                      ; RC.0 = how many
        LDI     0
        PHI     RC                      ; RC.1 = k
        SPTR    TN_WAMPUS+WP_FROM
dwp1:   GLO     RC
        BZ      dwp4
        GLO     R9                      ; from <= w
        SEX     R8
        SM
        INC     R8
        GHI     R9
        SMB
        DEC     R8
        SEX     R2
        BNF     dwp3
        INC     R8                      ; w < to
        INC     R8
        GLO     R9
        SEX     R8
        SM
        INC     R8
        GHI     R9
        SMB
        DEC     R8
        DEC     R8
        DEC     R8
        SEX     R2
        BDF     dwp3
        GHI     RC
        PUTV    DV_WP
dwp3:   GLO     R8                      ; the next entry
        ADI     WP_SIZE
        PLO     R8
        GHI     R8
        ADCI    0
        PHI     R8
        GHI     RC
        ADI     1
        PHI     RC
        DEC     RC
        BR      dwp1
dwp4:   FJMP    dv_wampus2

        RFIT    144
dv_wampus2:
        GETV    DV_WP
        SMI     12
        BDF     dwp9
        GETV    DV_WP                   ; its plot and mountain
        PLO     RB
        SPTR    TN_WAMPUS
dwp5:   GLO     RB
        BZ      dwp6
        DEC     RB
        GLO     R8
        ADI     WP_SIZE
        PLO     R8
        GHI     R8
        ADCI    0
        PHI     R8
        BR      dwp5
dwp6:   LDA     R8                      ; plot
        PLO     RB
        LDN     R8                      ; mount: 0-2
        PLO     RC
        GLO     RB
        SHL
        ADI     LO(plotxy)
        PLO     R8
        LDI     HI(plotxy)
        ADCI    0
        PHI     R8
        GLO     RC                      ; x = plot x + 1 + mount
        STR     R2
        LDA     R8
        ADD
        ADI     1
        PUTV    DV_WPX
        GLO     RC                      ; y = plot y + 1 + 2 * mount
        SHL
        STR     R2
        LDN     R8
        ADD
        ADI     1
        PUTV    DV_WPY
        GETV    FCL
        ANI     8
        BZ      dwp9
        PARS
        P       SLOTWP
        P       IM_WAMPUS
        GETV    DV_WPX
        PD
        GETV    DV_WPY
        PD
        P       TM_XOR
        GOP     TOP_SPRITE
        FRET
dwp9:   PARS
        P       SLOTWP
        P       100
        P       0
        GOP     TOP_SPRMOVE
        FRET

; dv_eventmsg: the player event, at the turn's start: x * 25 * (round/4 + 1),
; or per M.U.L.E. for three of them (mekkogx dv_event_msg).
        RFIT    221
dv_eventmsg:
        GETV    DV_EVSH
        BNZ     dem9
        SETV    DV_EVSH, 1
        SLD     TN_EVENT
        BZ      dem9
        SMI     23
        BNF     dem0
dem9:   FRET
dem0:
        GETV    RND                     ; v = 25 * (round / 4 + 1)
        SHR
        SHR
        ADI     1
        PLO     RC
        LDI     0
        PLO     R9
        PHI     R9
dem1:   GLO     R9
        ADI     25
        PLO     R9
        DEC     RC
        GLO     RC
        BNZ     dem1
        SETV    V2L, 0
        SETV    V2H, 0
        SLD     TN_EVENT                ; j = pevent_x[k - 1]
        ADI     LO(pev_x)-1
        PLO     R8
        LDI     HI(pev_x)
        ADCI    0
        PHI     R8
        LDN     R8
        PUTV    DV_K
        SMI     100
        BDF     dem4
        GLO     R9                      ; v * j
        PUTV    T0
        LDI     0
        PHI     R9
        PLO     R9
        GETV    DV_K
        PLO     RC
dem2:   GLO     RC
        BZ      dem7
        DEC     RC
        GETV    T0
        STR     R2
        GLO     R9
        ADD
        PLO     R9
        GHI     R9
        ADCI    0
        PHI     R9
        BR      dem2
dem4:   FCALL   dv_evcount              ; per M.U.L.E.: n
        PLO     RC
        GLO     R9
        PUTV    T0                      ; T0 = v
        LDI     0
        PLO     R9
        PHI     R9
dem5:   GLO     RC                      ; R9 = v * n
        BZ      dem6
        DEC     RC
        GETV    T0
        STR     R2
        GLO     R9
        ADD
        PLO     R9
        GHI     R9
        ADCI    0
        PHI     R9
        BR      dem5
dem6:   GETV    DV_K
        XRI     102
        BZ      dem7                    ; energy: v * n
        GLO     R9                      ; else v2 = 2vn, v = 2v
        SHL
        PUTV    V2L
        GHI     R9
        SHLC
        PUTV    V2H
        GETV    T0
        SHL
        PLO     R9
        LDI     0
        SHLC
        PHI     R9
        GETV    DV_K
        XRI     101
        BNZ     dem7
        GETV    V2L                     ; mining: v = v2
        PLO     R9
        GETV    V2H
        PHI     R9
dem7:   GLO     R9
        PUTV    V1L
        GHI     R9
        PUTV    V1H
        FJMP    dv_evmsg2

        RFIT    46
dv_evmsg2:
        LDR     R8, TXT                 ; "*** " then the event
        LDI     '*'
        STR     R8
        INC     R8
        STR     R8
        INC     R8
        STR     R8
        INC     R8
        LDI     ' '
        STR     R8
        SETV    TXTLEN, 4
        SLD     TN_EVENT
        ADI     E_1-1
        FCALL   str_cat
        FJMP    msg_start
; the amount's multiplier of each event: 0 none, 100 per food M.U.L.E. (x2),
; 101 per mining M.U.L.E. (x2), 102 per energy M.U.L.E. (x1)
pev_x:  DB      0, 0, 2, 4, 100, 4, 8, 2, 3, 6, 4, 2, 0, 0, 3, 101, 102, 6, 4, 4, 4, 0

; dv_evcount: D = how many of our plots have the event's kind of M.U.L.E.
        RFIT    79
dv_evcount:
        LDI     0
        PUTV    T1
        PLO     RC
dec1:   GLO     RC
        FCALL   mp_owner
        STR     R2
        GETV    MYSEAT
        XOR
        BNZ     dec3
        GLO     RC
        FCALL   mp_mule
        PLO     RB
        GETV    DV_K
        XRI     100
        BNZ     dec2
        GLO     RB                      ; food
        XRI     GOOD_FOOD
        BZ      dec4
        BR      dec3
dec2:   XRI     100!101
        BNZ     dec2a
        GLO     RB                      ; smithore, crystite
        XRI     GOOD_SMITHORE
        BZ      dec4
        XRI     GOOD_SMITHORE!GOOD_CRYSTITE
        BZ      dec4
        BR      dec3
dec2a:  GLO     RB                      ; energy
        XRI     GOOD_ENERGY
        BNZ     dec3
dec4:   GETV    T1
        ADI     1
        STR     RA
dec3:   INC     RC
        GLO     RC
        XRI     NUM_PLOTS
        BNZ     dec1
        GETV    T1
        FRET

; ---- a computer's turn (mekkogx dv_ai_walk): the server plays it at once,
; so the map shows what it did; its figure walks out of the store to the plot
; that gained a M.U.L.E. (or one of its plots) with the M.U.L.E. behind, which
; stays there, then back into the store to wait ------------------------------------
AISCR   EQU     CRAM+100H               ; the two scripts, made here
AISTX   EQU     29                      ; in the store
AISTY   EQU     MAPY0+35

        RFIT    143
dv_aiwalk:
        FCALL   fx_stop_all
        SETV    DV_K, 0FFH              ; the plot: one that gained a M.U.L.E.,
        SETV    T1, 0                   ;  else its first
        LDI     0
        PLO     RC
daw1:   GLO     RC
        XRI     STORE_PLOT
        BZ      daw3
        GLO     RC
        FCALL   mp_owner
        STR     R2
        GETV    DV_SEAT
        XOR
        BNZ     daw3
        GETV    DV_K
        XRI     0FFH
        BNZ     daw2
        GLO     RC
        PUTV    DV_K
daw2:   GLO     RC
        FCALL   mp_mule
        SMI     4
        BDF     daw3
        GLO     RC                      ; changed since the state before?
        ADI     LO(MP_MULE)
        PLO     R8
        LDI     HI(MP_MULE)
        ADCI    0
        PHI     R8
        LDN     R8
        STR     R2
        GHI     R8
        ADI     HI(PRVOFF)
        PHI     R8
        LDN     R8
        XOR
        BZ      daw3
        GLO     RC
        PUTV    DV_K
        SETV    T1, 1
daw3:   INC     RC
        GLO     RC
        XRI     NUM_PLOTS
        BNZ     daw1
        GETV    DV_K
        SMI     NUM_PLOTS
        BNF     daw4
        SETV    DV_K, 13                ; no land: out and back
daw4:   GETV    DV_AI                   ; the last one's land, unmarked
        XRI     2
        BNZ     daw5
        SETV    MAPQ, 0
daw5:   GETV    DV_K
        PUTV    DV_AIP
        SETV    DV_AI, 1
        FJMP    dv_aiscripts

; dv_aiscripts: the figure's script and (T1) its M.U.L.E.'s, poked into the
; cart's RAM, then started.
        RFIT    198
dv_aiscripts:
        GETV    DV_K                    ; the target: the plot's middle
        SHL
        ADI     LO(plotxy)
        PLO     R8
        LDI     HI(plotxy)
        ADCI    0
        PHI     R8
        LDA     R8
        ADI     1
        PUTV    T2                      ; T2 = x
        LDN     R8
        ADI     4
        PUTV    T3                      ; T3 = y
        LDI     0
        PUTV    T4                      ; T4: 0 the figure, 1 the M.U.L.E.
das1:   GETV    T4                      ; the script's place
        BZ      das2
        LDR     R9, AISCR+40H
        BR      das3
das2:   LDR     R9, AISCR
das3:   FCALL   cr_at
        GETV    T4
        BZ      das4
        LDI     FX_WAIT                 ; the M.U.L.E. a little behind
        FCALL   cr_put
        LDI     24
        FCALL   cr_put
das4:   FCALL   das_out                 ; across, then down or up
        GETV    T4
        BZ      das5
        LDI     FX_WAIT                 ; installed: it goes no further
        FCALL   cr_put
        LDI     20
        FCALL   cr_put
        LDI     FX_HIDE
        FCALL   cr_put
        LDI     FX_END
        FCALL   cr_put
        BR      das6
das5:   LDI     FX_SIG                  ; 90 frames on the plot (it blinks),
        FCALL   cr_put                  ; then back, and a long wait
        LDI     3
        FCALL   cr_put
        LDI     FX_WAIT
        FCALL   cr_put
        LDI     90
        FCALL   cr_put
        LDI     FX_SIG
        FCALL   cr_put
        LDI     4
        FCALL   cr_put
        FCALL   das_back
        LDI     FX_WAIT
        FCALL   cr_put
        LDI     255
        FCALL   cr_put
        LDI     FX_LOOP
        FCALL   cr_put
        LDI     200
        FCALL   cr_put
        LDI     2
        FCALL   cr_put
das6:   LDI     FX_REG                  ; (closes the poke stream)
        HOT     HI(FN_REGSEL)
        FCALL   das_actor
        GETV    T4
        ADI     1
        STR     RA
        XRI     1
        BNZ     das9
        GETV    T1                      ; a M.U.L.E. only if it gained one
        BNZ     das1
das9:   FRET
FX_REG  EQU     FN_REG_PTRLO

; das_out / das_back: the moves, 1 a step every 6 frames (das_move: RB.0
; dx, RB.1 dy, RC.0 steps).
        RFIT    85
das_out:
        FCALL   das_x                   ; across
        FCALL   das_move
        FCALL   das_y                   ; then down or up
        FJMP    das_move
das_back:
        FCALL   das_y                   ; up or down
        FCALL   das_rev
        FCALL   das_move
        FCALL   das_x                   ; then across, home
        FCALL   das_rev
        FJMP    das_move
; das_x: RB = (+-1, 0), RC = |x - store|. das_y: RB = (0, +-1), RC = |y - store|.
das_x:  LDI     0
        PHI     RB
        GETV    T2
        SMI     AISTX
        BDF     dx1
        SDI     0
        PLO     RC
        LDI     0FFH
        PLO     RB
        FRET
dx1:    PLO     RC
        LDI     1
        PLO     RB
        FRET
das_y:  LDI     0
        PLO     RB
        GETV    T3
        SMI     AISTY
        BDF     dy1
        SDI     0
        PLO     RC
        LDI     0FFH
        PHI     RB
        FRET
dy1:    PLO     RC
        LDI     1
        PHI     RB
        FRET
; das_rev: the step the other way.
das_rev:
        GLO     RB
        SDI     0
        PLO     RB
        GHI     RB
        SDI     0
        PHI     RB
        FRET

        RFIT    26
das_move:
        GLO     RC
        BZ      dsm9
        LDI     FX_MOVE
        FCALL   cr_put
        GLO     RB
        FCALL   cr_put
        GHI     RB
        FCALL   cr_put
        LDI     6
        FCALL   cr_put
        GLO     RC
        FJMP    cr_put
dsm9:   FRET

; das_actor: actor T4 set to look as it should, its script started.
        RFIT    150
das_actor:
        GETV    T4
        PLO     RC
        LDI     FK_SINGLE
        FPUT    FXKIND
        GLO     RC
        FPUT    FXSLOT
        LDI     AISTX
        FPUT    FXX
        LDI     AISTY
        FPUT    FXY
        LDI     0
        FPUT    FXFRAME
        GETV    T4
        BZ      dsa1
        LDI     IM_SMULEA               ; the M.U.L.E.
        FPUT    FXIMG
        LDI     IM_SMULEB-IM_SMULEA
        FPUT    FXD2
        BR      dsa2
dsa1:   LDR     R9, S_PLAYERS+PL_SPECIES        ; the figure: small, its species
        GETV    DV_SEAT
        FCALL   pl_byte
        ANI     7
        ADI     IM_MWA_MECHTRON
        PLO     RB
        GETV    T4
        PLO     RC
        GLO     RB
        FPUT    FXIMG
        LDI     IM_MWB_MECHTRON-IM_MWA_MECHTRON
        FPUT    FXD2
dsa2:   GETV    T4
        BZ      dsa3
        LDR     R9, AISCR+40H
        BR      dsa4
dsa3:   LDR     R9, AISCR
dsa4:   GETV    T4
        FCALL   fx_start
        GETV    T4
        PLO     RC
        FJMP    fx_draw

; dv_aimarks: a computer's land outlined for 40 frames as its turn starts,
; and the plot it works while it is there (mekkogx dv_ai_marks).
        RFIT    168
dv_aimarks:
        GETV    DV_AI
        XRI     1
        BNZ     dam1
        LDI     1
        FCALL   dv_ailand
        SETV    DV_AI, 2
        GETV    FCL
        PUTV    DV_AIT
        BR      dam2
dam1:   GETV    DV_AI
        XRI     2
        BNZ     dam2
        GETV    DV_AIT
        STR     R2
        GETV    FCL
        SM
        SMI     40
        BNF     dam2
        LDI     0
        FCALL   dv_ailand
        SETV    DV_AI, 0
dam2:   GETV    FXFLAG                  ; the worked plot: blinking, or not
        XRI     3
        BZ      dam3
        XRI     3!4
        BNZ     dam9
        SETV    MAPBLON, 0
        BR      dam4
dam3:   GETV    DV_AIP
        PUTV    MAPBLINK
        SETV    MAPBLON, 1
dam4:   GETV    DV_AIP
        SMI     NUM_PLOTS
        BDF     dam5
        HOLD1
        GETV    DV_AIP
        FCALL   map_draw_plot
        HOLD0
dam5:   SETV    FXFLAG, 0
        SETV    MAPBLINK, NOBODY
        SETV    MAPBLON, 0
dam9:   FRET

; dv_ailand: the computer's land outlined (D = 1) or as it is (0).
        RFIT    109
dv_ailand:
        PUTV    MAPBLON
        LDI     0
        PUTV    DV_K
dal1:   GETV    DV_K
        FCALL   mp_owner
        STR     R2
        GETV    DV_SEAT
        XOR
        BNZ     dal2
        GETV    DV_K
        PUTV    MAPBLINK
        HOLD1
        GETV    DV_K
        FCALL   map_draw_plot
        HOLD0
dal2:   GETV    DV_K
        ADI     1
        STR     RA
        XRI     NUM_PLOTS
        BNZ     dal1
        SETV    MAPBLINK, NOBODY
        SETV    MAPBLON, 0
        FRET
