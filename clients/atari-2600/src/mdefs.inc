; mdefs.inc -- what every bank of the M.U.L.E. client agrees on.
;
; EQUATES ONLY, included before the ORG next to vcs.inc and fujinet.inc.
;
; The server runs the game. This client polls /state?bin=1 (the 550-byte
; MuleState of clients/mule_proto.h) and draws it. The reply stays in the
; cartridge's reply window and is read in place, but only between a READ and
; the next transaction: every transaction repaints the window. So when a
; state arrives, what the screens need between polls is copied out of it --
; a few bytes into zero page (HOT), the map into a path buffer -- and the
; picture itself lives in the cartridge's planes.

; ---------------- the banks ----------------
; 32K: fifteen 2K banks at $1000 and the fixed half.
BKBOOT  EQU     0               ; cold start, appkeys, the name, the title
BKLOBBY EQU     1               ; the table list and the lobby
BKNET   EQU     2               ; the request machine, and a state's arrival
BKSTAT  EQU     3               ; the status screens: summary, goods, end
BKMAP   EQU     4               ; the map: grant, production, others' turns
BKDEV   EQU     5               ; the map again, walking: your turn
BKTOWN  EQU     6               ; the town
BKAUC   EQU     7               ; the auctions
BKTABLE EQU     8               ; the table list
NBANKS  EQU     15

; The trampoline, first thing in the fixed tail (mcore.inc ORGs it there).
MGOTO   EQU     FNTAIL

; A bank switch is a jump. Where the entered bank carries on is data, in MENT.
ENCOLD  EQU     0               ; power-on: boot
ENRUN   EQU     1               ; back from a net step: carry on
ENSTATE EQU     2               ; back from a net step: a state has arrived
ENENTER EQU     3               ; this screen is new: set it up
ENTABLE EQU     4               ; lobby bank: the table list
ENFAIL  EQU     5               ; back from a net step: the request failed

; ---------------- zero page ----------------
; $80-$8C is the transport's, laid out as the Battleship client's so the
; shared tail code is the same code.
FNSEQ   EQU     $80             ; the sequence we are waiting for
PAD3    EQU     $81             ; the kernels' 3-cycle pad target
SAVSP   EQU     $82             ; the stack pointer, parked across a text row
FNCNT   EQU     $83             ; timeout quanta remaining
MIDX    EQU     $84             ; a loop's index
MTMP    EQU     $85             ; scratch; any routine may clobber it
FNDEV   EQU     $86             ; transaction setup, read by FNBEG
FNCMD   EQU     $87
FNNPR   EQU     $88
FNTMO   EQU     $89             ; timeout in ~0.6s quanta
FNPTRL  EQU     $8A
FNPTRH  EQU     $8B
MTMP2   EQU     $8C             ; a second scratch cell

; The system.
MENT    EQU     $8D             ; EN*: what the bank entered should do
MFRAME  EQU     $8E             ; free-running frame counter, 16 bits
MFRAMEH EQU     $8F
INCUR   EQU     $90             ; input levels, active high (IN_*)
INPREV  EQU     $91
MINP    EQU     $92             ; this frame's newly pressed
INRPT   EQU     $93             ; auto-repeat countdown
SNDPTR  EQU     $94             ; the sound: which cue step, 0 = silent
SNDCNT  EQU     $95             ;   frames left on it
MSCR    EQU     $96             ; the screen bank a net step returns to
MROW    EQU     $97             ; text kernel: the row being drawn
MLINE   EQU     $98             ; text kernel: ink lines left in the row
MBAR    EQU     $99             ; text kernel: the highlighted row, or $FF

; The request machine (bank 2).
NSTATE  EQU     $9A             ; NS*
NREQ    EQU     $9B             ; the request in flight (RQ*)
NARG    EQU     $9C             ; its argument, 16 bits
NARGH   EQU     $9D
NAVL0   EQU     $9E             ; NET_STATUS bytes waiting
NAVL1   EQU     $9F
NPRV0   EQU     $A0             ; the previous reading, for the settle
NPRV1   EQU     $A1
NTRY    EQU     $A2             ; frames left before the step gives up
NPOLL   EQU     $A3             ; frames until the next poll
NERR    EQU     $A4             ; the last failure, 0 = none
NACT    EQU     $A5             ; the action queued for the next request
NACTA   EQU     $A6             ;   and its argument
NACTH   EQU     $A7

; HOT: the state between polls.
HPHASE  EQU     $A8             ; header.phase of the last state
HROUND  EQU     $A9
HSEAT   EQU     $AA             ; mySeat, $FF = spectator
HERR    EQU     $AB             ; header.err
HSCRB   EQU     $AC             ; the screen bank the last state chose
HRXF    EQU     $AD             ; MFRAME when that state arrived
HRXFH   EQU     $AE
HELAP   EQU     $AF             ; header.elapsed then
HELAPH  EQU     $B0
HFLAGS  EQU     $B1             ; HF*

; $B2-$D7: each screen's own, laid over each other (see the banks).
PV      EQU     $B2
PVEND   EQU     $D8

; $D8-$F5 spare. The stack is $F6-$FF: every bank switch resets it to $FF
; and nothing nests deeper than a JSR three calls down.
STKLOW  EQU     $F6

; HFLAGS
HFNEW   EQU     $01             ; a state arrived and has not been drawn
HFTBL   EQU     $02             ; a table is joined

; ---------------- the request machine ----------------
; Requests. RQSTATE polls; the actions are the server's routes, and each
; one's reply is the next state, so an action replaces a poll.
RQSTATE EQU     1               ; /state
RQREADY EQU     2               ; /ready/1
RQUNRDY EQU     3               ; /ready/0
RQSPEC  EQU     4               ; /setup/species/N
RQCLAIM EQU     5               ; /claim/N
RQPUSH  EQU     6               ; /auction/push/W/N
RQDECL  EQU     7               ; /declare/W
RQCOLL  EQU     8               ; /collude
RQBEGIN EQU     9               ; /turn/begin
RQBUY   EQU     10              ; /turn/buymule
RQOUTF  EQU     11              ; /turn/outfit/N
RQINST  EQU     12              ; /turn/install/N
RQRET   EQU     13              ; /turn/return
RQASSAY EQU     14              ; /turn/assay/N
RQSELL  EQU     15              ; /turn/sell/N
RQUNSEL EQU     16              ; /turn/unsell/N
RQWAMP  EQU     17              ; /turn/wampus
RQPUB   EQU     18              ; /turn/pub
RQEND   EQU     19              ; /turn/end
RQLEAVE EQU     20              ; /leave
RQTABLE EQU     21              ; /tables?bin=1 (not 0: NACT 0 = none)
RQHOLD  EQU     22              ; /auction/push/hold/0/PRICE: a line let go

; Request machine states.
NSIDLE  EQU     0               ; nothing in flight
NSCLOSE EQU     1               ; CLOSE launched
NSOPEN  EQU     2               ; OPEN launched
NSSTAT  EQU     3               ; STATUS launched
NSREAD  EQU     4               ; READ launched
NSOPENB EQU     5               ; OPEN half built (the query to come)

; The paths: cartridge buffers, because console RAM cannot hold strings.
PBEND   EQU     FP_SEL0         ; the endpoint, "https://host" with no '/'
PBTABLE EQU     FP_SEL1         ; the joined table's id
PBNAME  EQU     FP_SEL2         ; the player's name
PBEDIT  EQU     FP_SEL3         ; the keyboard's scratch

NAMEMAX EQU     8

; ---------------- the wire format (clients/mule_proto.h) ----------------
; MuleHeader, 16 bytes
SHMAGIC EQU     0
SHVER   EQU     1
SHTYPE  EQU     2
SHPHASE EQU     3
SHROUND EQU     4
SHSEAT  EQU     5
SHERR   EQU     6
SHSTOP  EQU     7               ; land grant: 1 + where a computer stops the cursor
SHT     EQU     8
SHELAP  EQU     10
SHCOUNT EQU     12
SHRES   EQU     14
; MulePlayer[4], 29 bytes each, from 16
SPLAYER EQU     16
PLSIZE  EQU     29
PLNAME  EQU     0
PLSPEC  EQU     9
PLCOLOR EQU     10
PLFLAGS EQU     11
PLMONEY EQU     12
PLGOODS EQU     14
PLSCORE EQU     18
PLRANK  EQU     20
PLPROD  EQU     21
PLSPOIL EQU     25
; MuleMap, from 132
SMAP    EQU     132
SMTERR  EQU     132
SMOWNER EQU     177
SMMULE  EQU     222
SMCRYS  EQU     267
SMPROD  EQU     312
; MuleStore, from 357
SSTORE  EQU     357
SSSTOCK EQU     357
SSPRICE EQU     361
SSBUY   EQU     369
SSSELL  EQU     377
SSMULES EQU     385
SSMPRC  EQU     386
; MuleGrant, from 388
SGRANT  EQU     388
SGCUR   EQU     388
SGCLAIM EQU     389
SGSTEP  EQU     393
SGNEXT  EQU     394
; MuleAuction, from 396
SAUC    EQU     396
; MuleTurn, from 454
STURN   EQU     454
; MuleColony, from 540
SCOLONY EQU     540
STSIZE  EQU     550

MSGSTATE EQU    1
PROTOVER EQU    2

; Phases.
PHLOBBY EQU     0
PHGRANT EQU     1
PHLAUC  EQU     2
PHDEV   EQU     3
PHPROD  EQU     4
PHDECL  EQU     5
PHTRADE EQU     6
PHSUMM  EQU     7
PHOVER  EQU     8

NPLOTS  EQU     45
STOREPL EQU     22

; /tables?bin=1: a count, then 33-byte records.
TBSIZE  EQU     33
TBID    EQU     0
TBNAME  EQU     8
TBPLYRS EQU     28
MAXTBL  EQU     8

; ---------------- input ----------------
IN_UP   EQU     $01
IN_DOWN EQU     $02
IN_LEFT EQU     $04
IN_RIGHT EQU    $08
IN_FIRE EQU     $10
IN_SEL  EQU     $20
IN_RST  EQU     $40
INDIRS  EQU     IN_UP|IN_DOWN|IN_LEFT|IN_RIGHT

; ---------------- colours (NTSC) ----------------
CBLACK  EQU     $00
CWHITE  EQU     $0E
CGREY   EQU     $06
CRED    EQU     $34
CBLUE   EQU     $84
CGOLD   EQU     $1A
CGREEN  EQU     $C4
; The players, in the Atari's colours (tools/extract's manifest).
CPLAY0  EQU     $84
CPLAY1  EQU     $54
CPLAY2  EQU     $C4
CPLAY3  EQU     $34

; The text screens' side border (disptext.inc): clocks 0-31 and 128-159,
; black. It hides where a row's background changes a line late (the seam
; line writes COLUBK at cycle 32).
PFEDGE  EQU     $F0

; ---------------- sound (sound.inc) ----------------
SNDBELL EQU     1               ; the bell: input is wanted
SNDTCK  EQU     2               ; the timer's tick
SNDALT  EQU     $D8             ; the tick's pitch, alternating
MNUM    EQU     $D9             ; MDEC's number, 16 bits
MNUMH   EQU     $DA
MDGT    EQU     $DB             ; MDEC's digit and leading-zero latch
NSCNT   EQU     $DC             ; STATUS polls left before the request gives up
NRXL0   EQU     $DD             ; the READ's reply length
NRXL1   EQU     $DE
HFERR   EQU     $04             ; HFLAGS: the last request failed
HFLEFT  EQU     $08             ; HFLAGS: /leave went out: back to the list
HMONEY  EQU     $DF             ; my money, 16 bits, from the last state
HMONEYH EQU     $E0
MSGPOS  EQU     $E1             ; the scrolling message: its first character
MSGCNT  EQU     $E2             ;   frames to its next step; 0 = none showing
; $E3 spare. (A path buffer's length is the cart's to say: FNPLNL, published
; for the selected buffer.)
SNOPOLL EQU     $E4             ; nonzero: this screen does not poll
MCOMP   EQU     $E5             ; a screen's recompose step, 0 = none due
MHOLD   EQU     $E6             ; frames RESET has been held

ENLIST  EQU     6               ; lobby bank: fetch and show the table list
ENNAME  EQU     7               ; boot bank: the keyboard
ENTOWN  EQU     8               ; dev bank: back from the town
ENPOST  EQU     9               ; back from a helper bank: carry on after STICK

AKNAME  EQU     0               ; appkey 0: the shared FujiNet username
AKROOM  EQU     10              ; appkey 10: the lobby's room URL
MBGMSK  EQU     $E7             ; the text kernel's row colours AND this

; MulePlayer.flags
PF_AI   EQU     $01
PF_READY EQU    $02
PF_DECL EQU     $04
PF_ONLN EQU     $08

; MuleAuction, from 396
SAKIND  EQU     SAUC+0          ; 0 none, 1 land, 2 goods
SAGOOD  EQU     SAUC+1
SAPLOT  EQU     SAUC+2
SASELLR EQU     SAUC+3
SAFLAGS EQU     SAUC+4          ; AF_*
SAWIN   EQU     SAUC+5
SACOLL  EQU     SAUC+6
SASTAGE EQU     SAUC+7          ; STG*
SASTEP  EQU     SAUC+8          ; dollars a line step
SATRADE EQU     SAUC+9          ; units traded
SASTART EQU     SAUC+10         ; land's opening price, 16 bits
SAASK   EQU     SAUC+12
SALO    EQU     SAUC+14         ; the scale's bottom and top
SAHI    EQU     SAUC+16
SASBUY  EQU     SAUC+18         ; the store buys at
SASSELL EQU     SAUC+20         ; the store sells at
SAPRICE EQU     SAUC+22         ; land's sale price
SALAST  EQU     SAUC+24         ; the last trade's price
SALEFT  EQU     SAUC+26         ; jiffies left in a timed stage
SATICKS EQU     SAUC+28         ; the clock's ticks left
SATLEFT EQU     SAUC+29
SAROLE  EQU     SAUC+30         ; [4] ROLE_*
SAPOS   EQU     SAUC+34         ; [4] 16 bits: the line price
SATGT   EQU     SAUC+42         ; [4] 16 bits
SAUNITS EQU     SAUC+50         ; [4] signed
SAPUSH  EQU     SAUC+54         ; [4] signed
STGSHOW EQU     0
STGDECL EQU     1
STGLEAD EQU     2
STGACT  EQU     3
STGDONE EQU     4
ROLENONE EQU    0
ROLEBUY EQU     1
ROLESELL EQU    2
AF_ACT  EQU     $01
AF_DONE EQU     $02
AF_COLL EQU     $04
AF_IDLE EQU     $08
AF_CRIT EQU     $20             ; "seller at critical level" (goods)

; MuleTurn, from 454
STSEAT  EQU     STURN+0
STFLAGS EQU     STURN+1         ; TF_*
STCARRY EQU     STURN+2         ; the outfit carried, $FF none
STEVENT EQU     STURN+3
STLEN   EQU     STURN+4         ; 16 bits
STELAP  EQU     STURN+6
STLEFT  EQU     STURN+8
STENDIN EQU     STURN+10
STPTU   EQU     STURN+12
STNWAMP EQU     STURN+13
STWAMP  EQU     STURN+14        ; [12] x 6: plot, mount, from(16), to(16)
TF_START EQU    $01
TF_ENDED EQU    $02
TF_MULE EQU     $04
TF_CAUGHT EQU   $08

; MuleColony, from 540
SCEVENT EQU     SCOLONY+0
SCPLOT  EQU     SCOLONY+1
SCRATE  EQU     SCOLONY+2

BKGOODS EQU     9               ; the per-good status and declaring
BKCOMP  EQU     10              ; text rows for the banks with no room
BKDEVX  EQU     11              ; the walker's logic on the map
BKTOWNX EQU     12              ; and in the town

; Long branches: the inverted branch over a JMP.
JCS     MACRO   dest
        bcc     *+5
        jmp     dest
        ENDM
JCC     MACRO   dest
        bcs     *+5
        jmp     dest
        ENDM
JEQ     MACRO   dest
        bne     *+5
        jmp     dest
        ENDM
JNE     MACRO   dest
        beq     *+5
        jmp     dest
        ENDM
JMI     MACRO   dest
        bpl     *+5
        jmp     dest
        ENDM
JPL     MACRO   dest
        bmi     *+5
        jmp     dest
        ENDM

; header.err (ERR_*)
ERR_NONE EQU    0
ERR_PHASE EQU   1
ERR_NOTTURN EQU 2
ERR_TIMEUP EQU  3
ERR_PLOT EQU    4
ERR_NOMULE EQU  5
ERR_HAVEMULE EQU 6
ERR_NOSTOCK EQU 7
ERR_MONEY EQU   8
ERR_OUTFIT EQU  9
ERR_DONE EQU    10
ERR_TERRAIN EQU 11

; ---------------- messages (banks 13 and 14) ----------------
MSGREQ  EQU     $E3             ; a message to show: its number, 0 = none
BKSTRP  EQU     13              ; the player events
BKSTRM  EQU     14              ; the other messages
; Bank 14's messages (tools/mkstr2600.py); a player event k (1-22) is
; MSEVENT + k, in bank 13.
MSLAND  EQU     1               ; you're landing on the planet Irata
MSSELECT EQU    2               ; press your button to select a plot
MSNOPLOT EQU    3               ; no new plots for sale
MSCPU   EQU     4               ; the computer is taking his turn
MSTURN  EQU     5               ; <name>'s turn
MSTIME  EQU     6               ; time has run out
MSPRESS EQU     7               ; press all player buttons to go on
MSSHORT EQU     8               ; food shortage decreases time
MSCOLONY EQU    8               ; + CE_*: the colony events, 9-17
MSBIDS  EQU     18              ; bids start at $n
MSRATE  EQU     19              ; + rating: overall, ... (19-25)
MSSHIP  EQU     26              ; the ship will be back in 12 months
MSEV20  EQU     27              ; player events 20-22 (27-29): bank 14 has
                                ;   room, bank 13 has not
MSEVENT EQU     64              ; + turn.event (1-19)
TICKRATE EQU    8               ; frames a character, scrolling
