; net.inc -- the network: one request at a time, a step a frame, never
; waiting (mekkogx net.c; the Arcadia's fixb.inc NBEGIN/NSTEP). Every call is
;     N:<endpoint><route>[<arg>]?table=<t>&player=<name>&bin=1
; and every one, action or poll, answers with the whole 550-byte state,
; which the cart COPYs from its reply window to SNAP, so a phase reads it in
; place until the next one is whole. The state before is copied to PREV
; first (mekkogx prev).

NS_IDLE  EQU    0
NS_CLOSE EQU    1               ; the last request's connection, closing
NS_OPEN  EQU    2
NS_STAT  EQU    3
NS_SWAIT EQU    4               ; between two STATUS readings
NS_READ  EQU    5
NS_COPY  EQU    6               ; the cart copying the reply to SNAP
NTIMEO   EQU    240             ; frames without progress before giving up
ERRHDR   EQU    CRAM+0F0H       ; 16: the header of a refusal (NH_ERR)

; ---- the mailbox ---------------------------------------------------------------
; mstart: a transaction on device R9.1, command R9.0, D parameters: TX
; rewound. Clobbers R8 (and RF's low byte).
        RFIT    100
mstart: PLO     R8
        REG     FN_REG_DATA_RST, 0
        LDI     FN_REG_DEVICE
        HOT     HI(FN_REGSEL)
        GHI     R9
        HOT     HI(FN_REGDATA)
        LDI     FN_REG_CMD
        HOT     HI(FN_REGSEL)
        GLO     R9
        HOT     HI(FN_REGDATA)
        LDI     FN_REG_NPARAM
        HOT     HI(FN_REGSEL)
        GLO     R8
        HOT     HI(FN_REGDATA)
        FRET

; txb: one TX byte, D. txpb: a one-byte parameter, D.
txpb:   PLO     R8
        LDI     1
        HOT     HI(FN_TXDATA)
        GLO     R8
txb:    HOT     HI(FN_TXDATA)
        FRET

; txs: the string at R8, to its NUL (R8 left past it).
        RFIT    14
txs:    LDI     HI(FN_TXDATA)
        PHI     RF
txs1:   LDA     R8
        BZ      txs2
        PLO     RF
        LDN     RF
        BR      txs1
txs2:   LDI     FN_PARK
        PHI     RF
        FRET

; txe: the string at R8 for a URL: letters, digits, '-', '_' and '.' as
; they are, the rest %XX (Battleship's txenc).
        RFIT    83
txe:    LDA     R8
        BZ      txe9
        PLO     R9
        SMI     '0'
        BNF     txe2
        SMI     10                      ; '0'-'9'
        BNF     txe1
        GLO     R9
        ANI     0DFH                    ; 'A'-'Z', 'a'-'z'
        SMI     'A'
        BNF     txe2
        SMI     26
        BNF     txe1
        GLO     R9
        XRI     '_'
        BZ      txe1
txe2:   GLO     R9
        XRI     '-'
        BZ      txe1
        GLO     R9
        XRI     '.'
        BZ      txe1
        LDI     '%'
        FCALL   txb
        GLO     R9
        SHR
        SHR
        SHR
        SHR
        FCALL   txhex
        GLO     R9
        ANI     0FH
        FCALL   txhex
        BR      txe
txe1:   GLO     R9
        FCALL   txb
        BR      txe
txe9:   FRET
; txhex: one hex digit, D (0-15). Keeps R9.
txhex:  SMI     10
        BDF     txh1
        ADI     '0'+10-'A'
txh1:   ADI     'A'
        HOT     HI(FN_TXDATA)
        FRET

; mpost: SEQ = ACKSEQ + 1 (255 wraps to 1): the transaction goes. NSEQ
; keeps it; ACKSEQ echoes it when the reply is in.
        RFIT    66
mpost:  PEEK    FN_ACKSEQ
        ADI     1
        BNZ     mpo1
        LDI     1
mpo1:   PUTV    NSEQ
        LDI     FN_REG_SEQ
        HOT     HI(FN_REGSEL)
        GETV    NSEQ
        HOT     HI(FN_REGDATA)
        FRET

; mdone: D = 0 once the posted transaction has answered.
mdone:  PEEK    FN_ACKSEQ
        STR     R2
        GETV    NSEQ
        XOR
        FRET

; mok: D = 0 when the answered transaction worked: no error, and ACK.
        RFIT    29
mok:    PEEK    FN_ERR
        BNZ     mok1
        PEEK    FN_REPLYCMD
        XRI     FCACK
mok1:   FRET

; ---- the main loop's part (mekkogx main.c) -------------------------------------
; net_tick: a step of the request under way, or the poll countdown (an
; action goes at once). The reply goes to `reply`.
        RFIT    59
net_tick:
        GETV    NSTATE
        BZ      ntk1
        FCALL   net_step
        BZ      ntk9                    ; not over
        FJMP    reply
ntk1:   GETV    POLLW
        BZ      ntk2
        SMI     1
        STR     RA
        BZ      ntk2
        GETV    ACT
        BZ      ntk9
ntk2:   GETV    TESTM
        BZ      ntk3
        BCALL   BK_TEST, E_TPOLL        ; the offline test mode answers itself
        FJMP    reply
ntk3:   FCALL   net_begin
        GETV    NSTATE
        BZ      ntk4                    ; it failed at once
ntk9:   FRET
ntk4:   FJMP    reply

; reply: the request is over (mekkogx reply()).
        RFIT    60
reply:  GETV    NHAVE
        XRI     NH_STATE
        BNZ     rpl2
        SETV    NFAIL, 0
        FCALL   st_arrive
        FJMP    poll_rate
rpl2:   XRI     NH_STATE!NH_ERR
        BNZ     rpl3
        SLD     ERRHDR+HDR_ERR          ; turned away: the table full, no seat
        XRI     ERR_TABLEFULL
        BZ      rpl4
        XRI     ERR_TABLEFULL!ERR_SEAT
        BNZ     rpl5
rpl4:   BCALL   BK_BOOT, E_RECHOOSE     ; another table
        FCALL   game_start
        BR      rpl5
rpl3:   GETV    NFAIL
        ADI     1
        STR     RA
rpl5:   FJMP    poll_rate

; poll_rate: frames to the next poll (mekkogx poll_rate).
        RFIT    64
poll_rate:
        LDI     30
        PLO     R9
        GETV    NFAIL
        BNZ     plr9
        GETV    PHASE
        SMI     9
        BDF     plr9
        GETV    PHASE
        ADI     LO(plrtab)
        PLO     R8
        LDI     HI(plrtab)
        PHI     R8
        LDN     R8
        PLO     R9
        GETV    PHASE
        XRI     PHASE_DEVELOP
        BNZ     plr9
        GETV    DVSTATE                 ; developing: walking, rarely
        XRI     DV_WALK
        BZ      plr9
        LDI     30
        PLO     R9
plr9:   GLO     R9
        PUTV    POLLW
        FRET
plrtab: DB      30,12,12,60,60,12,8,30,60

; net_sync: the queued action, waited for (the boot, leaving): D = NHAVE.
        RFIT    52
net_sync:
        FCALL   net_begin
ny1:    GETV    NSTATE
        BZ      ny2
        FCALL   frame_wait
        FCALL   snd_tick
        FCALL   net_step
        BZ      ny1
ny2:    GETV    NHAVE
        FRET

; act_send: queue action D, argument R9 (mekkogx act_send): it goes next.
act_send:
        PUTV    ACT
        GLO     R9
        PUTV    ACTARGL
        GHI     R9
        PUTV    ACTARGH
        SETV    POLLW, 0
        FRET

; ---- one request, a step a frame --------------------------------------------------
; net_begin: start the queued action, or a poll.
        RFIT    55
net_begin:
        GETV    ACT
        BNZ     nbg1
        LDI     ACT_STATE
nbg1:   PUTV    ACTSENT
        SETV    ACT, ACT_NONE
        SETV    NHAVE, NH_NONE
        LDR     R9, (NETDEV<<8)|NCCLOSE ; the last one's connection first
        LDI     0
        FCALL   mstart
        FCALL   mpost
        LDI     NS_CLOSE
nbgo:   PUTV    NSTATE
        SETV    NWAIT, 0
        FRET

; net_step: one step. D = 0 while under way; else over, NHAVE says how.
        RFIT    85
net_step:
        GETV    NWAIT
        ADI     1
        STR     RA
        SMI     NTIMEO
        BDF     nfail
        GETV    NSTATE
        XRI     NS_COPY
        BZ      ns_copy2
        XRI     NS_COPY!NS_SWAIT
        BZ      ns_swait
        FCALL   mdone                   ; the rest wait for their answer
        BNZ     nbusy
        GETV    NSTATE
        XRI     NS_CLOSE
        BZ      ns_close
        XRI     NS_CLOSE!NS_OPEN
        BZ      ns_open
        XRI     NS_OPEN!NS_STAT
        BZ      ns_stat2
        FJMP    ns_read
ns_stat2:
        FJMP    ns_stat
ns_copy2:
        FJMP    ns_copy
nbusy:  LDI     0                       ; under way
        FRET
ns_swait:
        GETV    NSUB                    ; a few frames between readings
        SMI     1
        STR     RA
        BNZ     nbusy
        FJMP    ns_status
nfail:  LDI     NH_FAIL
nover:  PUTV    NHAVE
        SETV    NSTATE, NS_IDLE
        LDI     1                       ; over
        FRET

; the close is in (a NAK is fine: there was nothing open): OPEN the URL
        RFIT    58
ns_close:
        LDR     R9, (NETDEV<<8)|NCOPEN
        LDI     2
        FCALL   mstart
        LDI     NMHTTPG
        FCALL   txpb
        LDI     0
        FCALL   txpb
        FCALL   bld_url
        FCALL   mpost
        SETV    BWL, 0
        SETV    BWH, 0
        LDI     NS_OPEN
        FCALL   nbgo
        LDI     0
        FRET

ns_open:
        FCALL   mok
        BNZ     nfail2
        FJMP    ns_status
nfail2: FJMP    nfail

; STATUS until the reply has settled: 550 waiting, or two readings that
; agree (a STATUS can come before all of it has)
        RFIT    33
ns_status:
        LDR     R9, (NETDEV<<8)|NCSTAT
        LDI     2
        FCALL   mstart
        LDI     0
        FCALL   txpb
        LDI     0
        FCALL   txpb
        FCALL   mpost
        SETV    NSTATE, NS_STAT
        LDI     0
        FRET

        RFIT    110
ns_stat:
        FCALL   mok
        BNZ     nfail4
        SPTR    FN_REPLY
        LDA     R8                      ; bytes waiting, LE
        PLO     R9
        LDA     R8
        PHI     R9
        INC     R8
        LDN     R8                      ; nDevStatus: 1 OK, 136 EOF
        XRI     1
        BZ      nst2
        XRI     1!136
        BZ      nst2
        GHI     R9                      ; an error with nothing waiting: over
        BNZ     nst2
        GLO     R9
        BZ      nfail4
nst2:   GETV    ACTSENT                 ; the table list: when it has settled
        XRI     ACT_TABLES
        BZ      nst4
        GHI     R9                      ; a state: 550 or more, read it
        SMI     HI(NREPLEN)
        BNF     nst4
        BNZ     nst5
        GLO     R9
        SMI     LO(NREPLEN)
        BDF     nst5
nst4:   GLO     R9                      ; nothing yet, or still coming: again
        STR     R2
        GHI     R9
        OR
        BZ      nst6
        GETV    BWL
        STR     R2
        GLO     R9
        XOR
        BNZ     nst6
        GETV    BWH
        STR     R2
        GHI     R9
        XOR
        BZ      nst5                    ; two readings agree: it has all come
nst6:   GLO     R9
        PUTV    BWL
        GHI     R9
        PUTV    BWH
        SETV    NSUB, 2                 ; read again in two frames
        SETV    NSTATE, NS_SWAIT
        LDI     0
        FRET
nst5:   FJMP    ns_rdgo
nfail4: FJMP    nfail

; READ what came (at most the state's 550)
        RFIT    62
ns_rdgo:
        LDR     R9, (NETDEV<<8)|NCREAD
        LDI     1
        FCALL   mstart
        LDI     2                       ; one 2-byte parameter: the length, LE
        FCALL   txb
        GETV    ACTSENT
        XRI     ACT_TABLES
        BNZ     nrd1
        GETV    BWL                     ; the list: what is there (1 + 33n)
        FCALL   txb
        GETV    BWH
        FCALL   txb
        BR      nrd2
nrd1:   LDI     LO(NREPLEN)
        FCALL   txb
        LDI     HI(NREPLEN)
        FCALL   txb
nrd2:   FCALL   mpost
        SETV    NSTATE, NS_READ
        LDI     0
        FRET

; the reply: a state, an error header, or nothing to keep
        RFIT    108
ns_read:
        FCALL   mok
        BNZ     nfail3
        GETV    ACTSENT                 ; the table list: left in the window
        XRI     ACT_TABLES
        BNZ     nrd3
        LDI     NH_TABLES
        FJMP    nover
nfail3: FJMP    nfail
nrd3:   SPTR    FN_REPLY                ; 'M', version 2
        LDA     R8
        XRI     'M'
        BNZ     nfail3
        LDA     R8
        XRI     MULE_PROTO_VERSION
        BNZ     nfail3
        LDN     R8
        XRI     MSG_STATE
        BZ      nrd4
        XRI     MSG_STATE!MSG_ERROR     ; a header only: why we were turned away
        BNZ     nfail3
        LDR     R9, ERRHDR
        LDI     HDR_SIZE
        BR      nrd6
nrd4:   PEEK    FN_RXLEN                ; the whole state
        XRI     LO(NREPLEN)
        BNZ     nfail3
        PEEK    FN_RXLEN+1
        XRI     HI(NREPLEN)
        BNZ     nfail3
        LDR     R9, SNAP
        LDI     0                       ; (0: the whole state)
nrd6:   FCALL   rxcopy
        SETV    NSTATE, NS_COPY
        LDI     0
        FRET

; the cart's COPY: done when FN_COPYDONE moves
        RFIT    39
ns_copy:
        PEEK    FN_COPYDONE
        STR     R2
        GETV    CPDONE
        XOR
        BZ      nbusy3
        GETV    CPLEN
        BZ      ncp1
        LDI     NH_ERR
        FJMP    nover
ncp1:   LDI     NH_STATE
        FJMP    nover
nbusy3: LDI     0
        FRET

; ccopy: the cart copies RB bytes from R8 to R9 (any of its bytes to an image
; page), queued behind what was sent before; CPDONE keeps FN_COPYDONE as it
; was, so ns_copy and ccopy_wait see it move.
        RFIT    207
ccopy:  PEEK    FN_COPYDONE
        PUTV    CPDONE
        LDI     FN_REG_CPSRCLO
        HOT     HI(FN_REGSEL)
        GLO     R8
        HOT     HI(FN_REGDATA)
        LDI     FN_REG_CPSRCHI
        HOT     HI(FN_REGSEL)
        GHI     R8
        HOT     HI(FN_REGDATA)
        LDI     FN_REG_CPDSTLO
        HOT     HI(FN_REGSEL)
        GLO     R9
        HOT     HI(FN_REGDATA)
        LDI     FN_REG_CPDSTHI
        HOT     HI(FN_REGSEL)
        GHI     R9
        HOT     HI(FN_REGDATA)
        LDI     FN_REG_CPLENLO
        HOT     HI(FN_REGSEL)
        GLO     RB
        HOT     HI(FN_REGDATA)
        LDI     FN_REG_CPLENHI
        HOT     HI(FN_REGSEL)
        GHI     RB
        HOT     HI(FN_REGDATA)          ; (this one runs it)
        FRET

; ccopy_wait: ccopy, then until the cart has done it.
ccopy_wait:
        FCALL   ccopy
ccw1:   PEEK    FN_COPYDONE
        STR     R2
        GETV    CPDONE
        XOR
        BZ      ccw1
        FRET

; rxcopy: the reply window to R9: D bytes (CPLEN), or 0 for the whole state.
rxcopy: PUTV    CPLEN
        BZ      rxc0
        PLO     RB
        LDI     0
        PHI     RB
        BR      rxc1
rxc0:   LDR     RB, NREPLEN
rxc1:   LDR     R8, FN_REPLY
        FJMP    ccopy

; snap_prev: SNAP to PREV, on the cart (mekkogx memcpy(&prev, &st)). The
; next state's COPY comes after it in the cart's queue.
snap_prev:
        LDR     R8, SNAP
        LDR     R9, PREV
        LDR     RB, NREPLEN
        FJMP    ccopy

; ---- the URL (mekkogx net.c compose) ------------------------------------------------
; bld_url: N:<endpoint><route>[arg]?table=<t>&player=<name>&bin=1 for
; ACTSENT into the TX stream.
        RFIT    129
bld_url:
        LDR     R8, u_n
        FCALL   txs
        SLD     CR_ENDPT                ; the room's server, or the build's
        BZ      bu1
        SPTR    CR_ENDPT
        BR      bu2
bu1:    LDR     R8, UENDPT
bu2:    FCALL   txs
        GETV    ACTSENT
        XRI     ACT_TABLES
        BNZ     bu3
        LDR     R8, u_tables            ; the list: no table, no player
        FJMP    txs
bu3:    GETV    ACTSENT                 ; the route
        SHL
        ADI     LO(routes)
        PLO     R8
        LDI     HI(routes)
        ADCI    0
        PHI     R8
        LDA     R8
        PHI     R9
        LDN     R8
        PLO     R8
        GHI     R9
        PHI     R8
        FCALL   txs                     ; (R8 past the NUL)
        DEC     R8
        DEC     R8
        LDN     R8
        XRI     '/'
        BNZ     bu9                     ; no argument
        FCALL   bld_arg
bu9:    LDR     R8, u_table
        FCALL   txs
        SPTR    CR_TABLE
        FCALL   txe
        LDR     R8, u_player
        FCALL   txs
        SPTR    CR_NAME
        FCALL   txe
        LDR     R8, u_bin
        FJMP    txs

; bld_arg: the route's argument (mekkogx compose): a push's word and the
; units we have shown, a hold's units and price, a declaration's letter,
; else the number.
        RFIT    119
bld_arg:
        GETV    ACTSENT
        XRI     ACT_PUSH
        BNZ     ba2
        GETV    ACTARGL                 ; hold, up, down
        SMI     3
        BNF     ba1
        SETV    ACTARGL, 0
ba1:    GETV    ACTARGL
        SHL
        ADI     LO(u_push)
        PLO     R8
        LDI     HI(u_push)
        PHI     R8
        LDA     R8
        PHI     R9
        LDN     R8
        PLO     R8
        GHI     R9
        PHI     R8
        FCALL   txs
        LDI     '/'
        FCALL   txb
        GETV    AUTRADE
        FJMP    txnum8
ba2:    XRI     ACT_PUSH!ACT_HOLD
        BNZ     ba3
        GETV    AUTRADE
        FCALL   txnum8
        LDI     '/'
        FCALL   txb
        BR      ba4
ba3:    XRI     ACT_HOLD!ACT_DECL
        BNZ     ba4
        GETV    ACTARGL                 ; n, b, s
        ADI     LO(u_decl)
        PLO     R8
        LDI     HI(u_decl)
        PHI     R8
        LDN     R8
        FJMP    txb
ba4:    GETV    ACTARGL
        PLO     R9
        GETV    ACTARGH
        PHI     R9
        FJMP    txnum
; txnum8 / txnum: the number D / R9 in decimal.
txnum8: PLO     R9
        LDI     0
        PHI     R9
txnum:  FCALL   fmt_num
        LDR     R8, NUMBUF
        FJMP    txs

; the URL's pieces (u_decl and u_push are indexed within their page)
        RFIT    63
u_decl: DB      "nbs"
u_push: DW      u_hold, u_up, u_down
u_hold: DB      "hold", 0
u_up:   DB      "up", 0
u_down: DB      "down", 0
u_n:    DB      "N:", 0
u_tables: DB    "/tables?bin=1", 0
u_table: DB     "?table=", 0
u_player: DB    "&player=", 0
u_bin:  DB      "&bin=1", 0
        DFIT    300
ROUTES_SEGEND:                          ; (data from here: no page rules)
routes: DW      r_none, r_state, r_ready1, r_ready0, r_species, r_claim, r_push
        DW      r_declare, r_collude, r_begin, r_buymule, r_outfit, r_install
        DW      r_return, r_assay, r_sell, r_unsell, r_wampus, r_pub, r_end
        DW      r_leave, r_hold
r_none: DB      0, 0
r_state: DB     "/state", 0
r_ready1: DB    "/ready/1", 0
r_ready0: DB    "/ready/0", 0
r_species: DB   "/setup/species/", 0
r_claim: DB     "/claim/", 0
r_push: DB      "/auction/push/", 0
r_declare: DB   "/declare/", 0
r_collude: DB   "/collude", 0
r_begin: DB     "/turn/begin", 0
r_buymule: DB   "/turn/buymule", 0
r_outfit: DB    "/turn/outfit/", 0
r_install: DB   "/turn/install/", 0
r_return: DB    "/turn/return", 0
r_assay: DB     "/turn/assay/", 0
r_sell: DB      "/turn/sell/", 0
r_unsell: DB    "/turn/unsell/", 0
r_wampus: DB    "/turn/wampus", 0
r_pub:  DB      "/turn/pub", 0
r_end:  DB      "/turn/end", 0
r_leave: DB     "/leave", 0
r_hold: DB      "/auction/push/hold/", 0
