; auram.inc -- the auctions' bookkeeping, shared by the status page (p_auc)
; and the arena (p_arena): they hand the phase to each other.

        ZONE    UNIONL,UNIONLE
        VAR     AUSTG,1         ; the stage on the screen (255 none)
        VAR     AUKIND,1        ; 1 land, 2 goods
        VAR     AUGOOD,1
        VAR     AUSTEPV,1       ; dollars a step (1 at least)
        VAR     AUROLE,4        ; ROLE_* as we show them
        VAR     AUPOS,8         ; each line's price (hi, lo), as walked here
        VAR     AUTGT,8         ; and where it walks to
        VAR     AUBASEH,1       ; the scale's base price
        VAR     AUBASEL,1
        VAR     AUTKS,1         ; frames to the lines' next step (of 8)
        VAR     AUPH,1          ; steps taken (the walk frames, blinking)
        VAR     AUON,1          ; 1: the lines walk
        VAR     AUPUSH,1        ; our stick as sent: 0 let go, 1 up, 2 down
        VAR     AUHELD,1        ; the frame (low byte) our line was let go
        VAR     AULN1,1         ; the sellers' line, the buyers' (lines; 255 none)
        VAR     AULN2,1
        VAR     AUBLINK,1       ; seats that blink (bits)
        VAR     AUMSG,1         ; the message shown (255 none)
        VAR     AUHBH,1         ; land: the price line shown
        VAR     AUHBL,1
        VAR     AUSBH,1         ; land: "bids start at" shown
        VAR     AUSBL,1
        VAR     AULBLI,1        ; the label taking its turn
        VAR     AUMALL,1        ; 1: every money field again
        VAR     AUSHOWT,1       ; status: the step shown (255 none)
        VAR     ARFLAGS,1       ; the arena: ARF_*, what its frame must draw

        ZONE    UNIONH,UNIONHE
        VAR     AULBL,8         ; each label as shown (0 BUY/SELL, 1 none, else a price)
        VAR     AUBAR,4         ; status: each bar's units shown
        VAR     AUBART,4        ; and its target
        VAR     AUMSIG,1        ; the players' records' signature (money, units)
AULBLO  EQU     AULBL-$2900     ; (offsets for PRAMH and FRRAMP2)
AUBARO  EQU     AUBAR-$2900
AUBARTO EQU     AUBART-$2900
AUMSIGO EQU     AUMSIG-$2900
        IF      AUMSIGO > $FF
        ERROR   "the auction's RAM-H runs past FRRAMP2's reach"
        ENDIF

; the arena (lines): base price at line 102, a step 2 lines, 41 steps up
; (to 20), 13 down (to 128); sellers start at 32, buyers at the bottom
AUYBASE EQU     102
AUYSELL EQU     32
AUYTOP  EQU     20
AUYBOT  EQU     128
AUSUP   EQU     41
AUSDN   EQU     13
