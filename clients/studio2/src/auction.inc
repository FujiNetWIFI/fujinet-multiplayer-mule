; auction.inc -- the per-good status, declaring, and both auctions (mekkogx
; auction.c). The screen:
;   row 0           the good and STATUS or AUCTION (land: the title; its
;                   price line, small, under it)
;   lines 12-80     the arena: a price step a line, the base (the store's
;                   buying price, the land's opening) at 67, 41 steps over it
;                   and 13 under; each player in a 16-pixel column, feet on
;                   its price line
;   row 7           BUY / SELL, or each player's price
;   row 8           money and units, small        row 9  the message
; 2 pushes the line up (sell: lower your price... up is up), 8 down; let go
; and it stays. Declaring: 2 a seller, 8 a buyer, 5 sits it out.

AV_KIND EQU     PH+0
AV_GOOD EQU     PH+1
AV_STG  EQU     PH+2                    ; the stage on the screen (255 none)
AV_NEW  EQU     PH+3
AV_STEP EQU     PH+4                    ; dollars a step
AV_LIML EQU     PH+5                    ; land: 30 steps, a bid's run over the base
AV_LIMH EQU     PH+6
AV_ROLE EQU     PH+7                    ; 4
AV_POSL EQU     PH+11                   ; 4: each line's price
AV_POSH EQU     PH+15                   ; 4
AV_TGTL EQU     PH+19                   ; 4: where it goes
AV_TGTH EQU     PH+23                   ; 4
AV_BASL EQU     PH+27                   ; the price at the base line
AV_BASH EQU     PH+28
AV_ON   EQU     PH+29
AV_TICK EQU     PH+30
AV_PHS  EQU     PH+31                   ; steps taken: walk frames, blinking
AV_MALL EQU     PH+32                   ; the money all again
AV_PUSH EQU     PH+33                   ; our stick as sent: 0 let go, 1 up, 2 down
AV_HELD EQU     PH+34                   ; the frame we let go
AV_LN1  EQU     PH+35                   ; the sellers' and buyers' lines (255 none)
AV_LN2  EQU     PH+36
AV_LBLL EQU     PH+37                   ; 4: what row 7 shows: 0 the word, 1 none,
AV_LBLH EQU     PH+41                   ; 4:  else a price
AV_HBL  EQU     PH+45                   ; land: the price line shown
AV_HBH  EQU     PH+46
AV_SBL  EQU     PH+47                   ; land: "bids start at" shown
AV_SBH  EQU     PH+48
AV_MSG  EQU     PH+49
AV_BLINK EQU    PH+50                   ; seats that blink, a bit each
AV_SHOWT EQU    PH+51                   ; the status's step shown
AV_BAR  EQU     PH+52                   ; 4: the status bars, lines
AV_BART EQU     PH+56                   ; 4: where they go
AV_LBLI EQU     PH+60
AV_I    EQU     PH+61
AV_W    EQU     PH+62
AUYBASE EQU     67
AUYSELL EQU     AUYBASE-35              ; the sellers' start
AUYTOP  EQU     AUYBASE-41
AUYBOT  EQU     AUYBASE+13
AUUP    EQU     41
AUDOWN  EQU     13
AULX    EQU     6                       ; the dotted lines
AULW    EQU     52
AUBARB  EQU     80                      ; the status bars' foot
AUBARM  EQU     50                      ; 2 lines a unit, 25 units at most

        IF      AV_W >= PH+67
        ERROR   "the auction's variables run past the phases' union"
        ENDIF

; ---- prices and lines ------------------------------------------------------
; au_ptr: RA on the array D's entry for seat RC.0 (D = LO(array)).
au_ptr: STR     R2
        GLO     RC
        ADD
        PLO     RA
        FRET

; au_pos: R9 = seat RC's line price. au_tgt: R9 = its target.
        RFIT    18
au_pos: LDI     LO(AV_POSL)
        BR      aup1
au_tgt: LDI     LO(AV_TGTL)
aup1:   FCALL   au_ptr
        LDN     RA
        PLO     R9
        GLO     RA
        ADI     4
        PLO     RA
        LDN     RA
        PHI     R9
        FRET

; price_y: D = the line for price R9 (mekkogx price_y). Keeps RC.
        RFIT    86
price_y:
        GETV    AV_BASL                 ; w >= base: up
        STR     R2
        GLO     R9
        SM
        PLO     RB
        GETV    AV_BASH
        STR     R2
        GHI     R9
        SMB
        PHI     RB
        BNF     py3
        FCALL   py_steps
        SMI     AUUP+1
        BNF     py1
        LDI     AUUP
        BR      py2
py1:    ADI     AUUP+1
py2:    SDI     AUYBASE
        FRET
py3:    GLO     RB                      ; under the base: down
        SDI     0
        PLO     RB
        GHI     RB
        SDBI    0
        PHI     RB
        FCALL   py_steps
        SMI     AUDOWN+1
        BNF     py4
        LDI     AUDOWN
        BR      py5
py4:    ADI     AUDOWN+1
py5:    ADI     AUYBASE
        FRET
; py_steps: D = RB in steps (/4 at 4 dollars a step), 255 at most.
py_steps:
        GETV    AV_STEP
        XRI     4
        BNZ     pys1
        GHI     RB
        SHR
        PHI     RB
        GLO     RB
        SHRC
        PLO     RB
        GHI     RB
        SHR
        PHI     RB
        GLO     RB
        SHRC
        PLO     RB
pys1:   GHI     RB
        BZ      pys2
        LDI     0FFH
        FRET
pys2:   GLO     RB
        FRET

; au_step: one step of seat RC's line towards its target (mekkogx au_step).
        RFIT    99
au_step:
        FCALL   au_tgt                  ; RB = target
        GLO     R9
        PLO     RB
        GHI     R9
        PHI     RB
        FCALL   au_pos                  ; R9 = pos
        GLO     RB                      ; pos < target: up a step, not past
        STR     R2
        GLO     R9
        SM
        GHI     RB
        STR     R2
        GHI     R9
        SMB
        BDF     ast2
        GETV    AV_STEP
        STR     R2
        GLO     R9
        ADD
        PLO     R9
        GHI     R9
        ADCI    0
        PHI     R9
        GLO     RB                      ; past it: the target
        STR     R2
        GLO     R9
        SD
        GHI     RB
        STR     R2
        GHI     R9
        SDB
        BDF     ast5
        BR      ast4
ast2:   GLO     RB                      ; pos > target: down
        STR     R2
        GLO     R9
        XOR
        BNZ     ast3
        GHI     RB
        STR     R2
        GHI     R9
        XOR
        BZ      ast9                    ; there
ast3:   GETV    AV_STEP
        STR     R2
        GLO     R9
        SM
        PLO     R9
        GHI     R9
        SMBI    0
        PHI     R9
        GLO     RB                      ; under it: the target
        STR     R2
        GLO     R9
        SM
        GHI     RB
        STR     R2
        GHI     R9
        SMB
        BDF     ast5
ast4:   GLO     RB
        PLO     R9
        GHI     RB
        PHI     R9
ast5:   LDI     LO(AV_POSL)             ; store it
        FCALL   au_ptr
        GLO     R9
        STR     RA
        GLO     RA
        ADI     4
        PLO     RA
        GHI     R9
        STR     RA
        FJMP    au_scroll
ast9:   FRET

; au_scroll: land: a bid past 30 steps over the base scrolls the scale up.
        RFIT    77
au_scroll:
        GETV    AV_KIND
        XRI     1
        BNZ     scl9
        LDI     LO(AV_ROLE)
        FCALL   au_ptr
        LDN     RA
        XRI     ROLE_BUYER
        BNZ     scl9
        GETV    AV_BASL                 ; base + lim
        STR     R2
        GETV    AV_LIML
        ADD
        PLO     RB
        GETV    AV_BASH
        STR     R2
        GETV    AV_LIMH
        ADC
        PHI     RB
        GLO     RB                      ; pos > it: base = pos - lim
        STR     R2
        GLO     R9
        SM
        GHI     RB
        STR     R2
        GHI     R9
        SMB
        BNF     scl9
        GETV    AV_LIML
        STR     R2
        GLO     R9
        SM
        PUTV    AV_BASL
        GETV    AV_LIMH
        STR     R2
        GHI     R9
        SMB
        PUTV    AV_BASH
scl9:   FRET

; auction_isr: every frame: the lines step every 8th, the players drawn.
        RFIT    61
auction_isr:
        GETV    AV_ON
        BZ      ai9
        GETV    AV_TICK
        ADI     1
        STR     RA
        SMI     8
        BNF     ai9
        SETV    AV_TICK, 0
        GETV    AV_PHS
        ADI     1
        STR     RA
        LDI     0
        PUTV    AV_I
ai1:    GETV    AV_I
        PLO     RC
        FCALL   au_step
        GETV    AV_I
        ADI     1
        STR     RA
        XRI     4
        BNZ     ai1
        FJMP    au_draw
ai9:    FRET

; au_draw: the players at their lines, walking while a line moves, the
; dealers blinking (mekkogx au_draw).
        RFIT    232
au_draw:
        LDI     0
        PUTV    AV_I
ad1:    GETV    AV_I
        PLO     RC
        LDI     LO(AV_ROLE)
        FCALL   au_ptr
        LDN     RA
        BZ      ad8                     ; out
        GETV    AV_STG                  ; before the lead: sellers up, buyers down
        SMI     STAGE_LEAD
        BDF     ad3
        LDI     LO(AV_ROLE)
        FCALL   au_ptr
        LDN     RA
        XRI     ROLE_SELLER
        BNZ     ad2
        LDI     AUYSELL
        BR      ad4
ad2:    LDI     AUYBOT
        BR      ad4
ad3:    FCALL   au_pos
        FCALL   price_y
ad4:    PUTV    AV_W                    ; the feet's line
        GETV    AV_I                    ; dealing: two steps on, two off
        ADI     LO(au_bits)
        PLO     R8
        LDI     HI(au_bits)
        ADCI    0
        PHI     R8
        LDN     R8
        STR     R2
        GETV    AV_BLINK
        AND
        BZ      ad5
        GETV    AV_PHS
        ANI     2
        BNZ     ad8
ad5:    LDR     R9, S_PLAYERS+PL_SPECIES        ; standing, or a step
        GETV    AV_I
        FCALL   pl_byte
        ANI     7
        PLO     RB
        GETV    AV_I
        PLO     RC
        FCALL   au_pos
        GLO     R9
        PHI     RB
        FCALL   au_tgt
        GLO     R9
        STR     R2
        GHI     RB
        XOR
        BZ      ad6
        GETV    AV_PHS
        ANI     1
        BZ      ad6
        LDI     IM_WALKA_MECHTRON
        BR      ad7
ad6:    LDI     IM_STAND_MECHTRON
ad7:    STR     R2
        GLO     RB
        ADD
        PLO     RB
        PARS
        GETV    AV_I
        PD
        GLO     RB
        PD
        GETV    AV_I                    ; x = 16i + 5
        SHL
        SHL
        SHL
        SHL
        ADI     5
        PD
        GETV    AV_W
        SMI     14
        PD
        P       TM_OR
        GOP     TOP_SPRITE
        BR      ad9
ad8:    PARS                            ; off
        GETV    AV_I
        PD
        P       100
        P       0
        GOP     TOP_SPRMOVE
ad9:    GETV    AV_I
        ADI     1
        STR     RA
        XRI     4
        BNZ     ad1
        FRET
au_bits: DB     1, 2, 4, 8

; au_lines: the sellers' line (the lowest ask, or the store's price while it
; has stock) and the buyers' (the highest bid, or the store's buying price /
; the land's base), dotted across (mekkogx au_lines).
        RFIT    177
au_lines:
        GETV    AV_STG
        BZ      aln9
        GETV    AV_KIND
        XRI     1
        BNZ     aln0
        GETV    AV_STG
        SMI     STAGE_LEAD
        BNF     aln9
aln0:    LDI     0FFH                    ; the sellers': lowest
        PLO     R9
        PHI     R9
        GETV    AV_KIND
        XRI     2
        BNZ     aln1
        GETV    AV_GOOD
        ADI     LO(SO_STOCK)
        PLO     R8
        LDI     HI(SO_STOCK)
        ADCI    0
        PHI     R8
        LDN     R8
        BZ      aln1
        SPTR    A_STORESELL
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
aln1:    LDI     ROLE_SELLER
        FCALL   al_best
        LDI     0FFH
        PUTV    AV_W                    ; AV_W = its y (255 none)
        GLO     R9
        XRI     0FFH
        BNZ     aln2
        GHI     R9
        XRI     0FFH
        BZ      aln3
aln2:    FCALL   price_y
        PUTV    AV_W
aln3:    GETV    AV_KIND                 ; the buyers': highest
        XRI     2
        BNZ     aln4
        SPTR    A_STOREBUY
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
        BR      aln5
aln4:    GETV    AV_BASL
        PLO     R9
        GETV    AV_BASH
        PHI     R9
aln5:    LDI     ROLE_BUYER
        FCALL   al_best
        FCALL   price_y
        PLO     RC                      ; RC.0 = the buyers' y
        GETV    AV_W                    ; never over the labels
        SMI     AUYBOT+1
        BNF     aln6
        SETV    AV_W, 0FFH
aln6:    GLO     RC
        SMI     AUYBOT+1
        BNF     aln7
        LDI     0FFH
        PLO     RC
aln7:    GETV    AV_LN1                  ; as they were: nothing to do
        STR     R2
        GETV    AV_W
        XOR
        BNZ     aln8
        GETV    AV_LN2
        STR     R2
        GLO     RC
        XOR
        BZ      aln9
aln8:    FJMP    al_redraw
aln9:    FRET

; al_best: R9 = the best line of the seats in role D at the lead or later:
; sellers the lowest under R9, buyers the highest over it.
        RFIT    79
al_best:
        PUTV    AV_I                    ; (AV_I: the role, for now)
        GETV    AV_STG
        SMI     STAGE_LEAD
        BNF     alb9
        LDI     0
        PLO     RC
alb1:   LDI     LO(AV_ROLE)
        FCALL   au_ptr
        LDN     RA
        STR     R2
        GETV    AV_I
        XOR
        BNZ     alb4
        GLO     R9                      ; (R9 kept in RB while au_pos runs)
        PLO     RB
        GHI     R9
        PHI     RB
        FCALL   au_pos                  ; R9 = its price
        GETV    AV_I
        XRI     ROLE_SELLER
        BNZ     alb2
        GLO     RB                      ; seller: pos < best?
        STR     R2
        GLO     R9
        SM
        GHI     RB
        STR     R2
        GHI     R9
        SMB
        BNF     alb4                    ; yes: R9 is it
        BR      alb3
alb2:   GLO     RB                      ; buyer: pos > best?
        STR     R2
        GLO     R9
        SD
        GHI     RB
        STR     R2
        GHI     R9
        SDB
        BNF     alb4
alb3:   GLO     RB                      ; no: the best stays
        PLO     R9
        GHI     RB
        PHI     R9
alb4:   INC     RC
        GLO     RC
        XRI     4
        BNZ     alb1
alb9:   FRET

; al_redraw: the new lines on, the old ones off (AV_W, RC.0 the new).
        RFIT    156
al_redraw:
        HOLD1
        GETV    AV_LN1                  ; the old: off, unless one stays
        FCALL   al_off
        GETV    AV_LN2
        FCALL   al_off
        GETV    AV_W
        PUTV    AV_LN1
        GLO     RC
        PUTV    AV_LN2
        GETV    AV_LN1
        FCALL   al_on
        GETV    AV_LN2
        FCALL   al_on
        HOLD0
        FRET
al_off: PLO     RB
        XRI     0FFH
        BZ      alo9
        PARS
        P       AULX
        GLO     RB
        PD
        P       AULW
        P       1
        P       TM_CLEAR
        GOP     TOP_RECT
alo9:   FRET
al_on:  PLO     RB
        XRI     0FFH
        BZ      alo9
        PARS
        P       AULX
        GLO     RB
        PD
        P       AULW
        P       1
        P       TM_SET|TP_DASH
        GOP     TOP_RECT
        FRET

; ---- row 7: BUY / SELL at the start, else each line's price ------------------
; au_lbltick: one label a frame, if it changed (mekkogx au_lbl_tick).
        RFIT    146
au_lbltick:
        LDI     4
        PUTV    AV_W
alt1:   GETV    AV_LBLI
        ADI     1
        ANI     3
        STR     RA
        PLO     RC
        LDI     LO(AV_ROLE)
        FCALL   au_ptr
        LDN     RA
        BNZ     alt2
        LDI     1                       ; out: none
        PLO     R9
        LDI     0
        PHI     R9
        BR      alt5
alt2:   GETV    AV_STG
        SMI     STAGE_LEAD
        BDF     alt3
        LDI     0                       ; the word
        PLO     R9
        PHI     R9
        BR      alt5
alt3:   FCALL   au_pos                  ; the price, unless at the scale's end
        LDI     LO(AV_ROLE)
        FCALL   au_ptr
        LDN     RA
        XRI     ROLE_BUYER
        BNZ     alt4
        SPTR    A_LO                    ; buyer at the bottom: the word
        LDA     R8
        STR     R2
        GLO     R9
        SD
        LDN     R8
        STR     R2
        GHI     R9
        SDB
        BNF     alt5
        BR      alt4b
alt4:   SPTR    A_HI                    ; seller at the top: the word
        LDA     R8
        STR     R2
        GLO     R9
        SM
        LDN     R8
        STR     R2
        GHI     R9
        SMB
        BNF     alt5
alt4b:  LDI     0
        PLO     R9
        PHI     R9
alt5:   LDI     LO(AV_LBLL)             ; changed?
        FCALL   au_ptr
        LDN     RA
        STR     R2
        GLO     R9
        XOR
        BNZ     alt6
        GLO     RA
        ADI     4
        PLO     RA
        LDN     RA
        STR     R2
        GHI     R9
        XOR
        BNZ     alt6
        GETV    AV_W
        SMI     1
        STR     RA
        BNZ     alt1
        FRET
alt6:   LDI     LO(AV_LBLL)
        FCALL   au_ptr
        GLO     R9
        STR     RA
        GLO     RA
        ADI     4
        PLO     RA
        GHI     R9
        STR     RA
        FJMP    au_lbldraw

; au_lbldraw: seat RC's label, R9 (0 the word, 1 none, else the price).
        RFIT    110
au_lbldraw:
        GLO     RC                      ; its field: row 7, 4 cells
        FCALL   au_inv
        GLO     RC
        SHL
        SHL
        ORI     70H
        PUTV    AV_W
        HOT     TX_CURSOR
        LDI     4
        PLO     RB
ald1:   LDI     ' '
        HOT     TX_PUTC
        DEC     RB
        GLO     RB
        BNZ     ald1
        GETV    AV_W
        HOT     TX_CURSOR
        GHI     R9
        BNZ     ald3
        GLO     R9
        BZ      ald2
        XRI     1
        BZ      ald9                    ; none
        BR      ald3
ald2:   LDI     LO(AV_ROLE)             ; BUY / SELL
        FCALL   au_ptr
        LDN     RA
        XRI     ROLE_SELLER
        BNZ     ald2a
        LDI     W_SELL
        BR      ald2b
ald2a:  LDI     W_BUY
ald2b:  FCALL   str_get
        LDR     R8, TXT
        FCALL   d_puts
        BR      ald9
ald3:   LDI     4                       ; the price, 4 cells
        FCALL   put_num
ald9:   OPN     TOP_INV_OFF
        FRET

; au_labels: the labels all again; the store's prices at the arena's ends.
        RFIT    21
au_labels:
        LDR     R8, AV_LBLL
        LDI     8
        PLO     RB
alb_1:  LDI     0FFH
        STR     R8
        INC     R8
        DEC     RB
        GLO     RB
        BNZ     alb_1
        FCALL   au_lbltick
alb_9:  FRET

; au_money: money and (goods) units, small, under the players (mekkogx au_money).
        RFIT    216
au_money:
        GETV    AV_MALL
        BNZ     am0
        SPTR    S_PLAYERS
        LDI     4*PL_SIZE
        FCALL   cmp_prev
        BZ      am9
am0:    SETV    AV_MALL, 0
        HOLD1
        PARS
        P       0
        P       96
        P       64
        P       12
        P       TM_CLEAR
        GOP     TOP_RECT
        LDI     0
        PUTV    AV_I
am1:    LDR     R9, S_PLAYERS+PL_MONEY
        GETV    AV_I
        FCALL   pl_byte
        PLO     R9
        INC     R8
        LDN     R8
        PHI     R9
        GETV    AV_I
        FCALL   am_x
        LDI     96
        PHI     RC
        LDI     TM_OR|TM_SHORT
        FCALL   pt_numk
        GETV    AV_KIND                 ; units, in a goods auction once it goes
        XRI     2
        BNZ     am2
        GETV    AV_STG
        SMI     STAGE_LEAD
        BNF     am2
        LDR     R9, S_PLAYERS+PL_GOODS
        GETV    AV_I
        FCALL   pl_byte
        STR     R2
        GETV    AV_GOOD
        STR     R2
        GLO     R8
        ADD
        PLO     R8
        GHI     R8
        ADCI    0
        PHI     R8
        LDN     R8
        PLO     R9
        LDI     0
        PHI     R9
        GETV    AV_I
        FCALL   am_x
        LDI     102
        PHI     RC
        LDI     TM_OR|TM_SHORT
        FCALL   pt_numk
am2:    GETV    AV_I
        ADI     1
        STR     RA
        XRI     4
        BNZ     am1
        LDI     16
        FCALL   au_zeb8
        LDI     48
        FCALL   au_zeb8
        HOLD0
am9:    FRET
; au_inv: odd seats' fields (1, 3) inverse, so the four read apart: row 7
; in the text's inverse, row 8 (au_zeb8: the field at x D) by XOR.
        RFIT    56
au_inv: SHR
        BNF     az9
        OPN     TOP_INV_ON
        FRET
au_zeb8:
        PLO     RB
        PARS
        GLO     RB
        PD
        P       96
        P       16
        P       12
        P       TM_XOR
        GOP     TOP_RECT
az9:    FRET
; am_x: RC.0 = the right edge of seat D's field.
am_x:   SHL
        SHL
        SHL
        SHL
        ADI     16
        PLO     RC
        FRET

; ---- the clock (mekkogx au_ticks_now, au_timebar): a line a tick left ----------
        RFIT    134
au_timebar:
        LDI     0
        PLO     RC
        GETV    AV_STG
        XRI     STAGE_ACTIVE
        BZ      atb1
        XRI     STAGE_ACTIVE!STAGE_DECLARE
        BNZ     atb9
atb1:   GETV    RXFRL                   ; el = since the state + per - tickLeft
        STR     R2
        GETV    FCL
        SM
        PLO     R9
        GETV    RXFRH
        STR     R2
        GETV    FCH
        SMB
        PHI     R9
        LDI     25                      ; per: 25, or 6 idle and declaring
        PLO     RB
        GETV    AV_STG
        XRI     STAGE_DECLARE
        BZ      atb2
        SLD     A_FLAGS
        ANI     AF_IDLE
        BZ      atb3
atb2:   LDI     6
        PLO     RB
atb3:   GLO     RB
        STR     R2
        GLO     R9
        ADD
        PLO     R9
        GHI     R9
        ADCI    0
        PHI     R9
        SLD     A_TICKLEFT
        STR     R2
        GLO     R9
        SM
        PLO     R9
        GHI     R9
        SMBI    0
        PHI     R9
        BDF     atb4
        LDI     0
        PLO     R9
        PHI     R9
atb4:   LDI     0                       ; / per
        PLO     RC
atb5:   GLO     RB
        STR     R2
        GLO     R9
        SM
        PLO     R8
        GHI     R9
        SMBI    0
        BNF     atb6
        PHI     R9
        GLO     R8
        PLO     R9
        INC     RC
        BR      atb5
atb6:   SLD     A_TICKS                 ; ticks - that
        STR     R2
        GLO     RC
        SD
        BDF     atb7
        LDI     0
atb7:   PLO     RC
atb9:   GLO     RC
        FCALL   bar_tick
        GLO     RC
        FJMP    bar_draw

; ---- the message (mekkogx au_message) and the land's price line ------------------
        RFIT    225
au_message:
        LDI     0FFH
        PLO     RC
        GETV    AV_STG
        XRI     STAGE_DECLARE
        BNZ     amg1
        GETV    MYSEAT
        SMI     4
        BDF     amg1
        LDI     M_DECLARE
        PLO     RC
amg1:   GETV    AV_STG
        XRI     STAGE_DONE
        BNZ     amg3
        LDI     M_AUCOVER
        PLO     RC
        GETV    AV_KIND
        XRI     1
        BNZ     amg3
        SLD     A_WINNER                ; land: sold, or no bids
        SMI     4
        BDF     amg2
        LDI     M_SOLD
        PLO     RC
        BR      amg3
amg2:   LDI     M_NOBIDS
        PLO     RC
amg3:   GETV    AV_KIND
        XRI     2
        BNZ     amg5
        GETV    AV_STG
        XRI     STAGE_ACTIVE
        BNZ     amg5
        SLD     A_FLAGS
        ANI     AF_COLLUDING
        BZ      amg4
        LDI     M_COLLUDE
        PLO     RC
amg4:   SLD     A_FLAGS
        ANI     AF_CRITICAL
        BZ      amg5
        LDI     M_CRITICAL
        PLO     RC
amg5:   GETV    AV_KIND                 ; land, going: the scale's bottom
        XRI     1
        BNZ     amg7
        GETV    AV_STG
        XRI     STAGE_ACTIVE
        BNZ     amg7
        GETV    AV_MSG
        STR     R2
        GLO     RC
        XOR
        BNZ     amg6
        GETV    AV_SBL
        STR     R2
        GETV    AV_BASL
        XOR
        BNZ     amg6
        GETV    AV_SBH
        STR     R2
        GETV    AV_BASH
        XOR
        BZ      amg9
amg6:   GLO     RC
        PUTV    AV_MSG
        GETV    AV_BASL
        PUTV    AV_SBL
        PUTV    V1L
        GETV    AV_BASH
        PUTV    AV_SBH
        PUTV    V1H
        LDI     M_BIDSTART
        FJMP    msg_show
amg7:   GETV    AV_MSG
        STR     R2
        GLO     RC
        XOR
        BZ      amg9
        GLO     RC
        PUTV    AV_MSG
        XRI     0FFH
        BZ      amg8
        GLO     RC
        FJMP    msg_show
amg8:   FJMP    msg_clear
amg9:   FRET

; au_landline: the land auction's price, small under the title: the opening,
; then the high bid, then what it sold for (mekkogx au_land_line).
        RFIT    162
au_landline:
        GETV    AV_KIND
        XRI     1
        BNZ     all9
        LDI     B_LSTART
        PUTV    AV_W
        SPTR    A_START
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
        LDI     0
        PLO     RC
all1:   LDI     LO(AV_ROLE)             ; the highest bid at or over it
        FCALL   au_ptr
        LDN     RA
        XRI     ROLE_BUYER
        BNZ     all2
        GLO     R9
        PLO     RB
        GHI     R9
        PHI     RB
        FCALL   au_pos
        GLO     RB
        STR     R2
        GLO     R9
        SM
        GHI     RB
        STR     R2
        GHI     R9
        SMB
        BNF     all1a
        SETV    AV_W, B_LHIGH
        BR      all2
all1a:  GLO     RB
        PLO     R9
        GHI     RB
        PHI     R9
all2:   INC     RC
        GLO     RC
        XRI     4
        BNZ     all1
        GETV    AV_STG                  ; sold: its price
        XRI     STAGE_DONE
        BNZ     all3
        SPTR    A_PRICE
        LDA     R8
        STR     R2
        LDN     R8
        OR
        BZ      all3
        DEC     R8
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
        SETV    AV_W, B_LSOLD
all3:   GETV    AV_HBL                  ; changed?
        STR     R2
        GLO     R9
        XOR
        BNZ     all4
        GETV    AV_HBH
        STR     R2
        GHI     R9
        XOR
        BZ      all9
all4:   GLO     R9
        PUTV    AV_HBL
        PUTV    V1L
        GHI     R9
        PUTV    AV_HBH
        PUTV    V1H
        GETV    AV_W
        FCALL   str_get
        LDI     1
        FJMP    text_center
all9:   FRET

; au_blinkers: who deals blinks: the land's high bidder (the winner once
; sold); a goods buyer and seller whose lines meet, or whoever meets the
; store's line (mekkogx au_blinkers).
        RFIT    98
au_blinkers:
        SETV    AV_BLINK, 0
        GETV    AV_STG
        SMI     STAGE_ACTIVE
        BNF     abk9
        GETV    AV_KIND
        XRI     1
        BNZ     abk3
        GETV    AV_STG                  ; land
        XRI     STAGE_DONE
        BNZ     abk1
        SLD     A_WINNER
        SMI     4
        BDF     abk9
        SLD     A_WINNER
        BR      abk2
abk1:   SPTR    A_START                 ; the high bidder
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
        LDI     ROLE_BUYER
        FCALL   abk_who
        GLO     RC
        SMI     4
        BDF     abk9
        GLO     RC
abk2:   ADI     LO(au_bits)
        PLO     R8
        LDI     HI(au_bits)
        ADCI    0
        PHI     R8
        LDN     R8
        PUTV    AV_BLINK
abk9:   FRET
abk3:   GETV    AV_STG                  ; goods, going
        XRI     STAGE_ACTIVE
        BNZ     abk9
        FJMP    abk_goods

; abk_who: RC.0 = the seat of role D with the best line from R9 (buyers
; the highest at or over it); 255 none. R9 = that line.
        RFIT    73
abk_who:
        PUTV    AV_I
        LDI     0FFH
        PUTV    AV_W
        LDI     0
        PLO     RC
abw1:   LDI     LO(AV_ROLE)
        FCALL   au_ptr
        LDN     RA
        STR     R2
        GETV    AV_I
        XOR
        BNZ     abw3
        GLO     R9
        PLO     RB
        GHI     R9
        PHI     RB
        FCALL   au_pos
        GLO     RB                      ; pos >= best
        STR     R2
        GLO     R9
        SM
        GHI     RB
        STR     R2
        GHI     R9
        SMB
        BNF     abw2
        GLO     RC
        PUTV    AV_W
        BR      abw3
abw2:   GLO     RB
        PLO     R9
        GHI     RB
        PHI     R9
abw3:   INC     RC
        GLO     RC
        XRI     4
        BNZ     abw1
        GETV    AV_W
        PLO     RC
        FRET

; abk_goods: the buyer and seller whose lines meet, or the store's.
        RFIT    229
abk_goods:
        LDI     0                       ; the highest buyer (from 0)
        PLO     R9
        PHI     R9
        LDI     ROLE_BUYER
        FCALL   abk_who
        GLO     RC
        PUTV    T0                      ; T0 = j, T1:T2 = its line
        GLO     R9
        PUTV    T1
        GHI     R9
        PUTV    T2
        LDI     0                       ; the lowest seller: the highest of
        PUTV    T3                      ;  the "not over" sense, done by hand
        LDI     0FFH
        PUTV    T4                      ; T4 = k
        PLO     R9
        PHI     R9
        LDI     0
        PLO     RC
abg1:   LDI     LO(AV_ROLE)
        FCALL   au_ptr
        LDN     RA
        XRI     ROLE_SELLER
        BNZ     abg3
        GLO     R9
        PLO     RB
        GHI     R9
        PHI     RB
        FCALL   au_pos
        GLO     RB                      ; pos <= lo
        STR     R2
        GLO     R9
        SD
        GHI     RB
        STR     R2
        GHI     R9
        SDB
        BNF     abg2
        GLO     RC
        PUTV    T4
        BR      abg3
abg2:   GLO     RB
        PLO     R9
        GHI     RB
        PHI     R9
abg3:   INC     RC
        GLO     RC
        XRI     4
        BNZ     abg1
        GETV    T0                      ; both, and they meet: both blink
        SMI     4
        BDF     abg5
        GETV    T4
        SMI     4
        BDF     abg5
        GLO     R9                      ; hi >= lo
        STR     R2
        GETV    T1
        SM
        GHI     R9
        STR     R2
        GETV    T2
        SMB
        BNF     abg5
        GETV    T0
        FCALL   abg_bit
        GETV    T4
        FJMP    abg_bit
abg5:   GETV    T0                      ; the buyer at the store's price
        SMI     4
        BDF     abg6
        SPTR    A_STORESELL
        GETV    T1
        SEX     R8
        SM
        INC     R8
        GETV    T2
        SEX     R8
        SMB
        SEX     R2
        BNF     abg6
        GETV    T0
        FCALL   abg_bit
abg6:   GETV    T4                      ; the seller at the store's
        SMI     4
        BDF     abg9
        SPTR    A_STOREBUY
        GLO     R9
        SEX     R8
        SD
        INC     R8
        GHI     R9
        SEX     R8
        SDB
        SEX     R2
        BNF     abg9
        GETV    T4
        FJMP    abg_bit
abg9:   FRET
; abg_bit: seat D blinks too.
abg_bit:
        ADI     LO(au_bits)
        PLO     R8
        LDI     HI(au_bits)
        ADCI    0
        PHI     R8
        LDN     R8
        STR     R2
        GETV    AV_BLINK
        OR
        STR     RA
        FRET
