; mule.asm -- M.U.L.E. (Tournament) for the ColecoVision, over FujiNet.
;
; A hand-written Z80 port of the Bally Astrocade client (clients/astrocade,
; itself a port of the MekkoGX C client), with the Coleco Adam's screen,
; art and sound (clients/mekkogx/src/tms9918, src/adam). The server runs
; the game; we poll /state?bin=1 (the 550-byte MuleState of
; clients/mule_proto.h), draw it the Atari's way and play the animations at
; the Atari's speed.
;
; A stock console: 1K of RAM. The FujiNet cartridge serves a banked image
; (APPBANK) with its mailbox live at F800H:
;   8000H-BFFFH  fixed: the header, the frame interrupt, the network, the
;                drawing, the art
;   C000H-DFFFH  the page: one read of FNBKSEL+n maps image page n
;   E000H-F7FFH  fixed
;   F800H-FFFFH  the mailbox
; Page 0 is image 4000H-5FFFH; page n >= 1 follows the first 32K. A console
; RESET leaves the page where it was: START selects page 0. Code in a page
; never maps another page itself: it goes through FARCALL or fixed code.
;
; zmac wants its ORGs in order, so the pages n >= 1 are assembled first, at
; 0000H.. (each PHASEd to C000H), then the window at 8000H; build.sh puts the
; window first in the image.
;
; The frame interrupt is the VDP's, on the NMI (core.inc): it counts frames
; and runs OS7's POLLER and sound engine. It never reads F800H and up.

        INCLUDE "os7.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "ram.inc"
        INCLUDE "build/flags.inc"

; ---- pages ------------------------------------------------------------
PGBOOT  EQU     0               ; boot, the strings, the test mode
PGSTR   EQU     0
PGTEST  EQU     0
PGSTAT  EQU     1               ; lobby, summary, game over
PGGRANT EQU     1               ; land grant, production, events, the fx
PGAUC   EQU     2               ; status per good, declaring, the auctions
PGDEV   EQU     3               ; development
NPAGES  EQU     4

; =======================================================================
; page 1: status, grant
; =======================================================================
        ORG     0000H
        PHASE   0C000H
MB_P1:  DB      1               ; the page's number (C000H says which is in)
MB_P1_STATUS:
        INCLUDE "status.inc"
MB_P1_FX:
        INCLUDE "fx.inc"
MB_P1_GRANT:
        INCLUDE "grant.inc"
MB_P1END:
        DEPHASE

; =======================================================================
; page 2: the auctions
; =======================================================================
        ORG     2000H
        PHASE   0C000H
MB_P2:  DB      2               ; the page's number (C000H says which is in)
MB_P2_AUC:
        INCLUDE "auction.inc"
MB_P2END:
        DEPHASE

; =======================================================================
; page 3: development
; =======================================================================
        ORG     4000H
        PHASE   0C000H
MB_P3:  DB      3               ; the page's number (C000H says which is in)
MB_P3_DEV:
        INCLUDE "dev1.inc"
        INCLUDE "dev2.inc"
        INCLUDE "dev2b.inc"
MB_P3END:
        DEPHASE

; =======================================================================
; the window: fixed, 8000H-BFFFH
; =======================================================================
        ORG     8000H
MB_F:
; The cartridge header. 55H AAH skips the BIOS's title screen: straight to
; START.
        DB      55H,0AAH
        DW      SPRORD          ; OS7's sprite tables and work buffer: never
        DW      SPRORD          ;   used (its object routines aren't)
        DW      SPRORD
        DW      CTLDAT          ; POLLER's controller area
        DW      START
        JP      RSTRET          ; RST 08H .. 38H
        JP      RSTRET
        JP      RSTRET
        JP      RSTRET
        JP      RSTRET
        JP      RSTRET
        JP      RSTRET
        JP      NMI             ; 8021H: the frame
        DB      "M.U.L.E./FUJINET/2026",0
SPRORD  EQU     SNDAREA         ; (harmless: OS7 never writes there for us)
RSTRET: RET

; START: a cold start, power-on or RESET.
START:  LD      A,1             ; first: the NMI waits until we are set up
        LD      (VDPBUSY),A
        DI
        LD      SP,STACK
        LD      A,(FNBKSEL+0)   ; page 0 (RESET leaves the page as it was)
        XOR     A
        LD      (CURPG),A
        LD      (DEFERWR),A     ; OS7: VRAM writes now, not queued
        LD      (MUXSPRT),A
        LD      HL,RAMBASE      ; our variables
        LD      DE,RAMBASE+1
        LD      BC,STKLOW-RAMBASE-1
        LD      (HL),0
        LDIR
        LD      HL,FRAMES       ; and the low ones
        LD      DE,FRAMES+1
        LD      BC,SPARE1F-FRAMES
        LD      (HL),0
        LDIR
        XOR     A
        LD      (VDPPEND),A
        LD      (VDPSEEN),A
        LD      HL,(RANDNUM)    ; RAND_GEN's seed: never 0
        LD      A,H
        OR      L
        JR      NZ,ST1
        LD      HL,0A55AH
        LD      (RANDNUM),HL
ST1:    CALL    SNDINI
        CALL    ININIT
        CALL    SPRINIT
        CALL    VINIT           ; the screen, and the frame interrupt on
        XOR     A
        LD      (VDPBUSY),A
        LD      A,C_WHITE
        LD      (MSGCOL),A
        LD      A,22
        LD      (MSGROW),A
        LD      A,0FFH
        LD      (MAPBLK),A
        CALL    FARCALL
        DB      PGBOOT
        DW      BOOT
        JP      MAINLP

MB_F_CORE:
        INCLUDE "core.inc"
MB_F_LOOP:
        INCLUDE "loop.inc"
MB_F_WIN:
        INCLUDE "win.inc"
MB_F_MSG:
        INCLUDE "msg.inc"
MB_F_INPUT:
        INCLUDE "input.inc"
MB_F_SOUND:
        INCLUDE "sound.inc"
MB_F_VDP:
        INCLUDE "vdp.inc"
MB_F_TEXT:
        INCLUDE "text.inc"
MB_F_GFX:
        INCLUDE "gfx.inc"
MB_F_SPR:
        INCLUDE "spr.inc"
MB_F_MAP:
        INCLUDE "map.inc"
MB_F_FUJI:
        INCLUDE "fujilib.inc"
MB_F_NET:
        INCLUDE "netpg.inc"
MB_F_ART:
        INCLUDE "gen/gfx.inc"
MB_F_DATA:
        INCLUDE "build/endpoint.inc"
MB_FEND:

; =======================================================================
; page 0: boot, the strings, the test mode (C000H at power-on)
; =======================================================================
        ORG     0C000H
MB_P0:  DB      0
MB_P0_BOOT:
        INCLUDE "boot.inc"
MB_P0_MSG:
        INCLUDE "msgpg.inc"
        INCLUDE "msgpg2.inc"
MB_P0_STR:
        INCLUDE "gen/stra.inc"
        INCLUDE "gen/strb.inc"
MB_P0_TEST:
        INCLUDE "test.inc"
MB_P0END:

; =======================================================================
; the window: fixed, E000H-F7FFH
; =======================================================================
        ORG     0E000H
MB_G:
MB_GEND:

; the mailbox: F800H-FFFFH, clear but for the claim build.sh stamps
        ORG     0FFFFH
        DB      0
