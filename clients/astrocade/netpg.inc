; netpg.inc -- the server, through the N: device (net.c). The net page.
;
; Every call is an HTTP GET of
;     N:<endpoint><route>[<arg>]?table=<t>&player=<name>&bin=1
; and every call, action or poll, returns the whole 550-byte MuleState.
; One request at a time; an action (ACTSEND) takes the place of the next
; poll. A request is spread over frames so the game keeps moving: CLOSE the
; last one and OPEN (one frame), then a STATUS a frame until the reply has
; settled, then one READ of all of it into the cart's window.
;
; The family's rules (battleship net.inc), carried forward:
;  * CLOSE at the START of the next request, never after the READ: every
;    transaction repaints the reply window, and the state is read from it.
;  * RXLEN belongs to the latest transaction: captured right after READ.
;  * STATUS reports as soon as SOME of the reply is in: wait for two equal
;    readings -- and here for the whole 550 bytes, since it must be read in
;    one piece (a second READ would replace the first in the window).
;  * The window is never cleared: check 'M' and the message type.

; ---- the request engine -------------------------------------------------------
NCMD:   XOR     A               ; the reply comes back in slice 0
        LD      (CURSLC),A
        LD      A,NETDEV
        JP      FNBEGIN

NCLOSE: LD      E,NCCLOSE
        LD      L,0
        CALL    NCMD
        LD      B,3
        JP      FNCOMMIT

; NOPEN: OPEN the devicespec for URLREQ as HTTP GET. Carry on failure.
NOPEN:  LD      E,NCOPEN
        LD      L,2
        CALL    NCMD
        LD      A,NMHTTPG
        CALL    FNPARB
        LD      A,NTRNONE
        CALL    FNPARB
        CALL    BLDURL
        LD      B,8
        CALL    FNCOMMIT
        RET     C
        JR      NZ,NOBAD
        LD      A,(FNREPLY)
        CP      FCACK
        RET     Z
NOBAD:  SCF
        RET

; NSTATUS: one STATUS. Carry on failure; else AVAIL, and A = devstatus.
NSTATUS: LD     E,NCSTAT
        LD      L,2
        CALL    NCMD
        XOR     A
        CALL    FNPARB
        XOR     A
        CALL    FNPARB
        LD      B,4
        CALL    FNCOMMIT
        RET     C
        JR      NZ,NOBAD
        LD      A,(FNREPLY)
        CP      FCACK
        JR      NZ,NOBAD
        LD      A,(FNRDATA+NSAVLO)
        LD      (AVAIL),A
        LD      A,(FNRDATA+NSAVHI)
        LD      (AVAIL+1),A
        XOR     A
        LD      (CURSLC),A
        LD      A,(FNRDATA+NSDEVST)
        OR      A               ; clear carry
        RET

; NREAD: READ min(AVAIL, 1024) into the window; RXLEN. Carry on failure.
NREAD:  LD      HL,(AVAIL)
        LD      DE,1024
        OR      A
        SBC     HL,DE
        JR      C,NRD1
        EX      DE,HL
        JR      NRD2
NRD1:   ADD     HL,DE
NRD2:   PUSH    HL
        LD      E,NCREAD
        LD      L,1
        CALL    NCMD
        LD      A,2
        CALL    FNTXBYT
        POP     HL
        PUSH    HL
        LD      A,L
        CALL    FNTXBYT
        POP     HL
        LD      A,H
        CALL    FNTXBYT
        LD      B,8
        CALL    FNCOMMIT
        RET     C
        JR      NZ,NOBAD
        LD      A,(FNREPLY)
        CP      FCACK
        JR      NZ,NOBAD
        LD      A,(FNRXLL)
        LD      (RXLEN),A
        LD      A,(FNRXLH)
        LD      (RXLEN+1),A
        XOR     A
        LD      (CURSLC),A
        RET

; NETBEGIN: start the queued action (or a poll).
NETBEGIN: LD    A,(NSTATE)
        OR      A
        RET     NZ
        LD      (NETHAVE),A
        LD      (STVALID),A     ; the window is about to be repainted
        LD      A,(ACT)
        OR      A
        JR      NZ,NB1
        LD      A,A_STATE
NB1:    LD      (URLREQ),A
        LD      (ACTSNT),A
        XOR     A
        LD      (ACT),A
        CALL    NCLOSE          ; the last request's connection
        CALL    NOPEN
        JR      C,NFIN0
        LD      HL,0
        LD      (PRVAVL),HL
        XOR     A
        LD      (NWAIT),A
        INC     A
        LD      (NSTATE),A
        RET

NFIN0:  XOR     A
NFIN:   LD      (NETHAVE),A
        XOR     A
        LD      (NSTATE),A
        SCF
        RET

; NETSTEP: one step. Carry set when the request is over (NETHAVE: 1 a
; state, 2 an error header only, 0 failed).
NETSTEP: LD     A,(NSTATE)
        OR      A
        RET     Z
        LD      HL,NWAIT
        INC     (HL)
        LD      A,(HL)
        CP      NTMOUT
        JR      NC,NFIN0
        CALL    NSTATUS
        JR      C,NFIN0
        LD      C,A             ; devstatus
        LD      HL,(AVAIL)
        LD      A,H
        OR      L
        JR      NZ,NS1
        LD      A,C             ; nothing, and not OK: failed
        CP      NDSOK
        JR      Z,NS9
        CP      NDSEOF
        JR      Z,NS9
        JR      NFIN0
NS1:    LD      DE,(PRVAVL)
        LD      (PRVAVL),HL
        OR      A
        SBC     HL,DE
        JR      NZ,NS9          ; still arriving
        LD      HL,(AVAIL)      ; settled: all of it, or the server is done
        LD      DE,STSIZE
        OR      A
        SBC     HL,DE
        JR      NC,NS2
        LD      A,C
        CP      NDSEOF
        JR      NZ,NS9
NS2:    XOR     A
        LD      (NWAIT),A
        CALL    NREAD
        JR      C,NFIN0
        LD      HL,(RXLEN)
        LD      DE,16
        OR      A
        SBC     HL,DE
        JR      C,NFIN0
        LD      A,(FNRDATA)
        CP      'M'
        JR      NZ,NFIN0
        LD      A,(FNRDATA+2)
        CP      1               ; MSG_STATE
        JR      Z,NS3
        LD      A,(FNRDATA+W_ERR)
        LD      (HOTH+W_ERR),A  ; an error header: st.h.err says why
        LD      A,2
        JR      NFIN
NS3:    LD      HL,(RXLEN)
        LD      DE,STSIZE
        OR      A
        SBC     HL,DE
        JP      C,NFIN0
        LD      A,1
        LD      (STVALID),A
        JP      NFIN
NS9:    OR      A               ; not over yet
        RET

; XNETSYNC: send the queued action now and wait for its answer (finishing
; any poll on its way first). A = NETHAVE.
XNETSYNC: LD    B,2
NSY1:   PUSH    BC
NSY2:   LD      A,(NSTATE)
        OR      A
        JR      Z,NSY3
        CALL    FRWAIT
        CALL    NETSTEP
        JR      NSY2
NSY3:   POP     BC
        DEC     B
        JR      Z,NSY4
        PUSH    BC
        CALL    NETBEGIN
        POP     BC
        JR      NSY1
NSY4:   LD      A,(NETHAVE)
        RET

; ---- the URL, streamed into the TX buffer ------------------------------------
BLDURL: LD      A,(ENDPT)
        CP      'N'
        JR      Z,BURL1
        LD      A,'N'
        CALL    FNTXBYT
        LD      A,':'
        CALL    FNTXBYT
BURL1:  LD      HL,ENDPT
        CALL    FNTXSTR
        LD      A,(URLREQ)
        CP      A_TABLES
        JR      NZ,BURL2
        LD      HL,UTABLS
        JP      FNTXSTR
BURL2:  ADD     A,A
        LD      L,A
        LD      H,0
        LD      DE,ROUTES
        ADD     HL,DE
        LD      A,(HL)
        INC     HL
        LD      H,(HL)
        LD      L,A
        CALL    FNTXSTR         ; HL ends on the NUL
        DEC     HL
        LD      A,(HL)
        CP      '/'
        JR      NZ,BURLQ
        LD      A,(URLREQ)
        CP      A_HOLD
        JR      Z,BURLH
        CP      A_PUSH
        JR      NZ,BURL3
        LD      HL,UPUSHW       ; hold / up / down, then the units shown
        CALL    BURLW
        LD      A,'/'
        CALL    FNTXBYT
        LD      A,(AUTRADE)
        LD      L,A
        LD      H,0
        CALL    BURLN
        JR      BURLQ
BURLH:  LD      HL,UHOLD        ; hold/, the units shown, /, our price
        CALL    FNTXSTR
        LD      A,(AUTRADE)
        LD      L,A
        LD      H,0
        CALL    BURLN
        LD      A,'/'
        CALL    FNTXBYT
        JR      BURL4
BURL3:  CP      A_DECL
        JR      NZ,BURL4
        LD      HL,UDECLW       ; n / b / s
        CALL    BURLW
        JR      BURLQ
BURL4:  LD      HL,(ACTARG)
        CALL    BURLN
BURLQ:  LD      HL,UQTAB
        CALL    FNTXSTR
        LD      HL,SCTABL
        CALL    FNTXSTR
        LD      HL,UQPLR
        CALL    FNTXSTR
        LD      HL,SCNAME
        CALL    FNTXSTR
        LD      HL,UQBIN
        JP      FNTXSTR

; BURLW: word ACTARG (0-2, else 0) of the NUL-separated list at HL.
BURLW:  LD      A,(ACTARG)
        CP      3
        JR      C,BUW1
        XOR     A
BUW1:   OR      A
        JP      Z,FNTXSTR
BUW2:   LD      C,A
BUW3:   LD      A,(HL)
        INC     HL
        OR      A
        JR      NZ,BUW3
        LD      A,C
        DEC     A
        JR      BUW1

; BURLN: HL in decimal.
BURLN:  CALL    NUMSTR
        LD      HL,LINBUF
        JP      FNTXSTR

ROUTES: DW      UR0,UR1,UR2,UR3,UR4,UR5,UR6,UR7,UR8,UR9,UR10
        DW      UR11,UR12,UR13,UR14,UR15,UR16,UR17,UR18,UR19,UR20
        DW      UR0,UR6         ; (A_TABLES has its own), A_HOLD
UR0:    DB      "/state",0
UR1:    DB      "/state",0
UR2:    DB      "/ready/1",0
UR3:    DB      "/ready/0",0
UR4:    DB      "/setup/species/",0
UR5:    DB      "/claim/",0
UR6:    DB      "/auction/push/",0
UR7:    DB      "/declare/",0
UR8:    DB      "/collude",0
UR9:    DB      "/turn/begin",0
UR10:   DB      "/turn/buymule",0
UR11:   DB      "/turn/outfit/",0
UR12:   DB      "/turn/install/",0
UR13:   DB      "/turn/return",0
UR14:   DB      "/turn/assay/",0
UR15:   DB      "/turn/sell/",0
UR16:   DB      "/turn/unsell/",0
UR17:   DB      "/turn/wampus",0
UR18:   DB      "/turn/pub",0
UR19:   DB      "/turn/end",0
UR20:   DB      "/leave",0
UPUSHW: DB      "hold",0,"up",0,"down",0
UHOLD:  DB      "hold/",0
UDECLW: DB      "n",0,"b",0,"s",0
UTABLS: DB      "/tables?bin=1",0
UQTAB:  DB      "?table=",0
UQPLR:  DB      "&player=",0
UQBIN:  DB      "&bin=1",0

; REPLY1: a request is over (main.c reply): A = 1 a state arrived (PHS
; and the rest set, NEWPH if the phase changed), 2 refused outright (the
; table is full, no seat), 0 nothing to do.
REPLY1: LD      A,(NETHAVE)
        CP      1
        JR      NZ,RP1
        XOR     A
        LD      (NETFAIL),A
        JR      ARRIVE
RP1:    CP      2
        JR      NZ,RP2
        LD      A,(HOTH+W_ERR)
        CP      ERR_TFULL
        JR      Z,RP1A
        CP      ERR_SEAT
        JR      NZ,RP0
RP1A:   LD      A,2
        RET
RP2:    LD      HL,NETFAIL
        INC     (HL)
        JR      NZ,RP0
        DEC     (HL)
RP0:    XOR     A
        RET

; POLLRATE: how often to ask (main.c poll_rate).
POLLRATE:
        LD      B,30
        LD      A,(NETFAIL)
        OR      A
        JR      NZ,PR9
        LD      A,(PHS)
        CP      PH_GRANT
        JR      Z,PR12
        CP      PH_LAUC
        JR      Z,PR12
        CP      PH_DECL
        JR      Z,PR12
        CP      PH_TRADE
        JR      Z,PR8
        CP      PH_PROD
        JR      Z,PR60
        CP      PH_OVER
        JR      Z,PR60
        CP      PH_DEVEL
        JR      NZ,PR9
        LD      A,(DVSTATE)
        CP      DV_WALK
        JR      NZ,PR9
PR60:   LD      B,60
        JR      PR9
PR12:   LD      B,12
        JR      PR9
PR8:    LD      B,8
PR9:    LD      A,B
        LD      (POLLW),A
        RET


; ARRIVE: a whole state is in the window (main.c state_arrived).
ARRIVE: LD      HL,W_PHASE
        CALL    RXGETB
        CP      PH_OVER+1
        JR      NC,RP0
        LD      (PHS),A
        LD      HL,W_ROUND
        CALL    RXGETB
        LD      (RND),A
        LD      HL,W_SEAT
        CALL    RXGETB
        LD      (MYSEAT),A
        LD      HL,W_ELAP
        CALL    RXGETW
        LD      (RXPE),DE
        LD      HL,(FRAMES)
        LD      (RXFRM),HL
        LD      A,1
        LD      (STOK),A
        LD      A,(LASTPH)      ; a new phase (or the grant of a new round)
        LD      B,A
        LD      A,(PHS)
        CP      B
        JR      NZ,AR1
        CP      PH_GRANT
        JR      NZ,AR0
        LD      A,(LASTRND)
        LD      B,A
        LD      A,(RND)
        CP      B
        JR      NZ,AR1
AR0:    XOR     A
        JR      AR2
AR1:    LD      A,1
AR2:    LD      (NEWPH),A
        LD      A,1
        RET

; HOTCPY: the window's state into HOT (prev = st).
HOTCPY: LD      HL,0
        LD      DE,HOTH
        LD      BC,16
        CALL    RXCOPY
        LD      HL,W_PL+PL_SPEC
        LD      B,4
HC1:    PUSH    BC
        PUSH    HL
        LD      BC,HPLSZ
        CALL    RXCOPY
        POP     HL
        LD      BC,PLSZ
        ADD     HL,BC
        POP     BC
        DJNZ    HC1
        LD      HL,W_TERR       ; the whole map
        LD      BC,135          ; terrain, owner, mule
        CALL    RXCOPY
        LD      A,(PHS)
        LD      HL,W_GRANT
        LD      BC,8
        CP      PH_GRANT
        JR      Z,HC2
        LD      HL,W_TURN
        LD      BC,14           ; the turn, without the wampus list
        CP      PH_DEVEL
        JR      Z,HC2
        LD      HL,W_AUC
        LD      BC,58
        CP      PH_LAUC
        JR      Z,HC2
        CP      PH_DECL
        JR      Z,HC2
        CP      PH_TRADE
        JR      NZ,HC9
HC2:    LD      DE,HOTU
        CALL    RXCOPY
HC9:    LD      A,(PHS)
        LD      (LASTPH),A
        LD      A,(RND)
        LD      (LASTRND),A
        RET


; ---- appkeys ------------------------------------------------------------------
; AKOPEN: key B, mode A. Carry/err from FNCOMMIT.
AKOPEN: PUSH    AF
        PUSH    BC
        LD      A,FNDFUJI
        LD      E,FCOKEY
        LD      L,0
        CALL    FNBEGIN
        LD      A,AKCRLO
        CALL    FNTXBYT
        LD      A,AKCRHI
        CALL    FNTXBYT
        LD      A,AKAPP
        CALL    FNTXBYT
        POP     BC
        LD      A,B
        CALL    FNTXBYT
        POP     AF
        CALL    FNTXBYT
        XOR     A
        CALL    FNTXBYT         ; reserved: must be sent
        LD      B,3
        JP      FNCOMMIT

AKCLOSE: LD     A,FNDFUJI
        LD      E,FCCKEY
        LD      L,0
        CALL    FNBEGIN
        LD      B,3
        JP      FNCOMMIT

; AKREAD: read key B into the window: A = its length (0 none, at most 64),
; the data at FNRDATA+2.
AKREAD: LD      A,AKMRD
        CALL    AKOPEN
        JR      C,AKR0
        JR      NZ,AKR0
        LD      A,FNDFUJI
        LD      E,FCRKEY
        LD      L,0
        CALL    FNBEGIN
        LD      B,3
        CALL    FNCOMMIT
        JR      C,AKR0
        JR      NZ,AKR0
        XOR     A
        LD      (CURSLC),A
        LD      A,(FNREPLY)
        CP      FCACK
        JR      NZ,AKR0
        LD      A,(FNRDATA+1)
        OR      A
        JR      NZ,AKR64
        LD      A,(FNRDATA)
        CP      65
        JR      C,AKR1
AKR64:  LD      A,64
AKR1:   LD      B,A             ; the reply stays in the window: no close
        RET                     ; (a close would repaint it)
AKR0:   XOR     A
        RET

; AKWRITE: write B bytes at HL to key C.
AKWRITE: PUSH   HL
        PUSH    BC
        LD      B,C
        LD      A,AKMWR
        CALL    AKOPEN
        POP     BC
        POP     HL
        JR      C,AKCLOSE
        PUSH    BC
        PUSH    HL
        LD      A,FNDFUJI
        LD      E,FCWKEY
        LD      L,0
        CALL    FNBEGIN
        POP     HL
        POP     BC
        LD      A,B
        OR      A
        JR      Z,AKW2
AKW1:   LD      A,(HL)
        CALL    FNTXBYT
        INC     HL
        DJNZ    AKW1
AKW2:   LD      B,3
        CALL    FNCOMMIT
        JP      AKCLOSE

; APKLOAD: the name and the table from the lobby's appkeys (net.c
; appkey_load).
APKLOAD: XOR    A
        LD      (SCNAME),A
        LD      (SCTABL),A
        CALL    ENDDEF
        ; the name: A-Z and 0-9 only go into the URL unescaped. Read while
        ; the reply is in the window (the close repaints it).
        LD      B,AKNAME
        CALL    AKREAD
        OR      A
        JR      Z,APK3
        CP      9
        JR      C,APK1
        LD      A,8
APK1:   LD      B,A
        LD      HL,FNRDATA+2
        LD      DE,SCNAME
APK2:   LD      A,(HL)
        OR      A
        JR      Z,APK2E
        CP      'a'
        JR      C,APK2A
        CP      'z'+1
        JR      NC,APK2A
        SUB     20H
APK2A:  CALL    ALNUM
        JR      C,APKBAD
        LD      (DE),A
        INC     HL
        INC     DE
        DJNZ    APK2
APK2E:  XOR     A
        LD      (DE),A
        JR      APK3
APKBAD: XOR     A
        LD      (SCNAME),A
APK3:   CALL    AKCLOSE
        ; the table: only a URL counts (an unset key can read back as zeros)
        LD      B,AKROOM
        CALL    AKREAD
        CP      5
        JR      C,APK9
        LD      C,A
        LD      A,(FNRDATA+2)
        CP      'h'
        JR      NZ,APK9
        LD      B,0
        LD      HL,FNRDATA+2
        LD      DE,ENDPT
        LDIR
        XOR     A
        LD      (DE),A
        CALL    SPLITURL
APK9:   CALL    AKCLOSE
        ; no trailing '/', so "<endpoint>/state" never has two
        LD      HL,ENDPT
        CALL    STRLEN
        OR      A
        RET     Z
        LD      E,A
        LD      D,0
        ADD     HL,DE
        DEC     HL
        LD      A,(HL)
        CP      '/'
        RET     NZ
        LD      (HL),0
        RET

; ENDDEF: the build's endpoint (make ENDPOINT=...).
ENDDEF: LD      HL,UENDPT
        LD      DE,ENDPT
        LD      BC,ENDPTN
        LDIR
        XOR     A
        LD      (ENDPT+ENDPTN),A
        RET

; SPLITURL: "https://host/?table=xyz" -> ENDPT "https://host/", SCTABL
; "xyz" (letters and digits only, else no table).
SPLITURL: LD    HL,ENDPT
SPU1:   LD      A,(HL)
        OR      A
        RET     Z
        CP      '?'
        JR      Z,SPU2
        INC     HL
        JR      SPU1
SPU2:   LD      (HL),0
        INC     HL
        LD      DE,SQTAB
        LD      B,6
SPU3:   LD      A,(DE)
        CP      (HL)
        RET     NZ
        INC     HL
        INC     DE
        DJNZ    SPU3
        LD      DE,SCTABL
        LD      B,8
SPU4:   LD      A,(HL)
        OR      A
        JR      Z,SPU6
        CP      '&'
        JR      Z,SPU6
        LD      C,A
        CP      'a'
        JR      C,SPU5
        CP      'z'+1
        JR      NC,SPU5
        SUB     20H
SPU5:   CALL    ALNUM
        JR      C,SPUBAD
        LD      A,C
        LD      (DE),A
        INC     HL
        INC     DE
        DJNZ    SPU4
SPU6:   XOR     A
        LD      (DE),A
        RET
SPUBAD: XOR     A
        LD      (SCTABL),A
        RET

SQTAB:  DB      "table="

; APKSAVE: key 10 = <endpoint>/?table=<id> (net.c appkey_save_room). The
; URL is built in the phase RAM (BTBUF and past it: 90 bytes free there).
APKSAVE: LD     HL,ENDPT
        LD      DE,PHRAM+32
        CALL    STRCPY
        LD      HL,SQURL
        CALL    STRCPY
        LD      HL,SCTABL
        CALL    STRCPY
        LD      HL,PHRAM+32     ; B = length
        EX      DE,HL
        OR      A
        SBC     HL,DE
        LD      B,L
        EX      DE,HL
        LD      C,AKROOM
        JP      AKWRITE

SQURL:  DB      "/?table=",0

; APKCLR: forget the table (net.c appkey_clear_room).
APKCLR: XOR     A
        LD      (SCTABL),A
        LD      B,A
        LD      C,AKROOM
        LD      HL,SCTABL
        JP      AKWRITE

; STRCPY: HL -> DE up to the NUL (not copied); DE ends on it.
STRCPY: LD      A,(HL)
        LD      (DE),A
        OR      A
        RET     Z
        INC     HL
        INC     DE
        JR      STRCPY


; TBFETCH: /tables?bin=1 into the window, blocking; TBN = the count.
TBFETCH: XOR    A
        LD      (TBN),A
        LD      A,A_TABLES
        LD      (URLREQ),A
        CALL    NCLOSE
        CALL    NOPEN
        RET     C
        LD      HL,0
        LD      (PRVAVL),HL
        LD      B,180
TBF1:   PUSH    BC
        CALL    FRWAIT
        CALL    FRWAIT
        CALL    NSTATUS
        POP     BC
        RET     C
        LD      HL,(AVAIL)
        LD      A,H
        OR      L
        JR      Z,TBF2
        LD      DE,(PRVAVL)
        LD      (PRVAVL),HL
        OR      A
        SBC     HL,DE
        JR      Z,TBF3
TBF2:   DJNZ    TBF1
        RET
TBF3:   CALL    NREAD
        RET     C
        LD      A,(FNRDATA)
        CP      TBMAX+1
        JR      C,TBF4
        LD      A,TBMAX
TBF4:   LD      B,A             ; no more than the reply holds
        LD      HL,(RXLEN)
        DEC     HL
        LD      C,0
TBF5:   LD      A,B
        OR      A
        JR      Z,TBF6
        LD      DE,33
        OR      A
        SBC     HL,DE
        JR      C,TBF6
        INC     C
        DEC     B
        JR      TBF5
TBF6:   LD      A,C
        LD      (TBN),A
        RET

