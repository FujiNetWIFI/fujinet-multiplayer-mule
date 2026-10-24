; equ.inc -- constants: the console, the cartridge, the screen.

; ---- P1 (all active low but COPY and LUM) ----
; Main code runs with external RAM and the cartridge on the bus; the interrupts
; switch to the VDC and put P1 back. COPY low always, or the cart's write
; strobe is gated off. The bank bits are left high: APPBANK pages are picked
; through the mailbox, not by P10/P11.
P1_MAIN equ 0AFh                ; XRAM + cart selected, VDC parked
P1_VDC  equ 0B7h                ; VDC selected
P1_JOY  equ 0BBh                ; keyboard scanner (joystick) selected

; ---- the mailbox (fuji_mailbox.h, protocol 2) ----
FN_W_REGSEL     equ 0E0h
FN_W_REGDATA    equ 0E1h
FN_W_DATA       equ 0E2h
FN_W_SEQ        equ 0E3h
FN_REG_DEVICE   equ 000h
FN_REG_CMD      equ 001h
FN_REG_NPARAM   equ 002h
FN_REG_TXLEN_LO equ 003h
FN_REG_TXLEN_HI equ 004h
FN_REG_DATA_RST equ 005h
FN_REG_RXSLICE  equ 006h
FN_REG_CODEBANK equ 008h
FN_REG_POKE_LO  equ 00Ah
FN_REG_POKE_HI  equ 00Bh
FN_REG_POKE     equ 00Ch
FN_REG_RXWIN    equ 00Dh
FN_R_ACKSEQ     equ 0F20h
FN_R_STATUS     equ 0F21h
FN_R_ERR        equ 0F22h
FN_R_REPLY_CMD  equ 0F23h
FN_R_RXLEN_LO   equ 0F24h
FN_R_RXLEN_HI   equ 0F25h
FN_R_MAGIC0     equ 0F29h
FN_R_MAGIC1     equ 0F2Ah
FN_R_PROTO_VER  equ 0F2Bh
FN_R_DATA       equ 0F30h       ; the read window: 208 bytes from RXWIN*16
FN_PROTO_VER    equ 2
O2_APP_HDR      equ 0F30h
O2_APP_VERSION  equ 1

FUJI_DEVICEID   equ 070h
FC_ACK          equ 006h
FC_AKOPEN       equ 0DCh        ; OPEN_APPKEY
FC_AKREAD       equ 0DDh        ; READ_APPKEY
FC_AKWRITE      equ 0DEh        ; WRITE_APPKEY
NET_DEVICEID    equ 071h
NC_OPEN         equ 04Fh
NC_CLOSE        equ 043h
NC_READ         equ 052h
NC_STATUS       equ 053h
NM_GET          equ 00Ch        ; open mode: HTTP GET
NS_EOF          equ 136         ; nDevStatus: end of file

; ---- the VDC's character generator (o2doc appendix C) ----
G_SPACE equ 12
G_DOLLAR equ 11
G_BLOCK equ 47
G_BALL  equ 49
G_MANR  equ 50
G_MANL  equ 53
G_SLOPEL equ 56
G_SLOPER equ 57
G_EQ    equ 43

; Character colours, as MAME's i8244 draws them (control byte bits 1-3)
C_DGREY  equ 0
C_RED    equ 1
C_GREEN  equ 2
C_TAN    equ 3
C_BLUE   equ 4
C_VIOLET equ 5
C_CYAN   equ 6
C_WHITE  equ 7

; ---- the screen ----
; Characters are 7 rows of 2 lines. A text line at Y covers Y..Y+13; the line
; counter interrupt at Y+14 re-points the characters for the next line.
X_TEXT  equ 16                  ; 16 characters at 8 px, centred over the grid
DV_NONE equ 0                   ; the development's DV_ST (p11-p13)
DV_PROMPT equ 1                 ; our turn, not begun: the button begins it
DV_WALK equ 2                   ; our turn
DV_WAIT equ 3                   ; another's turn, or ours over
SND_SELECT equ 0                ; fixed.inc beep
SND_ALERT equ 1
SND_ERROR equ 2
SND_TRADE equ 3
TX_SEATS equ 080h               ; a TXCOL: each four columns in a seat's colour
; the say queue's lines (pstr.inc slines)
L_TXT   equ 0                   ; 0-5: the text screen
L_MAP   equ 6                   ; 6-8: the map's title and two messages
L_AUC2  equ 9                   ; an auction's second line (under its title)
TXT_LEN equ 16
TXT_TAB equ 1+2*TXT_LEN         ; a text table: Y, then 16 x (pointer, control)
ROW_TAB equ 1+2*9               ; a map row: Y, then 9 x (pointer, control)

; The map: grid rows 1-5 (the grid's line j is at y=24+24j), one character
; per plot. Cells 0-3 quad A, 5-8 quad B, the river column single 10.
Y_ROW0  equ 54
ROW_PITCH equ 24
X_QA    equ 10                  ; 16*0+10: 2 px right of vertical line 0
X_QB    equ 90                  ; 16*5+10
X_COL4  equ 77                  ; 16*4+13: the river column, centred

; Screen kinds (the band list the main code pokes): see fixed.inc.

; ---- requests (net.inc) ----
ACT_STATE equ 1
ACT_READY equ 2
ACT_UNRDY equ 3
ACT_SPEC  equ 4
ACT_CLAIM equ 5
ACT_PUSH  equ 6                 ; arg: 0 hold, 1 up, 2 down
ACT_DECL  equ 7                 ; arg: 0 sit out, 1 buyer, 2 seller
ACT_COLL  equ 8
ACT_BEGIN equ 9
ACT_BUYM  equ 10
ACT_OUTF  equ 11                ; arg: GOOD_*
ACT_INST  equ 12                ; arg: plot
ACT_RETN  equ 13
ACT_ASSAY equ 14
ACT_SELL  equ 15
ACT_UNSEL equ 16
ACT_WAMP  equ 17
ACT_PUB   equ 18
ACT_END   equ 19
ACT_LEAVE equ 20
ACT_HOLD  equ 21                ; arg: the price our released line stopped at
ACT_TABLS equ 22                ; /tables?bin=1

NS_IDLE equ 0
NS_CLOS equ 1
NS_OPEN equ 2
NS_STAT equ 3
NS_READ equ 4

NH_FAIL   equ 0                 ; NETHAVE: how a request ended
NH_STATE  equ 1                 ;   a whole state is in the reply
NH_ERR    equ 2                 ;   an error header (16 bytes)
NH_TABLES equ 3                 ;   the table list


; ---- the stick (input.inc): JOY / JOYEV bits ----
J_UP    equ 01h
J_DOWN  equ 02h
J_LEFT  equ 04h
J_RIGHT equ 08h
J_FIRE  equ 10h
