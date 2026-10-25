; auction2.inc -- the auctions' second half (mekkogx auction.c): the per-good
; status, the screen's setup, a state, the stick.

; ---- the per-good status (stage 0, 561 jiffies): previous amount, usage,
; spoilage, production, then the store's stock; each player's bar beside them -----
; au_need: D = the units of the good seat D needs in round RB.0: food by the
; round (none after 12), energy one for each of its other M.U.L.E.s.
        RFIT    92
au_need:
        PLO     RC
        GETV    AV_GOOD
        BNZ     an4
        GLO     RB                      ; food
        SMI     13
        BDF     an9
        GLO     RB
        SMI     8
        BDF     an2
        GLO     RB
        SMI     4
        BDF     an1
        LDI     3
        FRET
an1:    LDI     4
        FRET
an2:    LDI     5
        FRET
an4:    XRI     GOOD_ENERGY
        BNZ     an9
        LDI     0                       ; energy: its other M.U.L.E.s
        PUTV    AV_W
        LDI     0
        PHI     RC
an5:    GHI     RC
        FCALL   mp_owner
        STR     R2
        GLO     RC
        XOR
        BNZ     an6
        GHI     RC
        FCALL   mp_mule
        SMI     4
        BDF     an6
        ADI     4
        XRI     GOOD_ENERGY
        BZ      an6
        GETV    AV_W
        ADI     1
        STR     RA
an6:    GHI     RC
        ADI     1
        PHI     RC
        XRI     NUM_PLOTS
        BNZ     an5
        GETV    AV_W
        FRET
an9:    LDI     0
        FRET

; au_bartgt: each bar's height for the step shown (mekkogx au_status_targets):
; previous = now - production + spoilage + usage, then less usage, then the
; production in, then now; 2 lines a unit.
        RFIT    219
au_bartgt:
        LDI     0
        PUTV    AV_I
abt1:   LDR     R9, S_PLAYERS+PL_GOODS  ; w, c (production), j (spoilage)
        GETV    AV_I
        FCALL   pl_byte
        FCALL   abt_good
        PUTV    T0                      ; w
        LDR     R9, S_PLAYERS+PL_PROD
        GETV    AV_I
        FCALL   pl_byte
        FCALL   abt_good
        PUTV    T1                      ; c
        LDR     R9, S_PLAYERS+PL_SPOIL
        GETV    AV_I
        FCALL   pl_byte
        FCALL   abt_good
        PUTV    T2                      ; j
        GETV    RND
        PLO     RB
        GETV    AV_I
        FCALL   au_need
        PUTV    T3                      ; n
        GETV    T0                      ; a = w + j + n - c (>= 0)
        STR     R2
        GETV    T2
        ADD
        STR     R2
        GETV    T3
        ADD
        STR     R2
        GETV    T1
        SD
        BDF     abt2
        LDI     0
abt2:   PLO     RC
        GETV    AV_SHOWT
        XRI     1
        BNZ     abt3
        GETV    T3                      ; then less usage
        STR     R2
        GLO     RC
        SM
        BDF     abt2a
        LDI     0
abt2a:  PLO     RC
abt3:   GETV    AV_SHOWT
        XRI     2
        BNZ     abt4
        GETV    T1                      ; w - c
        STR     R2
        GETV    T0
        SM
        BDF     abt3a
        LDI     0
abt3a:  PLO     RC
abt4:   GETV    AV_SHOWT
        SMI     3
        BNF     abt5
        GETV    T0
        PLO     RC
abt5:   GLO     RC                      ; 2 lines a unit, AUBARM at most
        SMI     AUBARM/2+1
        BNF     abt6
        LDI     AUBARM/2
        PLO     RC
abt6:   GETV    AV_I
        ADI     LO(AV_BART)
        PLO     RA
        GLO     RC
        SHL
        STR     RA
        GETV    AV_I
        ADI     1
        STR     RA
        XRI     4
        BNZ     abt1
        FRET
; abt_good: D = the byte for our good, R8 on the good array's start.
abt_good:
        GETV    AV_GOOD
        STR     R2
        GLO     R8
        ADD
        PLO     R8
        GHI     R8
        ADCI    0
        PHI     R8
        LDN     R8
        FRET

; au_needdraw: next month's need: a dotted line at that height beside the
; bar, and under the player how far short (-) or over (+) (mekkogx au_need_draw).
        RFIT    235
au_needdraw:
        LDI     0
        PUTV    AV_I
and1:   GETV    RND
        ADI     1
        PLO     RB
        GETV    AV_I
        FCALL   au_need
        BZ      and9
        PUTV    T3                      ; n
        SMI     AUBARM/2+1              ; its line
        BNF     and2
        LDI     AUBARB-AUBARM
        BR      and3
and2:   GETV    T3
        SHL
        SDI     AUBARB
and3:   PUTV    T4
        PARS                            ; dotted, beside the bar
        GETV    AV_I
        SHL
        SHL
        SHL
        SHL
        ADI     10
        PD
        GETV    T4
        PD
        P       6
        P       1
        P       TM_SET|TP_DASH
        GOP     TOP_RECT
        LDR     R9, S_PLAYERS+PL_GOODS  ; w - n, signed, in the label row
        GETV    AV_I
        FCALL   pl_byte
        FCALL   abt_good
        PUTV    T0
        GETV    T3
        STR     R2
        GETV    T0
        SM
        BNF     and4
        PLO     R9
        LDI     '+'
        BR      and5
and4:   SDI     0
        PLO     R9
        LDI     '-'
and5:   PUTV    T4                      ; blanks, the sign, the digits: the
        LDI     0                       ; field's 4 cells (odd seats inverse)
        PHI     R9
        FCALL   fmt_num
        SDI     3
        PLO     RB
        GETV    AV_I
        SHL
        SHL
        ORI     70H
        HOT     TX_CURSOR
        GETV    AV_I
        FCALL   au_inv
and6:   GLO     RB
        BZ      and7
        LDI     ' '
        HOT     TX_PUTC
        DEC     RB
        BR      and6
and7:   GETV    T4
        HOT     TX_PUTC
        LDR     R8, NUMBUF
        FCALL   d_puts
        OPN     TOP_INV_OFF
and9:   GETV    AV_I
        ADI     1
        STR     RA
        XRI     4
        BNZ     and1
        FRET

; au_statset: the players standing at the foot, the bars from nothing.
        RFIT    78
au_statset:
        LDR     R8, AV_ROLE
        LDI     4
        PLO     RB
ass1:   LDI     ROLE_BUYER
        STR     R8
        INC     R8
        DEC     RB
        GLO     RB
        BNZ     ass1
        SETV    AV_STG, STAGE_SHOW
        SETV    AV_BLINK, 0
        FCALL   au_draw
        SPTR    A_ROLE
        LDR     R9, AV_ROLE
        LDI     4
        PLO     RB
ass2:   LDA     R8
        STR     R9
        INC     R9
        DEC     RB
        GLO     RB
        BNZ     ass2
        LDR     R8, AV_BAR
        LDI     8
        PLO     RB
ass3:   LDI     0
        STR     R8
        INC     R8
        DEC     RB
        GLO     RB
        BNZ     ass3
        SETV    AV_SHOWT, 0FFH
        FRET

; au_stattick: the status's frame: its step by the time, the bars moving
; two lines a frame (mekkogx au_status_tick).
        RFIT    124
au_stattick:
        FCALL   pe_now                  ; k: 127 jiffies a step, 4 after 496
        LDI     0
        PLO     RC
        LDR     R8, ast_t
sat1:   LDA     R8
        PLO     RB
        LDA     R8
        PHI     RB
        GLO     RB
        STR     R2
        GLO     R9
        SM
        GHI     RB
        STR     R2
        GHI     R9
        SMB
        BNF     sat2
        INC     RC
        GLO     RC
        XRI     4
        BNZ     sat1
sat2:   GETV    AV_SHOWT
        STR     R2
        GLO     RC
        XOR
        BZ      sat5
        GLO     RC
        PUTV    AV_SHOWT
        FCALL   au_bartgt
        GETV    AV_SHOWT
        SMI     4
        BDF     sat3
        GETV    AV_SHOWT
        ADI     M_PREVIOUS
        FCALL   msg_show
        BR      sat5
sat3:   GETV    AV_GOOD                 ; the store's stock
        ADI     LO(SO_STOCK)
        PLO     R8
        LDI     HI(SO_STOCK)
        ADCI    0
        PHI     R8
        LDN     R8
        PUTV    V1L
        SETV    V1H, 0
        GETV    V1L
        BZ      sat4
        LDI     M_STOREHAS
        BR      sat4a
sat4:   LDI     M_STORENO
sat4a:  FCALL   msg_show
        FCALL   au_needdraw
sat5:   FJMP    au_barsmove
ast_t:  DB      127, 0, 250, 0, LO(373), HI(373), LO(496), HI(496)

; au_barsmove: each bar 2 lines a frame towards its height.
        RFIT    85
au_barsmove:
        LDI     0
        PUTV    AV_I
abm1:   GETV    AV_I
        ADI     LO(AV_BAR)
        PLO     RA
        LDN     RA
        PLO     RB                      ; was
        GETV    AV_I
        ADI     LO(AV_BART)
        PLO     RA
        LDN     RA
        PHI     RB                      ; target
        STR     R2
        GLO     RB
        XOR
        BZ      abm5
        GLO     RB
        SM
        BDF     abm2
        GLO     RB                      ; up 2, not past
        ADI     2
        STR     R2
        GHI     RB
        SM
        BDF     abm3
        GHI     RB
        BR      abm4
abm2:   GLO     RB                      ; down 2
        SMI     2
        BNF     abm2a
        STR     R2
        GHI     RB
        SD
        BDF     abm3
abm2a:  GHI     RB
        BR      abm4
abm3:   LDN     R2
abm4:   PHI     RB                      ; RB.1 = new
        GETV    AV_I
        ADI     LO(AV_BAR)
        PLO     RA
        GHI     RB
        STR     RA
        FCALL   au_bardraw
abm5:   GETV    AV_I
        ADI     1
        STR     RA
        XRI     4
        BNZ     abm1
        FRET

; au_bardraw: seat AV_I's bar from RB.0 lines to RB.1 (the lines between).
        RFIT    72
au_bardraw:
        PARS
        GETV    AV_I                    ; x = 16i + 11
        SHL
        SHL
        SHL
        SHL
        ADI     11
        PD
        GLO     RB
        STR     R2
        GHI     RB
        SM
        BNF     abd1
        GHI     RB                      ; grown: lit from foot - new
        SDI     AUBARB
        PD
        P       4
        GLO     RB
        STR     R2
        GHI     RB
        SM
        PD
        P       TM_SET
        BR      abd2
abd1:   GLO     RB                      ; shrunk: dark from foot - was
        SDI     AUBARB
        PD
        P       4
        GHI     RB
        STR     R2
        GLO     RB
        SM
        PD
        P       TM_CLEAR
abd2:   GOP     TOP_RECT
        FRET

; ---- the screen ---------------------------------------------------------------------
; au_enter: the auction's screen (mekkogx auction_enter).
        RFIT    83
au_enter:
        SETV    ANIM, AN_NONE
        SETV    AV_ON, 0
        FCALL   fx_stop_all
        FCALL   scr_clear
        SETV    SCRNOW, SCR_AUC
        SETV    AV_LN1, 0FFH
        SETV    AV_LN2, 0FFH
        SETV    AV_SHOWT, 0FFH
        SETV    AV_MSG, 0FFH
        SETV    AV_HBL, 0FFH
        SETV    AV_HBH, 0FFH
        SETV    MSGROW, 9
        SETV    MSG1, 1
        SETV    BARMAX, AUYBOT-12
        LDI     12
        FJMP    bar_arena

; au_title: "FOOD STATUS" / "FOOD AUCTION", or the land's (mekkogx auction_title).
        RFIT    150
au_title:
        GETV    AV_KIND
        XRI     1
        BNZ     aut1
        GETV    RND
        PUTV    V1L
        SETV    V1H, 0
        LDI     T_LANDAUC
        FJMP    title_show
aut1:   GETV    AV_STG                  ; trading: the store's prices about the
        SMI     STAGE_LEAD              ; good (what it pays, what it asks)
        BDF     aut4
        GETV    AV_GOOD
        ADI     G_FOOD
        FCALL   str_get
        ADI     LO(TXT)
        PLO     R8
        LDI     HI(TXT)
        PHI     R8
        LDI     ' '
        STR     R8
        GETV    TXTLEN
        ADI     1
        STR     RA
        LDI     B_STATUS
        FCALL   str_cat
        LDI     0
        FJMP    text_center
aut4:   GETV    AV_GOOD
        ADI     G_FOOD
        FCALL   str_get
        LDI     0
        FCALL   text_center
        LDI     0
        HOT     TX_CURSOR
        SPTR    A_STOREBUY
        FCALL   aut_num
        LDR     R8, NUMBUF
        FCALL   d_puts
        SPTR    A_STORESELL
        FCALL   aut_num
        SDI     TCOLS
        HOT     TX_CURSOR
        LDR     R8, NUMBUF
        FCALL   d_puts
        FRET
; aut_num: the word at R8 into NUMBUF; D its length.
aut_num:
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
        FJMP    fmt_num

; au_begin: a stage begins (mekkogx au_stage_begin).
        RFIT    227
au_begin:
        HOLD1
        PARS                            ; all but the title
        P       0
        P       12
        P       64
        P       108
        P       TM_CLEAR
        GOP     TOP_RECT
        LDI     7
        FCALL   clrrow
        LDI     8
        FCALL   clrrow
        LDI     9
        FCALL   clrrow
        HOLD0
        SETV    AV_LN1, 0FFH
        SETV    AV_LN2, 0FFH
        SETV    AV_MSG, 0FFH
        SETV    BARLAST, 0FFH
        SETV    AV_MALL, 1
        SETV    AV_SBL, 0FFH
        SETV    AV_SBH, 0FFH
        SETV    AV_HBL, 0FFH
        SETV    AV_HBH, 0FFH
        GETV    AV_STG                  ; a new plot or good: no push yet
        BNZ     aub1
        SETV    AV_PUSH, 0
        GETV    AV_KIND
        XRI     2
        BNZ     aub1
        SETV    AV_ON, 0                ; a good's status
        SETV    ANIM, AN_NONE
        FJMP    au_statset
aub1:   GETV    AV_STG                  ; declaring, or the lines about to go:
        XRI     STAGE_DECLARE           ; your hand to the keypad
        BZ      aub2
        XRI     STAGE_DECLARE!STAGE_LEAD
        BNZ     aub3
aub2:   FCALL   alert
aub3:   GETV    AV_STG
        XRI     STAGE_DONE
        BNZ     aub4
        LDI     SND_AUCOVR
        FCALL   snd_play
aub4:   FJMP    au_labels

; landauc_enter / goods_enter (mekkogx)
landauc_enter:
        SETV    AV_STG, 0FFH
        SETV    AV_PUSH, 0
        SETV    AUTRADE, 0
        SETV    MAPDOTS, 0
        FRET
        RFIT    51
goods_enter:
        GETV    LASTPH                  ; declare and trading share the screen
        XRI     PHASE_DECLARE
        BZ      gen2
        XRI     PHASE_DECLARE!PHASE_TRADING
        BZ      gen2
gen1:    SETV    SCRNOW, SCR_NONE
gen2:    GETV    PHASE
        XRI     PHASE_DECLARE
        BNZ     gen3
        SETV    SCRNOW, SCR_NONE
gen3:    SETV    AV_STG, 0FFH
        SETV    AV_PUSH, 0
        SETV    AUTRADE, 0
        FRET

; ---- a state in phases 2, 5, 6 (mekkogx auction_state) -----------------------------
        RFIT    213
auction_state:
        SLD     A_KIND
        PUTV    AV_KIND
        SLD     A_STAGE
        PUTV    AV_NEW
        SLD     A_GOOD
        ANI     3
        PUTV    AV_GOOD
        SLD     A_STEP
        BNZ     aus0
        LDI     1
aus0:   PUTV    AV_STEP
        PLO     RC                      ; lim = step * 30
        LDI     0
        PLO     R9
        PHI     R9
        LDI     30
        PLO     RB
aus0a:  GLO     RC
        STR     R2
        GLO     R9
        ADD
        PLO     R9
        GHI     R9
        ADCI    0
        PHI     R9
        DEC     RB
        GLO     RB
        BNZ     aus0a
        GLO     R9
        PUTV    AV_LIML
        GHI     R9
        PUTV    AV_LIMH
        SPTR    A_ROLE
        LDR     R9, AV_ROLE
        LDI     4
        PLO     RB
aus1:   LDA     R8
        STR     R9
        INC     R9
        DEC     RB
        GLO     RB
        BNZ     aus1
        GETV    AV_KIND                 ; the land: the plot blinks on the map
        XRI     1
        BNZ     aus3
        GETV    AV_NEW
        BNZ     aus3
        FJMP    au_landshow
aus3:   GETV    MAPBLINK                ; (that plot as it is again)
        SMI     NUM_PLOTS
        BDF     aus4
        GETV    SCRNOW
        XRI     SCR_MAP
        BNZ     aus4
        GETV    MAPBLINK
        PLO     RC
        SETV    MAPBLINK, NOBODY
        GLO     RC
        FCALL   map_mark
aus4:   SETV    MAPBLINK, NOBODY
        GETV    SCRNOW                  ; a new screen, or a new stage
        XRI     SCR_AUC
        BNZ     aus5
        GETV    AV_STG
        STR     R2
        GETV    AV_NEW
        XOR
        BZ      aus7
        BR      aus6
aus5:   FCALL   au_enter
aus6:   GETV    AV_NEW
        PUTV    AV_STG
        FCALL   au_title
        FCALL   au_begin
aus7:   FJMP    au_lines0

; au_landshow: the land auction's plot, blinking on the map, its start price.
        RFIT    95
au_landshow:
        GETV    SCRNOW
        XRI     SCR_MAP
        BNZ     als1
        GETV    AV_STG
        BZ      als3
als1:   GETV    SCRNOW
        XRI     SCR_MAP
        BZ      als2
        FCALL   map_enter
als2:   GETV    RND
        PUTV    V1L
        SETV    V1H, 0
        LDI     T_LANDAUC
        FCALL   title_show
        SPTR    A_START
        LDA     R8
        PUTV    V1L
        LDN     R8
        PUTV    V1H
        LDI     M_BIDSTART
        FCALL   msg_show
als3:   GETV    AV_NEW
        PUTV    AV_STG
        SLD     A_PLOT
        PUTV    MAPBLINK
        FRET

; au_lines0: the lines' base, positions caught up with the server's clock,
; and the screen's parts (the rest of mekkogx auction_state).
        RFIT    204
au_lines0:
        GETV    AV_KIND                 ; land: the base resets for a new plot;
        XRI     1                       ; goods: the store's buying price
        BNZ     alz2
        GETV    AV_STG
        SMI     STAGE_ACTIVE
        BNF     alz1
        SPTR    A_START                 ; base < start: start
        GETV    AV_BASL
        SEX     R8
        SM
        INC     R8
        GETV    AV_BASH
        SEX     R8
        SMB
        SEX     R2
        BDF     alz3
alz1:   SPTR    A_START
        BR      alz2a
alz2:   SPTR    A_STOREBUY
alz2a:  LDA     R8
        PUTV    AV_BASL
        LDN     R8
        PUTV    AV_BASH
alz3:   LDI     0FFH                    ; our let-go line: kept a moment
        PUTV    T5                      ; (T5: that seat, 255 none)
        GETV    AV_STG
        XRI     STAGE_ACTIVE
        BNZ     alz4
        GETV    AV_PUSH
        BNZ     alz4
        GETV    AV_HELD
        STR     R2
        GETV    FCL
        SM
        SMI     60
        BDF     alz4
        GETV    MYSEAT
        PUTV    T5
alz4:   SLD     H_T                     ; j = (t & 7) + rx_age; tick = j & 7
        ANI     7
        STR     R2
        GETV    RXAGE
        ADD
        PLO     RB
        ANI     7
        PUTV    AV_TICK
        GETV    AV_STG
        XRI     STAGE_ACTIVE
        BZ      alz5
        LDI     0
        PLO     RB
        BR      alz6
alz5:   GLO     RB
        SHR
        SHR
        SHR
        PLO     RB
alz6:   GLO     RB
        PUTV    T4                      ; T4: steps to catch up
        LDI     0FFH                    ; the critical seller's line is held
        PUTV    T3
        GETV    AV_KIND
        XRI     2
        BNZ     alz7
        SLD     A_FLAGS
        ANI     AF_CRITICAL
        BZ      alz7
        SLD     A_SELLER
        PUTV    T3
alz7:   FJMP    au_snap

; au_snap: each line from the state (but ours, held), caught up.
        RFIT    187
au_snap:
        LDI     0
        PUTV    AV_I
asn1:   GETV    AV_I
        PLO     RC
        STR     R2
        GETV    T5
        XOR
        BNZ     asn2
        GETV    AV_I                    ; ours, held: if it's still moving there
        ADI     LO(A_PUSH)
        PLO     R8
        LDI     HI(A_PUSH)
        ADCI    0
        PHI     R8
        LDN     R8
        BNZ     asn5
        FCALL   asn_pos
        FCALL   asn_same
        BNZ     asn5
asn2:   FCALL   asn_pos                 ; R9 = the state's pos
        LDI     LO(AV_POSL)
        FCALL   au_ptr
        GLO     R9
        STR     RA
        GLO     RA
        ADI     4
        PLO     RA
        GHI     R9
        STR     RA
        GETV    AV_I                    ; the target (the critical seller: here)
        STR     R2
        GETV    T3
        XOR
        BZ      asn3
        GETV    AV_I
        SHL
        ADI     LO(A_TARGET)
        PLO     R8
        LDI     HI(A_TARGET)
        ADCI    0
        PHI     R8
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
asn3:   LDI     LO(AV_TGTL)
        FCALL   au_ptr
        GLO     R9
        STR     RA
        GLO     RA
        ADI     4
        PLO     RA
        GHI     R9
        STR     RA
        GETV    T4                      ; the steps it missed
        PUTV    T0
asn4:   GETV    T0
        BZ      asn5
        SMI     1
        STR     RA
        GETV    AV_I
        PLO     RC
        FCALL   au_step
        BR      asn4
asn5:   GETV    AV_I
        ADI     1
        STR     RA
        XRI     4
        BNZ     asn1
        FJMP    au_parts
; asn_pos: R9 = seat AV_I's line in the state.
asn_pos:
        GETV    AV_I
        SHL
        ADI     LO(A_POS)
        PLO     R8
        LDI     HI(A_POS)
        ADCI    0
        PHI     R8
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
        FRET
; asn_same: D = 0 if the state's pos (R9) is its target.
asn_same:
        GETV    AV_I
        SHL
        ADI     LO(A_TARGET)
        PLO     R8
        LDI     HI(A_TARGET)
        ADCI    0
        PHI     R8
        LDA     R8
        STR     R2
        GLO     R9
        XOR
        BNZ     asm9
        LDN     R8
        STR     R2
        GHI     R9
        XOR
asm9:   FRET

; au_parts: the lines going, and the screen's parts.
        RFIT    106
au_parts:
        GETV    AV_STG
        SMI     STAGE_LEAD
        BNF     apt1
        SETV    AV_ON, 1
        SETV    ANIM, AN_AUC
apt1:   GETV    AV_KIND                 ; a good's status: its own frame does it
        XRI     2
        BNZ     apt2
        GETV    AV_STG
        BZ      apt9
apt2:   FCALL   au_money
        FCALL   au_lines
        FCALL   au_timebar
        FCALL   au_message
        FCALL   au_landline
        FCALL   au_blinkers
        FCALL   au_draw
        SLD     A_TRADED                ; a trade: the cue
        STR     R2
        GETV    AUTRADE
        XOR
        BZ      apt9
        SLD     A_TRADED
        STR     R2
        GETV    AUTRADE
        SD
        BNF     apt3
        BZ      apt3
        LDI     SND_TRADE
        FCALL   snd_play
apt3:   SLD     A_TRADED
        PUTV    AUTRADE
apt9:   FRET

; ---- the stick (mekkogx au_stick): sent when it changes (up, down, let go) ---------
        RFIT    199
au_stick:
        LDI     0                       ; k: 2 up, 8 down (held)
        PLO     RC
        GETV    KEY
        XRI     2
        BNZ     ask1
        LDI     1
        PLO     RC
ask1:   GETV    KEY
        XRI     8
        BNZ     ask2
        LDI     2
        PLO     RC
ask2:   GETV    AV_PUSH
        STR     R2
        GLO     RC
        XOR
        BZ      ask9
        GETV    ACT
        BNZ     ask9
        GLO     RC
        PUTV    AV_PUSH
        BNZ     ask4
        GETV    AV_STG                  ; let go while it goes: hold where it is
        XRI     STAGE_ACTIVE
        BNZ     ask4
        GETV    MYSEAT
        PLO     RC
        FCALL   au_pos
        LDI     ACT_HOLD
        FCALL   act_send
        GETV    FCL
        PUTV    AV_HELD
        BR      ask5
ask4:   GETV    AV_PUSH
        PLO     R9
        LDI     0
        PHI     R9
        LDI     ACT_PUSH
        FCALL   act_send
ask5:   GETV    AV_STG
        XRI     STAGE_ACTIVE
        BNZ     ask9
        GETV    MYSEAT                  ; our line moves at once
        PLO     RC
        GETV    AV_PUSH
        XRI     1
        BNZ     ask7
        GETV    AV_KIND                 ; up: land as high as our money goes
        XRI     1
        BNZ     ask6
        LDR     R9, S_PLAYERS+PL_MONEY
        GETV    MYSEAT
        FCALL   pl_byte
        ANI     0FCH
        PLO     R9
        INC     R8
        LDN     R8
        PHI     R9
        BR      ask8
ask6:   SPTR    A_HI
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
        BR      ask8
ask7:   GETV    AV_PUSH
        XRI     2
        BNZ     ask7a
        SPTR    A_LO                    ; down
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
        BR      ask8
ask7a:  FCALL   au_pos                  ; let go: where it is
ask8:   GETV    MYSEAT
        PLO     RC
        LDI     LO(AV_TGTL)
        FCALL   au_ptr
        GLO     R9
        STR     RA
        GLO     RA
        ADI     4
        PLO     RA
        GHI     R9
        STR     RA
ask9:   FRET

; ---- the phase's frame (mekkogx auction_tick) ------------------------------------
        RFIT    227
auction_tick:
        GETV    AV_KIND                 ; the land's plot blinking on the map
        XRI     1
        BNZ     atk3
        GETV    AV_STG
        BNZ     atk3
        GETV    SCRNOW
        XRI     SCR_MAP
        BNZ     atk2
        GETV    MAPBLINK
        SMI     NUM_PLOTS
        BDF     atk2
        GETV    FCL                     ; 16 frames on, 16 off
        SHR
        SHR
        SHR
        SHR
        ANI     1
        STR     R2
        GETV    MAPBLON
        XOR
        BZ      atk2
        LDN     R2
        STR     RA
        HOLD1
        GETV    MAPBLINK
        FCALL   map_draw_plot
        HOLD0
atk2:   FCALL   msg_tick
        GETV    MYSEAT
        SMI     4
        BDF     atk9
        GETV    KEDGE                   ; 5 skips the wait
        XRI     K_FIRE
        BNZ     atk2a
        LDI     ACT_READY
        FCALL   act_send
atk2a:  FJMP    au_stick
atk3:   GETV    SCRNOW
        XRI     SCR_AUC
        BNZ     atk9
        FCALL   msg_tick
        GETV    AV_KIND
        XRI     2
        BNZ     atk4
        GETV    AV_STG
        BNZ     atk4
        FJMP    au_stattick
atk4:   FCALL   au_lines
        FCALL   au_lbltick
        FCALL   au_timebar
        GETV    AV_STG
        XRI     STAGE_DECLARE
        BNZ     atk6
        GETV    MYSEAT
        SMI     4
        BDF     atk9
        FJMP    au_declare
atk6:   GETV    MYSEAT                  ; leading or going: the stick, 5 colludes
        SMI     4
        BDF     atk9
        GETV    AV_STG
        XRI     STAGE_ACTIVE
        BZ      atk7
        XRI     STAGE_ACTIVE!STAGE_LEAD
        BNZ     atk9
atk7:   FCALL   au_stick
        GETV    KEDGE
        XRI     K_FIRE
        BNZ     atk9
        GETV    AV_KIND
        XRI     2
        BNZ     atk9
        GETV    AV_STG
        XRI     STAGE_ACTIVE
        BNZ     atk9
        LDI     ACT_COLL
        FJMP    act_send
atk9:   FRET

; au_declare: 2 a seller, 8 a buyer, 5 sits out (mekkogx auction_tick's).
        RFIT    131
au_declare:
        GETV    MYSEAT
        PLO     RC
        LDI     LO(AV_ROLE)
        FCALL   au_ptr
        LDN     RA
        PUTV    T0                      ; the role now
        GETV    KEY
        XRI     2
        BNZ     adc1
        GETV    T0
        XRI     ROLE_SELLER
        BZ      adc9
        LDI     ROLE_SELLER
        PLO     RB
        LDI     2
        BR      adc3
adc1:   GETV    KEY
        XRI     8
        BNZ     adc2
        GETV    T0
        XRI     ROLE_BUYER
        BZ      adc9
        LDI     ROLE_BUYER
        PLO     RB
        LDI     1
        BR      adc3
adc2:   GETV    KEDGE                   ; 5: out (if we were in, or never said)
        XRI     K_FIRE
        BNZ     adc9
        GETV    T0
        BNZ     adc2a
        LDR     R9, S_PLAYERS+PL_FLAGS
        GETV    MYSEAT
        FCALL   pl_byte
        ANI     PF_DECLARED
        BNZ     adc9
adc2a:  LDI     ROLE_NONE
        PLO     RB
        LDI     0
adc3:   PLO     R9                      ; declare it
        LDI     0
        PHI     R9
        GETV    MYSEAT
        PLO     RC
        LDI     LO(AV_ROLE)
        FCALL   au_ptr
        GLO     RB
        STR     RA
        LDI     ACT_DECL
        FCALL   act_send
        FCALL   au_labels
        LDI     SND_SELECT
        FCALL   snd_play
adc9:   FJMP    au_draw
