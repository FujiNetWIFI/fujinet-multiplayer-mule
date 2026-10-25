; p_core.inc -- the core page: what fixed code far-calls (PGFAR) and fixed
; space has no room for -- the URL of a request, the clock's sync, the poll
; rate, the spectator's tag. Page-0 code: console RAM and RAM-L by name,
; the mailbox and RAM-H through fixed A's pointer words.

        RETC,UN                 ; E_ENTER (unused)
        NOP
        NOP
        RETC,UN                 ; E_STATE
        NOP
        NOP
        RETC,UN                 ; E_TICK
        NOP
        NOP
        BCTA,UN BLDURL          ; E_BLDURL
        BCTA,UN CLKSYNC         ; E_CLKSYN
        BCTA,UN POLLRT          ; E_POLLRT
        BCTA,UN SPECTAG         ; E_SPECTG

POLLRT:
; POLLRT: frames to the next poll (mekkogx poll_rate).
        LODI,R1 30
        LODA,R0 NFAIL
        BCFR,EQ PLR9
        LODA,R0 H_PHASE
        COMI,R0 9
        BCFR,LT PLR9
        STRZ    R2
        LODA,R0 PLRTAB,R2
        BCTR,EQ PLR9            ; 0: the default
        STRZ    R1
        COMI,R2 PHASE_DEVELOP
        BCFR,EQ PLR9
        LODA,R0 DVSTATE       ; walking: rarely
        COMI,R0 DV_WALK
        BCTR,EQ PLR9
        LODI,R1 30
PLR9:   STRA,R1 POLLW
        RETC,UN
PLRTAB: DB      0,12,12,60,60,12,8,0,60
DV_WALK EQU     2               ; (develop.inc: DVSTATE)


; CLKSYNC: the server's clock keeps ours honest (mekkogx clock_sync): the
; gap between the two only grows when we lost frames, so whenever it is
; larger than the largest seen, the difference goes on our clock; a reply
; slower than the fastest is RXAGE older than it looks. The lobby has no
; clock. Clobbers R0-R3, T0-T3.
CLKSYNC:
        EORZ    R0
        CPUT    RXAGE
        LODA,R0 TESTM
        BCFR,EQ CKS0
        LODA,R0 H_PHASE
        BCFR,EQ CKS1
CKS0:   EORZ    R0              ; (no clock: start afresh next time)
        BCTA,UN CPUTHAV
CKS1:   LODA,R0 H_T+1         ; lag = server t - our frame
        SUBA,R0 FRML
        STRA,R0 T1
        PPSL    PSLWC
        LODA,R0 H_T
        SUBA,R0 FRMH
        CPSL    PSLWC
        STRA,R0 T0            ; T0:T1 = lag
        LODA,R0 CLKHAV
        BCFR,EQ CKS2
        BSTA,UN CKSBASE         ; the first: it is the base
        LODI,R0 1
        BCTA,UN CPUTHAV
CKS2:   LODA,R0 T1            ; d = lag - base
        SUBA,R0 CLKBL
        STRA,R0 T3
        PPSL    PSLWC
        LODA,R0 T0
        SUBA,R0 CLKBH
        CPSL    PSLWC
        STRA,R0 T2            ; T2:T3 = d
        BCFR,EQ CKS4            ; d >= 256 (or negative)
        LODA,R0 T3
        RETC,EQ                 ; d = 0: nothing to do
        COMI,R0 60
        BCFR,LT CKS3            ; 60 and up: a long stall, start again
        ADDA,R0 FRML          ; lost frames: our clock on by d
        STRA,R0 FRML
        PPSL    PSLWC
        LODA,R0 FRMH
        ADDI,R0 0
        STRA,R0 FRMH
        CPSL    PSLWC
        RETC,UN                 ; (base stays: the lag is now what it was)
CKS3:   BCTA,UN CKSBASE
CKS4:   TMI,R0  $80
        BCFR,EQ CKSBASE         ; 256..32767: start again
        LODI,R0 0               ; negative: older than the fastest by -d
        SUBA,R0 T3
        STRA,R0 T3
        LODI,R0 0
        PPSL    PSLWC
        SUBA,R0 T2
        CPSL    PSLWC
        BCFR,EQ CKSBASE         ; more than 255: the server's clock went back
        LODA,R0 T3
        COMI,R0 120
        BCTR,GT CKSBASE
        COMI,R0 30
        BCTR,LT CKS5
        LODI,R0 30
CKS5:   CPUT    RXAGE
        RETC,UN
CKSBASE: LODA,R0 T0
        CPUT    CLKBH
        LODA,R0 T1
        CPUT    CLKBL
        RETC,UN
CPUTHAV: CPUT   CLKHAV
        RETC,UN


; ---- the URL (mekkogx net.c compose) --------------------------------------------------
; BLDURL: stream N:<endpoint><route>[arg]?table=<t>&player=<name>&bin=1&be=1
; for ACTSENT into the TX stream. Calls one down. Clobbers R0-R3, NUM*, T*.
BLDURL: LODI,R1 $4E             ; "N:"
        LODA,R0 *PTXD,R1
        LODI,R1 $3A
        LODA,R0 *PTXD,R1
        LODI,R3 SCENDPT-$2900   ; the room's server, or the build's
        LODA,R0 *PRAMH,R3
        BCTR,EQ BU1
        LODI,R3 SCENDPT-$2900
        BSTA,UN TXRAMH
        BCTR,UN BU2
BU1:    LODI,R0 UENDPT>>8
        LODI,R1 UENDPT&$FF
        BSTA,UN TXSTR
BU2:    LODA,R0 ACTSENT
        COMI,R0 ACT_TABLES
        BCFR,EQ BU3
        LODI,R0 UTABLES>>8    ; the list: no table, no player
        LODI,R1 UTABLES&$FF
        BCTA,UN TXSTR
BU3:    RRL,R0                  ; the route
        STRZ    R2
        LODA,R0 ROUTES+1,R2
        STRZ    R1
        LODA,R0 ROUTES,R2
        BSTA,UN TXSTR
        LODA,R0 ACTSENT       ; its argument
        COMI,R0 ACT_PUSH
        BCFR,EQ BU5
        LODA,R0 ACTARGL         ; "hold", "up", "down" / the units shown
        RRL,R0
        STRZ    R2
        LODA,R0 PUSHW+1,R2
        STRZ    R1
        LODA,R0 PUSHW,R2
        BSTA,UN TXSTR
        LODI,R1 $2F
        LODA,R0 *PTXD,R1
        EORZ    R0
        LODA,R1 AUTRD
        BSTA,UN TXNUM
        BCTR,UN BUQ
BU5:    COMI,R0 ACT_HOLD
        BCFR,EQ BU6
        EORZ    R0              ; the units shown / the price
        LODA,R1 AUTRD
        BSTA,UN TXNUM
        LODI,R1 $2F
        LODA,R0 *PTXD,R1
        BCTR,UN BU8
BU6:    COMI,R0 ACT_DECL
        BCFR,EQ BU7
        LODA,R2 ACTARGL       ; n, b, s
        LODA,R0 DECLW,R2
        STRZ    R1
        LODA,R0 *PTXD,R1
        BCTR,UN BUQ
BU7:    STRZ    R2              ; a route ending in '/' takes the number
        LODA,R0 RTSLASH,R2
        BCTR,EQ BUQ
BU8:    LODA,R0 ACTARGH
        LODA,R1 ACTARGL
        BSTA,UN TXNUM
BUQ:    LODI,R0 UQTAB>>8      ; ?table=
        LODI,R1 UQTAB&$FF
        BSTA,UN TXSTR
        LODI,R3 SCTABLE-$2900
        BSTA,UN TXRAMH
        LODI,R0 UQPLR>>8      ; &player=
        LODI,R1 UQPLR&$FF
        BSTA,UN TXSTR
        LODI,R3 SCNAME-$2900
        BSTA,UN TXRAMH
        LODI,R0 UQBIN>>8      ; &bin=1&be=1
        LODI,R1 UQBIN&$FF
        BCTA,UN TXSTR

; TXSTR: the NUL-terminated ASCII at R0:R1 into the TX stream. TXRAMH: the
; one in RAM-H at $2900 + R3 (SCNAME and friends sit in its first page).
; Leaves. Clobber R0, R1, R3, T0, T1.
TXSTR:  STRA,R0 T0
        STRA,R1 T1
        LODI,R3 $FF
TXS1:   LODA,R0 *T0,R3,+
        RETC,EQ
        STRZ    R1
        LODA,R0 *PTXD,R1
        BCTR,UN TXS1
TXRAMH: SUBI,R3 1
TXH1:   LODA,R0 *PRAMH,R3,+
        RETC,EQ
        STRZ    R1
        LODA,R0 *PTXD,R1
        BCTR,UN TXH1

; TXNUM: R0:R1 in decimal (ASCII) into the TX stream, no leading zeros.
; Leaf (the URL's numbers are its deepest call). Clobbers R0, R1, R3,
; NUM*, T1.
TXNUM:  STRA,R0 NUMM
        STRA,R1 NUML
        EORZ    R0
        STRA,R0 T1              ; T1: a digit sent (leading zeros are not)
        LODI,R3 9               ; 10^4 (POW10's index: 16 bits, so NUMH 0)
TXN1:   LODI,R1 $30
TXN2:   LODA,R0 NUMM            ; under 10^k?
        COMA,R0 POW10+1,R3
        BCFR,EQ TXN5
        LODA,R0 NUML
        COMA,R0 POW10+2,R3
TXN5:   BCTR,LT TXN4
        LODA,R0 NUML            ; NUM -= 10^k (C: no borrow)
        SUBA,R0 POW10+2,R3
        STRA,R0 NUML
        PPSL    PSLWC
        LODA,R0 NUMM
        SUBA,R0 POW10+1,R3
        STRA,R0 NUMM
        CPSL    PSLWC
        ADDI,R1 1
        BCTR,UN TXN2
TXN4:   COMI,R1 $30
        BCFR,EQ TXN6
        LODA,R0 T1
        BCTR,EQ TXN7
TXN6:   LODA,R0 *PTXD,R1
        STRA,R1 T1
TXN7:   ADDI,R3 3
        COMI,R3 21
        BCFA,EQ TXN1
        LODA,R0 NUML            ; the units, always
        IORI,R0 $30
        STRZ    R1
        LODA,R0 *PTXD,R1
        RETC,UN


; SPECTAG: watching, "WATCH" in the corner each screen leaves free, every
; 32 frames (a repaint takes it off). (mekkogx spec_tag)
SPECTAG: LODA,R0 H_MYSEAT
        COMI,R0 4
        RETC,LT
        LODA,R0 FRML
        ANDI,R0 $1F
        RETC,GT
        LODA,R0 SCRNOW
        RETC,EQ
        STRZ    R2
        LODA,R0 SPECROW,R2
        STRZ    R1
        LODI,R0 CWHITE
        STRA,R0 DCOLOR
        LODI,R2 $FF
        BSTA,UN DPOS
        LODI,R0 SWATCH>>8
        LODI,R1 SWATCH&$FF
        BCTA,UN DTXT
SPECROW: DB     0,0,1,1,16,25,1 ; (by SCRNOW: rows those screens leave free)
SWATCH: DB      "WATCH",$FF


; ---- the routes (ASCII) ---------------------------------------------------------
        ASCII
ROUTES: DWBE    URNONE
        DWBE    URSTATE
        DWBE    URREADY
        DWBE    URUNRDY
        DWBE    URSPEC
        DWBE    URCLAIM
        DWBE    URPUSH
        DWBE    URDECL
        DWBE    URCOLL
        DWBE    URBEGIN
        DWBE    URBUYMU
        DWBE    UROUTFT
        DWBE    URINST
        DWBE    URRETRN
        DWBE    URASSAY
        DWBE    URSELL
        DWBE    URUNSEL
        DWBE    URWAMP
        DWBE    URPUB
        DWBE    UREND
        DWBE    URLEAVE
        DWBE    URHOLD
RTSLASH: DB     0,0,0,0,1,1,0,0,0,0,0,1,1,0,1,1,1,0,0,0,0,1    ; takes a number
PUSHW:  DWBE    PUSHW0
        DWBE    PUSHW1
        DWBE    PUSHW2
DECLW:  DB      "nbs"
URNONE: DB      0
URSTATE: DB     "/state",0
URREADY: DB     "/ready/1",0
URUNRDY: DB     "/ready/0",0
URSPEC: DB      "/setup/species/",0
URCLAIM: DB     "/claim/",0
URPUSH: DB      "/auction/push/",0
URDECL: DB      "/declare/",0
URCOLL: DB      "/collude",0
URBEGIN: DB     "/turn/begin",0
URBUYMU: DB     "/turn/buymule",0
UROUTFT: DB     "/turn/outfit/",0
URINST: DB      "/turn/install/",0
URRETRN: DB     "/turn/return",0
URASSAY: DB     "/turn/assay/",0
URSELL: DB      "/turn/sell/",0
URUNSEL: DB     "/turn/unsell/",0
URWAMP: DB      "/turn/wampus",0
URPUB:  DB      "/turn/pub",0
UREND:  DB      "/turn/end",0
URLEAVE: DB     "/leave",0
URHOLD: DB      "/auction/push/hold/",0
PUSHW0: DB      "hold",0
PUSHW1: DB      "up",0
PUSHW2: DB      "down",0
UTABLES: DB     "/tables?bin=1",0
UQTAB:  DB      "?table=",0
UQPLR:  DB      "&player=",0
UQBIN:  DB      "&bin=1&be=1",0
        INCLUDE "build/endpoint.inc"
        GLYPHS

