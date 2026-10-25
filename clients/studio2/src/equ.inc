; equ.inc -- the game's own numbers. The protocol's are in build/proto.inc
; (tools/mkproto.py), the strings' in build/strids.inc, the pictures' in
; build/art.inc; the cart's in fujinet.inc.

; ---- banks ($C400-$CFFF) -----------------------------------------------------
NBANKS  EQU     7               ; build.sh assembles mule.asm once for each
BK_BOOT EQU     0               ; title, name, tables, the menu
BK_TEST EQU     1               ; test mode
BK_ART  EQU     2               ; the pictures (gfx_load)
STRBK   EQU     3               ; the strings, NSTRBK banks from here
BK_AP   EQU     6               ; AUTOPLAY builds' player

; A code bank starts with its entries, an FJMP each: fixed code reaches the
; bank through these (BCALL bank, entry), never by its labels, which the
; other banks' assemblies don't have.
E_TITLE    EQU  0C400H          ; BK_BOOT
E_BOOT     EQU  0C403H
E_RECHOOSE EQU  0C406H
E_MENU     EQU  0C409H
E_GFXLOAD  EQU  0C400H          ; BK_ART
E_TSTART   EQU  0C400H          ; BK_TEST
E_TPOLL    EQU  0C403H
E_AUTO     EQU  0C400H          ; BK_AP

; ---- the cart's RAM ($D400-$DFFF: image pages the client writes) ---------
PREV    EQU     SNAP+PRVOFF     ; the state before (the cart copies SNAP there)
CRAM    EQU     0DA00H          ; the client's own
CR_NAME EQU     CRAM+00H        ; 9: the player's name (appkey 1/1/0)
CR_TABLE EQU    CRAM+10H        ; 9: the table id
CR_ENDPT EQU    CRAM+20H        ; 66: the room's server (appkey 1/1/10), "" none
CRAMEND EQU     0DC00H          ; the cart RAM pages the image carries

; ---- the network --------------------------------------------------------------
NETDEV  EQU     71H             ; N1:
NCOPEN  EQU     4FH
NCCLOSE EQU     43H
NCREAD  EQU     52H
NCSTAT  EQU     53H
NMHTTPG EQU     0CH             ; OPEN mode: HTTP GET
FCACK   EQU     06H
FCCKEY  EQU     0DBH            ; CLOSE_APPKEY
FCOKEY  EQU     0DCH            ; OPEN_APPKEY
FCRKEY  EQU     0DDH            ; READ_APPKEY
FCWKEY  EQU     0DEH            ; WRITE_APPKEY
NREPLEN EQU     550

; ---- actions (mekkogx mule.h; the routes in net.inc) -------------------------
ACT_NONE   EQU  0
ACT_STATE  EQU  1
ACT_READY  EQU  2
ACT_UNRDY  EQU  3
ACT_SPEC   EQU  4
ACT_CLAIM  EQU  5
ACT_PUSH   EQU  6               ; arg 0 hold, 1 up, 2 down
ACT_DECL   EQU  7               ; arg 0 sit out, 1 buyer, 2 seller
ACT_COLL   EQU  8
ACT_BEGIN  EQU  9
ACT_BUYMU  EQU  10
ACT_OUTFIT EQU  11
ACT_INST   EQU  12
ACT_RETURN EQU  13
ACT_ASSAY  EQU  14
ACT_SELL   EQU  15
ACT_UNSELL EQU  16
ACT_WAMPUS EQU  17
ACT_PUB    EQU  18
ACT_END    EQU  19
ACT_LEAVE  EQU  20
ACT_HOLD   EQU  21              ; arg: the price our released line stopped at
ACT_TABLES EQU  22

; net results (NHAVE)
NH_NONE  EQU    0
NH_STATE EQU    1               ; a whole state is in SNAP
NH_ERR   EQU    2               ; a header only (turned away): in SNAP's header
NH_FAIL  EQU    3
NH_TABLES EQU   4               ; the table list is in the reply window

; ---- input ---------------------------------------------------------------------
K_NONE  EQU     0FFH            ; KEY / KEDGE: no key (else 0-9, either pad)
K_FIRE  EQU     5
K_MENU  EQU     0

; ---- sounds (mekkogx platform-specific/sound.h) ---------------------------------
SND_SELECT EQU  0
SND_SAMPLE EQU  1
SND_ERROR  EQU  2
SND_BUY    EQU  3
SND_ALERT  EQU  4
SND_AUCOVR EQU  5
SND_TRADE  EQU  6
SND_EVENT  EQU  7
SND_FOUND  EQU  8
SND_STEP   EQU  9               ; from here up: soft, never cuts another
SND_TICK   EQU  10
SND_TOCK   EQU  11

; ---- what is on the screen, what moves by itself (mekkogx mule.h) -------------
SCR_NONE EQU    0
SCR_TEXT EQU    1
SCR_MAP  EQU    2
SCR_TOWN EQU    3
SCR_AUC  EQU    4
SCR_SUM  EQU    5
SCR_FOUND EQU   6
AN_NONE  EQU    0
AN_WALK  EQU    1
AN_GRANT EQU    2
AN_AUC   EQU    3
DV_WAIT  EQU    0
DV_PROMPT EQU   1
DV_WALK  EQU    2
DV_OVER  EQU    3

NOBODY  EQU     0FFH

; ---- the screen ----------------------------------------------------------------
TCOLS   EQU     16
