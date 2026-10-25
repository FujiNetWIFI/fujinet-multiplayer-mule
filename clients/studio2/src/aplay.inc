; aplay.inc -- bank 6: AUTOPLAY=1 builds play by themselves, for soak tests
; and filmed games against a real server (mekkogx autoplay.c and its ap_*
; parts). Only what a player would do, through the keypad a frame at a time
; (KEY held, KEDGE pressed), so the screen shows a real game: ready, claim
; land, walk a turn (corral, outfitter, out to our plot, install, the pub),
; bid, declare and trade.

AP_PH   EQU     PH+63                   ; the phase seen (above every phase's own)
AP_T0L  EQU     PH+64                   ; the frame it (or a part) began
AP_T0H  EQU     PH+65
AP_PF   EQU     PH+66                   ; the frame (low) of the last press
AP_ST   EQU     PH+50                   ; our turn's part (APS_*): above the
AP_GOOD EQU     PH+51                   ;  development's own
AP_IN   EQU     PH+52                   ; been through a door
AP_TX   EQU     PH+53                   ; our plot, where we stand on it
AP_TY   EQU     PH+54
AP_SIDE EQU     PH+55                   ; the key out of town
AP_T    EQU     PH+56                   ; the turn's time left, in 256 jiffies
AP_KEY  EQU     T2                      ; this frame's keys: held, pressed
AP_EDGE EQU     T3
AP_HAVE EQU     T4                      ; an auction's good: ours, our need
AP_NEED EQU     T5
        IF      (AV_W >= AP_PH) || (DV_K >= AP_ST) || (AP_PF >= PHEND)
        ERROR   "autoplay's variables meet a phase's"
        ENDIF
APS_CORRAL EQU  0
APS_SHOP EQU    1
APS_OUT EQU     2
APS_PLOT EQU    3
APS_HOME EQU    4
APS_PUB EQU     5
APS_END EQU     6
APS_DONE EQU    7
APS_WAIT EQU    8
APS_PROMPT EQU  9
APS_OFF EQU     10

; autoplay: once a frame, before the phase's frame sees the keys.
        RFIT    155
autoplay:
        GETV    MYSEAT
        SMI     4
        BDF     apl9
        GETV    AP_PH                   ; a new phase: its clock
        STR     R2
        GETV    PHASE
        XOR
        BZ      apl1
        GETV    PHASE
        PUTV    AP_PH
        SETV    AP_ST, APS_OFF
        FCALL   ap_t0
apl1:   SETV    AP_KEY, K_NONE
        SETV    AP_EDGE, K_NONE
        GETV    PHASE
        BZ      apl2
        XRI     PHASE_SUMMARY
        BNZ     apl3
apl2:   FCALL   ap_status
        BR      apl8
apl3:   XRI     PHASE_SUMMARY!PHASE_GRANT
        BNZ     apl4
        FCALL   ap_grant
        BR      apl8
apl4:   XRI     PHASE_GRANT!PHASE_DEVELOP
        BNZ     apl5
        FCALL   ap_develop
        BR      apl8
apl5:   XRI     PHASE_DEVELOP!PHASE_LANDAUCTION
        BZ      apl6
        XRI     PHASE_LANDAUCTION!PHASE_DECLARE
        BZ      apl6
        XRI     PHASE_DECLARE!PHASE_TRADING
        BNZ     apl8
apl6:   FCALL   ap_auction
apl8:   GETV    AP_KEY                  ; the keypad, as if pressed
        PUTV    KEY
        GETV    AP_EDGE
        PUTV    KEDGE
        XRI     K_NONE
        BZ      apl9
        GETV    AP_EDGE
        PUTV    KEY
apl9:   FRET

; ap_t0: a part begins now. ap_since: R9 = the frames since.
ap_t0:  GETV    FCL
        PUTV    AP_T0L
        GETV    FCH
        PUTV    AP_T0H
        FRET
        RFIT    23
ap_since:
        GETV    AP_T0L
        STR     R2
        GETV    FCL
        SM
        PLO     R9
        GETV    AP_T0H
        STR     R2
        GETV    FCH
        SMB
        PHI     R9
        FRET

; ap_every: D = 1 at most every D frames (a press the server may not have
; seen yet isn't repeated at once).
        RFIT    38
ap_every:
        PLO     RB
        GETV    AP_PF
        STR     R2
        GETV    FCL
        SM
        STR     R2
        GLO     RB
        SD
        LDI     0
        BNF     ape9
        GETV    FCL
        PUTV    AP_PF
        LDI     1
ape9:   FRET

; ap_btn: 5 pressed. ap_sincegt: DF = more than D frames since the part began.
ap_btn: SETV    AP_EDGE, K_FIRE
        FRET
        RFIT    19
ap_sincegt:
        PLO     RB
        FCALL   ap_since
        GHI     R9
        BNZ     asg1
        GLO     RB
        STR     R2
        GLO     R9
        SM
        BNZ     asg9                    ; not equal: DF says it
        SHR                             ; equal: DF = 0
asg9:   FRET
asg1:   LDI     1
        SHR
        FRET

; ---- the lobby and the summary (mekkogx ap_status): ready once all four seats
; are people; the summary seen a while first ------------------------------------
        RFIT    114
ap_status:
        LDR     R9, S_PLAYERS+PL_FLAGS
        GETV    MYSEAT
        FCALL   pl_byte
        ANI     PF_READY
        BNZ     aps9
        GETV    PHASE
        BNZ     aps5
        LDI     0                       ; the lobby: people in every seat
        PUTV    AP_IN
        PLO     RC
aps1:   LDR     R9, S_PLAYERS+PL_NAME
        GLO     RC
        FCALL   pl_byte
        BZ      aps2
        LDR     R9, S_PLAYERS+PL_FLAGS
        GLO     RC
        FCALL   pl_byte
        ANI     PF_AI
        BNZ     aps2
        GETV    AP_IN
        ADI     1
        PUTV    AP_IN
aps2:   INC     RC
        GLO     RC
        XRI     4
        BNZ     aps1
        GETV    AP_IN
        XRI     4
        BNZ     aps9
        BR      aps6
aps5:   GETV    SU_DRAWN
        BZ      aps9
        FCALL   ap_since                ; past 560 frames
        GLO     R9
        SMI     LO(561)
        GHI     R9
        SMBI    HI(561)
        BNF     aps9
aps6:   LDI     45
        FCALL   ap_every
        BZ      aps9
        FJMP    ap_btn
aps9:   FRET

; ---- the grant (mekkogx ap_grant): river for seat 0, mountains for 1, plains
; for 2 and 3; from the third row any plains, from the fifth anything ---------
        RFIT    97
ap_grant:
        GETV    GN_CLMD
        BNZ     apg9
        GETV    GN_DEP
        BNZ     apg9
        GETV    GN_SHOW
        BZ      apg9
        GETV    GN_CUR
        SMI     NUM_PLOTS
        BDF     apg9
        GETV    GN_STEPF                ; never just after a step
        STR     R2
        GETV    FCL
        SM
        SMI     3
        BNF     apg9
        GETV    GN_CUR
        FCALL   mp_terrain
        ANI     3
        PLO     RC
        GETV    MYSEAT
        ADI     LO(ap_land)
        PLO     R8
        LDI     HI(ap_land)
        ADCI    0
        PHI     R8
        LDN     R8
        STR     R2
        GLO     RC
        XOR
        BZ      apg8
        GETV    GN_CUR
        SMI     36
        BDF     apg8
        GETV    GN_CUR
        SMI     18
        BNF     apg9
        GLO     RC
        XRI     TERRAIN_PLAIN
        BNZ     apg9
apg8:   FJMP    ap_btn
apg9:   FRET
ap_land: DB     TERRAIN_RIVER, TERRAIN_MOUNTAIN, TERRAIN_PLAIN, TERRAIN_PLAIN

; ---- the auctions (mekkogx ap_auction) ---------------------------------------------
; land: bid a little over the opening (fewer than six plots, the money for
; it), each seat to its own limit. Goods: a seller with more than next month
; needs, a buyer with less; a seller walks down until its surplus is sold, a
; buyer up to the store's price until it has enough.
        RFIT    56
ap_auction:
        GETV    AV_KIND
        XRI     1
        BNZ     apa5
        GETV    AV_STG                  ; land: the plot shown: seen
        BNZ     apa1
        LDI     90
        FCALL   ap_sincegt
        BNF     apa9
        LDI     45
        FCALL   ap_every
        BZ      apa9
        FJMP    ap_btn
apa1:   SMI     STAGE_LEAD
        BNF     apa9
        FCALL   ap_myrole
        XRI     ROLE_BUYER
        BNZ     apa9
        FCALL   ap_nplots
        SMI     6
        BDF     apa9
        FJMP    ap_landbid
apa5:   FJMP    ap_goods
apa9:   FRET

; ap_myrole: D = our role. ap_nplots: D = the plots we own.
        RFIT    36
ap_myrole:
        GETV    MYSEAT
        ADI     LO(AV_ROLE)
        PLO     RA
        LDN     RA
        FRET
ap_nplots:
        LDI     0
        PLO     RC
        PHI     RC
apn1:   GHI     RC
        FCALL   mp_owner
        STR     R2
        GETV    MYSEAT
        XOR
        BNZ     apn2
        INC     RC
apn2:   GHI     RC
        ADI     1
        PHI     RC
        XRI     NUM_PLOTS
        BNZ     apn1
        GLO     RC
        FRET

; ap_landbid: up while money > start + 300 and our line < start + (seat + 2)
; * 4 steps.
        RFIT    101
ap_landbid:
        LDR     R9, S_PLAYERS+PL_MONEY
        GETV    MYSEAT
        FCALL   pl_byte
        PLO     RC
        INC     R8
        LDN     R8
        PHI     RC                      ; RC = money
        SPTR    A_START                 ; RB = start + 300
        LDA     R8
        ADI     LO(300)
        PLO     RB
        LDN     R8
        ADCI    HI(300)
        PHI     RB
        GLO     RB
        STR     R2
        GLO     RC
        SM
        GHI     RB
        STR     R2
        GHI     RC
        SMB
        BNF     apb9                    ; money < it
        GETV    MYSEAT                  ; RB = start + (seat + 2) * 4 steps
        ADI     2
        SHL
        SHL
        PLO     RC
        SPTR    A_START
        LDA     R8
        PLO     RB
        LDN     R8
        PHI     RB
apb1:   GETV    AV_STEP
        STR     R2
        GLO     RB
        ADD
        PLO     RB
        GHI     RB
        ADCI    0
        PHI     RB
        DEC     RC
        GLO     RC
        BNZ     apb1
        GETV    MYSEAT                  ; our line under it: up
        PLO     RC
        FCALL   au_pos
        GLO     RB
        STR     R2
        GLO     R9
        SM
        GHI     RB
        STR     R2
        GHI     R9
        SMB
        BDF     apb9
        SETV    AP_KEY, 2
apb9:   FRET

; ap_goods: have and next month's need; declare, then walk the line.
        RFIT    135
ap_goods:
        LDR     R9, S_PLAYERS+PL_GOODS
        GETV    MYSEAT
        FCALL   pl_byte
        GETV    AV_GOOD
        STR     R2
        GLO     R8
        ADD
        PLO     R8
        GHI     R8
        ADCI    0
        PHI     R8
        LDN     R8
        PUTV    AP_HAVE
        GETV    RND
        ADI     1
        PLO     RB
        GETV    MYSEAT
        FCALL   au_need
        PUTV    AP_NEED
        GETV    AV_STG
        XRI     STAGE_DECLARE
        BZ      apgd0
        FJMP    ap_line
apgd0:  LDI     40                      ; declaring: a moment first
        FCALL   ap_sincegt
        BNF     apgd9
        LDI     ROLE_NONE               ; what we want to be
        PLO     RC
        GETV    AP_NEED
        STR     R2
        GETV    AP_HAVE
        SM
        BZ      apgd2
        LDI     ROLE_SELLER
        BDF     apgd1
        LDI     ROLE_BUYER
apgd1:  PLO     RC
apgd2:  FCALL   ap_myrole
        STR     R2
        GLO     RC
        XOR
        BZ      apgd9
        GLO     RC
        XRI     ROLE_SELLER
        BNZ     apgd3
        SETV    AP_KEY, 2
        FRET
apgd3:  GLO     RC
        XRI     ROLE_BUYER
        BNZ     apgd4
        SETV    AP_KEY, 8
        FRET
apgd4:  LDI     45
        FCALL   ap_every
        BZ      apgd9
        FJMP    ap_btn
apgd9:  FRET

; ap_line: leading or going: a seller with more than it needs down, a buyer
; short up to the store's price.
        RFIT    89
ap_line:
        GETV    AV_STG
        XRI     STAGE_LEAD
        BZ      apln6
        XRI     STAGE_LEAD!STAGE_ACTIVE
        BNZ     apln9
apln6:  FCALL   ap_myrole
        XRI     ROLE_SELLER
        BNZ     apln7
        GETV    AP_NEED                   ; a seller with more than it needs: down
        STR     R2
        GETV    AP_HAVE
        SM
        BZ      apln9
        BNF     apln9
        SETV    AP_KEY, 8
        FRET
apln7:  XRI     ROLE_SELLER!ROLE_BUYER
        BNZ     apln9
        GETV    AP_HAVE                   ; a buyer short: up to the store's price
        STR     R2
        GETV    AP_NEED
        SM
        BZ      apln9
        BNF     apln9
        GETV    MYSEAT
        PLO     RC
        FCALL   au_pos
        SPTR    A_STORESELL
        GLO     R9
        SEX     R8
        SM
        INC     R8
        GHI     R9
        SEX     R8
        SMB
        SEX     R2
        BDF     apln9
        SETV    AP_KEY, 2
apln9:  FRET

; ---- our turn (mekkogx ap_develop) ---------------------------------------------
; Buy a M.U.L.E. at the corral, outfit it for our first empty plot (river:
; food, mountains: smithore, plains: food on the odd seats, energy on the
; even), leave town on the plot's side, down or up to its row and along to
; it, install, then back into town to the pub if there is time, else end the
; turn. Short of time, money, M.U.L.E.s or land: straight to the pub.
        RFIT    151
ap_develop:
        GETV    DVSTATE
        XRI     DV_PROMPT
        BNZ     apd2
        GETV    AP_ST                   ; "press 5 to start": a moment, then 5
        XRI     APS_PROMPT
        BZ      apd1
        SETV    AP_ST, APS_PROMPT
        FJMP    ap_t0
apd1:   LDI     60
        FCALL   ap_sincegt
        BNF     apd9
        LDI     45
        FCALL   ap_every
        BZ      apd9
        FJMP    ap_btn
apd2:   GETV    DVSTATE
        XRI     DV_WALK
        BZ      apd3
        SETV    AP_ST, APS_OFF
        FRET
apd3:   GETV    DV_BWAIT                ; in a doorway: the server has it
        BZ      apd4
        SETV    AP_IN, 1
        FRET
apd4:   GETV    WK_HOLD
        BNZ     apd9
        GETV    WK_IN
        BNZ     apd9
        GETV    WK_EVENT
        BNZ     apd9
        FCALL   ap_left
        GETV    AP_ST
        SMI     APS_WAIT
        BNF     apd5
        FCALL   ap_plan
        BNZ     apd9
apd5:   GETV    AP_ST
        SHL
        ADI     LO(ap_parts)
        PLO     R8
        LDI     HI(ap_parts)
        ADCI    0
        PHI     R8
        LDA     R8
        PHI     R9
        LDN     R8
        PLO     R9
        LDR     R8, jmp_r9
        SEP     R8
apd9:   FRET
ap_parts:
        DW      ap_corral, ap_shop, ap_out, ap_plot, ap_home, ap_pub, ap_end
        DW      ap_none, ap_none, ap_none, ap_none

; ap_left: AP_T = the turn's time left in 256-jiffy units (the bar's units
; left times the jiffies a unit).
        RFIT    35
ap_left:
        LDI     0
        PLO     R9
        PHI     R9
        GETV    DV_PX
        PLO     RC
apl_1:  GLO     RC
        BZ      apl_2
        GETV    DV_PTU
        STR     R2
        GLO     R9
        ADD
        PLO     R9
        GHI     R9
        ADCI    0
        PHI     R9
        DEC     RC
        BR      apl_1
apl_2:  GHI     R9
        PUTV    AP_T
        FRET

; ap_plan: the turn begins: a moment standing in town, then our first empty
; plot and what it grows. D = 1 while standing.
        RFIT    79
ap_plan:
        GETV    AP_ST
        XRI     APS_WAIT
        BZ      apn_1
        SETV    AP_ST, APS_WAIT
        FCALL   ap_t0
apn_1:  LDI     40
        FCALL   ap_sincegt
        BDF     apn_2
        LDI     1
        FRET
apn_2:  SETV    AP_ST, APS_PUB
        SETV    AP_IN, 0
        LDI     0                       ; our first plot with no M.U.L.E.
        PLO     RC
apn_3:  GLO     RC
        FCALL   mp_owner
        STR     R2
        GETV    MYSEAT
        XOR
        BNZ     apn_4
        GLO     RC
        XRI     STORE_PLOT
        BZ      apn_4
        GLO     RC
        FCALL   mp_mule
        SMI     4
        BNF     apn_4
        FJMP    ap_plan2
apn_4:  INC     RC
        GLO     RC
        XRI     NUM_PLOTS
        BNZ     apn_3
        LDI     0
        FRET

; ap_plan2: plot RC.0 is it: the time, a M.U.L.E. to buy, the good, the money,
; where it is.
        RFIT    129
ap_plan2:
        GETV    AP_T
        SMI     4
        BNF     apn_9
        SLD     SO_MULES
        BZ      apn_9
        GLO     RC                      ; the good: river food, mountains
        FCALL   mp_terrain              ; smithore, plains food or energy
        ANI     3
        XRI     TERRAIN_RIVER
        BZ      apn_6
        XRI     TERRAIN_RIVER!TERRAIN_MOUNTAIN
        BNZ     apn_7
        LDI     GOOD_SMITHORE
        BR      apn_8
apn_6:  LDI     GOOD_FOOD
        BR      apn_8
apn_7:  GETV    MYSEAT
        ANI     1
        BNZ     apn_6
        LDI     GOOD_ENERGY
apn_8:  PUTV    AP_GOOD
        FCALL   ap_afford
        BNF     apn_9
        GLO     RC                      ; where: column, row
        PLO     RB
        LDI     0
        PHI     RB
apn_a:  GLO     RB
        SMI     9
        BNF     apn_b
        PLO     RB
        GHI     RB
        ADI     1
        PHI     RB
        BR      apn_a
apn_b:  GLO     RB                      ; x = 7c + 1, the side c <= 4: left
        SHL
        SHL
        SHL
        STR     R2
        GLO     RB
        SD
        ADI     1
        PUTV    AP_TX
        GLO     RB
        SMI     5
        LDI     4
        BNF     apn_c
        LDI     6
apn_c:  PUTV    AP_SIDE
        GHI     RB                      ; y = 13 + 16r + 2
        SHL
        SHL
        SHL
        SHL
        ADI     MAPY0+2
        PUTV    AP_TY
        SETV    AP_ST, APS_CORRAL
apn_9:  LDI     0
        FRET

; ap_afford: DF = money >= the M.U.L.E.'s price + 25 * (good + 1).
        RFIT    56
ap_afford:
        LDR     R9, S_PLAYERS+PL_MONEY
        GETV    MYSEAT
        FCALL   pl_byte
        PLO     R9
        INC     R8
        LDN     R8
        PHI     R9                      ; R9 = money
        SPTR    SO_MULEPRICE            ; less the price
        GLO     R9
        SEX     R8
        SM
        PLO     R9
        INC     R8
        GHI     R9
        SMB
        PHI     R9
        SEX     R2
        BNF     apf9
        GETV    AP_GOOD                 ; less 25 a good's number + 1
        ADI     1
        PLO     RB
apf1:   GLO     R9
        SMI     25
        PLO     R9
        GHI     R9
        SMBI    0
        PHI     R9
        BNF     apf9
        DEC     RB
        GLO     RB
        BNZ     apf1
apf9:   FRET

; ap_door: the key to door x D along the street, then in (RB.0: 2 up, 8 down).
        RFIT    29
ap_door:
        STR     R2
        GETV    WK_X
        SM
        BZ      apdr2
        LDI     6
        BNF     apdr1                   ; left of it: right
        LDI     4
apdr1:  PUTV    AP_KEY
        FRET
apdr2:  GLO     RB
        PUTV    AP_KEY
        FRET

; the turn's parts
        RFIT    117
ap_corral:
        GETV    WK_MULE                 ; ours: to the outfitter
        BZ      apc1
        SETV    AP_ST, APS_SHOP
        SETV    AP_IN, 0
        FRET
apc1:   LDI     8
        PLO     RB
        LDI     53
        FCALL   ap_door
        GETV    AP_IN                   ; been in: none for us
        BZ      apc9
        SETV    AP_ST, APS_PUB
apc9:   FRET
ap_shop:
        SLD     TN_CARRYING
        STR     R2
        GETV    AP_GOOD
        XOR
        BNZ     aps_1
        SETV    AP_ST, APS_OUT          ; outfitted
        FRET
aps_1:  GETV    AP_IN
        BZ      aps_2
        SETV    AP_ST, APS_PUB
        FRET
aps_2:  LDI     2
        PLO     RB
        GETV    AP_GOOD
        ADI     LO(doorup)
        PLO     R8
        LDI     HI(doorup)
        ADCI    0
        PHI     R8
        LDN     R8
        FJMP    ap_door
ap_out: GETV    WK_TOWN                 ; out of town on the plot's side
        BNZ     apo1
        SETV    AP_ST, APS_PLOT
apo1:   GETV    AP_SIDE
        PUTV    AP_KEY
        FRET

        RFIT    105
ap_plot:
        GETV    WK_MULE
        BNZ     app1
        GETV    AP_T                    ; installed: home if there is time
        SMI     3
        LDI     APS_HOME
        BDF     app0
        LDI     APS_END
app0:   PUTV    AP_ST
        FRET
app1:   GETV    AP_TY                   ; to its row
        STR     R2
        GETV    WK_Y
        SM
        BZ      app3
        LDI     8
        BNF     app2
        LDI     2
app2:   PUTV    AP_KEY
        FRET
app3:   GETV    AP_TX                   ; along it
        STR     R2
        GETV    WK_X
        ADI     1
        SM
        BDF     app4
        SETV    AP_KEY, 6
        FRET
app4:   GETV    AP_TX
        ADI     1
        STR     R2
        GETV    WK_X
        SM
        BZ      app5
        BNF     app5
        SETV    AP_KEY, 4
        FRET
app5:   LDI     45                      ; there: install
        FCALL   ap_every
        BZ      app9
        FJMP    ap_btn
app9:   FRET

        RFIT    92
ap_home:
        GETV    WK_TOWN
        BZ      aph1
        SETV    AP_ST, APS_PUB
        FRET
aph1:   LDI     MAPY0+34                ; the store's row, then in from the side
        STR     R2
        GETV    WK_Y
        SM
        BZ      aph3
        LDI     8
        BNF     aph2
        LDI     2
aph2:   PUTV    AP_KEY
        FRET
aph3:   GETV    WK_X
        SMI     29
        LDI     6
        BNF     aph4
        LDI     4
aph4:   PUTV    AP_KEY
        FRET
ap_pub: GETV    WK_TOWN
        BNZ     apu1
        SETV    AP_ST, APS_HOME         ; out by mistake: back
        FRET
apu1:   LDI     8
        PLO     RB
        LDI     37
        FJMP    ap_door
ap_end: SETV    AP_ST, APS_DONE         ; no time to go home: the turn ends
        LDI     0
        PLO     R9
        PHI     R9
        LDI     ACT_END
        FJMP    act_send
ap_none:
        FRET
