; ---------------------------------------------------------------------------
; ram.inc -- the cart's 30K of RAM ($8000-$F7FF).
;
; It survives a console RESET (the cart has no reset line), so START sets
; every byte it relies on. tools/checksize.py checks that nothing here runs
; into the next region or past RAMTOP.
; ---------------------------------------------------------------------------

SHADOW  EQU 08000H              ; 128 x 64 bytes: the background, a value a pixel
DATA    EQU 0A000H              ; 8K: strings, scripts, art (copied from PGDATA)
ST      EQU 0C000H              ; the last good state (550)
PREV    EQU 0C300H              ; the state drawn last (550)
TBL     EQU 0C600H              ; /tables: count + 9 x 33
PV      EQU 0C800H              ; 512: each phase's own variables, overlaid
SPRS    EQU 0D500H              ; 10 sprite slots x 128 (to $D9FF)
FXA     EQU 0CB00H              ; 4 fx actors x 24
VARS    EQU 0CC00H              ; 512: the globals below
TXT     EQU 0CE00H              ; 128: the message being shown
LINBUF  EQU 0CE80H              ; 48: a line or a number being built
URL     EQU 0CF00H              ; 256: the request being sent
ROWKEY  EQU 0D000H              ; 64: colour-table offset of each row
ROWPAL  EQU 0D040H              ; 64: palette of each row
PLOTBUF EQU 0D080H              ; 80: a plot, composed (10 x 8)
AKBUF   EQU 0D100H              ; 72: an appkey
TOWNBUF EQU 0D180H              ; 64: a line of the town, unpacked
DROWNZ  EQU 0D1C0H              ; 64: the row has a pixel that is not 0
RXBUF   EQU 0D200H              ; 550: a reply being read (then into ST)
RAMEND  EQU 0DA00H              ; first byte nothing uses
RAMTOP  EQU 0F000H              ; never past here (DC0 runs into the mailbox)

; --- globals ($CC00-$CDFF) ------------------------------------------------
; core: the clock and the pages
FRAMES  EQU VARS+000H           ; 2: the cart's tick when the loop last woke
DONE    EQU VARS+002H           ; 2: frames the animations have caught up to
CURPG   EQU VARS+004H           ; page mapped at $4800
PGSP    EQU VARS+005H           ; page stack depth
PGSTK   EQU VARS+006H           ; 4: pages under a FAR call
TESTHK  EQU VARS+00AH           ; 4: test hook (see test.inc)
TESTMD  EQU VARS+00EH           ; 1: offline test mode
DBGAT   EQU VARS+00FH           ; where the loop is (the harness peeks it)
RNDS    EQU VARS+010H           ; 2: random state

; the game (main.c)
PHASE   EQU VARS+020H
RND     EQU VARS+021H           ; round
MYSEAT  EQU VARS+022H
LASTPH  EQU VARS+023H
LASTRND EQU VARS+024H
STOK    EQU VARS+025H
RXPE    EQU VARS+026H           ; 2: phase-elapsed at the last state
RXFRAME EQU VARS+028H           ; 2: frame of the last state
RXAGE   EQU VARS+02AH
PHT0    EQU VARS+02BH           ; 2: frame a phase's own animation started
SCRNOW  EQU VARS+02DH           ; what is on the screen (SCR_*)
ANIM    EQU VARS+02EH           ; what the frame handler animates (AN_*)
CSBASE  EQU VARS+02FH           ; 2: clock_sync's base
CSHAVE  EQU VARS+031H
SPECAT  EQU VARS+032H
DVSTATE EQU VARS+033H           ; develop's dv_state (the poll rate needs it)
NCATCH  EQU VARS+034H           ; animation steps still to run this frame
NSATUR  EQU VARS+035H           ; times the animations fell 8 frames behind
BND_A   EQU VARS+036H           ; BANDS3: the first light row
BND_B   EQU VARS+037H           ;   and the last
SPCOL   EQU VARS+03EH           ; SPSPEC's colour (0: the seat's)

; the network (net.c)
ACT     EQU VARS+040H
ACTARG  EQU VARS+041H           ; 2
ACTSENT EQU VARS+043H
NETHAVE EQU VARS+044H
NETFAIL EQU VARS+045H
POLLW   EQU VARS+046H           ; poll_wait
AUTRADE EQU VARS+047H           ; au_traded
NSTATE  EQU VARS+048H           ; request step
NSEQ    EQU VARS+049H           ; the sequence we posted
NWAIT   EQU VARS+04AH           ; frames without progress
NBWLAST EQU VARS+04BH           ; 2: bytes waiting at the last status
NGOT    EQU VARS+04DH           ; 2: bytes of the reply read so far
NARG    EQU VARS+038H           ; 2: the argument of the request in flight
NBW     EQU VARS+03AH           ; 2: bytes waiting, this status
NDEVST  EQU VARS+03CH           ; the status's nDevStatus
SCNAME  EQU VARS+050H           ; 9: the player's name
SCTABL  EQU VARS+060H           ; 9: the table
SCENDPT EQU VARS+070H           ; 66: the endpoint (the room URL's front)

; input and sound
INPDIR  EQU VARS+0C0H           ; held directions (DIR_*)
INPBTN  EQU VARS+0C1H           ; button held
EVBTN   EQU VARS+0C2H           ; taken: the button was pressed
EVKEY   EQU VARS+0C3H           ; taken: a key
INLBTN  EQU VARS+0C4H           ; latched until taken
INLKEY  EQU VARS+0C5H
INPREV  EQU VARS+0C6H           ; last scan: controller
INPREV0 EQU VARS+0C7H           ; last scan: console buttons
INPHK   EQU VARS+0C8H           ; test hook: nonzero = use the next three
INPHKD  EQU VARS+0C9H
INPHKB  EQU VARS+0CAH
INPHKK  EQU VARS+0CBH
SNDID   EQU VARS+0D0H           ; playing (255 none)
SNDT    EQU VARS+0D1H           ; frames left in this note
SNDN    EQU VARS+0D2H           ; note: 0 first, 1 second
SNDMUTE EQU VARS+0D3H
SNDTONE EQU VARS+0D4H           ; tone bits held (port 5 bits 7-6)

; display, text and the message line
TXTPTR  EQU VARS+100H           ; 2
MSGROW  EQU VARS+102H           ; pixel row of the message's first line
MSGCOL  EQU VARS+103H
MSGSCR  EQU VARS+104H           ; msg_scroll
MSGLEN  EQU VARS+105H
MSGPOS  EQU VARS+106H
MSGLAST EQU VARS+107H           ; 2
MSGONE  EQU VARS+109H           ; the message has one row, the second
DTMP    EQU VARS+110H           ; 16: display scratch
EXPN    EQU VARS+120H           ; text_expand: characters so far
EXPV1   EQU VARS+121H           ; 2: the number for the next '%'
EXPV2   EQU VARS+123H           ; 2: and the one after
TCLEN   EQU VARS+125H           ; text_center / msg_start scratch
TCCUT   EQU VARS+126H
TCROW   EQU VARS+127H
TCCOL   EQU VARS+128H
TCPTR   EQU VARS+129H           ; 2

; the map and the time bar
MAPDOTS EQU VARS+140H
MAPBLNK EQU VARS+141H
MAPBON  EQU VARS+142H
BARX    EQU VARS+143H
BARTOP  EQU VARS+144H
BARH    EQU VARS+145H
BARCOL  EQU VARS+146H
BARLAST EQU VARS+147H
BARTK   EQU VARS+148H
BARTG   EQU VARS+149H
BARFR   EQU VARS+14AH

; the sprites (spr.inc)
SPA     EQU VARS+150H           ; SPRSET's arguments:
SPA_SL  EQU SPA+0               ;   slot
SPA_SH  EQU SPA+1               ;   shape
SPA_FR  EQU SPA+2               ;   frame
SPA_XL  EQU SPA+3               ;   x (Atari colour clocks, signed 16)
SPA_XH  EQU SPA+4
SPA_YL  EQU SPA+5               ;   y (Atari lines, signed 16)
SPA_YH  EQU SPA+6
SPA_CO  EQU SPA+7               ;   colour
SPA_FL  EQU SPA+8               ;   flags (SPR_FLIPX)
SPT     EQU VARS+1A0H           ; 32: the engine's scratch
SPANY   EQU VARS+1C0H           ; slots on the screen (0: nothing to touch)
ROWFL   EQU VARS+1C2H           ; 40: a row of a sprite's union (bit 0 old, 1 new)

; the fx engine
FXON    EQU VARS+160H
FXFLAG  EQU VARS+161H
APLAST  EQU VARS+162H           ; 2: autoplay's last press

; scratch for the game logic (any routine; never across a call)
GT      EQU VARS+180H           ; 32
