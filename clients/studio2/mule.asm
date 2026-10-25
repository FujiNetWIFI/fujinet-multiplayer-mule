; mule.asm -- M.U.L.E. for the RCA Studio II on the FujiNet cart (protocol 2).
; Assembled once per bank: build.sh -D BANK=n (see there).

        CPU     1802
        INCLUDE "fujinet.inc"
        INCLUDE "s2macro.inc"
        INCLUDE "macros.inc"
        INCLUDE "flags.inc"
        INCLUDE "proto.inc"
        INCLUDE "strids.inc"
        INCLUDE "art.inc"
        INCLUDE "equ.inc"
        INCLUDE "ram.inc"

; ---- the runtime: $0400-$07FB ----------------------------------------------------
        ORG     0400H
        INCLUDE "s2call.inc"
        INCLUDE "fujidisp.inc"
        INCLUDE "fujilib.inc"
        INCLUDE "input.inc"
        INCLUDE "calls.inc"
RT_SEGEND:
        SEGEND  FN_CLAIM
        S2CLAIM

; ---- fixed code: $0C00-$0FFF, then $x400-$xFFF up to $BFFF ------------------------
        ORG     0C00H
        INCLUDE "core.inc"
        INCLUDE "state.inc"
        INCLUDE "net.inc"
        INCLUDE "text.inc"
        INCLUDE "gfx.inc"
        INCLUDE "snd.inc"
        INCLUDE "fx.inc"
        INCLUDE "map.inc"
        INCLUDE "game.inc"
        INCLUDE "grant.inc"
        INCLUDE "lobby.inc"
        INCLUDE "summary.inc"
        INCLUDE "develop.inc"
        INCLUDE "town.inc"
        INCLUDE "auction.inc"
        INCLUDE "auction2.inc"
        INCLUDE "prod.inc"
        RFIT    80
        INCLUDE "endpoint.inc"
FIX_SEGEND:
        SEGEND  0C000H

; ---- the bank window: $C400-$CFFF ------------------------------------------------
        IF      BANK == BK_BOOT
        ORG     0C400H
        INCLUDE "boot.inc"
BK0_SEGEND:
        SEGEND  0D000H
        ENDIF
        IF      BANK == BK_TEST
        ORG     0C400H
        INCLUDE "test.inc"
BK1_SEGEND:
        SEGEND  0D000H
        ENDIF
        IF      BANK == BK_ART
        ORG     0C400H
        INCLUDE "artbank.inc"
BK2_SEGEND:
        SEGEND  0D000H
        ENDIF
        INCLUDE "str.inc"

; ---- the cart's RAM: image pages the client writes ----------------------------------
        ORG     SNAP
        REPT    CRAMEND-SNAP
        DB      0
        ENDM
