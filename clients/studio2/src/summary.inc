; summary.inc -- the status summary and the game-over screen (mekkogx
; status.c). Round 0's summary follows the colonists' landing. The players
; walk in from the right along the bottom, then climb to their bands in rank
; order; then the table: each band the figure, the name and the total, and
; under them cash, land and goods; the colony's total; then everyone
; presses 5. Game over: FIRST FOUNDER, then the final table and the rating.

SU_ORD  EQU     PH+0                    ; 4: the seats by rank
SU_DRAWN EQU    PH+4
SU_LAND EQU     PH+5                    ; the landing plays first
SU_T0L  EQU     PH+6                    ; the frame it began
SU_T0H  EQU     PH+7
SU_I    EQU     PH+8
SU_K    EQU     PH+9
SU_Y    EQU     PH+10
GO_STG  EQU     PH+11                   ; game over: 0 the founder, 1 the table
SU_LANDL EQU    PH+12                   ; a seat's land value
SU_LANDH EQU    PH+13
SUBANDY EQU     19                      ; band i's top: 19 + 18i

; su_rank: the seats in rank order (rank 1 first), anyone unranked last.
        RFIT    115
su_rank:
        SETV    SU_I, 0                 ; (SU_I: how many placed)
        LDI     1
        PUTV    SU_K                    ; (SU_K: the rank sought)
sr1:    LDI     0
        PLO     RC
sr2:    LDR     R9, S_PLAYERS+PL_RANK
        GLO     RC
        FCALL   pl_byte
        STR     R2
        GETV    SU_K
        XOR
        BNZ     sr3
        GLO     RC
        FCALL   sr_put
sr3:    INC     RC
        GLO     RC
        XRI     4
        BNZ     sr2
        GETV    SU_K
        ADI     1
        STR     RA
        XRI     5
        BNZ     sr1
        LDI     0                       ; the rest, in seat order
        PLO     RC
sr4:    GETV    SU_I
        PLO     RB
        LDR     R8, SU_ORD
sr5:    GLO     RB
        BZ      sr6
        DEC     RB
        LDA     R8
        STR     R2
        GLO     RC
        XOR
        BZ      sr7                     ; placed already
        BR      sr5
sr6:    GLO     RC
        FCALL   sr_put
sr7:    INC     RC
        GLO     RC
        XRI     4
        BNZ     sr4
        FRET
; sr_put: seat D next in the order (4 at most). Keeps RC.
sr_put: PLO     RB
        GETV    SU_I
        SMI     4
        BDF     srp9
        GETV    SU_I
        ADI     1
        STR     RA
        ADI     LO(SU_ORD)-1
        PLO     RA
        GLO     RB
        STR     RA
srp9:   FRET

; summary_enter: round 0's landing first (mekkogx summary_enter).
        RFIT    76
summary_enter:
        SETV    ANIM, AN_NONE
        FCALL   fx_stop_all
        SETV    SU_LAND, 0
        GETV    RND
        BNZ     se1
        FCALL   pe_now
        GHI     R9
        BNZ     se1
        GLO     R9
        SMI     LO(300)
        BDF     se1
        GHI     R9
        SMI     HI(300)
        BDF     se1
se0:    SETV    SU_LAND, 1              ; "you're landing on the planet Irata."
        FCALL   map_enter
        LDI     T_TRANSPORT
        FCALL   title_show
        LDI     M_LANDING
        FCALL   msg_show
        SETV    FXFLAG, 0
        LDR     R9, fx_land
        LDI     0
        FJMP    fx_start
se1:    FJMP    su_begin

; su_begin: the walk-ins, or (arriving late) the table at once.
        RFIT    81
su_begin:
        SETV    ANIM, AN_NONE
        FCALL   fx_stop_all
        FCALL   scr_clear
        GETV    FCL
        PUTV    SU_T0L
        GETV    FCH
        PUTV    SU_T0H
        SETV    SCRNOW, SCR_SUM
        SETV    SU_DRAWN, 0
        SETV    MSGROW, 9
        FCALL   su_rank
        GETV    RND                     ; late: straight to the table
        BZ      sb1
        FCALL   pe_now
        GHI     R9
        BNZ     sb0
        GLO     R9
        SMI     LO(400)
        BNF     sb1
        GHI     R9
        SMI     HI(400)
        BNF     sb1
sb0:    FJMP    su_table
sb1:    FJMP    su_walkin

; su_walkin: actor i walks in for band i.
        RFIT    196
su_walkin:
        SETV    SU_I, 0
sb2:    GETV    SU_I
        PLO     RC
        LDI     FK_SINGLE
        FPUT    FXKIND
        GLO     RC
        FPUT    FXSLOT
        FCALL   su_seat                 ; its species' walking frames
        PLO     RB
        LDR     R9, S_PLAYERS+PL_SPECIES
        GLO     RB
        FCALL   pl_byte
        ANI     7
        ADI     IM_WALKA_MECHTRON
        PHI     RD
        GETV    SU_I
        PLO     RC
        GHI     RD
        FPUT    FXIMG
        LDI     0
        FPUT    FXFRAME
        LDI     IM_WALKB_MECHTRON-IM_WALKA_MECHTRON
        FPUT    FXD2
        LDI     64
        FPUT    FXX
        LDI     100
        FPUT    FXY
        GETV    SU_I                    ; its script
        SHL
        ADI     LO(su_walks)
        PLO     R8
        LDI     HI(su_walks)
        ADCI    0
        PHI     R8
        LDA     R8
        PHI     R9
        LDN     R8
        PLO     R9
        GETV    SU_I
        FCALL   fx_start
        GETV    SU_I
        ADI     1
        STR     RA
        XRI     4
        BNZ     sb2
        FRET

; su_seat: D = the seat in band SU_I.
su_seat:
        GETV    SU_I
        ADI     LO(SU_ORD)
        PLO     RA
        LDN     RA
        FRET

; the walk-ins: in 30 frames apart, left along the bottom, up to the band
su_walks:
        DW      su_w0, su_w1, su_w2, su_w3
su_w0:  DB      FX_WAIT, 1, FX_MOVE, -1, 0, 3, 63, FX_MOVE, 0, -2, 3, 40, FX_END
su_w1:  DB      FX_WAIT, 30, FX_MOVE, -1, 0, 3, 63, FX_MOVE, 0, -2, 3, 31, FX_END
su_w2:  DB      FX_WAIT, 60, FX_MOVE, -1, 0, 3, 63, FX_MOVE, 0, -2, 3, 22, FX_END
su_w3:  DB      FX_WAIT, 90, FX_MOVE, -1, 0, 3, 63, FX_MOVE, 0, -2, 3, 13, FX_END

; su_table: the table (mekkogx su_table): the players standing at their
; bands, the title, the headings, the numbers; the summary asks for 5.
        RFIT    222
su_table:
        SETV    SU_DRAWN, 1
        FCALL   fx_stop_all
        HOLD1
        SETV    SU_I, 0
stb1:    PARS                            ; band i's player, standing
        GETV    SU_I
        PD
        FCALL   su_seat
        PLO     RC
        LDR     R9, S_PLAYERS+PL_SPECIES
        GLO     RC
        FCALL   pl_byte
        ANI     7
        ADI     IM_STAND_MECHTRON
        PD
        P       1
        FCALL   su_bandy
        ADI     1
        PD
        P       TM_OR
        GOP     TOP_SPRITE
        GETV    SU_I
        ADI     1
        STR     RA
        XRI     4
        BNZ     stb1
        GETV    RND
        PUTV    V1L
        SETV    V1H, 0
        LDI     W_SUMMARY
        FCALL   title_show
        LDI     B_SUMHEAD               ; CASH LAND GOOD, small, over their fields
        FCALL   str_get
        PARS
        P       8
        P       13
        P       TM_OR|TM_SHORT
        FCALL   lb_ptxt
        LDI     W_COLONY                ; COLONY, then its total on row 8
        FCALL   str_get
        AT      80H
        LDR     R8, TXT
        FCALL   d_puts
        FCALL   su_values
        FCALL   su_colony
        HOLD0
        GETV    PHASE
        XRI     PHASE_SUMMARY
        BNZ     stb9
        LDI     M_PRESSALL
        FCALL   msg_show
        FJMP    alert
stb9:    FRET

; su_bandy: D = band SU_I's top line.
su_bandy:
        GETV    SU_I
        SHL
        SHL
        SHL
        STR     R2
        GETV    SU_I
        ADD
        SHL
        ADI     SUBANDY
        FRET

; su_values: each band's numbers (mekkogx su_values).
        RFIT    249
su_values:
        HOLD1
        SETV    SU_I, 0
sv1:    FCALL   su_bandy
        PUTV    SU_Y
        FCALL   su_seat
        PUTV    SU_K
        PARS                            ; the band's numbers cleared
        P       8
        GETV    SU_Y
        PD
        P       56
        P       18
        P       TM_CLEAR
        GOP     TOP_RECT
        PARS                            ; the name
        P       8
        GETV    SU_Y
        PD
        P       TM_OR
        LDR     R9, S_PLAYERS+PL_NAME
        GETV    SU_K
        FCALL   pl_byte
        LDI     TX_PARAM
        PHI     RF
        LDI     8
        PLO     RC
sv2:    LDA     R8
        BZ      sv3
        PD
        DEC     RC
        GLO     RC
        BNZ     sv2
sv3:    GOP     TOP_PTEXT
        LDR     R9, S_PLAYERS+PL_SCORE  ; the total, at the right
        GETV    SU_K
        FCALL   pl_byte
        PLO     R9
        INC     R8
        LDN     R8
        PHI     R9
        GETV    SU_Y
        PHI     RC
        LDI     64
        PLO     RC
        LDI     TM_OR
        FCALL   pt_num
        LDR     R9, S_PLAYERS+PL_MONEY  ; cash
        GETV    SU_K
        FCALL   pl_byte
        PLO     R9
        INC     R8
        LDN     R8
        PHI     R9
        LDI     24
        FCALL   sv_small
        GETV    SU_K                    ; land
        FCALL   su_landof
        LDI     43
        FCALL   sv_small
        GETV    SU_K                    ; goods: each good's units at its price
        FCALL   su_goodsof
        LDI     62
        FCALL   sv_small
        GETV    SU_I
        ADI     1
        STR     RA
        XRI     4
        BNZ     sv1
        HOLD0
        FRET

; sv_small: R9 small (4 characters), ending at x D, on the band's second line.
sv_small:
        PLO     RC
        GETV    SU_Y
        ADI     12
        PHI     RC
        LDI     TM_OR|TM_SHORT
        FJMP    pt_numk

; su_landof: R9 = seat D's land: 500 a plot, the outfit + 35 for each
; M.U.L.E. on one (the server's ScoreOf).
        RFIT    72
su_landof:
        PUTV    SU_K
        LDI     0
        PLO     R9
        PHI     R9
        PLO     RC
sl1:    GLO     RC
        FCALL   mp_owner
        STR     R2
        GETV    SU_K
        XOR
        BNZ     sl3
        GLO     R9                      ; + 500
        ADI     LO(500)
        PLO     R9
        GHI     R9
        ADCI    HI(500)
        PHI     R9
        GLO     RC
        FCALL   mp_mule
        SMI     4
        BDF     sl3
        ADI     4+1                     ; + (g + 1) * 25 + 35
        PLO     RB
        LDI     35
sl2:    ADI     25
        DEC     RB
        STR     R2
        GLO     RB
        BZ      sl2a
        LDN     R2
        BR      sl2
sl2a:   GLO     R9
        ADD
        PLO     R9
        GHI     R9
        ADCI    0
        PHI     R9
sl3:    INC     RC
        GLO     RC
        XRI     NUM_PLOTS
        BNZ     sl1
        FRET

; su_goodsof: R9 = seat D's goods at the store's prices.
        RFIT    60
su_goodsof:
        PLO     RB
        LDR     R9, S_PLAYERS+PL_GOODS
        GLO     RB
        FCALL   pl_byte                 ; R8 on goods[0]
        LDI     0
        PLO     R9
        PHI     R9
        PLO     RC                      ; g
sg_1:   LDA     R8                      ; units
        PLO     RB
        GLO     R8                      ; (kept)
        STXD
        GHI     R8
        STXD
        GLO     RC                      ; the price, LE
        SHL
        ADI     LO(SO_PRICE)
        PLO     R8
        LDI     HI(SO_PRICE)
        ADCI    0
        PHI     R8
sg_2:   GLO     RB                      ; units times: += price
        BZ      sg_3
        DEC     RB
        GLO     R9
        SEX     R8
        ADD
        PLO     R9
        INC     R8
        GHI     R9
        ADC
        PHI     R9
        DEC     R8
        SEX     R2
        BR      sg_2
sg_3:   IRX
        LDXA
        PHI     R8
        LDX
        PLO     R8
        INC     RC
        GLO     RC
        XRI     4
        BNZ     sg_1
        FRET

; su_colony: the colony's total on row 8, at the right.
        RFIT    50
su_colony:
        AT      87H
        OPN     TOP_CLREOL
        SPTR    C_TOTAL
        FCALL   fmt32
        SDI     TCOLS                   ; right-aligned
        ORI     80H
        HOT     TX_CURSOR
        LDR     R8, NUMBUF
        FJMP    d_puts

; summary_state: the table at 410 frames; the numbers again when they change.
        RFIT    115
summary_state:
        GETV    SU_LAND
        BNZ     sus9
        GETV    SU_DRAWN
        BNZ     sus2
        FCALL   su_late
        BZ      sus9
        FJMP    su_table
sus2:   SPTR    S_PLAYERS               ; the players, the land, the prices
        LDI     4*PL_SIZE
        FCALL   cmp_prev
        BNZ     sus3
        SPTR    MP_OWNER
        LDI     2*NUM_PLOTS
        FCALL   cmp_prev
        BNZ     sus3
        SPTR    SO_PRICE
        LDI     8
        FCALL   cmp_prev
        BZ      sus4
sus3:   FCALL   su_values
sus4:   SPTR    C_TOTAL                 ; the colony's total
        LDI     4
        FCALL   cmp_prev
        BZ      sus9
        FJMP    su_colony
sus9:   FRET

; su_late: D != 0 once 410 frames have gone since the summary began.
su_late:
        GETV    SU_T0L
        STR     R2
        GETV    FCL
        SM
        PLO     R9
        GETV    SU_T0H
        STR     R2
        GETV    FCH
        SMB
        PHI     R9
        GLO     R9
        SMI     LO(410)
        GHI     R9
        SMBI    HI(410)
        BDF     sul1
        LDI     0
        FRET
sul1:   LDI     1
        FRET

; cmp_prev: D = 0 if the D bytes at R8 are as they were in PREV.
        RFIT    18
cmp_prev:
        PLO     RB
        GLO     R8
        PLO     R9
        GHI     R8
        ADI     HI(PRVOFF)
        PHI     R9
cp1:    LDA     R8
        STR     R2
        LDA     R9
        XOR
        BNZ     cp9
        DEC     RB
        GLO     RB
        BNZ     cp1
cp9:    FRET

; summary_tick: the landing, then the walk-ins, the table, and 5 (again
; while the server doesn't show us ready).
        RFIT    95
summary_tick:
        GETV    SU_LAND
        BZ      stt2
        FCALL   msg_tick
        GETV    FXFLAG
        XRI     1
        BZ      stt1
        GETV    FXST
        ANI     FS_RUN
        BNZ     stt9
stt1:   SETV    SU_LAND, 0
        FJMP    su_begin
stt2:   GETV    SU_DRAWN
        BNZ     stt3
        FCALL   su_late
        BZ      stt9
        FJMP    su_table
stt3:   FCALL   msg_tick
        GETV    KEDGE
        XRI     K_FIRE
        BNZ     stt9
        GETV    MYSEAT
        SMI     4
        BDF     stt9
        LDR     R9, S_PLAYERS+PL_FLAGS
        GETV    MYSEAT
        FCALL   pl_byte
        ANI     PF_READY
        BNZ     stt9
        LDI     ACT_READY
        FCALL   act_send
        LDI     SND_SELECT
        FJMP    snd_play
stt9:   FRET

; ---- game over: FIRST FOUNDER, then the final table and the rating ----------------
        RFIT    159
gameover_enter:
        SETV    ANIM, AN_NONE
        FCALL   fx_stop_all
        FCALL   scr_clear
        SETV    SCRNOW, SCR_FOUND
        SETV    GO_STG, 0
        SETV    MSGROW, 9
        FCALL   su_rank
        LDI     B_FIRST                 ; FIRST, the winner, FOUNDER
        FCALL   str_get
        LDI     1
        FCALL   text_center
        LDI     B_FOUNDER
        FCALL   str_get
        LDI     6
        FCALL   text_center
        SETV    SU_I, 0
        FCALL   su_seat
        PLO     RC
        LDR     R9, S_PLAYERS+PL_NAME
        GLO     RC
        FCALL   pl_byte
        LDI     3                       ; (the name: R8 on it)
        FCALL   go_name
        SETV    SU_I, 1                 ; the others stand down the left
ge1:    PARS
        GETV    SU_I
        PD
        FCALL   su_seat
        PLO     RC
        LDR     R9, S_PLAYERS+PL_SPECIES
        GLO     RC
        FCALL   pl_byte
        ANI     7
        ADI     IM_STAND_MECHTRON
        PD
        P       2
        GETV    SU_I                    ; y = 24i
        SHL
        SHL
        SHL
        STR     R2
        SHL
        ADD
        PD
        P       TM_OR
        GOP     TOP_SPRITE
        GETV    SU_I
        ADI     1
        STR     RA
        XRI     4
        BNZ     ge1
        LDI     SND_FOUND
        FJMP    snd_play

; go_name: the name at R8 (8 at most) centred on row D.
        RFIT    43
go_name:
        PUTV    T0
        LDR     R9, TXT
        LDI     8
        PLO     RC
gn1:    LDA     R8
        STR     R9
        BZ      gn2
        INC     R9
        DEC     RC
        GLO     RC
        BNZ     gn1
        LDI     0
        STR     R9
gn2:    GLO     R9
        SMI     LO(TXT)
        PUTV    TXTLEN
        GETV    T0
        FJMP    text_center

; gameover_tick: the winner pulses; after 540, the table and the rating.
        RFIT    118
gameover_tick:
        GETV    GO_STG
        BNZ     gt5
        PARS                            ; the winner, in the middle, pulsing
        P       0
        SETV    SU_I, 0
        FCALL   su_seat
        PLO     RC
        LDR     R9, S_PLAYERS+PL_SPECIES
        GLO     RC
        FCALL   pl_byte
        ANI     7
        ADI     IM_STAND_MECHTRON
        PD
        P       30
        P       54
        GETV    FCL
        ANI     20H
        BZ      gt1
        LDI     TM_XOR
        BR      gt2
gt1:    LDI     TM_OR
gt2:    PD
        GOP     TOP_SPRITE
        FCALL   pe_now
        GHI     R9
        SMI     HI(540)
        BNF     gt9
        BNZ     gt3
        GLO     R9
        SMI     LO(540)
        BNF     gt9
gt3:    SETV    GO_STG, 1
        FCALL   scr_clear
        FCALL   su_table
        SLD     C_RATING                ; "OVERALL, " + the rating
        SMI     7
        BDF     gt9
        FJMP    go_rating
gt5:    FJMP    msg_tick
gt9:    FRET

; go_rating: "OVERALL, " and the rating, scrolling.
        RFIT    36
go_rating:
        LDI     W_OVERALL
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
        SLD     C_RATING
        ADI     R_0
        FCALL   str_cat
        FJMP    msg_start
