; fixb.inc -- fixed B, $2000-$28FF (2650 page 1, with the mailbox and RAM-H):
; the same in every page's assembly and always mapped.
;
; The main loop and the phases' dispatch, the mailbox, the network (one
; request at a time, one step a frame, never waiting), the URL, and the
; copy of each reply into cart RAM. Page-1 code reaches console RAM and
; RAM-L through the pointer words at the end.

; ---- start ------------------------------------------------------------------
MAININIT:
        EORZ    R0
        CPUT    STOK
        EORZ    R0
        CPUT    TESTM
        EORZ    R0
        CPUT    CLKHAV
        EORZ    R0
        CPUT    AUTRD
        LODI,R0 $FF
        CPUT    LASTPH
        LODI,R0 $FF
        CPUT    LASTRD
        LODI,R0 13              ; the message box until a screen sets its own
        CPUT    MSGROW
        LODI,R0 4
        CPUT    MSGN
        EORZ    R0
        STRA,R0 *PNSTATE
        LODI,R1 PG_TITLE        ; the title (the harness may ask for test mode)
        LODI,R3 E_ENTER
        BSTA,UN PGCALL
        LODI,R1 PG_BOOT         ; the boot: name, table
        LODI,R3 E_ENTER
        BSTA,UN PGCALL
        LODA,R0 *P_TESTM        ; or the offline test mode (its fake state)
        BCTR,EQ MI1
        LODI,R1 PG_TEST
        LODI,R3 E_ENTER
        BSTA,UN PGCALL
MI1:    BSTA,UN GAMESTART
; ---- the main loop (mekkogx main.c) -------------------------------------------
MAINLP: BSTA,UN VBSYNC
        BSTA,UN TICKRD
        BSTA,UN KEYDEC
        LODA,R0 *PEVKEY         ; Select: sound off, or on
        COMI,R0 K_SEL
        BCFR,EQ ML0
        LODA,R0 *P_MUTE
        EORI,R0 1
        CPUT    MUTE
ML0:    LODA,R0 *PSTOK          ; watching (no seat): only Option counts
        BCTR,EQ ML1
        LODA,R0 *P_H_MYSEAT
        COMI,R0 4
        BCTR,LT ML1
        EORZ    R0
        STRA,R0 *PINDIR
        STRA,R0 *PEVBTN
        LODA,R0 *PEVKEY
        COMI,R0 K_OPT
        BCTR,EQ ML1
        LODI,R0 K_NONE
        STRA,R0 *PEVKEY
ML1:
        IF      AUTOPLAY
        LODA,R0 *PSTOK
        BCTR,EQ ML1A
        LODI,R1 PG_AUTO         ; plays by itself: the stick and the button
        LODI,R3 E_ENTER
        BSTA,UN PGCALL
ML1A:
        ENDIF
        BCTA,UN NETTICK         ; (inline: it comes back to NETDONE)
NETDONE: LODA,R0 *PSTOK
        BCTA,EQ MAINLP
        LODA,R0 *PEVKEY         ; Option: leave the table (asked in a game)
        COMI,R0 K_OPT
        BCFR,EQ ML2
        LODI,R1 PG_BOOT         ; the question, and leaving
        LODI,R3 E_LEAVE
        BSTA,UN PGCALL
        BCTA,UN MAINLP
ML2:    LODA,R1 *PPHPAGE        ; the phase's frame
        LODI,R3 E_TICK
        BSTR,UN PGCALL
        BSTA,UN PGNEWCK
        BSTA,UN SERVICES        ; the map, the sprite scripts
        LODA,R0 *PFRML          ; watching: the tag, every 32 frames
        ANDI,R0 $1F
        BCFA,EQ MAINLP
        LODI,R1 PG_CORE
        LODI,R3 E_SPECTG
        BSTR,UN PGFAR
        BCTA,UN MAINLP

; SERVICES: what the phases share, each frame: the map (drawn whole when
; asked, else kept current a few plots a frame, with the time bar), the
; sprite scripts.
SERVICES:
        LODA,R0 *P_MAPREQ
        BCTR,EQ SV1
        EORZ    R0
        LPUT    MAPREQ
        LODI,R1 PG_MAP
        LODI,R3 E_ENTER
        BSTA,UN PGCALL
SV1:    LODA,R0 *P_MAPON
        BCTR,EQ SV2
        LODI,R1 PG_MAP
        LODI,R3 E_TICK
        BSTA,UN PGCALL
SV2:    LODA,R0 *P_FXON
        RETC,EQ
        LODI,R1 PG_FX
        LODI,R3 E_TICK
        BCTA,UN PGCALL

; PGCALL: entry R3 of page R1, which stays in the window. Calls the page,
; costing one return address, not two.
PGCALL: STRA,R1 *PCURPG
        LODA,R0 FNBANK,R1
        BXA     WIN,R3          ; (the entry's return is our caller's)
; PGFAR: the same, then the page that was there back: so a page may call a
; fixed routine that calls another page. One level only (PGSAVE).
PGFAR:  LODA,R0 *PCURPG
        STRA,R0 *PPGSAVE
        BSTR,UN PGCALL
        LODA,R1 *PPGSAVE
        STRA,R1 *PCURPG
        LODA,R0 FNBANK,R1
        RETC,UN

; A page that wants another to take over sets PHPAGE and PGNEW: the new
; page's E_ENTER runs at once (pages never call each other).
PGNEWCK: LODA,R0 *PPGNEW
        RETC,EQ
        EORZ    R0
        CPUT    PGNEW
        LODA,R1 *PPHPAGE
        LODI,R3 E_ENTER
        BSTR,UN PGCALL
        BCTR,UN PGNEWCK

; A new game, or a new table (mekkogx game_start).
GAMESTART:
        LODI,R0 $FF
        CPUT    LASTPH
        LODI,R0 $FF
        CPUT    LASTRD
        EORZ    R0
        CPUT    STOK
        EORZ    R0
        STRA,R0 *PPOLLW
        STRA,R0 *PACT
        LODI,R1 PG_BOOT         ; "JOINING..."
        LODI,R3 E_EXTRA
        BCTA,UN PGCALL          ; tail call

; ---- the clock ----------------------------------------------------------------
; TICKRD: DT = the cart's ticks since the last loop (1-8), added to our
; frame clock. The tick and the vblank are not locked together, so a frame
; can see 0 or 2: 0 counts as 1. Leaf. Clobbers R0-R2.
TICKRD: LODA,R0 FNTICKH         ; HI, LO, HI: the bytes change together
        STRZ    R1
        LODA,R2 FNTICKL
        LODA,R0 FNTICKH
        COMZ    R1
        BCFR,EQ TICKRD
        LODZ    R2
        SUBA,R0 *PTICKL1
        STRA,R1 *PTICKH1
        STRA,R2 *PTICKL1
        BCFR,EQ TKR1
        LODI,R0 1
TKR1:   COMI,R0 8
        BCTR,LT TKR2
        LODI,R0 8
TKR2:   STRA,R0 *PDT
        ADDA,R0 *PFRML          ; (C: the carry)
        STRA,R0 *PFRML
        PPSL    PSLWC
        LODA,R0 *PFRMH
        ADDI,R0 0
        STRA,R0 *PFRMH
        CPSL    PSLWC
        RETC,UN

; ---- the network, as the main loop runs it ----------------------------------------
; NETTICK: a step of the request under way, or (none) the poll countdown:
; an action goes at once, else a poll when it runs out. Calls the phase.
NETTICK: LODA,R0 *PNSTATE
        BCTR,EQ NTK1
        BSTA,UN NSTEP
        BCTA,EQ NETDONE         ; not over
        BCTA,UN REPLY
NTK1:   LODA,R0 *PPOLLW
        BCTR,EQ NTK2
        SUBI,R0 1
        STRA,R0 *PPOLLW
        BCTR,EQ NTK2
        LODA,R0 *PACT
        BCTA,EQ NETDONE
NTK2:   LODA,R0 *P_TESTM
        BCTR,EQ NTK3
        LODI,R1 PG_TEST         ; the offline test mode answers itself
        LODI,R3 E_EXTRA
        BSTA,UN PGCALL
        BCTA,UN REPLY
NTK3:   BSTA,UN NBEGIN
        LODA,R0 *PNSTATE
        BCTA,GT NETDONE         ; under way
        BCTA,UN REPLY           ; it failed at once

; REPLY: the request is over (mekkogx reply()).
REPLY:  LODA,R0 *PNHAVE
        COMI,R0 NH_STATE
        BCFR,EQ RPL2
        EORZ    R0
        STRA,R0 *PNFAIL
        BSTA,UN STARRIVE
        BSTA,UN POLLRATE
        BCTA,UN NETDONE
RPL2:   COMI,R0 NH_ERR
        BCFR,EQ RPL3
        LODA,R0 *P_H_ERR         ; turned away (the table full, no seat)
        COMI,R0 ERR_TABLEFULL
        BCTR,EQ RPL4
        COMI,R0 ERR_SEAT
        BCFR,EQ RPL5
RPL4:   LODI,R1 PG_BOOT         ; another table
        LODI,R3 E_EXTRA+6
        BSTA,UN PGCALL
        BSTA,UN GAMESTART
        BCTR,UN RPL5
RPL3:   LODA,R0 *PNFAIL
        ADDI,R0 1
        STRA,R0 *PNFAIL
RPL5:   BSTA,UN POLLRATE
        BCTA,UN NETDONE
POLLRATE: LODI,R1 PG_CORE       ; frames to the next poll (mekkogx poll_rate)
        LODI,R3 E_POLLRT
        BCTA,UN PGFAR
; ---- a state is in: the phase runs it (mekkogx state_arrived) ---------------
STARRIVE:
        LODA,R0 *P_H_PHASE
        COMI,R0 9
        RETC,GT
        RETC,EQ
        LODI,R1 PG_CORE
        LODI,R3 E_CLKSYN
        BSTA,UN PGFAR
        LODA,R0 *P_H_ELAPSED     ; rx_pe, rx_frame = now - rx_age
        CPUT    RXPEH
        LODA,R0 *P_H_ELAPSEDL
        CPUT    RXPEL
        LODA,R1 *P_RXAGE
        LODA,R0 *PFRML
        SUBZ    R1
        STRA,R0 *PNSUB          ; (scratch: NSUB is free outside a request)
        PPSL    PSLWC
        LODA,R0 *PFRMH
        SUBI,R0 0
        CPSL    PSLWC
        CPUT    RXFRH
        LODA,R0 *PNSUB
        CPUT    RXFRL
        LODI,R0 1
        CPUT    STOK
        LODA,R0 *P_H_PHASE       ; a new phase (or a new round's grant): enter it
        COMA,R0 *P_LASTPH
        BCFR,EQ STA2
        COMI,R0 PHASE_GRANT
        BCFR,EQ STA3
        LODA,R0 *P_H_ROUND
        COMA,R0 *P_LASTRD
        BCTR,EQ STA3
STA2:   LODA,R0 *P_H_PHASE
        STRZ    R2
        LODA,R0 PHTAB,R2
        CPUT    PHPAGE
        LODA,R1 *PPHPAGE
        LODI,R3 E_ENTER
        BSTA,UN PGCALL
        BSTA,UN PGNEWCK
STA3:   LODA,R1 *PPHPAGE
        LODI,R3 E_STATE
        BSTA,UN PGCALL
        BSTA,UN PGNEWCK
        LODA,R0 *P_H_PHASE
        CPUT    LASTPH
        LODA,R0 *P_H_ROUND
        CPUT    LASTRD
        RETC,UN
PHTAB:  DB      PG_LOBBY,PG_GRANT,PG_AUC,PG_DEV,PG_PROD,PG_AUC,PG_AUC,PG_SUM,PG_SUM

; ---- cart RAM ---------------------------------------------------------------------
; RAMPUT: cart RAM = R0 at pointer register R1 (FRRAMP0-3) + R3 (see CPUT).
; Leaf. Clobbers R0, R2.
RAMPUT: STRZ    R2
        LODA,R0 FNRSEL,R1       ; the pointer
        LODA,R0 FNRDAT,R3
        LODA,R0 FNRSEL+FRRAMPK  ; one byte
        LODA,R0 FNRDAT,R2
        LODA,R0 FNRSEL+FRRAMP0  ; closed: a stray REGDATA can't store
        RETC,UN
; LPUTR: RAM-L $0700 + R3 = R0 (LPUT). Clobbers R0-R2.
LPUTR:  LODI,R1 FRRAMP1
        BCTR,UN RAMPUT
; RAMSET: the pointer at R1 + R3, a burst open (CPTR). RAMPUT1: R0 there,
; the pointer on. RAMPEND: the burst closed. Leaves. Clobber R0 (RAMPUT1 R2).
RAMSET: LODA,R0 FNRSEL,R1
        LODA,R0 FNRDAT,R3
        LODA,R0 FNRSEL+FRRAMPK
        RETC,UN
RAMPUT1: STRZ   R2
        LODA,R0 FNRDAT,R2
        RETC,UN
RAMPEND: LODA,R0 FNRSEL+FRRAMP0
        RETC,UN

; ---- the mailbox ------------------------------------------------------------------------
; MSTARTN: an N: transaction: TX rewound, command R3, R2 parameters.
; Leaf. Clobbers R0.
MSTARTN: LODA,R0 FNRSEL+FRDATRS
        LODA,R0 FNRDAT
        LODA,R0 FNRSEL+FRDEVIC
        LODA,R0 FNRDAT+NETDEV
MST1:   LODA,R0 FNRSEL+FRCMD
        LODA,R0 FNRDAT,R3
        LODA,R0 FNRSEL+FRNPARM
        LODA,R0 FNRDAT,R2
        RETC,UN
; MSTARTF: the same on the Fuji device.
MSTARTF: LODA,R0 FNRSEL+FRDATRS
        LODA,R0 FNRDAT
        LODA,R0 FNRSEL+FRDEVIC
        LODA,R0 FNRDAT+FNDFUJI
        BCTR,UN MST1
; MPARB: a 1-byte parameter, value R1. Leaf. Clobbers R0.
MPARB:  LODA,R0 FNDATA+1
        LODA,R0 FNDATA,R1
        RETC,UN
; MPOST: SEQ = ACKSEQ + 1 (255 wraps to 1): the transaction goes. The
; SEQ, which ACKSEQ echoes when it is done, is kept. Leaf. Clobbers R0, R2.
MPOST:  LODA,R0 FNACKSQ
        ADDI,R0 1
        BCFR,EQ MPO1
        LODI,R0 1
MPO1:   STRZ    R2
        LODA,R0 FNRSEL+FRSEQ
        LODA,R0 FNRDAT,R2
        LODZ    R2
        STRA,R0 *PNSEQ
        RETC,UN
; MDONE: CC EQ when the posted transaction has answered. Leaf. Clobbers R0.
MDONE:  LODA,R0 FNACKSQ
        COMA,R0 *PNSEQ
        RETC,UN
; MWAIT: post, then wait for the answer (boot only: it blocks). R0 = FNERR
; or $FF on a timeout (~10 s). Calls one down. Clobbers R0-R3.
MWAIT:  BSTR,UN MPOST
        LODI,R3 0
MWT1:   LODI,R1 0
MWT2:   LODI,R0 8
MWT3:   BDRR,R0 MWT3
        BSTR,UN MDONE
        BCTR,EQ MWT4
        BDRR,R1 MWT2
        BDRR,R3 MWT1
        LODI,R0 $FF
        RETC,UN
MWT4:   LODA,R0 FNERR
        RETC,UN

; ---- one request a time, a step a frame (mekkogx net.c) -------------------------------
NS_IDLE  EQU    0
NS_CLOSE EQU    1               ; the last request's connection, closing
NS_OPEN  EQU    2
NS_STAT  EQU    3
NS_SWAIT EQU    4               ; between two STATUS readings
NS_READ  EQU    5
NS_ING   EQU    6               ; the reply into cart RAM
NTIMEO   EQU    240             ; frames without progress before giving up
NREPLEN  EQU    550

; NBEGIN: start the queued action (or a poll). Clobbers R0-R3.
NBEGIN: LODA,R0 *PACT
        BCFR,EQ NBG1
        LODI,R0 ACT_STATE
        STRA,R0 *PACT
NBG1:   STRA,R0 *PACTSENT
        EORZ    R0
        STRA,R0 *PACT
        STRA,R0 *PNHAVE
        LODI,R3 NCCLOSE         ; the last one's connection first
        LODI,R2 0
        BSTA,UN MSTARTN
        BSTA,UN MPOST
        LODI,R0 NS_CLOSE
NBGO:   STRA,R0 *PNSTATE
        EORZ    R0
        STRA,R0 *PNWAIT
        RETC,UN

; NSTEP: one step. CC EQ while under way; else over, NHAVE says how.
; Clobbers R0-R3, T0-T3.
NSTEP:  LODA,R0 *PNWAIT
        ADDI,R0 1
        STRA,R0 *PNWAIT
        COMI,R0 NTIMEO
        BCTA,GT NFAILD
        LODA,R0 *PNSTATE
        COMI,R0 NS_ING
        BCTA,EQ INGEST
        COMI,R0 NS_SWAIT
        BCTR,EQ NSW
        BSTA,UN MDONE           ; the others wait for their answer
        BCFR,EQ NSBUSY
        LODA,R0 *PNSTATE
        COMI,R0 NS_CLOSE
        BCTR,EQ NSCL
        COMI,R0 NS_OPEN
        BCTR,EQ NSOP
        COMI,R0 NS_STAT
        BCTA,EQ NSST
        BCTA,UN NSRD
NSBUSY: EORZ    R0              ; (EQ: under way)
        RETC,UN
NSW:    LODA,R0 *PNSUB          ; a few frames between readings
        SUBI,R0 1
        STRA,R0 *PNSUB
        BCFR,EQ NSBUSY
        BCTR,UN NSPST
; the close is in (a NAK is fine: there was nothing open): OPEN the URL
NSCL:   LODI,R3 NCOPEN
        LODI,R2 2
        BSTA,UN MSTARTN
        LODI,R1 NMHTTPG
        BSTA,UN MPARB
        LODI,R1 0
        BSTA,UN MPARB
        LODI,R1 PG_CORE
        LODI,R3 E_BLDURL
        BSTA,UN PGFAR
        BSTA,UN MPOST
        EORZ    R0
        STRA,R0 *PBWH
        STRA,R0 *PBWL
        LODI,R0 NS_OPEN
        BSTA,UN NBGO
        BCTR,UN NSBUSY
NSOP:   LODA,R0 FNERR
        BCTA,GT NFAILD
        LODA,R0 FNREPLY
        COMI,R0 FCACK
        BCTA,EQ NSPST
        BCTA,UN NFAILD
; STATUS until the reply has settled: 550 waiting, or two readings that
; agree (a STATUS can come before all of it has)
NSPST:  LODI,R3 NCSTAT
        LODI,R2 2
        BSTA,UN MSTARTN
        LODI,R1 0
        BSTA,UN MPARB
        LODI,R1 0
        BSTA,UN MPARB
        BSTA,UN MPOST
        LODI,R0 NS_STAT
        STRA,R0 *PNSTATE
        BCTA,UN NSBUSY
NSST:   LODA,R0 FNERR
        BCTA,GT NFAILD
        LODA,R0 FNREPLY
        COMI,R0 FCACK
        BCTA,EQ NSST1
        BCTA,UN NFAILD
NSST1:  LODA,R2 FNRDATA         ; bytes waiting (LE)
        LODA,R1 FNRDATA+1
        LODA,R0 FNRDATA+3       ; nDevStatus: 1 OK, 136 EOF
        COMI,R0 1
        BCTR,EQ NSST2
        COMI,R0 136
        BCTR,EQ NSST2
        LODZ    R1              ; an error with nothing waiting: over
        BCFR,EQ NSST2
        LODZ    R2
        BCTA,EQ NFAILD
NSST2:  LODA,R0 *PACTSENT        ; the table list: when it has settled
        COMI,R0 ACT_TABLES
        BCTR,EQ NSST4
        LODZ    R1              ; a state: 550 or more, read it
        COMI,R0 NREPLEN>>8
        BCTR,LT NSST4
        BCTR,GT NSRDG
        LODZ    R2
        COMI,R0 NREPLEN&$FF
        BCFR,LT NSRDG
NSST4:  LODZ    R1              ; nothing yet, or not settled: again
        IORZ    R2
        BCTR,EQ NSST5
        LODZ    R1
        COMA,R0 *PBWH
        BCFR,EQ NSST5
        LODZ    R2
        COMA,R0 *PBWL
        BCTR,EQ NSRDG           ; two readings agree: it has all come
NSST5:  STRA,R1 *PBWH
        STRA,R2 *PBWL
        LODI,R0 2               ; read again in two frames
        STRA,R0 *PNSUB
        LODI,R0 NS_SWAIT
        STRA,R0 *PNSTATE
        BCTA,UN NSBUSY
; READ what came (at most the state's 550)
NSRDG:  LODI,R3 NCREAD
        LODI,R2 1
        BSTA,UN MSTARTN
        LODI,R1 2               ; one 2-byte parameter: the length, LE
        LODA,R0 FNDATA,R1
        LODA,R0 *PACTSENT
        COMI,R0 ACT_TABLES
        BCFR,EQ NSRDS
        LODA,R1 *PBWL           ; the list: what is there (its 1 + 33n
        LODA,R0 FNDATA,R1       ;  bytes fit the window easily)
        LODA,R1 *PBWH
        LODA,R0 FNDATA,R1
        BCTR,UN NSRDP
NSRDS:  LODI,R1 NREPLEN&$FF
        LODA,R0 FNDATA,R1
        LODI,R1 NREPLEN>>8
        LODA,R0 FNDATA,R1
NSRDP:
        BSTA,UN MPOST
        LODI,R0 NS_READ
        STRA,R0 *PNSTATE
        BCTA,UN NSBUSY
; the reply: a state, an error header, or nothing to keep
NSRD:   LODA,R0 FNERR
        BCTA,GT NFAILD
        LODA,R0 FNREPLY
        COMI,R0 FCACK
        BCTA,EQ NSRD1
        BCTA,UN NFAILD
NSRD1:  LODA,R0 *PACTSENT        ; the table list: left in the window
        COMI,R0 ACT_TABLES
        BCFR,EQ NSRD1A
        LODI,R0 NH_TABLES
        BCTA,UN NOVER
NSRD1A: LODA,R0 FNRDATA         ; 'M', version 2
        COMI,R0 $4D
        BCTA,EQ NSRD2
        BCTA,UN NFAILD
NSRD2:  LODA,R0 FNRDATA+1
        COMI,R0 MULE_PROTO_VERSION
        BCTA,EQ NSRD3
        BCTA,UN NFAILD
NSRD3:  LODA,R0 FNRDATA+2
        COMI,R0 MSG_STATE
        BCTR,EQ NSRD4
        COMI,R0 MSG_ERROR       ; a header only: why we were turned away
        BCTA,EQ NSRD6
        BCTA,UN NFAILD
NSRD4:  LODA,R0 FNRXLH          ; the whole state
        COMI,R0 NREPLEN>>8
        BCTA,EQ NSRD5
        BCTA,UN NFAILD
NSRD5:  LODA,R0 FNRXLL
        COMI,R0 NREPLEN&$FF
        BCTA,EQ NSRD7
        BCTA,UN NFAILD
NSRD6:  LODI,R0 INGHDR          ; the header's run only: the last one
        BCTR,UN NSRD8
NSRD7:  EORZ    R0
NSRD8:  STRA,R0 *PINGRUN
        EORZ    R0
        STRA,R0 *PINGPOS
        STRA,R0 *PINGSLC        ; the reply shows slice 0
        LODI,R0 NS_ING
        BSTA,UN NBGO
        BCTA,UN NSBUSY

NFAILD: LODI,R0 NH_FAIL
NOVER:  STRA,R0 *PNHAVE
        EORZ    R0
        STRA,R0 *PNSTATE
        LODI,R0 1               ; (not EQ: over)
        RETC,UN

; INGEST: up to INGBUD bytes of the reply into cart RAM this frame, run by
; run (build/proto.inc INGTAB: state offset, length, cart address), a
; slice at a time; at the end the staged header goes to $0600 (so a new
; phase is never half in). An error reply copies only the header's run.
INGBUD  EQU     160
INGEST: LODA,R0 *PINGSLC        ; a slice asked for: wait for its repaint
        TMI,R0  $80
        BCFR,EQ ING1
        ANDI,R0 3
        COMA,R0 FNSECHO
        BCFA,EQ NSBUSY
        STRA,R0 *PINGSLC
ING1:   LODI,R0 INGBUD
        STRA,R0 *PT3            ; T3: the budget left
ING2:   LODA,R1 *PINGRUN
        LODA,R0 INGT,R1
        COMI,R0 $FF
        BCTA,EQ INGEND
        STRA,R0 *PT0            ; T0:T1 = the state offset reached
        LODA,R0 INGT+1,R1
        ADDA,R0 *PINGPOS
        STRA,R0 *PT1
        PPSL    PSLWC
        LODA,R0 *PT0
        ADDI,R0 0
        CPSL    PSLWC
        STRA,R0 *PT0
        COMA,R0 *PINGSLC        ; in the slice shown?
        BCTR,EQ ING3
        STRZ    R2              ; no: ask for it, and wait
        LODA,R0 FNRSEL+FRRXSLC
        LODA,R0 FNRDAT,R2
        LODZ    R2
        IORI,R0 $80
        STRA,R0 *PINGSLC
        BCTA,UN NSBUSY
ING3:   LODA,R0 INGT+4,R1       ; the cart pointer: address + the run's offset
        ADDA,R0 *PINGPOS
        STRZ    R3
        PPSL    PSLWC
        LODA,R0 INGT+3,R1
        ADDI,R0 FRRAMP0
        CPSL    PSLWC
        STRZ    R2
        LODA,R0 FNRSEL,R2
        LODA,R0 FNRDAT,R3
        LODA,R0 FNRSEL+FRRAMPK
; this many: the run's rest, the slice's rest, the budget's: the least
        LODA,R0 INGT+2,R1
        SUBA,R0 *PINGPOS
        STRA,R0 *PT2
        LODA,R0 *PT1            ; the slice's rest: 256 - offset (0 = 256)
        STRZ    R3
        EORZ    R0
        SUBZ    R3
        BCTR,EQ ING4
        COMA,R0 *PT2
        BCFR,LT ING4
        STRA,R0 *PT2
ING4:   LODA,R0 *PT2
        COMA,R0 *PT3
        BCTR,LT ING5
        LODA,R0 *PT3
        STRA,R0 *PT2
ING5:   LODA,R0 *PT3            ; budget -= n
        SUBA,R0 *PT2
        STRA,R0 *PT3
        LODA,R0 *PINGPOS        ; pos += n
        ADDA,R0 *PT2
        STRA,R0 *PINGPOS
        LODA,R3 *PT1            ; the copy: window byte R3 -> cart RAM
        SUBI,R3 1
        LODA,R2 *PT2
ING6:   LODA,R0 FNRDATA,R3,+
        STRZ    R1
        LODA,R0 FNRDAT,R1
        BDRR,R2 ING6
        LODA,R0 FNRSEL+FRRAMP0  ; closed
        LODA,R1 *PINGRUN        ; the run done?
        LODA,R0 *PINGPOS
        COMA,R0 INGT+2,R1
        BCFR,EQ ING7
        ADDI,R1 5
        STRA,R1 *PINGRUN
        EORZ    R0
        STRA,R0 *PINGPOS
ING7:   LODA,R0 *PT3
        BCFA,EQ ING2
        BCTA,UN NSBUSY
; all in: the staged header to $0600, and the request is over
INGEND: LODI,R1 FRRAMP0
        LODI,R3 0
        BSTA,UN RAMSET
        LODI,R3 0
ING8:   LODA,R0 CR_HSTAGE,R3
        STRZ    R1
        LODA,R0 FNRDAT,R1
        ADDI,R3 1
        COMI,R3 16
        BCFR,EQ ING8
        BSTA,UN RAMPEND
        LODA,R0 CR_HSTAGE+2     ; a state, or an error header
        COMI,R0 MSG_STATE
        BCFR,EQ ING9
        LODI,R0 NH_STATE
        BCTA,UN NOVER
ING9:   LODI,R0 NH_ERR
        BCTA,UN NOVER
INGT:   INGTAB

; ---- services moved out of fixed A (it is the tight one) --------------------------
; The sounds: pitch, frames, second pitch, its frames, VOLUME (b3 tone,
; b4 noise, b0-2 loudness). Hardware f = 7867 / (pitch + 1) Hz; mekkogx's
; table is 36428 / d Hz (platform-specific/sound.h).
SNDTAB: DB      8,3,0,0,$0D             ; select       874 Hz
        DB      27,3,0,0,$0D            ; sample       281
        DB      54,8,0,0,$1D            ; error        143, a buzz
        DB      6,4,0,0,$0D             ; buy         1124
        DB      7,8,4,14,$0F            ; alert        983 then 1573: the loud one
        DB      14,6,0,0,$0D            ; auction over 524
        DB      5,2,0,0,$0D             ; trade       1311
        DB      35,8,0,0,$1D            ; event        218, rough
        DB      9,10,0,0,$0D            ; founder      787
        DB      42,1,0,0,$0A            ; step         183, soft
        DB      2,2,0,0,$0A             ; tick        2622, soft
        DB      3,2,0,0,$0A             ; tock        1967, soft
SND2ND  EQU     12                      ; (its row: none, loud)
        DB      0,0,0,0,$0F

; the lowest set bit wins, so two keys at once never read as a third
KT0:    DB      K_NONE,K_CLR,7,K_CLR,4,K_CLR,7,K_CLR
        DB      1,K_CLR,7,K_CLR,4,K_CLR,7,K_CLR
KT1:    DB      K_NONE,0,8,0,5,0,8,0
KT2:    DB      K_NONE,K_ENT,9,K_ENT,6,K_ENT,9,K_ENT
        DB      3,K_ENT,9,K_ENT,6,K_ENT,9,K_ENT
KTP:    DB      K_NONE,K_START,K_OPT,K_START,K_SEL,K_START,K_OPT,K_START

; SPRCOL: sprite R1's colour R2 (0-7), plus $80 for 16 lines (each row
; twice). Leaf. Clobbers R0-R3.
SPRCOL: LODZ    R2
        ANDI,R0 7
        STRZ    R3              ; colour
        LODZ    R1
        ANDI,R0 1
        BCFR,EQ SPC1
        LODZ    R3              ; even sprites: colour in b3-5, size in b7
        RRL,R0
        RRL,R0
        RRL,R0
        STRZ    R3
        LODI,R0 $47             ; keep the odd sprite's bits
        BCTR,UN SPC2
SPC1:   LODI,R0 $B8             ; odd sprites: colour b0-2, size b6
SPC2:   STRA,R0 *PT0              ; bits to keep
        LODZ    R2
        ANDI,R0 $80
        BCFR,EQ SPC3            ; 16 lines: size bit clear
        LODZ    R1              ; 8 lines: the size bit set
        ANDI,R0 1
        BCFR,EQ SPC6
        LODI,R0 $80
        BCTR,UN SPC4
SPC6:   LODI,R0 $40
SPC4:   IORZ    R3
        STRZ    R3
SPC3:   LODZ    R1              ; the UVI's registers are write-only: shadows
        COMI,R0 2
        BCTR,LT SPC5
        LODA,R0 *PSC23SH
        ANDA,R0 *PT0
        IORZ    R3
        STRA,R0 *PSC23SH
        STRA,R0 *PSPRC23
        RETC,UN
SPC5:   LODA,R0 *PSC01SH
        ANDA,R0 *PT0
        IORZ    R3
        STRA,R0 *PSC01SH
        STRA,R0 *PSPRC01
        RETC,UN

; UDCQUE: user glyph R2 (0-7) from the 8 bytes at R0:R1, in page R3 ($FF:
; fixed code), at the next vblank. Three at most a frame. Leaf.
; Clobbers R0, R3.
UDCQUE: STRA,R0 *PT0
        LODA,R0 *PUDCQN
        COMI,R0 3
        BCFR,LT UDQ9            ; full: dropped (callers stay within it)
        RRL,R0                  ; entry offset: n * 4
        RRL,R0
        STRA,R3 *PT1
        STRZ    R3
        LODZ    R2
        STRA,R0 *PUDCQ0,R3
        LODA,R0 *PT1
        STRA,R0 *PUDCQ1,R3
        LODA,R0 *PT0
        STRA,R0 *PUDCQ2,R3
        LODZ    R1
        STRA,R0 *PUDCQ3,R3
        LODA,R0 *PUDCQN
        ADDI,R0 1
        STRA,R0 *PUDCQN
UDQ9:   RETC,UN

; ---- the time bar's tick and tock (mekkogx bar_tick) ----------------------------------
; BARTICK: R0 = the units the bar has left: under 30, a tick every that
; many frames, tick and tock in turn; none or 30 and up, quiet (the first
; tick then comes at once). Clobbers R0-R2.
BARTICK: STRZ   R2
        LODA,R0 *PFRML          ; frames gone by
        SUBA,R0 *PBARFR
        RETC,EQ
        STRZ    R1
        LODA,R0 *PFRML
        STRA,R0 *PBARFR
        LODZ    R2
        BCTR,EQ BTK1
        COMI,R0 30
        BCFR,LT BTK1
        LODA,R0 *PBARTK
        COMZ    R1
        BCTR,GT BTK2            ; not yet
        LODZ    R2
        STRA,R0 *PBARTK
        LODA,R0 *PBARTG
        ADDI,R0 SND_TICK
        BSTA,UN SNDQUE
        LODA,R0 *PBARTG
        EORI,R0 1
        STRA,R0 *PBARTG
        RETC,UN
BTK2:   SUBZ    R1
        STRA,R0 *PBARTK
        RETC,UN
BTK1:   LODI,R0 1
        STRA,R0 *PBARTK
        EORZ    R0
        STRA,R0 *PBARTG
        RETC,UN

; ---- arithmetic --------------------------------------------------------------------
; MUL8: R1:R2 = R1 * R2. Leaf. Clobbers R0, R3, T0.
MUL8:   STRA,R1 *PT0              ; multiplicand
        LODZ    R2              ; the multiplier in the high byte: its bits
        STRZ    R1              ;  leave as the product grows under them
        LODI,R2 0
        LODI,R3 8
MUL1:   LODZ    R2              ; R1:R2 <<= 1, the multiplier's top bit out
        ADDZ    R2              ;  of R1 into C
        STRZ    R2
        PPSL    PSLWC
        LODZ    R1
        ADDZ    R1
        STRZ    R1
        CPSL    PSLWC
        TPSL    PSLC
        BCFR,EQ MUL2
        LODZ    R2              ; + multiplicand
        ADDA,R0 *PT0
        STRZ    R2
        PPSL    PSLWC
        LODZ    R1
        ADDI,R0 0
        STRZ    R1
        CPSL    PSLWC
MUL2:   BDRR,R3 MUL1
        RETC,UN

; DIV8: R2 = R1:R2 / R3, R1 = the remainder (a quotient under 256).
; Leaf. Clobbers R0, T0.
DIV8:   STRA,R3 *PT0
        LODI,R3 8
DIV1:   LODZ    R2              ; R1:R2 <<= 1
        ADDZ    R2
        STRZ    R2
        PPSL    PSLWC
        LODZ    R1
        ADDZ    R1
        STRZ    R1
        CPSL    PSLWC
        TPSL    PSLC            ; a bit carried out: it goes surely
        BCTR,EQ DIV2
        LODZ    R1
        COMA,R0 *PT0
        BCTR,LT DIV3
DIV2:   LODZ    R1
        SUBA,R0 *PT0
        STRZ    R1
        LODZ    R2
        IORI,R0 1
        STRZ    R2
DIV3:   BDRR,R3 DIV1
        LODA,R3 *PT0
        RETC,UN

; ---- actions -----------------------------------------------------------------------
; ACTSEND: send action R0 with argument R1:R2 at the next chance: it takes
; the place of the next poll (mekkogx act_send). Leaf.
ACTSEND: STRA,R0 *PACT
        STRA,R1 *PACTARGH
        STRA,R2 *PACTARGL
        EORZ    R0
        STRA,R0 *PPOLLW
        RETC,UN


; ---- pointer words: page-1 code's way into page 0 ----------------------------------------
PNSTATE: DWBE   NSTATE
PNWAIT: DWBE    NWAIT
PNSUB:  DWBE    NSUB
PNSEQ:  DWBE    NSEQ
PACT:   DWBE    ACT
PACTARGH: DWBE  ACTARGH
PACTARGL: DWBE  ACTARGL
PPOLLW: DWBE    POLLW
PACTSENT: DWBE  ACTSENT
PNHAVE: DWBE    NHAVE
PNFAIL: DWBE    NFAIL
PINGRUN: DWBE   INGRUN
PINGPOS: DWBE   INGPOS
PINGSLC: DWBE   INGSLC
PBWH:   DWBE    BWH
PBWL:   DWBE    BWL
PTICKH1: DWBE   TICKH
PTICKL1: DWBE   TICKL
PFRMH:  DWBE    FRMH
PFRML:  DWBE    FRML
PDT:    DWBE    DT
PCURPG: DWBE    CURPG
PINDIR: DWBE    INDIR
PEVBTN: DWBE    EVBTN
PEVKEY: DWBE    EVKEY
PDCOLOR: DWBE   DCOLOR
PNUMH:  DWBE    NUMH
PNUMM:  DWBE    NUMM
PNUML:  DWBE    NUML
PT0:    DWBE    T0
PT1:    DWBE    T1
PT2:    DWBE    T2
PT3:    DWBE    T3
PSTOK:  DWBE    STOK
PPHPAGE: DWBE   PHPAGE
PPGNEW: DWBE    PGNEW
PPGSAVE: DWBE   PGSAVE
PSC01SH: DWBE   SC01SH
PSC23SH: DWBE   SC23SH
PSPRC01: DWBE   SPRC01
PSPRC23: DWBE   SPRC23
PUDCQN: DWBE    UDCQN
PUDCQ0: DWBE    UDCQ
PUDCQ1: DWBE    UDCQ+1
PUDCQ2: DWBE    UDCQ+2
PUDCQ3: DWBE    UDCQ+3
PBARTK: DWBE    BARTK
PBARTG: DWBE    BARTG
PBARFR: DWBE    BARFR
; RAM-L and the copied state, as page-1 code names them
P_AUTRD: DWBE AUTRD
P_CLKBH: DWBE CLKBH
P_CLKBL: DWBE CLKBL
P_CLKHAV: DWBE CLKHAV
P_DVSTATE: DWBE DVSTATE
P_H_ELAPSED: DWBE H_ELAPSED
P_H_ELAPSEDL: DWBE H_ELAPSED+1
P_H_ERR: DWBE H_ERR
P_H_MYSEAT: DWBE H_MYSEAT
P_H_PHASE: DWBE H_PHASE
P_H_ROUND: DWBE H_ROUND
P_H_T: DWBE H_T
P_H_TL: DWBE H_T+1
P_LASTPH: DWBE LASTPH
P_LASTRD: DWBE LASTRD
P_RXAGE: DWBE RXAGE
P_SCRNOW: DWBE SCRNOW
P_TESTM: DWBE TESTM
P_MAPREQ: DWBE  MAPREQ
P_MAPON: DWBE   MAPON
P_FXON: DWBE    FXON
P_MUTE: DWBE    MUTE
