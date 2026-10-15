; ---------------------------------------------------------------------------
; net.inc -- talking to the server (mekkogx net.c), never waiting.
;
; Every call is an HTTP GET of
;     N:<endpoint><route>[<arg>]?table=<t>&player=<name>&bin=1
; and every call, action or poll, returns the whole 550-byte MuleState.
;
; A request is CLOSE (the last one), OPEN, STATUS until the reply settles
; (two equal non-zero byte counts), READ. Each is one cart transaction, and
; the loop never waits for one: NETSTEP posts a transaction (one store of
; the sequence number) and returns; on a later frame it finds the cart's
; echo (FNDONE) and posts the next. On hardware the round trip is the
; network's; MAME answers inside the store, so it shows the order of things,
; not their time.
;
; Rules carried from the family's other carts:
;   - the sequence number is the cart's ACKSEQ + 1, never a counter of ours
;     (a RESET restarts us, not the cart);
;   - nothing else may write the mailbox while a transaction is in flight;
;   - every completion repaints the 1K window: a READ's bytes are copied out
;     before anything else is posted;
;   - TX helpers point DC0 at the TX page themselves.
; ---------------------------------------------------------------------------

ACT_NONE  EQU 0
ACT_STATE EQU 1
ACT_READY EQU 2
ACT_UNRDY EQU 3
ACT_SPEC  EQU 4
ACT_CLAIM EQU 5
ACT_PUSH  EQU 6                 ; arg: 0 hold, 1 up, 2 down
ACT_DECL  EQU 7                 ; arg: 0 sit out, 1 buyer, 2 seller
ACT_COLL  EQU 8
ACT_BEGIN EQU 9
ACT_BUYM  EQU 10
ACT_OUTF  EQU 11                ; arg: GOOD_*
ACT_INST  EQU 12                ; arg: plot
ACT_RETN  EQU 13
ACT_ASSAY EQU 14
ACT_SELL  EQU 15
ACT_UNSEL EQU 16
ACT_WAMP  EQU 17
ACT_PUB   EQU 18
ACT_END   EQU 19
ACT_LEAVE EQU 20
ACT_HOLD  EQU 21                ; arg: the price our released line stopped at
ACT_TABLS EQU 22                ; ours: /tables?bin=1 into TBL

NS_IDLE EQU 0
NS_CLOS EQU 1
NS_OPEN EQU 2
NS_STAT EQU 3
NS_READ EQU 4
NTMOUT  EQU 240                 ; frames without progress before giving up
TBLMAX  EQU 1+9*33

; --- the mailbox -----------------------------------------------------------

; FNBEG -- r0 = device, r1 = command, r2 = parameters: the three registers
;   (consecutive: three stores off one DCI), the TX stream rewound, DC0 left
;   on the TX page.  Leaf.  Clobbers A, DC0.
FNBEG:  DCI FNREG+FR_DEV
        LR A,0
        ST
        LR A,1
        ST
        LR A,2
        ST
        DCI FNREG+FR_DRST
        CLR
        ST
        DCI FNTX
        POP

; FNPB -- a one-byte parameter, r0.  FNPW -- a two-byte one, r0:r1.  Leaves.
;   Clobber A, DC0.
FNPB:   DCI FNTX
        LIS 1
        ST
        LR A,0
        ST
        POP
FNPW:   DCI FNTX
        LIS 2
        ST
        LR A,0
        ST
        LR A,1
        ST
        POP

; FNTXS -- the string at DC1, without its NUL.  Leaf.  Clobbers A, DC0, DC1.
FNTXS:  DCI FNTX
FNTS1:  XDC
        LM
        XDC
        NI 0FFH
        BZ FNTSX
        ST
        BR FNTS1
FNTSX:  POP

; FNFIX -- the string at DC1, NUL-padded to r0 bytes.  Leaf.
;   Clobbers A, r0, DC0, DC1.
FNFIX:  DCI FNTX
FNFX1:  XDC
        LM
        XDC
        NI 0FFH
        BZ FNFX2
        ST
        DS 0
        BNZ FNFX1
        POP
FNFX2:  CLR
FNFX3:  ST
        DS 0
        BNZ FNFX3
        POP

; FNTX0 -- a NUL.  Leaf.  Clobbers A, DC0.
FNTX0:  DCI FNTX
        CLR
        ST
        POP

; FNPOST -- launch the transaction: SEQ = the cart's ACKSEQ + 1 (255 wraps
;   to 1), kept in NSEQ.  Leaf.  Clobbers A, DC0.
FNPOST: DCI FNACKSQ
        LM
        INC
        BNZ FNPS1
        LIS 1
FNPS1:  DCI NSEQ
        ST
        DCI FNREG+FR_SEQ
        ST
        POP

; FNDONE -- Z set once the cart has echoed NSEQ.  Leaf.  Clobbers A, DC0.
FNDONE: DCI NSEQ
        LM
        DCI FNACKSQ
        CM
        POP

; FNGO -- post and wait (boot only: the loop never waits).  A = the
;   transaction's error.  Leaf.  Clobbers A, DC0.
FNGO:   DCI FNACKSQ
        LM
        INC
        BNZ FNGO1
        LIS 1
FNGO1:  DCI NSEQ
        ST
        DCI FNREG+FR_SEQ
        ST
FNGO2:  DCI NSEQ
        LM
        DCI FNACKSQ
        CM
        BNZ FNGO2
        DCI FNERR
        LM
        POP

; FNACK -- A = 0 if the last transaction worked and was ACKed.  Leaf.
;   Clobbers A, DC0.
FNACK:  DCI FNERR
        LM
        NI 0FFH
        BNZ FNACKX
        DCI FNRCMD
        LM
        CI 006H
        BZ FNACKO
        LI 0EEH
        POP
FNACKO: CLR
FNACKX: POP

; MEMCPY -- r0:r1 bytes (>= 1) from DC0 to DC1.  Leaf.  Clobbers A, r0, r1.
MEMCPY: LR A,0
        NI 0FFH
        BZ MCPY1
        LR A,1
        INC
        LR 1,A
MCPY1:  LM
        XDC
        ST
        XDC
        DS 0
        BNZ MCPY1
        DS 1
        BNZ MCPY1
        POP

; MEMZERO -- r0:r1 bytes (>= 1) at DC0 zeroed.  Leaf.  Clobbers A, r0, r1.
MEMZERO: LR A,0
        NI 0FFH
        BZ MZER1
        LR A,1
        INC
        LR 1,A
MZER1:  CLR
        ST
        DS 0
        BNZ MZER1
        DS 1
        BNZ MZER1
        POP

; --- the URL -----------------------------------------------------------------

; BLDURL -- the devicespec for ACTSENT (argument NARG), into the TX stream.
BLDURL: ENTER
        DCI U_N
        XDC
        PI FNTXS
        DCI SCENDPT
        XDC
        PI FNTXS
        LDB ACTSENT
        CI ACT_TABLS
        BNZ BU1
        DCI U_TABLS
        XDC
        PI FNTXS
        JMP BUNUL
BU1:    SL 1                    ; the route
        DCI ROUTES
        ADC
        LM
        LR 10,A
        LM
        LR 11,A
        LR DC,H
        XDC
        PI FNTXS
        LDB ACTSENT             ; what follows the route
        DCI ROUTEARG
        ADC
        LM
        NI 0FFH
        BZ BUQ
        CI 1
        BNZ BU2
        LDW 0, 1, NARG          ; a number
        PI UNUM
        BR BUQ
BU2:    CI 2
        BNZ BU3
        LDB NARG                ; push: hold/up/down, '/', the units shown
        CI 2
        BC BU2A
        CLR
BU2A:   SL 1
        DCI PUSHW
        ADC
        LM
        LR 10,A
        LM
        LR 11,A
        LR DC,H
        XDC
        PI FNTXS
        PI USLASH
        LDB AUTRADE
        LR 0,A
        CLR
        LR 1,A
        PI UNUM
        BR BUQ
BU3:    CI 3
        BNZ BU4
        LDB NARG                ; declare: n/b/s
        CI 2
        BC BU3A
        CLR
BU3A:   DCI DECLW
        ADC
        LM
        LR 0,A
        DCI FNTX
        LR A,0
        ST
        BR BUQ
BU4:    LDB AUTRADE             ; hold: the units shown, '/', the price
        LR 0,A
        CLR
        LR 1,A
        PI UNUM
        PI USLASH
        LDW 0, 1, NARG
        PI UNUM
BUQ:    DCI U_TABLE
        XDC
        PI FNTXS
        DCI SCTABL
        XDC
        PI FNTXS
        DCI U_PLAYR
        XDC
        PI FNTXS
        DCI SCNAME
        XDC
        PI FNTXS
        DCI U_BIN
        XDC
        PI FNTXS
BUNUL:  PI FNTX0
        LEAVE

; UNUM -- r0:r1 in decimal into the TX stream.
UNUM:   ENTER
        DCI LINBUF+32
        PI FMTNUM
        DCI LINBUF+32
        XDC
        PI FNTXS
        LEAVE

; USLASH -- a '/' into the TX stream.  Leaf.  Clobbers A, DC0.
USLASH: DCI FNTX
        LI '/'
        ST
        POP

U_N:    DB "N:",0
U_TABLS: DB "/tables?bin=1",0
U_TABLE: DB "?table=",0
U_PLAYR: DB "&player=",0
U_BIN:  DB "&bin=1",0

; The routes by ACT_*, and what follows each: 0 nothing, 1 the argument,
; 2 push word + units, 3 declare letter, 4 units + price.
ROUTES: DW R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13
        DW R14, R15, R16, R17, R18, R19, R20, R21
ROUTEARG: DB 0,0,0,0,1,1,2,3,0,0,0,1,1,0,1,1,1,0,0,0,0,4
R0:     DB 0
R1:     DB "/state",0
R2:     DB "/ready/1",0
R3:     DB "/ready/0",0
R4:     DB "/setup/species/",0
R5:     DB "/claim/",0
R6:     DB "/auction/push/",0
R7:     DB "/declare/",0
R8:     DB "/collude",0
R9:     DB "/turn/begin",0
R10:    DB "/turn/buymule",0
R11:    DB "/turn/outfit/",0
R12:    DB "/turn/install/",0
R13:    DB "/turn/return",0
R14:    DB "/turn/assay/",0
R15:    DB "/turn/sell/",0
R16:    DB "/turn/unsell/",0
R17:    DB "/turn/wampus",0
R18:    DB "/turn/pub",0
R19:    DB "/turn/end",0
R20:    DB "/leave",0
R21:    DB "/auction/push/hold/",0
PUSHW:  DW PW0, PW1, PW2
PW0:    DB "hold",0
PW1:    DB "up",0
PW2:    DB "down",0
DECLW:  DB "nbs"

; --- the transactions --------------------------------------------------------

; NPNET -- r1 = an N: command with r2 parameters: FNBEG for device N1:.
NPNET:  LI FD_NET
        LR 0,A
        JMP FNBEG

NPCLOS: ENTER
        LI NC_CLOS
        LR 1,A
        CLR
        LR 2,A
        PI NPNET
        PI FNPOST
        LEAVE

NPOPEN: ENTER
        LI NC_OPEN
        LR 1,A
        LIS 2
        LR 2,A
        PI NPNET
        LI NM_GET
        LR 0,A
        PI FNPB
        CLR
        LR 0,A
        PI FNPB
        PI BLDURL
        PI FNPOST
        LEAVE

NPSTAT: ENTER
        LI NC_STAT
        LR 1,A
        LIS 2
        LR 2,A
        PI NPNET
        CLR
        LR 0,A
        PI FNPB
        CLR
        LR 0,A
        PI FNPB
        PI FNPOST
        LEAVE

; NPREAD -- READ r0:r1 bytes.
NPREAD: ENTER
        LR A,0
        LR 3,A
        LR A,1
        LR 4,A
        LI NC_READ
        LR 1,A
        LIS 2
        LR 2,A
        PI NPNET
        LR A,3
        LR 0,A
        LR A,4
        LR 1,A
        PI FNPW
        CLR
        LR 0,A
        PI FNPB
        PI FNPOST
        LEAVE

; --- the engine --------------------------------------------------------------

; ACTSEND -- queue action r0 with argument r1:r2 (net.c act_send): it takes
;   the place of the next poll.  Leaf.  Clobbers A, DC0.
ACTSEND: DCI ACT
        LR A,0
        ST
        LR A,1
        ST
        LR A,2
        ST
        CLR
        STB POLLW
        POP

; NETBUSY -- Z clear while a request is under way.  Leaf.  Clobbers A, DC0.
NETBUSY: LDB NSTATE
        NI 0FFH
        POP

; NETBEGIN -- start the queued action (or a poll).
NETBEGIN: ENTER
        LDB NSTATE
        NI 0FFH
        BNZ NBX
        CLR
        STB NETHAVE
        STB NWAIT
        STB NGOT
        ST                      ; (NGOT+1)
        STB NBWLAST
        ST
        LDB ACT
        NI 0FFH
        BNZ NB1
        LIS ACT_STATE
NB1:    STB ACTSENT
        LDW 0, 1, ACTARG
        STW 0, 1, NARG
        CLR
        STB ACT
        PI NPCLOS
        LIS NS_CLOS
        STB NSTATE
NBX:    LEAVE

; NETSTEP -- one step: A = 1 when the request is over (NETHAVE: 1 a state
;   in ST, 2 an error header in ST's, 0 it failed), else 0.
NETSTEP: ENTER
        LDB NSTATE
        NI 0FFH
        BNZ NST1
        CLR
        LEAVE
NST1:   PI FNDONE
        BZ NST2
        INCB NWAIT              ; still out: give up after NTMOUT frames
        CI NTMOUT
        BC NSTW
        JMP NSFAIL
NSTW:   CLR
        LEAVE
NST2:   CLR
        STB NWAIT
        LDB NSTATE
        CI NS_CLOS
        BNZ NST3
        PI NPOPEN               ; (a CLOSE of nothing NAKs: either is fine)
        LIS NS_OPEN
        STB NSTATE
        CLR
        LEAVE
NST3:   CI NS_OPEN
        BNZ NST4
        PI FNACK
        NI 0FFH
        JNZ NSFAIL
NSTPS:  PI NPSTAT
        LIS NS_STAT
        STB NSTATE
        CLR
        LEAVE
NST4:   CI NS_STAT
        JNZ NSRD
        PI FNACK
        NI 0FFH
        JNZ NSFAIL
        DCI FNREPLY             ; bytes waiting, connected, nDevStatus
        LM
        LR 0,A
        LM
        LR 1,A
        LM
        LM
        STB NDEVST
        CI 1                    ; OK
        BZ NST5
        CI NS_EOF               ; end of file
        BZ NST5
        LR A,0                  ; an error, and nothing waiting: give up
        NI 0FFH
        BNZ NST5
        LR A,1
        NI 0FFH
        JZ NSFAIL
NST5:   LR A,0                  ; nothing yet: ask again ...
        NI 0FFH
        BNZ NST6
        LR A,1
        NI 0FFH
        BNZ NST6
        LDB NDEVST              ; ... unless it has all come: a short reply
        CI NS_EOF               ; (/leave answers "bye") ends here, not at
        BNZ NSTAG               ; the timeout
        LDB NGOT
        LR 2,A
        LDB NGOT+1
        OI 0
        BNZ NSTEOF
        LR A,2
        NI 0FFH
        BZ NSTAG
NSTEOF: JMP NSFAIL
NST6:   LDW 2, 3, NBWLAST       ; not what it was last time: ask again
        LR A,0
        XS 2
        BNZ NSTAG
        LR A,1
        XS 3
        BNZ NSTAG
        ; settled: read what is waiting, at most what is still wanted
        PI NWANT                ; r2:r3 = bytes still wanted
        PI GE16                 ; waiting >= wanted?
        BNC NST7
        LR A,2
        LR 0,A
        LR A,3
        LR 1,A
NST7:   PI NPREAD
        LIS NS_READ
        STB NSTATE
        CLR
        LEAVE
NSTAG:  STW 0, 1, NBWLAST
        JMP NSTPS

; the READ is in
NSRD:   PI FNACK
        NI 0FFH
        JNZ NSFAIL
        LDW 0, 1, FNRXLO        ; what came (nothing: give up)
        LR A,0
        NI 0FFH
        BNZ NSRD1
        LR A,1
        NI 0FFH
        JZ NSFAIL
NSRD1:  DCI FNREPLY             ; copy it out before anything else
        XDC
        PI NDEST                ; DC0 = where it goes (RXBUF or TBL, + got)
        XDC
        LR A,0
        LR 4,A
        LR A,1
        LR 5,A
        PI MEMCPY
        LDW 2, 3, NGOT          ; got += n
        LR A,4
        LR 0,A
        LR A,5
        LR 1,A
        PI ADD16
        STW 0, 1, NGOT
        LDB ACTSENT
        CI ACT_TABLS
        BNZ NSR1
        LIS 1                   ; the table list: one READ of a settled reply
        STB NETHAVE
        JMP NSFIN
NSR1:   LR A,1                  ; 16 or more: the header
        NI 0FFH
        BNZ NSR2
        LR A,0
        CI 15
        BC NSRMORE
NSR2:   LDB RXBUF+H_MAGIC
        CI 'M'
        JNZ NSFAIL
        LDB RXBUF+H_TYPE
        CI 1
        BZ NSR3
        DCI RXBUF               ; an error header only: keep it
        XDC
        DCI ST
        XDC
        LI 16
        LR 0,A
        CLR
        LR 1,A
        PI MEMCPY
        LIS 2
        STB NETHAVE
        JMP NSFIN
NSR3:   LDW 0, 1, NGOT
        LI STSIZE&0FFH
        LR 2,A
        LI STSIZE>>8
        LR 3,A
        PI GE16
        BNC NSRMORE
        DCI RXBUF               ; a whole state
        XDC
        DCI ST
        XDC
        LI STSIZE&0FFH
        LR 0,A
        LI STSIZE>>8
        LR 1,A
        PI MEMCPY
        LIS 1
        STB NETHAVE
        JMP NSFIN
NSRMORE: CLR                    ; more to come: back to STATUS
        STB NBWLAST
        ST
        JMP NSTPS

NSFAIL: CLR
        STB NETHAVE
NSFIN:  CLR                     ; (JMP loads A: NETHAVE is stored first)
        STB NSTATE
        LIS 1
        LEAVE

; NWANT -- r2:r3 = the bytes still wanted (STSIZE or TBLMAX, less NGOT);
;   keeps r0:r1.  Clobbers A, r4, r5, DC0.
NWANT:  ENTER
        LR A,0
        LR 4,A
        LR A,1
        LR 5,A
        LDB ACTSENT
        CI ACT_TABLS
        LI STSIZE&0FFH
        LR 0,A
        LI STSIZE>>8
        LR 1,A
        BNZ NWNT1               ; (LI and LR keep the flags of the CI)
        LI TBLMAX&0FFH
        LR 0,A
        LI TBLMAX>>8
        LR 1,A
NWNT1:  LDW 2, 3, NGOT
        PI SUB16
        LR A,0
        LR 2,A
        LR A,1
        LR 3,A
        LR A,4
        LR 0,A
        LR A,5
        LR 1,A
        LEAVE

; NDEST -- DC0 = where the next READ's bytes go: RXBUF or TBL, + NGOT.
;   Leaf.  Clobbers A, H, DC0.
NDEST:  LDB ACTSENT
        CI ACT_TABLS
        DCI RXBUF
        BNZ NDST1
        DCI TBL
NDST1:  LR H,DC
        LDB NGOT
        AS 11
        LR 11,A
        LDB NGOT+1              ; (DCI and LM keep the carry)
        LNK
        AS 10
        LR 10,A
        LR DC,H
        POP

; NSYNC -- send the queued action and wait for its answer, finishing any
;   request already under way first (net.c net_sync).  A = NETHAVE.
NSYNC:  ENTER
        PI NSYNCW
        PI NETBEGIN
        PI NSYNCW
        LDB NETHAVE
        LEAVE

; NSYNCW -- step the request in flight to its end, a frame at a time.
NSYNCW: ENTER
NSYW1:  PI NETBUSY
        BZ NSYWX
        PI FRWAIT
        PI NETSTEP
        BR NSYW1
NSYWX:  LEAVE
