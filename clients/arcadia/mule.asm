; mule.asm -- M.U.L.E. for the Emerson Arcadia 2001, on the FujiNet cart.
;
; Assembled once per page (build.sh writes build/mule_pN.asm: PAGE EQU N,
; then this): every assembly holds the fixed halves, identically (the
; build checks), and its own page in the window at $0800-$0FFF. So a page
; can call fixed code but never another page -- another page's labels
; don't exist in its assembly. The image (protocol 2, APPBANK) is page 0's
; 8K, the mailbox empty and claimed, then pages 1.. (tools/mkmule.py).
;
;   $0000-$05FF  fixed A   src/fixa.inc   start, vblank, input, sound, display
;   $0600-$07FF  RAM-L     cart RAM: the copy of the state (src/ram.inc)
;   $0800-$0FFF  window    a 2K page: entries at $0800 + 3k, its id at $0FFF
;   $2000-$28FF  fixed B   src/fixb.inc   main loop, network, cart RAM writes
;   $2900-$2AFF  RAM-H     cart RAM: wampus list, names, bookkeeping
;   $2B00-$2FFF  mailbox
;
; It ports the MekkoGX client (clients/mekkogx/src/*.c): comments name the
; C routine each piece comes from.

        CPU     2650
        INCLUDE "src/macros.inc"
        GLYPHS
        INCLUDE "build/flags.inc"
        INCLUDE "build/proto.inc"
        INCLUDE "build/strids.inc"
        INCLUDE "build/artequ.inc"
        INCLUDE "src/equ.inc"
        INCLUDE "src/ram.inc"
        INCLUDE "src/pages.inc"

        INCLUDE "src/fixa.inc"

        ORG     WIN
        IF      PAGE == PG_BOOT
        INCLUDE "src/p_boot.inc"
        ELSEIF  PAGE == PG_CORE
        INCLUDE "src/p_core.inc"
        ELSEIF  PAGE == PG_ART
        INCLUDE "build/art.inc"
        ELSEIF  PAGE == PG_MAP
        INCLUDE "src/p_map.inc"
        ELSEIF  PAGE == PG_TEST
        INCLUDE "src/p_test.inc"
        ELSEIF  PAGE == PG_GRANT
        INCLUDE "src/p_grant.inc"
        ELSEIF  PAGE == PG_LOBBY
        INCLUDE "src/p_lobby.inc"
        ELSEIF  PAGE == PG_SUM
        INCLUDE "src/p_sum.inc"
        ELSEIF  PAGE == PG_AUC
        INCLUDE "src/p_auc.inc"
        ELSEIF  PAGE == PG_ARENA
        INCLUDE "src/p_arena.inc"
        ELSEIF  PAGE == PG_ARB
        INCLUDE "src/p_arb.inc"
        ELSEIF  PAGE == PG_ARC
        INCLUDE "src/p_arc.inc"
        ELSEIF  PAGE == PG_DEV
        INCLUDE "src/p_dev.inc"
        ELSEIF  PAGE == PG_TOWN
        INCLUDE "src/p_town.inc"
        ELSEIF  PAGE == PG_DEVT
        INCLUDE "src/p_devt.inc"
        ELSEIF  PAGE == PG_PROD
        INCLUDE "src/p_prod.inc"
        ELSEIF  (PAGE == PG_AUTO) && AUTOPLAY
        INCLUDE "src/p_auto.inc"
        ELSEIF  PAGE == PG_TITLE
        INCLUDE "src/p_title.inc"
        ELSEIF  PAGE == PG_FX
        INCLUDE "src/p_fx.inc"
        ELSEIF  (PAGE >= STRPG) && (PAGE < STRPG + NSTRPG)
        INCLUDE "build/str.inc"
        ELSE
        INCLUDE "src/p_stub.inc"
        ENDIF
        IF      $ > $0FFF
        ERROR   "this page is full"
        ENDIF
        ORG     $0FFF
        DB      PAGE            ; the page's id

        ORG     $2000
        INCLUDE "src/fixb.inc"
        IF      $ > $2900
        ERROR   "fixed B runs into RAM-H"
        ENDIF
