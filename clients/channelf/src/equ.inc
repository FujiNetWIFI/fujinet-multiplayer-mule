; ---------------------------------------------------------------------------
; equ.inc -- the console, the cart, and the server's numbers.
;
; The cart is fujinet-firmware pico/channelf at protocol 2 (APPBANK pages and
; the tick, fuji_mailbox.h). The server's numbers are clients/mule_proto.h.
; ---------------------------------------------------------------------------

; --- the BIOS --------------------------------------------------------------
PUSHK   EQU 00107H              ; push K onto the K-stack (clobbers r7, A)
POPK    EQU 0011EH              ; pop it back
; The BIOS's K-stack: KU, KL at the scratch register r59 points to, r59 += 2.
; The BIOS starts it at r40 (9 levels, r40-r57); nothing here uses r16-r39,
; so it starts at r16: 21 levels. A 10th level at r40 overwrote r59 itself.
KSTK0   EQU 16
BCLRSCR EQU 000D0H              ; clear the screen to the colour in r3

; --- the cart: painted status page ($FC00) ----------------------------------
FNACKSQ EQU 0FC00H              ; echoes SEQ when the reply is ready
FNSTATE EQU 0FC01H
FNERR   EQU 0FC02H              ; FN_ERR_* of the last transaction
FNRCMD  EQU 0FC03H              ; $06 ACK / $15 NAK
FNRXLO  EQU 0FC04H              ; reply length, little-endian
FNRXHI  EQU 0FC05H
FNBSTAT EQU 0FC06H
FNBPCT  EQU 0FC07H
FNBERR  EQU 0FC08H
FNMAG0  EQU 0FC09H              ; 'F'
FNMAG1  EQU 0FC0AH              ; 'N'
FNPVER  EQU 0FC0BH              ; 2: APPBANK and the tick
FNTICK  EQU 0FC0EH              ; 60 Hz count, LE: read HI, LO, HI

FNREPLY EQU 0F800H              ; the whole 1K reply, read in place

; --- console -> cart: stores ------------------------------------------------
FNREG   EQU 0FD00H              ; store V at +n: register n = V
FNBANK  EQU 0FD80H              ; store anything at +n: page n at $4800
FNSWAP  EQU 0FDFEH
FNTX    EQU 0FF00H              ; store anywhere in the page: append

FR_DEV  EQU 000H
FR_CMD  EQU 001H
FR_NPAR EQU 002H
FR_DRST EQU 005H
FR_SEQ  EQU 010H

FD_FUJI EQU 070H
FD_NET  EQU 071H
FD_NET1 EQU 071H

; N: commands (fujinet-firmware NDevice)
NC_OPEN EQU 04FH                ; 'O'
NC_CLOS EQU 043H                ; 'C'
NC_READ EQU 052H                ; 'R'
NC_STAT EQU 053H                ; 'S'
NM_GET  EQU 00CH                ; open mode: HTTP GET
NS_EOF  EQU 136                 ; status: end of file

; appkeys (the FujiNet lobby's: creator 1, app 1)
FC_AKOPN EQU 0DCH               ; OPEN_APPKEY
FC_AKRD EQU 0DDH                ; READ_APPKEY
FC_AKWR EQU 0DEH                ; WRITE_APPKEY

; --- the page window -------------------------------------------------------
PGWIN   EQU 04800H
PGSIZE  EQU 02000H

; --- the console's I/O ports -----------------------------------------------
; port 0: bits 6,5 the VRAM write strobe (a $60 then $50 pulse); bits 0-3
;         the console buttons when read (bit 6 clear)
; port 1: pixel value in bits 7-6 (complemented); the right controller when read
; port 4: column (complemented); the left controller when read
; port 5: row in bits 5-0 (complemented); bits 7-6 are the SOUND and must be
;         kept on every write (read it back: INS 5)

; --- the server (clients/mule_proto.h) -------------------------------------
PH_LOBBY EQU 0
PH_GRANT EQU 1
PH_LAUC  EQU 2
PH_DEV   EQU 3
PH_PROD  EQU 4
PH_DECL  EQU 5
PH_TRADE EQU 6
PH_SUM   EQU 7
PH_OVER  EQU 8

G_FOOD  EQU 0
G_ENER  EQU 1
G_SMIT  EQU 2
G_CRYS  EQU 3

T_PLAIN EQU 0
T_RIVER EQU 1
T_MOUNT EQU 2
T_STORE EQU 3

ROLE_NONE EQU 0
ROLE_BUY  EQU 1
ROLE_SELL EQU 2

STG_SHOW EQU 0
STG_DECL EQU 1
STG_LEAD EQU 2
STG_ACT  EQU 3
STG_DONE EQU 4

CE_PEST  EQU 1
CE_PIRAT EQU 2
CE_ACID  EQU 3
CE_QUAKE EQU 4
CE_SUN   EQU 5
CE_METEO EQU 6
CE_RADIA EQU 7
CE_FIRE  EQU 8
CE_SHIP  EQU 9

ERR_MONEY EQU 8
ERR_NOSTK EQU 7
ERR_SEAT  EQU 12
ERR_TFULL EQU 14

PF_AI    EQU 1
PF_READY EQU 2
PF_DECL  EQU 4
AF_COLL  EQU 4
AF_IDLE  EQU 8
AF_CRIT  EQU 32
TF_START EQU 1
TF_ENDED EQU 2
TF_MULE  EQU 4

NPLOTS  EQU 45
STOREPL EQU 22
NOBODY  EQU 255

; --- the state's layout (MuleState, 550 bytes, little-endian) ---------------
STSIZE  EQU 550
H_MAGIC EQU 0
H_VER   EQU 1
H_TYPE  EQU 2
H_PHASE EQU 3
H_ROUND EQU 4
H_SEAT  EQU 5
H_ERR   EQU 6
H_T     EQU 8
H_ELAP  EQU 10
H_CDOWN EQU 12
H_RES   EQU 14

PLAYERS EQU 16                  ; 4 x 29
PLSIZE  EQU 29
P_NAME  EQU 0
P_SPEC  EQU 9
P_COL   EQU 10
P_FLAGS EQU 11
P_MONEY EQU 12
P_GOODS EQU 14
P_SCORE EQU 18
P_RANK  EQU 20
P_PROD  EQU 21
P_SPOIL EQU 25

M_TERR  EQU 132
M_OWNER EQU 177
M_MULE  EQU 222
M_CRYS  EQU 267
M_PROD  EQU 312

S_STOCK EQU 357
S_PRICE EQU 361
S_BUY   EQU 369
S_SELL  EQU 377
S_MULES EQU 385
S_MPRIC EQU 386

GR_CUR  EQU 388
GR_CLM  EQU 389
GR_STEP EQU 393
GR_NEXT EQU 394

A_KIND  EQU 396
A_GOOD  EQU 397
A_PLOT  EQU 398
A_SELLR EQU 399
A_FLAGS EQU 400
A_WIN   EQU 401
A_COLL  EQU 402
A_STAGE EQU 403
A_STEP  EQU 404
A_TRADE EQU 405
A_START EQU 406
A_ASK   EQU 408
A_LO    EQU 410
A_HI    EQU 412
A_SBUY  EQU 414
A_SSELL EQU 416
A_PRICE EQU 418
A_LAST  EQU 420
A_SLEFT EQU 422
A_TICKS EQU 424
A_TLEFT EQU 425
A_ROLE  EQU 426
A_POS   EQU 430
A_TGT   EQU 438
A_UNITS EQU 446
A_PUSH  EQU 450

T_SEAT  EQU 454
T_FLAGS EQU 455
T_CARRY EQU 456
T_EVENT EQU 457
T_LEN   EQU 458
T_ELAP  EQU 460
T_LEFT  EQU 462
T_ENDIN EQU 464
T_PTU   EQU 466
T_NWAMP EQU 467
T_WAMP  EQU 468                 ; 12 x {plot, mount, from16, to16}

C_EVENT EQU 540
C_PLOT  EQU 541
C_RATE  EQU 542
C_WIN   EQU 543
C_FAIL  EQU 544
C_TOTAL EQU 546                 ; 32 bits

; --- the game's colours (gfx.h): disp.inc maps them per row -----------------
C_BLACK  EQU 0
C_WHITE  EQU 1
C_BLUE   EQU 2
C_PURPLE EQU 3
C_GREEN  EQU 4
C_ORANGE EQU 5
C_RED    EQU 6
C_YELLOW EQU 7
C_GREY   EQU 8
C_CYAN   EQU 9
C_BROWN  EQU 10
C_TAN    EQU 11
C_LTBLUE EQU 12
C_DKGRN  EQU 13
C_PINK   EQU 14
C_P3     EQU 15

; fx script bytes: a position (x + 64), a signed step
XB      FUNCTION x, ((x)+64)&0FFH
SB      FUNCTION v, (v)&0FFH

; auction stages (mule_proto.h), goods
STAGE_SHOW    EQU 0
STAGE_DECLARE EQU 1
STAGE_LEAD    EQU 2
STAGE_ACTIVE  EQU 3
STAGE_DONE    EQU 4
GOOD_FOOD     EQU 0
GOOD_ENERGY   EQU 1
GOOD_SMITH    EQU 2
GOOD_CRYS     EQU 3
