; ---------------------------------------------------------------------------
; p3auc.inc -- page 3: the per-good status, declaring, and both auctions
; (mekkogx auction.c), on 24 columns:
;
;   y 4     the title: STATUS #n FOOD / AUCTION #n FOOD / LAND AUCTION #n
;   y10-37  the arena: the price lines, 2 steps a row, the base on y31 (41
;           steps up, 13 down); the store's prices at its left (y10 it
;           sells, y32 it buys); the seats stand at x 22/44/66/88, feet on
;           their line; the time bar at x 97
;   y38     BUY / SELL, or each seat's price      (6 cells a seat)
;   y44     money                                 (all light grey, 10-55)
;   y50     units (goods), or the land's price line
;   y56     the message: one row (MSGONE)
; The land auction opens on the map, its plot blinking.
; ---------------------------------------------------------------------------

AU_KIND EQU PV+0                ; 1 land, 2 goods
AU_GOOD EQU PV+1
AU_STAGE EQU PV+2               ; the stage drawn (255 none)
AU_NEW  EQU PV+3
AU_STEPV EQU PV+4               ; price per step
AU_LIM  EQU PV+5                ; 2: land: 30 steps, how far a bid runs over the base
AU_ROLE EQU PV+7                ; 4
AU_POS  EQU PV+11               ; 8: 4 x 16: each line's price
AU_TGT  EQU PV+19               ; 8: and where it is going
AU_BASE EQU PV+27               ; 2: the price on the base row
AU_ON   EQU PV+29
AU_TICK EQU PV+30               ; frames to the next step
AU_PH   EQU PV+31               ; steps taken: the walk frames, the blinking
AU_MALL EQU PV+32               ; the money fields again, all of them
AU_PUSH EQU PV+33               ; our stick as sent: 0 let go, 1 up, 2 down
AU_HELD EQU PV+34               ; frame (low byte) our line was let go
AU_LN1  EQU PV+35               ; rows of the sellers' / buyers' lines (255 none)
AU_LN2  EQU PV+36
AU_LBL  EQU PV+37               ; 8: per seat, the labels' row: 0 BUY/SELL,
                                ; 1 nothing, else a price
AU_HB   EQU PV+45               ; 2: land: the price line shown
AU_SB   EQU PV+47               ; 2: "bids start at" shown
AU_MSG  EQU PV+49               ; the message shown (255 none)
AU_BLINK EQU PV+50              ; seats that blink (a bit each)
AU_SHOWT EQU PV+51              ; the status: the step shown
AU_BAR  EQU PV+52               ; 4: the status bars (rows)
AU_BART EQU PV+56               ; 4: and their targets
AU_LBLI EQU PV+60               ; the label whose turn it is
AU_I    EQU PV+61               ; scratch
AU_W    EQU PV+62               ; 2
AU_V    EQU PV+64               ; 2
AU_J    EQU PV+66
AU_K    EQU PV+67
AU_C    EQU PV+68
AU_N    EQU PV+69
AU_Y    EQU PV+70
AU_X    EQU PV+71
AU_F    EQU PV+72

AUBASEY EQU 31                  ; the base price's row
AUSELLY EQU 14                  ; a seller's start (the Atari's line 48)
AUBOTY  EQU 37                  ; a buyer's start (144)
AUBARB  EQU 29                  ; the status bars' bottom row
AULBLY  EQU 38
AUMONY  EQU 44
AUUNTY  EQU 50
LINEX   EQU 22                  ; the dotted lines
LINEW   EQU 74

AUCFX:  DB 22, 44, 66, 88       ; each seat's column on the screen
AUCOLX: DB 34, 69, 104, 140     ; the same in the Atari's colour clocks
BITS:   DB 1, 2, 4, 8
STHR:   DW 127, 250, 373, 496   ; the status's steps (jiffies)

; ---------------------------------------------------------------------------
; Prices and lines
; ---------------------------------------------------------------------------

; AUPY -- A = the arena row of price r0:r1 (price_y).  Clobbers r0-r8.
AUPY:   ENTER
        LR A,0
        LR 6,A
        LR A,1
        LR 7,A
        LDW 2, 3, AU_BASE
        PI GE16
        BNC APY2
        PI SUB16                ; up: w - base
        CLR
        LR 8,A
        BR APY3
APY2:   LDW 0, 1, AU_BASE       ; down: base - w
        LR A,6
        LR 2,A
        LR A,7
        LR 3,A
        PI SUB16
        LIS 1
        LR 8,A
APY3:   LDB AU_STEPV            ; in steps
        CI 4
        BNZ APY4
        PI ASR16
        PI ASR16
APY4:   LI 41                   ; 41 steps up at most, 13 down
        LR 5,A
        LR A,8
        NI 0FFH
        BZ APY5
        LI 13
        LR 5,A
APY5:   LR A,1
        NI 0FFH
        BNZ APY6
        LR A,5
        COM
        AS 0                    ; d + ~max carries: d > max
        BNC APY7
APY6:   LR A,5
        LR 0,A
APY7:   LR A,0                  ; two steps a row
        SR 1
        LR 0,A
        LR A,8
        NI 0FFH
        BNZ APY8
        LR A,0
        COM
        INC
        AI AUBASEY
        LEAVE
APY8:   LR A,0
        AI AUBASEY
        LEAVE

; AUDRAW -- the seats placed (au_draw): at each step of the lines, and
;   after a state. The walk frames and the blinking go by the steps.
AUDRAW: ENTER
        CLR
        STB AU_I
AD1:    LDB AU_I
        DCI AU_ROLE
        ADC
        LM
        NI 0FFH
        BNZ AD2
ADH:    LDB AU_I
        LR 0,A
        PI SPRHIDE
        JMP ADN
AD2:    LDB AU_STAGE
        CI STAGE_LEAD-1
        BNC AD3
        LDB AU_I                ; before the lines: sellers up, buyers down
        DCI AU_ROLE
        ADC
        LM
        CI ROLE_SELL
        LI AUSELLY
        BZ AD4
        LI AUBOTY
        BR AD4
AD3:    LDB AU_I
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        PI AUPY
AD4:    STB AU_Y
        LDB AU_I                ; dealing: two steps on, two off
        DCI BITS
        ADC
        LM
        LR 4,A
        LDB AU_BLINK
        NS 4
        BZ AD5
        LDB AU_PH
        NI 2
        JNZ ADH
AD5:    LIS 5                   ; standing, or walking on odd steps
        LR 6,A
        LDB AU_PH
        NI 1
        BZ AD6
        LDB AU_I
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LDB AU_I
        SL 1
        DCI AU_TGT
        ADC
        LM
        XS 0
        BNZ AD5A
        LM
        XS 1
        BZ AD6
AD5A:   LIS 6
        LR 6,A
AD6:    LDB AU_I
        LR 0,A
        LR 5,A
        DCI AUCOLX
        ADC
        LM
        LR 1,A
        LDB AU_Y                ; feet on the row: the Atari's 4 row - 44
        SL 1
        SL 1
        AI 0D4H
        LR 2,A
        PI SPSPEC
ADN:    INCB AU_I
        CI 3
        JC AD1
        LEAVE

; AUSTEP -- line r0 a step towards its target (au_step).
AUSTEP: ENTER
        LR A,0
        STB AU_J
        LDB AU_J
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LDB AU_J
        SL 1
        DCI AU_TGT
        ADC
        LM
        LR 2,A
        LM
        LR 3,A
        PI GE16                 ; p < t: up a step, not past it
        BC AS2
        LR A,2
        LR 6,A
        LR A,3
        LR 7,A
        LDB AU_STEPV
        LR 2,A
        CLR
        LR 3,A
        PI ADD16
        LR A,6
        LR 2,A
        LR A,7
        LR 3,A
        PI GE16
        BNC AS4
        LR A,6
        LR 0,A
        LR A,7
        LR 1,A
        BR AS4
AS2:    LR A,0                  ; p == t: as it is
        XS 2
        BNZ AS3
        LR A,1
        XS 3
        BZ AS5
AS3:    LR A,2                  ; p > t: down a step, not past it
        LR 6,A
        LR A,3
        LR 7,A
        LR A,0
        LR 4,A
        LR A,1
        LR 5,A
        LR A,6
        LR 0,A
        LR A,7
        LR 1,A
        LDB AU_STEPV
        LR 2,A
        CLR
        LR 3,A
        PI ADD16                ; t + step
        LR A,0
        LR 2,A
        LR A,1
        LR 3,A
        LR A,4
        LR 0,A
        LR A,5
        LR 1,A
        PI GE16
        BNC AS3T
        LDB AU_STEPV
        LR 2,A
        CLR
        LR 3,A
        PI SUB16
        BR AS4
AS3T:   LR A,6
        LR 0,A
        LR A,7
        LR 1,A
AS4:    LDB AU_J
        SL 1
        DCI AU_POS
        ADC
        LR A,0
        ST
        LR A,1
        ST
AS5:    LDB AU_KIND             ; land: a bid past 30 steps over the base
        CI 1                    ; scrolls the scale up
        BNZ ASX
        LDB AU_J
        DCI AU_ROLE
        ADC
        LM
        CI ROLE_BUY
        BNZ ASX
        LDB AU_J
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 6,A
        LM
        LR 7,A
        LDW 0, 1, AU_BASE
        LDW 2, 3, AU_LIM
        PI ADD16
        LR A,0
        LR 2,A
        LR A,1
        LR 3,A
        LR A,6
        LR 0,A
        LR A,7
        LR 1,A
        PI GE16
        BNC ASX
        LDW 2, 3, AU_LIM
        PI SUB16
        STW 0, 1, AU_BASE
ASX:    LEAVE

; AUCISR -- every frame (auction_isr): the lines step every 8th.
AUCISR: ENTER
        LDB AU_ON
        NI 0FFH
        BZ AIX
        INCB AU_TICK
        CI 7
        BC AIX
        CLR
        STB AU_TICK
        INCB AU_PH
        CLR
        LR 0,A
        PI AUSTEP
        LIS 1
        LR 0,A
        PI AUSTEP
        LIS 2
        LR 0,A
        PI AUSTEP
        LIS 3
        LR 0,A
        PI AUSTEP
        PI AUDRAW
AIX:    LEAVE

; AUHD -- the price line on row AU_X in colour AU_C, unless that is 255 or
;   row AU_V or AU_V+1.
AUHD:   ENTER
        LDB AU_X
        CI 0FFH
        BZ AHX
        LR 4,A
        LDB AU_V
        XS 4
        BZ AHX
        LDB AU_V+1
        XS 4
        BZ AHX
        LI LINEX
        LR 1,A
        LR A,4
        LR 2,A
        LI LINEW
        LR 3,A
        LDB AU_C
        LR 0,A
        PI DDOTS
AHX:    LEAVE

; AULINES -- the dotted price lines (au_lines): the sellers' (lowest ask, or
;   the store's price while it has stock) and the buyers' (highest bid, or
;   the store's buying price / the bottom of the land's scale).
AULINES: ENTER
        LDB AU_STAGE
        NI 0FFH
        JZ ALX
        LDB AU_KIND
        CI 1
        BNZ AL0
        LDB AU_STAGE
        CI STAGE_LEAD-1
        JC ALX
AL0:    LI 0FFH                 ; the sellers': w
        LR 6,A
        LR 7,A
        LDB AU_KIND
        CI 2
        BNZ AL1
        LDB AU_GOOD
        DCI ST+S_STOCK
        ADC
        LM
        NI 0FFH
        BZ AL1
        LDW 6, 7, ST+A_SSELL
AL1:    LDB AU_STAGE
        CI STAGE_LEAD-1
        BC AL3
        CLR
        STB AU_I
AL2:    LDB AU_I
        DCI AU_ROLE
        ADC
        LM
        CI ROLE_SELL
        BNZ AL2N
        LDB AU_I
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LR A,6
        LR 2,A
        LR A,7
        LR 3,A
        PI GE16
        BC AL2N
        LR A,0
        LR 6,A
        LR A,1
        LR 7,A
AL2N:   INCB AU_I
        CI 3
        BC AL2
AL3:    LI 0FFH
        STB AU_J
        LR A,6
        CI 0FFH
        BNZ AL3A
        LR A,7
        CI 0FFH
        BZ AL4
AL3A:   LR A,6
        LR 0,A
        LR A,7
        LR 1,A
        PI AUPY
        STB AU_J
AL4:    LDW 6, 7, AU_BASE       ; the buyers': w
        LDB AU_KIND
        CI 2
        BNZ AL5
        LDW 6, 7, ST+A_SBUY
AL5:    LDB AU_STAGE
        CI STAGE_LEAD-1
        BC AL7
        CLR
        STB AU_I
AL6:    LDB AU_I
        DCI AU_ROLE
        ADC
        LM
        CI ROLE_BUY
        BNZ AL6N
        LDB AU_I
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 2,A
        LM
        LR 3,A
        LR A,6
        LR 0,A
        LR A,7
        LR 1,A
        PI GE16
        BC AL6N
        LR A,2
        LR 6,A
        LR A,3
        LR 7,A
AL6N:   INCB AU_I
        CI 3
        BC AL6
AL7:    LR A,6
        LR 0,A
        LR A,7
        LR 1,A
        PI AUPY
        STB AU_Y
        LDB AU_J                ; as they are?
        LR 4,A
        LDB AU_LN1
        XS 4
        BNZ AL8
        LDB AU_Y
        LR 4,A
        LDB AU_LN2
        XS 4
        JZ ALX
AL8:    LDB AU_LN1              ; the new lines first (one that stays is not
        STB AU_W                ; drawn again), then the old ones off
        STB AU_V
        LDB AU_LN2
        STB AU_W+1
        STB AU_V+1
        LDB AU_J
        STB AU_LN1
        STB AU_X
        LDB AU_Y
        STB AU_LN2
        LIS C_WHITE
        STB AU_C
        PI AUHD
        LDB AU_Y
        STB AU_X
        PI AUHD
        LDB AU_LN1
        STB AU_V
        LDB AU_LN2
        STB AU_V+1
        CLR
        STB AU_C
        LDB AU_W
        STB AU_X
        PI AUHD
        LDB AU_W+1
        STB AU_X
        PI AUHD
ALX:    LEAVE

; AUFIELD -- seat r0's field on the labels' row cleared; DTMP+0 = its first
;   cell.  Clobbers A, r1-r9, DC0.
AUFIELD: ENTER
        LR A,0
        SL 1
        LR 4,A
        SL 1
        AS 4
        STB DTMP+0
        SL 1
        SL 1
        AI TX0
        LR 1,A
        LI AULBLY
        LR 2,A
        LIS 6
        LR 3,A
        PI DCLRT
        LEAVE

; AULBLT -- the labels' row (au_lbl_tick): BUY / SELL while a seat stands at
;   its start, else its price. One a call, the seats taking turns.
AULBLT: ENTER
        LIS 4
        STB AU_N
ALB1:   LDB AU_LBLI
        INC
        NI 3
        STB AU_LBLI
        STB AU_I
        DCI AU_ROLE
        ADC
        LM
        NI 0FFH
        BNZ ALB2
        LIS 1                   ; no role: nothing
        LR 0,A
        CLR
        LR 1,A
        JMP ALB5
ALB2:   LDB AU_STAGE
        CI STAGE_LEAD-1
        BNC ALB3
        CLR                     ; before the lines: the label
        LR 0,A
        LR 1,A
        JMP ALB5
ALB3:   LDB AU_I                ; the price, or the label at its start
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LDB AU_I
        DCI AU_ROLE
        ADC
        LM
        CI ROLE_BUY
        BNZ ALB4
        LR A,0                  ; a buyer at or under lo
        LR 2,A
        LR A,1
        LR 3,A
        LDW 0, 1, ST+A_LO
        PI GE16
        BC ALBZ
        LR A,2
        LR 0,A
        LR A,3
        LR 1,A
        BR ALB4A
ALB4:   LDW 2, 3, ST+A_HI       ; a seller at or over hi
        PI GE16
        BC ALBZ
ALB4A:  LR A,1                  ; (1 means nothing: 2)
        NI 0FFH
        BNZ ALB5
        LR A,0
        CI 1
        BNZ ALB5
        LIS 2
        LR 0,A
        BR ALB5
ALBZ:   CLR
        LR 0,A
        LR 1,A
ALB5:   LDB AU_I                ; as shown?
        SL 1
        DCI AU_LBL
        ADC
        LM
        XS 0
        BNZ ALB6
        LM
        XS 1
        JZ ALBN
ALB6:   LDB AU_I
        SL 1
        DCI AU_LBL
        ADC
        LR A,0
        ST
        LR A,1
        ST
        STW 0, 1, AU_W
        LDB AU_I
        LR 0,A
        PI AUFIELD
        LDW 0, 1, AU_W
        LR A,1
        NI 0FFH
        BNZ ALB8
        LR A,0
        NI 0FFH
        BNZ ALB7
        LDB AU_I                ; 0: BUY or SELL, a cell in
        DCI AU_ROLE
        ADC
        LM
        CI ROLE_SELL
        LI WD_SELL
        BZ ALB6A
        LI WD_BUY
ALB6A:  LR 0,A
        DCI STW
        PI STRP
        LR H,DC
        LDB AU_I
        LR 0,A
        PI PLCOLR
        LR 6,A
        LDB DTMP+0
        INC
        SL 1
        SL 1
        AI TX0
        LR 1,A
        LI AULBLY
        LR 2,A
        LR DC,H
        PI DTEXT
        LEAVE
ALB7:   CI 1                    ; 1: nothing
        BZ ALBX
ALB8:   LDB AU_I                ; a price
        LR 0,A
        PI PLCOLR
        LR 6,A
        LDW 0, 1, AU_W
        LIS 5
        LR 4,A
        LI AULBLY
        LR 2,A
        PI PUTNUM
ALBX:   LEAVE
ALBN:   DECB AU_N
        JNZ ALB1
        LEAVE

; AULABELS -- the labels all again (au_labels), and the store's prices.
AULABELS: ENTER
        LI 0FFH
        DCI AU_LBL
        ST
        ST
        ST
        ST
        ST
        ST
        ST
        ST
        PI AULBLT
        LDB AU_KIND
        CI 2
        BNZ ALSX
        CLR                     ; it sells at the top left, buys at the
        STB DTMP+0              ; bottom left
        LDW 0, 1, ST+A_SSELL
        LIS 4
        LR 4,A
        LI 10
        LR 2,A
        LIS C_WHITE
        LR 6,A
        PI PUTNUM
        CLR
        STB DTMP+0
        LDW 0, 1, ST+A_SBUY
        LIS 4
        LR 4,A
        LI 32
        LR 2,A
        LIS C_WHITE
        LR 6,A
        PI PUTNUM
ALSX:   LEAVE

; AUMONEY -- money and (goods) units under the seats (au_money): only when
;   a player's changed.
AUMONEY: ENTER
        LDB AU_MALL
        NI 0FFH
        BNZ AM1
        DCI ST+PLAYERS
        XDC
        DCI PREV+PLAYERS
        LI 4*PLSIZE
        LR 0,A
        PI MEMCMP
        NI 0FFH
        JZ AMX
AM1:    CLR
        STB AU_MALL
        STB AU_I
AM2:    LDB AU_I
        LR 0,A
        PI PLCOLR
        LR 6,A
        LDB AU_I
        SL 1
        LR 4,A
        SL 1
        AS 4
        STB DTMP+0
        LDB AU_I
        LR 0,A
        PI PLDC
        LI P_MONEY
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LIS 5
        LR 4,A
        LI AUMONY
        LR 2,A
        PI PUTNUM
        LDB AU_KIND             ; units: goods, the lines under way
        CI 2
        BNZ AM3
        LDB AU_STAGE
        CI STAGE_LEAD-1
        BC AM3
        LDB AU_I
        SL 1
        LR 4,A
        SL 1
        AS 4
        INC
        STB DTMP+0
        LDB AU_GOOD
        AI P_GOODS
        LR 5,A
        LDB AU_I
        LR 0,A
        PI PLDC
        LR A,5
        ADC
        LM
        LR 0,A
        CLR
        LR 1,A
        LIS 4
        LR 4,A
        LI AUUNTY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        PI PUTNUM
AM3:    INCB AU_I
        CI 3
        JC AM2
AMX:    LEAVE

; AUTICKS -- A = the auction clock's ticks left now (au_ticks_now): the
;   tick under way has tickLeft jiffies left, each after it 25 (6 idle and
;   declaring).
AUTICKS: ENTER
        LI 25
        LR 6,A
        LDB AU_STAGE
        CI STAGE_DECLARE
        BZ AT1
        LDB ST+A_FLAGS
        NI AF_IDLE
        BZ AT2
AT1:    LIS 6
        LR 6,A
AT2:    LDW 0, 1, FRAMES        ; since the state, as if its tick were whole
        LDW 2, 3, RXFRAME
        PI SUB16
        LR A,6
        LR 2,A
        CLR
        LR 3,A
        PI ADD16
        LDB ST+A_TLEFT
        LR 2,A
        CLR
        LR 3,A
        PI GE16
        BNC AT3Z
        PI SUB16
        LR A,6
        LR 2,A
        PI DIV8
        BR AT4
AT3Z:   CLR
        LR 0,A
        LR 1,A
AT4:    LR A,1
        NI 0FFH
        BNZ AT5Z
        LDB ST+A_TICKS
        LR 4,A
        LR A,0
        COM
        AS 4                    ; ticks + ~el carries: ticks > el
        BNC AT5Z
        INC
        LEAVE
AT5Z:   CLR
        LEAVE

; AUTBAR -- the time bar (au_timebar).
AUTBAR: ENTER
        LIS C_WHITE
        STB BARCOL
        CLR
        LR 0,A
        LDB AU_STAGE
        CI STAGE_ACTIVE
        BZ ATB1
        CI STAGE_DECLARE
        BNZ ATB2
ATB1:   PI AUTICKS
        LR 0,A
ATB2:   LR A,0
        STB AU_K
        PI BARTICK
        LDB AU_K
        LR 0,A
        PI BARDRAW
        LEAVE

; AUMSG -- the message (au_message).
AUMSG:  ENTER
        LI MSGY
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        LIS 1
        STB MSGONE
        LI 0FFH
        LR 5,A
        LDB AU_STAGE
        CI STAGE_DECLARE
        BNZ AMS1
        LDB MYSEAT
        CI 3
        BNC AMS1
        LI MS_DECLARE
        LR 5,A
AMS1:   LDB AU_STAGE
        CI STAGE_DONE
        BNZ AMS2
        LI MS_AUCOVER
        LR 5,A
        LDB AU_KIND
        CI 1
        BNZ AMS2
        LDB ST+A_WIN
        CI 3
        LI MS_SOLD
        BC AMS1A
        LI MS_NOBIDS
AMS1A:  LR 5,A
AMS2:   LDB AU_KIND
        CI 2
        BNZ AMS3
        LDB AU_STAGE
        CI STAGE_ACTIVE
        BNZ AMS3
        LDB ST+A_FLAGS
        NI AF_COLL
        BZ AMS2A
        LI MS_COLLUDE
        LR 5,A
AMS2A:  LDB ST+A_FLAGS
        NI AF_CRIT
        BZ AMS3
        LI MS_CRITICAL
        LR 5,A
AMS3:   LDB AU_KIND             ; land, bidding: the bottom of the scale
        CI 1
        BNZ AMS4
        LDB AU_STAGE
        CI STAGE_ACTIVE
        BNZ AMS4
        LDB AU_MSG
        XS 5
        BNZ AMS3A
        LDW 0, 1, AU_BASE
        LDW 2, 3, AU_SB
        LR A,0
        XS 2
        BNZ AMS3A
        LR A,1
        XS 3
        BZ AMSX
AMS3A:  LR A,5
        STB AU_MSG
        LDW 0, 1, AU_BASE
        STW 0, 1, AU_SB
        STW 0, 1, EXPV1
        CLR
        STB EXPV2
        ST
        LI MS_BIDSTART
        LR 0,A
        PI MSGSHOW
        BR AMSX
AMS4:   LDB AU_MSG
        XS 5
        BZ AMSX
        LR A,5
        STB AU_MSG
        CI 0FFH
        BNZ AMS5
        PI MSGCLR
        BR AMSX
AMS5:   LR 0,A
        CLR
        STB EXPV1
        ST
        STB EXPV2
        ST
        PI MSGSHOW
AMSX:   LEAVE

; AULAND -- the land auction's price line (au_land_line): the opening
;   price, then the high bid, then what it sold for. On the units' row.
AULAND: ENTER
        LDB AU_KIND
        CI 1
        JNZ ALDX
        LI MS_BIDSTART
        STB AU_K
        LDW 6, 7, ST+A_START
        CLR
        STB AU_I
ALD1:   LDB AU_I
        DCI AU_ROLE
        ADC
        LM
        CI ROLE_BUY
        BNZ ALD2
        LDB AU_I
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LR A,6
        LR 2,A
        LR A,7
        LR 3,A
        PI GE16
        BNC ALD2
        LR A,0
        LR 6,A
        LR A,1
        LR 7,A
        LI MS_HIGHBID
        STB AU_K
ALD2:   INCB AU_I
        CI 3
        BC ALD1
        LDB AU_STAGE
        CI STAGE_DONE
        BNZ ALD3
        LDW 0, 1, ST+A_PRICE
        LR A,0
        NI 0FFH
        BNZ ALD2A
        LR A,1
        NI 0FFH
        BZ ALD3
ALD2A:  LR A,0
        LR 6,A
        LR A,1
        LR 7,A
        LI MS_SALEPRICE
        STB AU_K
ALD3:   LDW 0, 1, AU_HB
        LR A,0
        XS 6
        BNZ ALD4
        LR A,1
        XS 7
        BZ ALDX
ALD4:   STW 6, 7, AU_HB
        STW 6, 7, EXPV1
        CLR
        STB EXPV2
        ST
        LDB AU_K
        LR 0,A
        DCI STM
        PI STRP
        XDC
        DCI LINBUF
        XDC
        PI TXTEXP
        LI AUUNTY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI LINBUF
        PI TCENTER
ALDX:   LEAVE

; AUBLINK -- who deals blinks (au_blinkers): in a land auction the high
;   bidder (the winner once sold); in a goods auction the buyer and seller
;   whose lines meet, or whoever meets the store's line.
AUBLINK: ENTER
        CLR
        STB AU_BLINK
        LDB AU_STAGE
        CI STAGE_ACTIVE-1
        JC ABX
        LDB AU_KIND
        CI 1
        JNZ ABG
        LDB AU_STAGE            ; land
        CI STAGE_DONE
        BNZ ABL1
        LDB ST+A_WIN
        CI 3
        JNC ABX
        DCI BITS
        ADC
        LM
        STB AU_BLINK
        JMP ABX
ABL1:   LI 0FFH
        STB AU_J
        LDW 6, 7, ST+A_START
        CLR
        STB AU_I
ABL2:   LDB AU_I
        DCI AU_ROLE
        ADC
        LM
        CI ROLE_BUY
        BNZ ABL3
        LDB AU_I
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LR A,6
        LR 2,A
        LR A,7
        LR 3,A
        PI GE16
        BNC ABL3
        LR A,0
        LR 6,A
        LR A,1
        LR 7,A
        LDB AU_I
        STB AU_J
ABL3:   INCB AU_I
        CI 3
        BC ABL2
        LDB AU_J
        CI 3
        JNC ABX
        DCI BITS
        ADC
        LM
        STB AU_BLINK
        JMP ABX
ABG:    LDB AU_STAGE            ; goods, under way
        CI STAGE_ACTIVE
        JNZ ABX
        CLR                     ; w: the highest bid, AU_V: the lowest ask
        LR 6,A
        LR 7,A
        LI 0FFH
        STB AU_V
        ST
        STB AU_J
        STB AU_K
        CLR
        STB AU_I
ABG1:   LDB AU_I
        DCI AU_ROLE
        ADC
        LM
        STB AU_C
        LDB AU_I
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LDB AU_C
        CI ROLE_BUY
        BNZ ABG2
        LR A,6
        LR 2,A
        LR A,7
        LR 3,A
        PI GE16
        BNC ABGN
        LR A,0
        LR 6,A
        LR A,1
        LR 7,A
        LDB AU_I
        STB AU_J
        BR ABGN
ABG2:   CI ROLE_SELL
        BNZ ABGN
        LR A,0
        LR 2,A
        LR A,1
        LR 3,A
        LDW 0, 1, AU_V
        PI GE16                 ; lo >= pos
        BNC ABGN
        STW 2, 3, AU_V
        LDB AU_I
        STB AU_K
ABGN:   INCB AU_I
        CI 3
        JC ABG1
        LDB AU_J                ; a buyer and a seller meet
        CI 3
        BNC ABG4
        LDB AU_K
        CI 3
        BNC ABG4
        LR A,6
        LR 0,A
        LR A,7
        LR 1,A
        LDW 2, 3, AU_V
        PI GE16
        BNC ABG4
        LDB AU_J
        DCI BITS
        ADC
        LM
        LR 4,A
        LDB AU_K
        DCI BITS
        ADC
        LM
        AS 4
        STB AU_BLINK
        JMP ABX
ABG4:   LDB AU_J                ; a buyer at the store's selling price
        CI 3
        BNC ABG5
        LR A,6
        LR 0,A
        LR A,7
        LR 1,A
        LDW 2, 3, ST+A_SSELL
        PI GE16
        BNC ABG5
        LDB AU_J
        DCI BITS
        ADC
        LM
        STB AU_BLINK
ABG5:   LDB AU_K                ; a seller at its buying price
        CI 3
        JNC ABX
        LDW 0, 1, ST+A_SBUY
        LDW 2, 3, AU_V
        PI GE16
        JNC ABX
        LDB AU_K
        DCI BITS
        ADC
        LM
        LR 4,A
        LDB AU_BLINK
        AS 4
        STB AU_BLINK
ABX:    LEAVE

; ---------------------------------------------------------------------------
; The per-good status (stage 0, 561 jiffies): previous amount, usage,
; spoilage, production, then what the store holds; each seat's bar (a row
; a unit, 20 at most) stands over it.
; ---------------------------------------------------------------------------

; AUNEED -- A = the units of AU_GOOD seat r0 needs in round r1 (au_need):
;   food by round (none after 12), energy one for each of its other
;   M.U.L.E.s.  Leaf.  Clobbers r4, r5, DC0.
AUNEED: CLR
        LR 5,A
        LDB AU_GOOD
        CI GOOD_FOOD
        BNZ AN2
        LR A,1
        CI 12
        BNC AN9
        LIS 3
        LR 5,A
        LR A,1
        CI 3
        BC AN9
        LIS 4
        LR 5,A
        LR A,1
        CI 7
        BC AN9
        LIS 5
        LR 5,A
        BR AN9
AN2:    CI GOOD_ENERGY
        BNZ AN9
        CLR
        LR 4,A
AN3:    DCI ST+M_OWNER
        LR A,4
        ADC
        LM
        XS 0
        BNZ AN4
        DCI ST+M_MULE
        LR A,4
        ADC
        LM
        CI 3
        BNC AN4
        CI GOOD_ENERGY
        BZ AN4
        LR A,5
        INC
        LR 5,A
AN4:    LR A,4
        INC
        LR 4,A
        CI NPLOTS-1
        BC AN3
AN9:    LR A,5
        POP

; AUBARD -- bar r0, which was r1 rows high, now AU_BAR[r0]: only the rows
;   between change (au_bar_draw).
AUBARD: ENTER
        LR A,1
        STB AU_F
        LR A,0
        STB AU_X
        PI PLCOLR
        STB AU_C
        LDB AU_X
        DCI AU_BAR
        ADC
        LM
        STB AU_N
        LDB AU_X
        DCI AUCFX
        ADC
        LM
        LR 1,A
        LDB AU_N
        LR 4,A
        LDB AU_F
        LR 5,A
        COM
        AS 4                    ; n + ~was carries: n > was
        BNC ABD2
        LR A,4                  ; taller: from AUBARB + 1 - n, n - was rows
        COM
        INC
        AI AUBARB+1
        LR 2,A
        LR A,5
        COM
        INC
        AS 4
        LR 4,A
        LIS 5
        LR 3,A
        LDB AU_C
        LR 0,A
        PI DFILL
        LEAVE
ABD2:   LR A,4
        XS 5
        BZ ABDX
        LR A,5                  ; shorter: from AUBARB + 1 - was, was - n
        COM
        INC
        AI AUBARB+1
        LR 2,A
        LR A,4
        COM
        INC
        AS 5
        LR 4,A
        LIS 5
        LR 3,A
        CLR
        LR 0,A
        PI DFILL
ABDX:   LEAVE

; AUSTGT -- the bars' targets for the step shown (au_status_targets):
;   previous = now - production + spoilage + usage, then less usage, then
;   less spoilage... in units.
AUSTGT: ENTER
        CLR
        STB AU_I
SGT1:   LDB AU_GOOD             ; w, c (production), j (spoilage), n (usage)
        AI P_GOODS
        LR 5,A
        LDB AU_I
        LR 0,A
        PI PLDC
        LR A,5
        ADC
        LM
        STB AU_W
        LDB AU_GOOD
        AI P_PROD
        LR 5,A
        LDB AU_I
        LR 0,A
        PI PLDC
        LR A,5
        ADC
        LM
        STB AU_C
        LDB AU_GOOD
        AI P_SPOIL
        LR 5,A
        LDB AU_I
        LR 0,A
        PI PLDC
        LR A,5
        ADC
        LM
        STB AU_J
        LDB RND
        LR 1,A
        LDB AU_I
        LR 0,A
        PI AUNEED
        STB AU_N
        LDB AU_W                ; a = w + j + n (at most 255), less c
        LR 4,A
        LDB AU_J
        AS 4
        BNC SGT2
        LI 0FFH
SGT2:   LR 4,A
        LDB AU_N
        AS 4
        BNC SGT3
        LI 0FFH
SGT3:   LR 4,A
        LDB AU_C
        LR 6,A
        PI SUBSAT
        LR 4,A
        LDB AU_SHOWT
        CI 1
        BNZ SGT4
        LDB AU_N                ; usage gone
        LR 6,A
        PI SUBSAT
        LR 4,A
SGT4:   LDB AU_SHOWT
        CI 2
        BNZ SGT5
        LDB AU_W                ; spoilage gone: w - c
        LR 4,A
        LDB AU_C
        LR 6,A
        PI SUBSAT
        LR 4,A
SGT5:   LDB AU_SHOWT
        CI 2
        BC SGT6
        LDB AU_W                ; production in: now
        LR 4,A
SGT6:   LR A,4                  ; 20 rows at most
        CI 20
        BC SGT7
        LI 20
SGT7:   LR 4,A
        LDB AU_I
        DCI AU_BART
        ADC
        LR A,4
        ST
        INCB AU_I
        CI 3
        JC SGT1
        LEAVE

; SUBSAT -- A = r4 - r6, or 0 if that is below 0.  Leaf.
SUBSAT: LR A,4
        COM
        AS 6                    ; r6 + ~r4 carries: r6 > r4
        BC SBS0
        LR A,6
        COM
        INC
        AS 4
        POP
SBS0:   CLR
        POP

; AUNEEDD -- once the store's stock is shown (au_need_draw): what each seat
;   needs next round, dotted beside its bar at that height, and under it how
;   far short (-) or over (+).
AUNEEDD: ENTER
        CLR
        STB AU_I
AND1:   LDB RND
        INC
        LR 1,A
        LDB AU_I
        LR 0,A
        PI AUNEED
        NI 0FFH
        JZ ANDN
        STB AU_N
        LDB AU_GOOD
        AI P_GOODS
        LR 5,A
        LDB AU_I
        LR 0,A
        PI PLDC
        LR A,5
        ADC
        LM
        STB AU_W
        LDB AU_N                ; the need's row (20 at most)
        CI 20
        BC AND2
        LI 20
AND2:   COM
        INC
        AI AUBARB+1
        STB AU_Y
        LDB AU_I
        DCI AUCFX
        ADC
        LM
        AI 0FBH                 ; x - 5
        LR 1,A
        LDB AU_Y
        LR 2,A
        LIS 4
        LR 3,A
        LIS C_WHITE
        LR 0,A
        PI DDOTS
        LDB AU_I
        DCI AUCFX
        ADC
        LM
        AI 7
        LR 1,A
        LDB AU_Y
        LR 2,A
        LIS 4
        LR 3,A
        LIS C_WHITE
        LR 0,A
        PI DDOTS
        LDB AU_N                ; over or short
        LR 4,A
        LDB AU_W
        LR 5,A
        COM
        AS 4                    ; n + ~w carries: n > w, short
        BC AND3
        LR A,4
        COM
        INC
        AS 5
        STB AU_V
        LI '+'
        STB AU_F
        LIS C_WHITE
        STB AU_C
        BR AND4
AND3:   LR A,5
        COM
        INC
        AS 4
        STB AU_V
        LI '-'
        STB AU_F
        LIS C_ORANGE
        STB AU_C
AND4:   CLR
        STB AU_V+1
        LDB AU_I
        LR 0,A
        PI AUFIELD
        LDW 0, 1, AU_V
        LDB AU_F
        DCI LINBUF
        ST
        PI FMTNUM
        LDB DTMP+0
        INC
        SL 1
        SL 1
        AI TX0
        LR 1,A
        LI AULBLY
        LR 2,A
        LDB AU_C
        LR 6,A
        DCI LINBUF
        PI DTEXT
ANDN:   INCB AU_I
        CI 3
        JC AND1
        LEAVE

; AUSSET -- the status's start (au_status_setup): everyone at the bottom.
AUSSET: ENTER
        LIS ROLE_BUY
        DCI AU_ROLE
        ST
        ST
        ST
        ST
        CLR
        STB AU_STAGE
        STB AU_BLINK
        PI AUDRAW
        DCI ST+A_ROLE
        XDC
        DCI AU_ROLE
        XDC
        LIS 4
        LR 0,A
        CLR
        LR 1,A
        PI MEMCPY
        CLR
        DCI AU_BAR              ; (AU_BART follows)
        ST
        ST
        ST
        ST
        ST
        ST
        ST
        ST
        LI 0FFH
        STB AU_SHOWT
        LEAVE

; AUSTICK -- every frame of the status (au_status_tick).
AUSTICK: ENTER
        PI PENOW                ; the step: 127, 250, 373, 496 jiffies in
        CLR
        LR 6,A
        DCI STHR
AST1:   LM
        LR 3,A
        LM
        LR 2,A
        PI GE16
        BNC AST2
        LR A,6
        INC
        LR 6,A
        CI 4
        BNZ AST1
AST2:   LR A,6
        LR 4,A
        LDB AU_SHOWT
        XS 4
        JZ AST5
        LR A,4
        STB AU_SHOWT
        PI AUSTGT
        LI MSGY
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        LIS 1
        STB MSGONE
        LDB AU_SHOWT
        CI 4
        BZ AST3
        AI MS_PREVIOUS
        LR 0,A
        CLR
        STB EXPV1
        ST
        STB EXPV2
        ST
        PI MSGSHOW
        BR AST5
AST3:   LDB AU_GOOD             ; what the store holds
        DCI ST+S_STOCK
        ADC
        LM
        STB EXPV1
        LR 4,A
        CLR
        STB EXPV1+1
        STB EXPV2
        ST
        LR A,4
        NI 0FFH
        LI MS_STOREHAS
        BNZ AST4
        LI MS_STORENO
AST4:   LR 0,A
        PI MSGSHOW
        PI AUNEEDD
AST5:   CLR                     ; the bars: a row a frame
        STB AU_I
AST6:   LDB AU_I
        DCI AU_BAR
        ADC
        LM
        LR 4,A
        LDB AU_I
        DCI AU_BART
        ADC
        LM
        LR 5,A
        LR A,4
        XS 5
        BZ AST8
        LR A,5
        COM
        AS 4                    ; k + ~t carries: k > t
        LR A,4
        BC AST7
        INC
        BR AST7A
AST7:   AI 0FFH
AST7A:  LR 6,A
        LDB AU_I
        DCI AU_BAR
        ADC
        LR A,6
        ST
        LDB AU_I
        LR 0,A
        LR A,4
        LR 1,A
        PI AUBARD
AST8:   INCB AU_I
        CI 3
        BC AST6
        LEAVE

; ---------------------------------------------------------------------------
; Screen setup
; ---------------------------------------------------------------------------

; AUENTER -- the auction screen (auction_enter).
AUENTER: ENTER
        CLR
        STB ANIM
        STB AU_ON
        STB SPCOL
        PI FXSTOP
        PI DCLS
        LI 10
        LR 4,A
        LI 55
        LR 5,A
        PI BANDS3
        LIS SCR_AUC
        STB SCRNOW
        LI 0FFH
        STB AU_LN1
        STB AU_LN2
        STB AU_SHOWT
        STB AU_MSG
        STB AU_HB
        ST
        LI 97
        STB BARX
        LI 10
        STB BARTOP
        LI 28
        STB BARH
        LI 0FFH
        STB BARLAST
        LEAVE

; AUTITLE -- "STATUS #n FOOD" / "AUCTION #n FOOD", or the land's title.
AUTITLE: ENTER
        LDB AU_KIND
        CI 1
        BNZ AUT1
        LI TI_LANDAUCTION
        LR 0,A
        LDB RND
        LR 1,A
        PI TITLE
        LEAVE
AUT1:   LDB RND
        LR 7,A
        LDB AU_STAGE
        CI STAGE_DECLARE
        LI WD_STATUS
        BC AUT2
        LI WD_AUCTION
AUT2:   LR 0,A
        DCI STW
        PI STRP
        XDC
        DCI LINBUF
        XDC
        PI STRCPY
        XDC
        LR A,7
        LR 0,A
        CLR
        LR 1,A
        PI FMTNUM
        LI 0FFH                 ; (FMTNUM left DC0 past its NUL)
        ADC
        LI ' '
        ST
        XDC
        LDB AU_GOOD
        LR 0,A
        DCI STG
        PI STRP
        PI STRCPY
        LI TITLEY
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI LINBUF
        PI TCENTER
        LEAVE

; AUSBEG -- a new stage (au_stage_begin): all but the title cleared.
AUSBEG: ENTER
        LIS VX0
        LR 1,A
        LI 10
        LR 2,A
        LI VX1-VX0+1
        LR 3,A
        LI 52
        LR 4,A
        CLR
        LR 0,A
        PI DFILL
        LI 0FFH
        STB AU_LN1
        STB AU_LN2
        STB AU_MSG
        STB BARLAST
        STB AU_SB
        ST
        STB AU_HB
        ST
        LIS 1
        STB AU_MALL
        LDB AU_STAGE            ; a new plot or good: no push from us yet
        NI 0FFH
        BNZ ASB1
        CLR
        STB AU_PUSH
        LDB AU_KIND
        CI 2
        BNZ ASB1
        CLR
        STB AU_ON
        STB ANIM
        PI AUSSET
        LEAVE
ASB1:   LDB AU_STAGE            ; declaring, and the lines about to go: your
        CI STAGE_DECLARE        ; hand to the stick
        BZ ASB2
        CI STAGE_LEAD
        BNZ ASB3
ASB2:   LDB MYSEAT
        CI 3
        BNC ASB3
        LIS SND_ALERT
        LR 0,A
        PI SNDPLAY
ASB3:   LDB AU_STAGE
        CI STAGE_DONE
        BNZ ASB4
        LIS SND_AUOVER
        LR 0,A
        PI SNDPLAY
ASB4:   PI AULABELS
        LEAVE

; LAUCE -- landauc_enter.
LAUCE:  ENTER
        LI 0FFH
        STB AU_STAGE
        CLR
        STB AU_PUSH
        STB AUTRADE
        STB MAPDOTS
        LEAVE

; GOODSE -- goods_enter: declaring and trading share the screen; a new good
;   starts it afresh.
GOODSE: ENTER
        LDB LASTPH
        CI PH_DECL
        BZ GSE1
        CI PH_TRADE
        BZ GSE1
        CLR
        STB SCRNOW
GSE1:   LDB PHASE
        CI PH_DECL
        BNZ GSE2
        CLR
        STB SCRNOW
GSE2:   LI 0FFH
        STB AU_STAGE
        CLR
        STB AU_PUSH
        STB AUTRADE
        LEAVE

; ---------------------------------------------------------------------------
; AUCS -- a new state in phases 2, 5 or 6 (auction_state).
; ---------------------------------------------------------------------------
AUCS:   ENTER
        LDB ST+A_KIND
        STB AU_KIND
        LDB ST+A_STAGE
        STB AU_NEW
        LDB ST+A_GOOD
        NI 3
        STB AU_GOOD
        LDB ST+A_STEP
        NI 0FFH
        BNZ AC1
        LIS 1
AC1:    STB AU_STEPV
        LR 2,A                  ; lim = step * 30
        LI 30
        LR 0,A
        CLR
        LR 1,A
        PI MUL8
        STW 0, 1, AU_LIM
        DCI ST+A_ROLE
        XDC
        DCI AU_ROLE
        XDC
        LIS 4
        LR 0,A
        CLR
        LR 1,A
        PI MEMCPY
        LDB AU_KIND             ; the land auction opens with its plot
        CI 1                    ; blinking on the map
        JNZ AC3
        LDB AU_NEW
        NI 0FFH
        JNZ AC3
        LDB SCRNOW
        CI SCR_MAP
        BNZ AC2A
        LDB AU_STAGE
        NI 0FFH
        BZ AC2B
AC2A:   LDB SCRNOW
        CI SCR_MAP
        BZ AC2C
        PI MAPENTER
AC2C:   LI TI_LANDAUCTION
        LR 0,A
        LDB RND
        LR 1,A
        PI TITLE
        LI MSGY
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        CLR
        STB MSGONE
        LDW 0, 1, ST+A_START
        STW 0, 1, EXPV1
        CLR
        STB EXPV2
        ST
        LI MS_BIDSTART
        LR 0,A
        PI MSGSHOW
AC2B:   LDB AU_NEW
        STB AU_STAGE
        LDB ST+A_PLOT
        STB MAPBLNK
        LEAVE
AC3:    LDB MAPBLNK             ; off the map: its plot plain again
        CI NPLOTS-1
        BNC AC3A
        LR 4,A
        LDB SCRNOW
        CI SCR_MAP
        BNZ AC3A
        LR A,4
        LR 0,A
        LI NOBODY
        STB MAPBLNK
        PI MAPPLOT
AC3A:   LI NOBODY
        STB MAPBLNK
        LDB SCRNOW              ; a new screen, or a new stage
        CI SCR_AUC
        BNZ AC4
        LDB AU_NEW
        LR 4,A
        LDB AU_STAGE
        XS 4
        BZ AC5
AC4:    LDB SCRNOW
        CI SCR_AUC
        BZ AC4A
        PI AUENTER
AC4A:   LDB AU_NEW
        STB AU_STAGE
        PI AUTITLE
        PI AUSBEG
AC5:    LDB AU_KIND             ; the base: land, reset only for a new plot
        CI 1                    ; (the scale scrolls up with the bidding)
        BNZ AC5B
        LDB AU_STAGE
        CI STAGE_ACTIVE-1
        BC AC5A
        LDW 0, 1, AU_BASE
        LDW 2, 3, ST+A_START
        PI GE16
        BC AC6
AC5A:   LDW 0, 1, ST+A_START
        STW 0, 1, AU_BASE
        BR AC6
AC5B:   LDW 0, 1, ST+A_SBUY
        STW 0, 1, AU_BASE
AC6:    LI 0FFH                 ; our line let go: it stays put a second
        STB AU_K                ; while the server's catches up
        LDB AU_STAGE
        CI STAGE_ACTIVE
        BNZ AC7
        LDB AU_PUSH
        NI 0FFH
        BNZ AC7
        LDB AU_HELD
        COM
        INC
        LR 4,A
        LDB FRAMES
        AS 4
        CI 59
        BNC AC7
        LDB MYSEAT
        STB AU_K
AC7:    LDB ST+H_T              ; the server steps the lines every 8th jiffy:
        NI 7                    ; ours in time, moved on by the steps the
        LR 4,A                  ; state (rx_age old) missed
        LDB RXAGE
        AS 4
        LR 4,A
        NI 7
        STB AU_TICK
        LDB AU_STAGE
        CI STAGE_ACTIVE
        LIS 0
        BNZ AC7A
        LR A,4
        SR 1
        SR 1
        SR 1
AC7A:   STB AU_J
        LI 0FFH                 ; critical: the seller's line is held
        STB AU_C
        LDB AU_KIND
        CI 2
        BNZ AC8
        LDB ST+A_FLAGS
        NI AF_CRIT
        BZ AC8
        LDB ST+A_SELLR
        STB AU_C
AC8:    CLR
        STB AU_I
AC9:    LDB AU_I
        LR 4,A
        LDB AU_K
        XS 4
        BNZ AC10
        LDB AU_I                ; ours, let go: until the server's rests
        DCI ST+A_PUSH
        ADC
        LM
        NI 0FFH
        JNZ ACN
        LDB AU_I
        SL 1
        DCI ST+A_POS
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LDB AU_I
        SL 1
        DCI ST+A_TGT
        ADC
        LM
        XS 0
        JNZ ACN
        LM
        XS 1
        JNZ ACN
AC10:   LDB AU_I                ; snap; the frame handler walks it on
        SL 1
        DCI ST+A_POS
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
        LDB AU_I
        SL 1
        DCI AU_POS
        ADC
        LR A,0
        ST
        LR A,1
        ST
        LDB AU_I
        LR 4,A
        LDB AU_C
        XS 4
        BZ AC11
        LDB AU_I
        SL 1
        DCI ST+A_TGT
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
AC11:   LDB AU_I
        SL 1
        DCI AU_TGT
        ADC
        LR A,0
        ST
        LR A,1
        ST
        LDB AU_J
        STB AU_N
AC12:   LDB AU_N
        NI 0FFH
        BZ ACN
        AI 0FFH
        STB AU_N
        LDB AU_I
        LR 0,A
        PI AUSTEP
        BR AC12
ACN:    INCB AU_I
        CI 3
        JC AC9
        LDB AU_STAGE
        CI STAGE_LEAD-1
        BC AC13
        LIS 1
        STB AU_ON
        LIS AN_AUC
        STB ANIM
AC13:   LDB AU_KIND
        CI 2
        BNZ AC14
        LDB AU_STAGE
        NI 0FFH
        JZ ACX
AC14:   PI AUMONEY
        PI AULINES
        PI AUTBAR
        PI AUMSG
        PI AULAND
        PI AUBLINK
        PI AUDRAW
        LDB ST+A_TRADE          ; a trade: its sound
        LR 4,A
        LDB AUTRADE
        XS 4
        BZ ACX
        LDB AUTRADE
        COM
        AS 4                    ; traded + ~ours carries: more
        BNC AC15
        LIS SND_TRADE
        LR 0,A
        PI SNDPLAY
AC15:   LDB ST+A_TRADE
        STB AUTRADE
ACX:    LEAVE

; ---------------------------------------------------------------------------
; AUSTK -- our stick to the server when it changes (au_stick): up, down,
;   let go. Our own line moves at once; the server confirms.
; ---------------------------------------------------------------------------
AUSTK:  ENTER
        CLR
        LR 5,A
        LDB INPDIR
        NI DIR_UP
        BZ AK1
        LIS 1
        LR 5,A
AK1:    LDB INPDIR
        NI DIR_DOWN
        BZ AK2
        LIS 2
        LR 5,A
AK2:    LDB AU_PUSH
        XS 5
        JZ AKX
        LDB ACT
        NI 0FFH
        JNZ AKX
        LR A,5
        STB AU_PUSH
        NI 0FFH
        BNZ AK3
        LDB AU_STAGE
        CI STAGE_ACTIVE
        BNZ AK3
        LDB MYSEAT              ; let go under way: hold where it is
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 1,A
        LM
        LR 2,A
        LI ACT_HOLD
        LR 0,A
        PI ACTSEND
        LDB FRAMES
        STB AU_HELD
        BR AK4
AK3:    LI ACT_PUSH
        LR 0,A
        LDB AU_PUSH
        LR 1,A
        CLR
        LR 2,A
        PI ACTSEND
AK4:    LDB AU_STAGE
        CI STAGE_ACTIVE
        JNZ AKX
        LDB AU_PUSH
        CI 1
        BNZ AK6
        LDB AU_KIND
        CI 1
        BNZ AK5
        LDB MYSEAT              ; land, up: as high as our money goes
        LR 0,A
        PI PLDC
        LI P_MONEY
        ADC
        LM
        NI 0FCH
        LR 0,A
        LM
        LR 1,A
        BR AKT
AK5:    LDW 0, 1, ST+A_HI
        BR AKT
AK6:    CI 2
        BNZ AK7
        LDW 0, 1, ST+A_LO
        BR AKT
AK7:    LDB MYSEAT              ; let go: where it is
        SL 1
        DCI AU_POS
        ADC
        LM
        LR 0,A
        LM
        LR 1,A
AKT:    LDB MYSEAT
        SL 1
        DCI AU_TGT
        ADC
        LR A,0
        ST
        LR A,1
        ST
AKX:    LEAVE

; ---------------------------------------------------------------------------
; AUCT -- every frame (auction_tick).
; ---------------------------------------------------------------------------
AUCT:   ENTER
        LDB AU_KIND
        CI 1
        JNZ AT10
        LDB AU_STAGE
        NI 0FFH
        JNZ AT10
        LDB SCRNOW              ; the plot for sale blinks on the map, 16
        CI SCR_MAP              ; frames on and 16 off
        BNZ AT2X
        LDB MAPBLNK
        CI NPLOTS-1
        BNC AT2X
        LDB FRAMES
        SR 4
        NI 1
        LR 4,A
        LDB MAPBON
        XS 4
        BZ AT2X
        LR A,4
        STB MAPBON
        LDB MAPBLNK
        LR 0,A
        PI MAPPLOT
AT2X:   PI MSGTICK
        LDB MYSEAT
        CI 3
        JNC ATX
        LDB EVBTN               ; the button skips the wait
        NI 0FFH
        BZ AT3X
        LIS ACT_READY
        LR 0,A
        CLR
        LR 1,A
        LR 2,A
        PI ACTSEND
AT3X:   PI AUSTK
        JMP ATX
AT10:   LDB SCRNOW
        CI SCR_AUC
        JNZ ATX
        PI MSGTICK
        LDB AU_KIND
        CI 2
        BNZ AT11
        LDB AU_STAGE
        NI 0FFH
        BNZ AT11
        PI AUSTICK
        JMP ATX
AT11:   PI AULINES
        PI AULBLT
        PI AUTBAR
        LDB AU_STAGE
        CI STAGE_DECLARE
        JNZ AT20
        LDB MYSEAT
        CI 3
        JNC AT20
        LDB MYSEAT              ; the stick declares: up seller, down buyer
        DCI AU_ROLE
        ADC
        LM
        STB AU_K
        LDB INPDIR
        NI DIR_UP
        BZ AT12
        LDB AU_K
        CI ROLE_SELL
        BZ AT12
        LIS ROLE_SELL
        LR 4,A
        LIS 2
        BR AT13
AT12:   LDB INPDIR
        NI DIR_DOWN
        BZ AT14
        LDB AU_K
        CI ROLE_BUY
        BZ AT14
        LIS ROLE_BUY
        LR 4,A
        LIS 1
AT13:   LR 1,A
        LDB MYSEAT
        DCI AU_ROLE
        ADC
        LR A,4
        ST
        LIS ACT_DECL
        LR 0,A
        CLR
        LR 2,A
        PI ACTSEND
        PI AULABELS
        LIS SND_SELECT
        LR 0,A
        PI SNDPLAY
AT14:   LDB EVKEY               ; HOLD: sit this one out
        CI KEY_SITOUT
        BNZ AT16
        LDB AU_K
        NI 0FFH
        BNZ AT15
        LDB MYSEAT
        LR 0,A
        PI PLDC
        LI P_FLAGS
        ADC
        LM
        NI PF_DECL
        BNZ AT16
AT15:   LDB MYSEAT
        DCI AU_ROLE
        ADC
        CLR
        ST
        LIS ACT_DECL
        LR 0,A
        CLR
        LR 1,A
        LR 2,A
        PI ACTSEND
        PI AULABELS
AT16:   PI AUDRAW
        JMP ATX
AT20:   LDB AU_STAGE
        CI STAGE_ACTIVE
        BZ AT21
        CI STAGE_LEAD
        BNZ ATX
AT21:   LDB MYSEAT
        CI 3
        BNC ATX
        PI AUSTK
        LDB EVBTN               ; the button in a goods auction: collusion
        NI 0FFH
        BZ ATX
        LDB AU_KIND
        CI 2
        BNZ ATX
        LDB AU_STAGE
        CI STAGE_ACTIVE
        BNZ ATX
        LIS ACT_COLL
        LR 0,A
        CLR
        LR 1,A
        LR 2,A
        PI ACTSEND
ATX:    LEAVE
