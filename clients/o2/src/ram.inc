; ram.inc -- where everything lives. tools/checkram.py holds the ledger.
;
; Internal RAM (64 bytes):
;   $00-$07  register bank 0: the main program
;   $08-$17  the stack, 8 levels: the interrupts take up to 3
;   $18-$1F  register bank 1: the interrupts' own
;   $20-$3C  below
;   $3D-$3F  the BIOS's (its keyboard and IRQ routines)

ROWBUF  equ 020h                ; 19: the next map row, staged for its writer
FRAMES  equ 033h                ; vblanks, for the main loop's clock
DIRTY   equ 034h                ; bit 0 the static characters, bit 1 the grid
MODE    equ 035h                ; bit 0 the screen has map rows (see fixed.inc)
CURPG   equ 036h                ; the page mapped at $800
OLDPG   equ 037h                ; farcall's caller's page
; $38-$3C: the development's (p12.inc, p13.inc)
IR_IN   equ 038h                ; town: 1 walking in up, 2 down, 0 not
IR_BLD  equ 039h                ; town: the building (0-3 the goods' shops,
                                ; 4 assay, 5 land, 6 pub, 7 corral)
IR_BW   equ 03Ah                ; town: frames left waiting in a door
IR_HOLD equ 03Bh                ; town: states a message holds the top line
SNDCNT  equ 03Ch                ; frames the sound has left (fixed.inc beep)
; (XRAM scratch, the line buffer's tail: p13's wampus check and event)
WX_W    equ LINEBUF+12          ; the turn's clock / 16
WX_P    equ LINEBUF+13          ; a wampus's plot, its mountain, when it
WX_M    equ LINEBUF+14          ; shows / 16; an event's kind
WX_F    equ LINEBUF+15

DIRTY_STATIC equ 001h
DIRTY_GRID   equ 002h
MODE_ROWS    equ 001h

; External RAM (128 bytes, $00-$7F, MOVX with P1_MAIN):
LINEBUF equ 000h                ; 16: a text line being built (disp.inc)
TXCOL   equ 010h                ; its colour, and for characters with bit 7 set
TXCOL2  equ 011h
; the request in flight (net.inc)
NSTATE  equ 014h                ; NS_*
NSEQ    equ 015h                ; the sequence number posted
NWAIT   equ 016h                ; frames without progress
ACT     equ 017h                ; the action queued (0: none, a poll)
ACTARG  equ 018h                ; 2: its argument
ACTSENT equ 01Ah                ; the action under way
NARG    equ 01Bh                ; 2: its argument
NBWL    equ 01Dh                ; 2: STATUS's last bytes-waiting
NETHAVE equ 01Fh                ; how it ended: NH_*
; who and where
SCNAME  equ 020h                ; 9: the player's name, NUL-terminated
SCTABL  equ 029h                ; 9: the table's id
; input (input.inc)
JOY     equ 032h                ; the stick now: J_* bits
JOYP    equ 033h                ; last frame's
JOYEV   equ 034h                ; pressed since last frame
REP     equ 035h                ; auto-repeat countdown
; the boot's
AKLEN   equ 036h
PICK    equ 037h                ; the name picker's letter
NAMEN   equ 038h                ; its length
SEL     equ 039h                ; a list's selection
NTBL    equ 03Ah                ; tables listed
; the lobby's (p3.inc), over the boot's
LB_SPEC equ 036h                ; our species as last sent (0FFh: the state's)
LB_CD   equ 037h                ; 80h the countdown ran at the last state, 1 our ready
; the time bar (tbar.inc), over the boot's: the auctions, the development
TB_U    equ 036h                ; units left (ticks, PTU); 0 none
TB_PER  equ 037h                ; frames a unit
TB_CNT  equ 038h                ; frames to the next unit
TB_SEG  equ 039h                ; units a segment of the bar (0: no bar)
TB_TK   equ 03Ah                ; frames to the next tick; bit 7 the tock's
TMP0    equ 03Bh                ; a loop's index across calls that keep nothing
; the state's header, kept (HOT): what the per-frame code reads
PHS     equ 03Ch                ; the phase
RND     equ 03Dh                ; the round
MYSEAT  equ 03Eh                ; our seat (0FFh: watching)
LASTPH  equ 03Fh                ; the phase shown
POLLW   equ 040h                ; frames until the next poll
HERR    equ 041h                ; the last action's error
HT      equ 042h                ; 2: the server's clock (jiffies)
HPE     equ 044h                ; 2: jiffies since the phase began
HCD     equ 046h                ; 2: the lobby's countdown
HRES    equ 048h                ; 2: the last action's result
LAGB    equ 04Ah                ; the server's clock less ours, fastest reply
RXAGE   equ 04Bh                ; jiffies this state is older than the fastest
; the map (map.inc)
MAPDIG  equ 04Ch                ; nonzero: production digits on the M.U.L.E.s
; the land grant (p4.inc, mekkogx grant.c)
G_CUR   equ 04Dh                ; the cursor's plot (0FFh before it starts)
G_NEXT  equ 04Eh                ; frames to its next step
G_STEP  equ 04Fh                ; frames a step
G_SHOW  equ 050h                ; the cursor is shown
GR_CLD  equ 051h                ; we have claimed
GR_SRV  equ 052h                ; the cursor in the last state
GR_PRS  equ 053h                ; the cursor's plot when the button went down
GR_WAS  equ 054h                ; the plot before the cursor's last step
GR_WNT  equ 055h                ; the plot we claimed and wait on
GR_LIM  equ 056h                ; the furthest plot the cursor may step to
GR_PRK  equ 057h                ; our claim came early: sent again later
; the summary and game over (p3.inc), over the grant's: never both at once
SU_ORD  equ 04Dh                ; 4: the seats by rank
SU_COL  equ 051h                ; the column shown: 0 total, 1 money, 2 land, 3 goods
SU_K    equ 052h                ; the seat a line is for
NUMV    equ 053h                ; 3: a number for lb_num, low byte first
SU_VIEW equ 056h                ; game over: 1 when the button swapped the views
SU_DRAWN equ 057h               ; game over: the view on screen (0FFh none)
; the auctions (p8.inc), over the grant's, the map builder's and the
; production's ($4D-$60, $6D-$70): page 8 sets them all from each state
AV_POS  equ 04Dh                ; 8: each seat's line, a price (low byte first)
AV_TGT  equ 055h                ; 8: where each line is going
AV_BASE equ 05Dh                ; 2: the price at AY_BASE (land: scrolls up)
AV_KIND equ 05Fh                ; 1 land, 2 goods
AV_STG  equ 060h                ; the stage on screen (0FFh none)
AV_ROLE equ 06Dh                ; 4: ROLE_* by seat
; map_build's scratch (map.inc)
MB_ROW  equ 058h
MB_K    equ 059h
MB_P    equ 05Ah
MB_Y    equ 05Bh
MB_M    equ 05Ch                ; right halves written
MB_COL  equ 05Dh
MB_MNT  equ 05Eh                ; 0: the plot is a mountain
MB_RB   equ 05Fh                ; the row's table, low byte
MB_C    equ 060h                ; the cell being drawn
; the say queue (sayq.inc, pstr.inc): two requests, SAYA served first
SAYA    equ 061h                ; SAY_SIZE each: SAY_* below
SAYB    equ 066h
SV_PTR  equ 06Bh                ; the string being said: its offset (0 done)
SV_PG   equ 06Ch                ; and its data page
; the production phase (p15.inc)
PR_WAIT equ 06Dh                ; frames the colony event stays up
PR_EV   equ 06Eh                ; the event (CE_*), its plot
PR_PLOT equ 06Fh
GR_DEP  equ 070h                ; the grant: 0FFh its words not yet said (gr_dep)
LAGH    equ 071h                ; LAGB is set (rx_age)
TICKF   equ 072h                ; FRAMES at the last tick (or state)
TICKN   equ 073h                ; frames since then, for this tick: a state
                                ; can take the main loop a few frames
DRAWQ   equ 074h                ; the text screen's next line to draw (p3, p7):
                                ; a state's six, one a tick; 6 none
AUSTG   equ 075h                ; the auction's stage (STAGE_*), from each state
LASTPG  equ 076h                ; the page the last state went to
AV_STEPV equ 077h               ; dollars a step (1, or 4 for land)
AV_TICK equ 078h                ; frames to the lines' next step
AV_PUSH equ 079h                ; our stick as sent: 0 let go, 1 up, 2 down
AV_GOOD equ 07Ah                ; the good (0-3)
AV_LN1  equ 07Bh                ; the sellers' line's Y on screen (0FFh none)
AV_LN2  equ 07Ch                ; the buyers'
AV_MSG  equ 07Dh                ; the message on screen (0FFh none, 0 units)
AV_TRD  equ 07Eh                ; units traded so far (a click each)
; the map's set pieces (p15.inc), over the auction's and the development's
FX_PC   equ 077h                ; the actor's script: its op (0: stopped)
FX_X    equ 078h                ; its top left
FX_Y    equ 079h
FX_CNT  equ 07Ah                ; frames to the op's end, or a MOVE's step
FX_N    equ 07Bh                ; a MOVE's steps left
FX_LP   equ 07Ch                ; a LOOP's count left
FX_COL  equ 07Dh                ; its colour; FX_HID; its pieces << 6
FXSH    equ 07Eh                ; the ship's phase: PHS while it shows (page
                                ; 15), | 80h once it's gone (p2.inc pp_ship)
DV_PH   equ 07Fh                ; the round whose development is under way
; the development (p11.inc, p12.inc), over the grant's ($4D-$57), the
; production's ($6D-$70) and the auction's ($77-$7A)
DV_ST   equ 04Dh                ; DV_* (p11.inc)
DV_TOWN equ 04Eh                ; 1: we walk in town (page 12), 0 the map (11)
DV_MSG  equ 050h                ; another's turn said: seat * 4 + kind
DV_SEAT equ 051h                ; whose turn
DV_FLAGS equ 052h               ; its TF_*
DV_MULE equ 053h                ; nonzero: we lead a M.U.L.E.
DV_SAMPLE equ 054h              ; the plot we took a sample from (0FFh none)
DV_OFFICE equ 055h              ; 1: the land office waits for a plot
DV_CNT  equ 056h                ; frames to our next step
DV_ACK  equ 057h                ; the action whose answer we said
DV_X    equ 06Dh                ; the figure's top left (map or street)
DV_Y    equ 06Eh
DV_MX   equ 06Fh                ; where it was a step ago: the M.U.L.E.
DV_MY   equ 070h
DV_FR   equ 077h                ; the walking frame
DV_WP   equ 078h                ; the wampus showing (0FFh none) ...
DV_WPX  equ 079h                ; ... and where
DV_WPY  equ 07Ah
DV_SPEC equ 07Bh                ; our species (its figure: p11.inc dv_fig)

SAY_ID  equ 0                   ; the string (S_*, T_*, M_*; 0 none)
SAY_LN  equ 1                   ; its first line (L_*), and lines << 4
SAY_COL equ 2                   ; its colour
SAY_NUM equ 3                   ; 2: its '%'
SAY_SIZE equ 5
