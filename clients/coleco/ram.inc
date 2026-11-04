; ram.inc -- the RAM ledger.
;
; The console has 1K, at 7000H-73FFH (mirrored from 6000H; 7800H up must
; never be touched, see fujilib.inc). The BIOS keeps 73B9H up, the sound
; engine 7020H-702AH. tools/checksize.py fails the build if the variables
; run into the stack.
;
; VAR allocates upward from RAMBASE. The PHASE block at the end is shared:
; each phase page lays its own variables over it (PV), because only one
; phase runs at a time.
;
; The state: the C client keeps three 550-byte copies (st, prev, rx). Here
; the reply stays where the cart painted it (the 1K window at F800H, read
; with RXGETB) from its READ until the next request begins, and a compact
; HOT copy -- the header, the players, the map's terrain/owner/M.U.L.E. and
; the phase's own section -- is what the per-frame code reads. When a new
; state arrives the phase code compares the window with HOT (st with prev),
; then HOT is refreshed.

CTLDAT  EQU     7000H           ; 12: POLLER's controller area (the header's)
SNDAREA EQU     700CH           ; 11: the one sound data area, and its end
VDPBUSY EQU     7017H           ; the game is at the VDP: the NMI waits
VDPPEND EQU     7018H           ; ... and owes a frame
VDPSEEN EQU     7019H           ; status bits seen (bit 6: a fifth sprite)
BGC     EQU     701AH           ; what black is: the background's colour
FRAMES  EQU     701BH           ; 60 Hz, counted in the NMI (2)
MUTED   EQU     701DH
SNDCUR  EQU     701EH           ; the effect playing (its song), for SNDPLAY
SPARE1F EQU     701FH
RAMBASE EQU     702BH           ; past the sound engine's 7020H-702AH

STACK   EQU     OS7STK          ; grows down from the BIOS's own
STKLOW  EQU     7339H           ; 128 bytes of stack: the NMI and its OS7
                                ; calls take ~45 on top of the game's (measured)

RAMP    DEFL    RAMBASE
VAR     MACRO   nm, sz
nm      EQU     RAMP
RAMP    DEFL    RAMP+sz
        ENDM

; ---- the clock and paging ----------------------------------------------
        VAR     DONE,2          ; frames the loop has caught up with
        VAR     CURPG,1         ; the page mapped at C000H
        VAR     FCHL,2          ; FARCALL scratch
        VAR     FCTGT,2
        VAR     FCA,1
        VAR     TESTHK,4        ; the harness's hook (see BOOT)
        VAR     DBGAT,1         ; where the loop is (the harness peeks it)
        VAR     APLAST,2        ; autoplay's last press
        VAR     FXON,1          ; the actors (fx.inc) are running
        VAR     FXFLAG,1        ; a script's FX_SIG
        VAR     TPH,1           ; the offline test mode's state (test.inc):
        VAR     TARG,1          ;   phase, event/stage/seat, player event
        VAR     TPEV,1
        VAR     TERR,1
        VAR     TELAP,2
        VAR     TRES,2
        VAR     TCUR,1          ;   the grant's cursor
        VAR     TNEXT,2
        VAR     TCLM,1
        VAR     TFL,1           ;   the turn
        VAR     TCARRY,1
        VAR     TLEFT,2
        VAR     TTEL,2
        VAR     TNP,1           ;   plots changed: {plot, owner, mule} x 4
        VAR     TPAT,12
        VAR     TSW,2           ;   frames since the last (made-up) state

; ---- the network --------------------------------------------------------
        VAR     AVAIL,2         ; NET_STATUS bytes waiting
        VAR     PRVAVL,2        ; the settle loop's previous reading
        VAR     RXLEN,2         ; reply length, captured after a READ
        VAR     NSTATE,1        ; request: 0 idle, else what is posted (netpg.inc)
        VAR     NWAIT,1         ; frames without progress
        VAR     NSEQ,1          ; the posted transaction's sequence number
        VAR     NETHAVE,1       ; 1 a state, 2 an error header, 0 failed
        VAR     NETFAIL,1
        VAR     POLLW,1         ; frames to the next poll
        VAR     ACT,1           ; queued action (ACT_*)
        VAR     ACTARG,2
        VAR     ACTSNT,1        ; the action the last request carried
        VAR     AUTRADE,1       ; units traded as shown (pushes carry it)
        VAR     STVALID,1       ; 1: the window holds the last state
        VAR     TESTMD,1        ; offline test mode
        VAR     CSBASE,2        ; clock_sync
        VAR     CSHAVE,1
        VAR     URLREQ,1        ; the request being composed

; ---- who and where --------------------------------------------------------
        VAR     SCNAME,9        ; player name, NUL-terminated
        VAR     SCTABL,9        ; table id
ENDPTN  EQU     64
        VAR     ENDPT,ENDPTN+1  ; server URL (no trailing '/')

; ---- the game ---------------------------------------------------------------
        VAR     PHS,1
        VAR     RND,1
        VAR     MYSEAT,1
        VAR     LASTPH,1
        VAR     LASTRND,1
        VAR     STOK,1
        VAR     NEWPH,1         ; the state that arrived began a phase
        VAR     RXPE,2          ; phase-elapsed at the last state ...
        VAR     RXFRM,2         ; ... and the frame it came
        VAR     PHT0,2          ; a phase's own animation began
        VAR     SCRNOW,1
        VAR     ANIM,1
        VAR     DVSTATE,1       ; develop's state (the poll rate needs it)
        VAR     DVASM,6         ; develop: plots seen assayed (a bit each; kept across phases)

; ---- HOT: the state the per-frame code reads (see above) --------------------
HOTSZ   EQU     16+48+135+58
        VAR     HOT,HOTSZ
HOTH    EQU     HOT             ; the header, 16 bytes
HOTP    EQU     HOT+16          ; 4 x 12: player bytes 9..20 (species ..
HPLSZ   EQU     12              ;   rank; see PL_* in win.inc)
HMAP    EQU     HOTP+48         ; the map's first 135 bytes:
HTERR   EQU     HMAP            ;   terrain[45]
HOWNER  EQU     HMAP+45         ;   owner[45]
HMULE   EQU     HMAP+90         ;   mule[45] (crystite and production are
                                ;   read from the window, STVALID)
HOTU    EQU     HMAP+135        ; the phase's section: grant (8), auction
HOTUSZ  EQU     58              ;   (58), turn (its first 14; develop
                                ;   copies the wampus list itself)

; ---- drawing (gfx.inc, spr.inc) ----------------------------------------------
        VAR     SPA_SH,1        ; SPRSET's arguments, in this order
        VAR     SPA_FR,1
        VAR     SPA_X,2
        VAR     SPA_Y,2         ; Atari lines
        VAR     SPA_COL,1
        VAR     SPA_FL,1
        VAR     PLK_ST,1        ; the plot's look (gfx.h PlotLook)
        VAR     PLK_RIV,1
        VAR     PLK_MNT,1
        VAR     PLK_OWN,1
        VAR     PLK_ICON,1
        VAR     PLK_DOTS,1
        VAR     PLK_COL,1
        VAR     CELLP,8         ; a cell being painted: 8 pattern bytes,
        VAR     CELLC,8         ;   8 colour bytes,
        VAR     CELLA,2         ;   its VRAM address
        VAR     GTMP,16         ; one drawing at a time: its own scratch
        VAR     TXCOL,1         ; text: the colour byte (fg, background)
        VAR     TXPOS,2         ; the next character's VRAM address
        VAR     SPDIRTY,1       ; a sprite moved: SPRFLUSH has work
        VAR     SPROT,1         ; which slot goes first (five on a line)

; ---- input ----------------------------------------------------------------
        VAR     INDIR,1         ; DIR_* level
        VAR     INBTN,1         ; button level
        VAR     EVBTN,1         ; taken: pressed
        VAR     EVKEY,1         ; taken: a key
        VAR     LBTN,1          ; latches
        VAR     LKEY,1
        VAR     KPPREV,1        ; last raw key (edges)
        VAR     BTPREV,1        ; last button level
        VAR     INNAME,1        ; name entry: the digits are digits
        VAR     INPHK,1         ; test hook: nonzero -> the three below
        VAR     INPHKD,1
        VAR     INPHKB,1
        VAR     INPHKK,1
        VAR     SPAT,1          ; the SPECTATOR tag: bits 5-7 FRAMES' when
                                ; drawn, bit 0 it is up (status.inc SPTICK)

; ---- the time bar's tick ------------------------------------------------------
        VAR     BTK,1           ; frames to the next, high/low, FRAMES' low
        VAR     BTG,1           ; byte when last run
        VAR     BTF,1

; ---- text and messages --------------------------------------------------------
        VAR     LINBUF,8        ; number formatting
        VAR     NUMTMP,4        ; NUM32R
        VAR     SHTXK,1         ; XSHTX
        VAR     SHTXR,1
        VAR     SHTXC,1
        VAR     SDMODE,1        ; SDRAWM: text/big, centred, padded
        VAR     SDPAD,1
        VAR     MSGROW,1        ; the message's first row
        VAR     MSGCOL,1
        VAR     MSGSCR,1        ; 1: scrolling along the second row
        VAR     MSGPG,1         ; the string: page, address
        VAR     MSGADR,2
        VAR     MSGPFX,2        ; a RAM string printed first (a name), or 0
        VAR     MSGV1,2         ; the numbers for its '%'s
        VAR     MSGV2,2
        VAR     MSGLEN,1
        VAR     MSGPOS,1
        VAR     MSGLST,2
        VAR     MSGCUT,1
        VAR     MSGK,1          ; walker: characters emitted
        VAR     MSGVN,1         ; walker: which number next
        VAR     MSGSKP,1        ; walker: characters to skip
        VAR     MSGMAX,1        ; walker: characters to emit
        VAR     MSGNP,2         ; walker: the number being emitted

; ---- sprites ------------------------------------------------------------------
SPRN    EQU     10
SPRSZ   EQU     7               ; shape, frame shown, x lo, x hi, y lo, y hi, flags
SP_SH   EQU     0               ; 0FFH: nothing sent yet
SP_FR   EQU     1               ; the frame its patterns hold (mirrored ones
SP_XL   EQU     2               ;   are frames of their own)
SP_XH   EQU     3
SP_YL   EQU     4
SP_YH   EQU     5
SP_CF   EQU     6               ; bit 7 on, bits 0-3 the colour (C_*)
        VAR     SPRT,SPRN*SPRSZ

; ---- the map ----------------------------------------------------------------------
        VAR     MAPSRC,1        ; 1: the state is in the window (ENTER/STATE)
        VAR     MAPDOTS,1
        VAR     MAPDSTEP,1      ; 1-8: production shown only up to this many
        VAR     MAPBLK,1        ; the plot drawn outlined ...
        VAR     MAPBLON,1       ; ... when this is 1
        VAR     BARX,1          ; the time bar (Atari units)
        VAR     BARTOP,1
        VAR     BARH,1
        VAR     BARCOL,1
        VAR     BARLAST,1
        VAR     MPP,1           ; XMAPPLOT's scratch
        VAR     MPO,1
        VAR     MPG,1
        VAR     MPBL,1
        VAR     MPQ,1

; ---- the phase's own (keep this last) --------------------------------------------
        VAR     PHRAM,0
PV      MACRO   nm, sz          ; a phase variable (PVP DEFL PHRAM first)
nm      EQU     PVP
PVP     DEFL    PVP+sz
        ENDM
PVP     DEFL    PHRAM

RAMEND  DEFL    RAMP            ; raised by the phase pages' PV blocks

; ---- the actors (fx.c): at the phase RAM's start, for the phases that use
; them (grant, production, summary); their own variables follow FXEND.
FXN     EQU     4
FXSZ    EQU     24
FXA     EQU     PHRAM
FXEND   EQU     PHRAM+FXN*FXSZ  ; (FXON and FXFLAG are in main RAM: FXSTOP
                                ; touches the actors only if FXON says so)
