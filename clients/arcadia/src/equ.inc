; equ.inc -- the Arcadia 2001, the FujiNet cart's mailbox, and the game's
; own numbers. The protocol's are in build/proto.inc (tools/mkproto.py).

; ---- the 2637 UVI and the console's RAM -------------------------------------
SCR0    EQU     $1800           ; rows 0-12, 16 cells each
SCR1    EQU     $1A00           ; rows 13-25 (26-line mode)
SPRPOS  EQU     $18F0           ; sprite n: Y at +2n (256 - y), X at +2n+1 (x + 44)
VPOS    EQU     $18FC           ; rows start at line 255 - value ($FF: the top)
PITCH   EQU     $18FD           ; b7 multicolor (never set here), b0-6 pitch
VOLUME  EQU     $18FE           ; b0-2 volume, b3 tone, b4 noise, b5-7 shift
ROWNUM  EQU     $18FF           ; the character row being drawn
KEYPAD  EQU     $1900           ; P1 keypad columns $1900-$1902 (b3..b0, active high)
PANEL   EQU     $1908           ; b0 Start, b1 Option, b2 Select
UDC     EQU     $1980           ; 8 user glyphs $38-$3F, 8 bytes each; $38-$3B are
                                ; also sprites 0-3's pictures
GFXMOD  EQU     $19F8           ; b7 rows start in block graphics, b6 26 lines
BGCOL   EQU     $19F9           ; b7 8x8 cells, b6 pot mux, b3 ink parity, b0-2 ground
SPRC23  EQU     $19FA           ; sprites 2/3: colour b3-5 / b0-2, b7/b6 1 = 8 lines
SPRC01  EQU     $19FB           ; sprites 0/1, the same
COLLBG  EQU     $19FC
COLLSP  EQU     $19FD
POT     EQU     $19FF           ; P1 disc: mux clear vertical, set horizontal

; The game's screen: 26 lines of 8x8 cells, white/cyan/magenta/blue ink on
; black. A cell is its glyph ($00-$3F) plus one of these.
GFX26   EQU     $40
BGBASE  EQU     $87             ; 8x8 cells, even inks, black ground
BG13    EQU     $07             ; 13-line screens: 8x16 cells (doubled)
MUXH    EQU     $40             ; BGCOL: the pot reads the horizontal axis
CWHITE  EQU     $00
CCYAN   EQU     $40
CMAGEN  EQU     $80
CBLUE   EQU     $C0
GMOSAIC EQU     $C0             ; this exact cell starts block graphics to the row's end
GTEXT   EQU     $40             ; and this one ends it
; sprite colours (any of the eight)
SWHITE  EQU     0
SYELLOW EQU     1
SCYAN   EQU     2
SGREEN  EQU     3
SMAGEN  EQU     4
SRED    EQU     5
SBLUE   EQU     6
SBLACK  EQU     7

; The ROM's glyphs worth naming.
GSPACE  EQU     $00
GSLASH  EQU     $01             ; / (a whole cell)
GBSLSH  EQU     $02             ; \ (a whole cell)
GSOLID  EQU     $03
GTOPBAR EQU     $04
GRTBAR  EQU     $05
GBOTBAR EQU     $06
GLFBAR  EQU     $07
GCNRTR  EQU     $08             ; corners: top-right, top-left, bottom-left, bottom-right
GCNRTL  EQU     $09
GCNRBL  EQU     $0A
GCNRBR  EQU     $0B
GTRILR  EQU     $0C             ; triangles: lower right, lower left, upper left, upper right
GTRILL  EQU     $0D
GTRIUL  EQU     $0E
GTRIUR  EQU     $0F
GDIGIT  EQU     $10             ; '0'
GALPHA  EQU     $1A             ; 'A'
GDOT    EQU     $34
GCOMMA  EQU     $35
GPLUS   EQU     $36
GDOLLAR EQU     $37
GUDC    EQU     $38             ; user glyph 0

; ---- the cart's mailbox (fujinet-firmware pico/arcadia fuji_mailbox.h) ------
FNRDATA EQU     $2B00           ; the reply slice RXSLICE shows
FNACKSQ EQU     $2C00
FNSTATS EQU     $2C01
FNERR   EQU     $2C02
FNREPLY EQU     $2C03
FNRXLL  EQU     $2C04
FNRXLH  EQU     $2C05
FNMAGF  EQU     $2C09
FNMAGN  EQU     $2C0A
FNPVER  EQU     $2C0B           ; 2: APPBANK, cart RAM, the tick
FNSECHO EQU     $2C0C
FNTICKL EQU     $2C0E           ; 60 Hz, read HI, LO, HI
FNTICKH EQU     $2C0F
FNRSEL  EQU     $2D00
FNBANK  EQU     $2D80           ; read +n: the window shows page n
FNRDAT  EQU     $2E00
FNDATA  EQU     $2F00
FRDEVIC EQU     $00
FRCMD   EQU     $01
FRNPARM EQU     $02
FRDATRS EQU     $05
FRRXSLC EQU     $06
FRSEQ   EQU     $10
FRRAMP0 EQU     $14             ; cart RAM pointer = $000 + value (RAM-L)
FRRAMP1 EQU     $15             ; $100 + value
FRRAMP2 EQU     $16             ; $200 + value (RAM-H)
FRRAMP3 EQU     $17             ; $300 + value
FRRAMPK EQU     $18             ; each REGDATA stores and steps the pointer
FCACK   EQU     $06
FNDFUJI EQU     $70
NETDEV  EQU     $71             ; N1:
NCOPEN  EQU     $4F             ; 'O'
NCCLOSE EQU     $43             ; 'C'
NCREAD  EQU     $52             ; 'R'
NCSTAT  EQU     $53             ; 'S'
NMHTTPG EQU     $0C
FCCKEY  EQU     $DB             ; CLOSE_APPKEY
FCOKEY  EQU     $DC             ; OPEN_APPKEY
FCRKEY  EQU     $DD             ; READ_APPKEY
FCWKEY  EQU     $DE             ; WRITE_APPKEY

; ---- the window ---------------------------------------------------------------
WIN     EQU     $0800           ; every page: entries at $0800 + 3k, its id at $0FFF
E_ENTER EQU     0               ; the page's entries (offsets for BSXA)
E_STATE EQU     3
E_TICK  EQU     6
E_EXTRA EQU     9

; ---- input -----------------------------------------------------------------------
D_UP    EQU     $01             ; INDIR bits
D_DOWN  EQU     $02
D_LEFT  EQU     $04
D_RIGHT EQU     $08
JOYLO   EQU     $40             ; a pot under this: up / left
JOYHI   EQU     $C0             ; over this: down / right
K_NONE  EQU     $FF             ; EVKEY: a key that went down this frame
K_ENT   EQU     $0A             ; (0-9: the digits; FIRE is '2' and reads as EVBTN)
K_CLR   EQU     $0B
K_START EQU     $0C
K_OPT   EQU     $0D
K_SEL   EQU     $0E

; ---- sounds (mekkogx platform-specific/sound.h) ---------------------------------
SND_SELECT EQU  0
SND_SAMPLE EQU  1
SND_ERROR  EQU  2
SND_BUY    EQU  3
SND_ALERT  EQU  4
SND_AUCOVR EQU  5
SND_TRADE  EQU  6
SND_EVENT  EQU  7
SND_FOUND  EQU  8
SND_STEP   EQU  9               ; from here up: soft, never cuts another
SND_TICK   EQU  10
SND_TOCK   EQU  11

; ---- actions (mekkogx mule.h; the routes in fixb.inc) ----------------------------
ACT_NONE   EQU  0
ACT_STATE  EQU  1
ACT_READY  EQU  2
ACT_UNRDY  EQU  3
ACT_SPEC   EQU  4
ACT_CLAIM  EQU  5
ACT_PUSH   EQU  6               ; arg 0 hold, 1 up, 2 down
ACT_DECL   EQU  7               ; arg 0 sit out, 1 buyer, 2 seller
ACT_COLL   EQU  8
ACT_BEGIN  EQU  9
ACT_BUYMU  EQU  10
ACT_OUTFIT EQU  11
ACT_INST   EQU  12
ACT_RETURN EQU  13
ACT_ASSAY  EQU  14
ACT_SELL   EQU  15
ACT_UNSELL EQU  16
ACT_WAMPUS EQU  17
ACT_PUB    EQU  18
ACT_END    EQU  19
ACT_LEAVE  EQU  20
ACT_HOLD   EQU  21              ; arg: the price our released line stopped at
ACT_TABLES EQU  22

; net results (NHAVE)
NH_NONE EQU     0
NH_STATE EQU    1               ; a whole state is in cart RAM
NH_ERR  EQU     2               ; the server answered with a header only
NH_FAIL EQU     3
NH_TABLES EQU   4               ; the table list is in the reply window

NOBODY  EQU     $FF
