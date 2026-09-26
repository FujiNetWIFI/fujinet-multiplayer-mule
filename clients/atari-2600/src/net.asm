; net.asm -- bank 2: the request machine, one step per visit.
;
; A screen bank comes here once a frame, in the overscan, while a request is
; in flight or when one is due, and is sent straight back: this bank draws
; nothing and never waits. Each visit does ONE thing -- launch a transaction,
; or look at whether the last one has been answered and launch the next --
; so a figure keeps walking and a cursor keeps sweeping through a poll.
;
; The order is the family's (the Battleship 2600 client's net.inc, the
; Arcadia's, the Astrocade's), and each rule in it was paid for:
;   * CLOSE the previous request at the START of the next one. Every
;     transaction repaints the reply window, and the screens read the last
;     state out of it right up until this bank begins the next request.
;   * The HTTP GET is deferred until the first STATUS, whose fourth byte is
;     the only place an HTTP error shows.
;   * STATUS reports as soon as SOME of the reply has come: settle on two
;     equal readings AND a minimum length (or the whole state).
;   * The reply length belongs to the most recent transaction: take it at
;     once after the READ.
;   * Sequence numbers come from the cart's own ACKSEQ (FNLNCH, in the tail).
;
; When a state arrives, the few bytes the screens need between polls go into
; HOT (mdefs.inc), and the bank for the state's phase is entered: ENENTER if
; the phase changed, ENSTATE if not. The screen reads the rest out of the
; window and clears HFNEW when it has; no request starts while HFNEW is set,
; so the window holds still for it.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     0
MHASDEC EQU     0

        INCLUDE "../build/tail.inc"

STEPTMO EQU     240             ; frames a transaction may take (~4s)
NSTRIES EQU     180             ; STATUS polls before a request gives up
FAILFRM EQU     20              ; frames before the next try after a failure
STILLMAX EQU    10              ; readings a short count may hold before it is
                                ;   read anyway

        ORG     $1000

NENTRY: lda     NSTATE
        beq     NIDLE
        cmp     #NSOPENB        ; the OPEN's second half: no transaction out
        bne     NBUSY
        jsr     BLDURL2
        lda     #NSOPEN
        jmp     NLAUNCH

NIDLE:

; ---------------------------------------------------------------------------
; Idle: begin a request. An action if one is queued, else a /state poll
; (the lobby's /tables and the RESET switch's /leave are queued as actions
; too). The previous request's connection is closed first.
        lda     NACT
        bne     NB1
        lda     #RQSTATE
        sta     NREQ
        jmp     NB2
NB1:    sta     NREQ
        lda     NACTA
        sta     NARG
        lda     NACTH
        sta     NARGH
        lda     #0
        sta     NACT
NB2:    lda     #NETDEV
        sta     FNDEV
        lda     #NCCLOSE
        sta     FNCMD
        lda     #0
        sta     FNNPR
        jsr     FNBEG
        lda     #NSCLOSE
        jmp     NLAUNCH

; ---------------------------------------------------------------------------
; A transaction is in flight. Not answered yet: count the frame and go back.
NBUSY:  lda     FNACKS
        cmp     FNSEQ
        beq     NDONE
        dec     NTRY
        beq     NBTO
        jmp     NBACK
NBTO:   lda     #FNEWAIT
        jmp     NFAIL

NDONE:  lda     NSTATE
        cmp     #NSCLOSE
        beq     NOPEN
        cmp     #NSOPEN
        beq     NOPENED
        cmp     #NSSTAT
        bne     NDN1
        jmp     NSTATD
NDN1:   jmp     NREADD

; ---- CLOSE answered (a NAK is fine: there may have been nothing open) ----
NOPEN:  lda     #NETDEV
        sta     FNDEV
        lda     #NCOPEN
        sta     FNCMD
        lda     #2
        sta     FNNPR
        jsr     FNBEG
        lda     #NMHTTPG
        jsr     FNPB
        lda     #NTRNONE
        jsr     FNPB
; The devicespec in two visits: the route now, the query and the launch next
; time (the TX stream keeps what is in it). One visit was a line too long
; for the overscan when a screen's helper bank had had it too.
        jsr     BLDURL
        lda     #NSOPENB
        sta     NSTATE
        jmp     NBACK

; ---- OPEN answered ----
NOPENED: lda    FNERR
        bne     NFAILA
        jsr     FNACK
        bne     NFAILA
        lda     #$FF
        sta     NPRV0
        sta     NPRV1
        lda     #NSTRIES
        sta     NSCNT
        lda     #0
        sta     NARGH           ; readings the count has held still
NSTATL: lda     #NETDEV         ; launch a STATUS
        sta     FNDEV
        lda     #NCSTAT
        sta     FNCMD
        lda     #2
        sta     FNNPR
        jsr     FNBEG
        lda     #0
        jsr     FNPB
        lda     #0
        jsr     FNPB
        lda     #NSSTAT
        jmp     NLAUNCH

NFAILA: jmp     NFAIL

; ---- STATUS answered ----
NSTATD: lda     FNERR
        bne     NFAILA
        jsr     FNACK
        bne     NFAILA
        lda     FNRPLY+NSDEVST
        cmp     #1
        beq     NST1
        cmp     #136            ; EOF: the whole body is in
        beq     NST1
        lda     FNRPLY+NSAVLO   ; an HTTP error with nothing to read
        ora     FNRPLY+NSAVHI
        bne     NST1
        lda     FNRPLY+NSDEVST
        jmp     NFAIL
NST1:   lda     FNRPLY+NSAVLO
        sta     NAVL0
        lda     FNRPLY+NSAVHI
        sta     NAVL1
; The whole state is there: read it now.
        cmp     #(STSIZE)>>8
        bcc     NST2
        bne     NREAD
        lda     NAVL0
        cmp     #(STSIZE)&$FF
        bcs     NREAD
; A /state reply is 550 bytes, or a 16-byte error header. Over the internet
; it can arrive in pieces, and a piece can sit still for two readings: so a
; state is read short only at 16 bytes (held twice), or when the count has
; not moved for STILLMAX readings. A table list is any length: twice.
NST2:   lda     NAVL0
        ora     NAVL1
        beq     NST3A
        lda     NAVL0
        cmp     NPRV0
        bne     NST3A
        lda     NAVL1
        cmp     NPRV1
        bne     NST3A
        lda     NREQ            ; the same as last time
        cmp     #RQTABLE
        beq     NREAD
        lda     NAVL1
        bne     NST2A
        lda     NAVL0
        cmp     #16
        beq     NREAD
NST2A:  inc     NARGH           ; (free once the OPEN is built)
        lda     NARGH
        cmp     #STILLMAX
        bcs     NREAD
        bcc     NST3
NST3A:  lda     #0              ; it moved: count again
        sta     NARGH
NST3:   lda     NAVL0
        sta     NPRV0
        lda     NAVL1
        sta     NPRV1
        dec     NSCNT
        JNE     NSTATL
        lda     #FNEWAIT
        jmp     NFAIL

; ---- READ min(available, cap) ----
NREAD:  lda     #NETDEV
        sta     FNDEV
        lda     #NCREAD
        sta     FNCMD
        lda     #1
        sta     FNNPR
        jsr     FNBEG
; The cap: the state is 550 (which the window holds in two slices); a table
; list fits the first slice.
        lda     #(STSIZE)&$FF
        ldx     #(STSIZE)>>8
        ldy     NREQ
        cpy     #RQTABLE
        bne     NRD1
        lda     #0              ; 512
        ldx     #2
NRD1:   cpx     NAVL1
        bcc     NRD3            ; the cap is smaller
        bne     NRD2
        cmp     NAVL0
        bcc     NRD3
NRD2:   lda     NAVL0
        ldx     NAVL1
NRD3:   jsr     FNPW
        lda     #NSREAD
        jmp     NLAUNCH

NFAILB: jmp     NFAIL

; ---- READ answered ----
NREADD: lda     FNERR
        bne     NFAILB
        jsr     FNACK
        bne     NFAILB
        lda     FNRXLO
        sta     NRXL0
        lda     FNRXHI
        sta     NRXL1
        lda     #NSIDLE
        sta     NSTATE
        lda     HFLAGS
        and     #~HFERR&$FF
        sta     HFLAGS
        lda     NREQ
        cmp     #RQTABLE
        bne     NRD4
; The table list: the lobby bank reads it out of the window.
        lda     #120
        sta     NPOLL
        lda     #ENTABLE
        sta     MENT
        lda     #BKTABLE
        sta     MSCR
        jmp     MGOTO
NRD4:   cmp     #RQLEAVE
        bne     NARRIV
; Left: the seat is the computer's. Back to the table list, which forgets
; the room (HFLEFT).
        lda     HFLAGS
        and     #~(HFTBL|HFNEW)&$FF
        ora     #HFLEFT
        sta     HFLAGS
        lda     #$FF
        sta     HSCRB
        lda     #ENLIST
        sta     MENT
        lda     #BKTABLE
        sta     MSCR
        jmp     MGOTO

; ---------------------------------------------------------------------------
; A state has arrived: check it, keep HOT, enter the phase's bank.
NARRIV: lda     NRXL1           ; at least a header
        bne     NAR1
        lda     NRXL0
        cmp     #16
        bcc     NBADR
NAR1:   lda     FNRPLY+SHMAGIC
        cmp     #'M'
        bne     NBADR
        lda     FNRPLY+SHVER
        cmp     #PROTOVER
        bne     NBADR
        lda     FNRPLY+SHERR
        sta     HERR
        lda     FNRPLY+SHTYPE
        cmp     #MSGSTATE
        beq     NAR2
; An error header only: the last state stands. Say so to the screen.
        lda     #30
        sta     NPOLL
        lda     #ENRUN
        sta     MENT
        lda     MSCR
        jmp     MGOTO
NBADR:  lda     #FNEBADL
        jmp     NFAIL
NAR2:   lda     NRXL1           ; and the whole of it
        cmp     #(STSIZE)>>8
        bcc     NBADR
        bne     NAR3
        lda     NRXL0
        cmp     #(STSIZE)&$FF
        bcc     NBADR
NAR3:   lda     FNRPLY+SHPHASE
        sta     HPHASE
        lda     FNRPLY+SHROUND
        sta     HROUND
        lda     FNRPLY+SHSEAT
        sta     HSEAT
        lda     FNRPLY+SHELAP
        sta     HELAP
        lda     FNRPLY+SHELAP+1
        sta     HELAPH
        lda     MFRAME
        sta     HRXF
        lda     MFRAMEH
        sta     HRXFH
; My money, for the screens that show it.
        lda     #0
        sta     HMONEY
        sta     HMONEYH
        ldx     HSEAT
        cpx     #4
        bcs     NAR4
        lda     PLOFS,x
        tax
        lda     FNRPLY+SPLAYER+PLMONEY,x
        sta     HMONEY
        lda     FNRPLY+SPLAYER+PLMONEY+1,x
        sta     HMONEYH
NAR4:   lda     HFLAGS
        ora     #HFNEW|HFTBL
        sta     HFLAGS
        lda     #120            ; the screen's stall guard (scrloop.inc)
        sta     NTRY
        ldx     HPHASE
        cpx     #PHOVER+1
        bcc     NAR5
        ldx     #PHLOBBY
NAR5:   lda     PHPOLL,x
        sta     NPOLL
; The screen: by phase, and within a phase by the auction's stage and whose
; turn it is. A new screen is entered afresh (ENENTER); the same one -- or a
; bank it handed on to, like the town from the map -- gets ENSTATE.
        lda     PHBANK,x
        cpx     #PHLAUC
        bne     NAR5A
        ldy     FNRPLY+SASTAGE  ; the plot for sale is shown on the map
        bne     NAR5C
        lda     #BKMAP
        bne     NAR5C
NAR5A:  cpx     #PHDEV
        bne     NAR5C
        ldy     FNRPLY+STSEAT   ; my turn: the walking map
        cpy     HSEAT
        bne     NAR5C
        lda     #BKDEV
NAR5C:  cmp     HSCRB
        beq     NAR6
        sta     HSCRB
        sta     MSCR
        lda     #ENENTER
        sta     MENT
        lda     MSCR            ; MGOTO takes the bank in A
        jmp     MGOTO
NAR6:   lda     #ENSTATE
        sta     MENT
        lda     MSCR
        jmp     MGOTO

; ---------------------------------------------------------------------------
; NLAUNCH -- NSTATE = A, launch the transaction set up, back to the screen.
NLAUNCH: sta    NSTATE
        lda     #STEPTMO
        sta     NTRY
        jsr     FNLNCH
NBACK:  lda     #ENRUN
        sta     MENT
        lda     MSCR
        jmp     MGOTO

; NFAIL -- A = why. The request is dropped; try again in a while.
NFAIL:  sta     NERR
        lda     #NSIDLE
        sta     NSTATE
        lda     #FAILFRM
        sta     NPOLL
        lda     HFLAGS
        ora     #HFERR
        sta     HFLAGS
        lda     NREQ
        cmp     #RQTABLE
        bne     NBACK
        lda     #ENTABLE        ; no list: the lobby sees HFERR and says so
        sta     MENT
        lda     #BKTABLE
        sta     MSCR
        jmp     MGOTO

; The screen bank for each phase (the bank may hand on within the phase).
PHBANK: DB      BKLOBBY         ; lobby
        DB      BKMAP           ; grant
        DB      BKAUC           ; land auction
        DB      BKMAP           ; development
        DB      BKMAP           ; production
        DB      BKGOODS         ; declare
        DB      BKAUC           ; trading
        DB      BKSTAT          ; summary
        DB      BKSTAT          ; game over
; Frames between polls: brisk while something moves in real time.
PHPOLL: DB      30,4,4,6,20,6,4,30,60

PLOFS:  DB      0,PLSIZE,2*PLSIZE,3*PLSIZE

        INCLUDE "url.inc"

; ---------------- data: tables, not code (tools/dataranges.py) ----------
DSTART:
        INCLUDE "urltab.inc"

        END
