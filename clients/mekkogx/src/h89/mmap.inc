;
; The H89 build's memory map (-pragma-define:MMAP=-1): z88dk's own, then the
; five phases' overlays. Each is linked at the window, H89_OVL_WIN, with its
; number in the 64K digit, so the linker keeps them apart and
; tools/h89/ovl.py can tell a reference into an overlay from outside it
; (main.c's phase tables are the only ones allowed). z80asm writes each one
; out as mule_OVL_<name>.bin; ovl.py packs them into MULE.OVL (CP/M) or on
; the end of MULE.ABS (HDOS).
;
; The OVn names are bank.h's: OV1 boot.c, teststart.c; OV2 status.c; OV3
; grant.c, events.c; OV4 auction.c; OV5 develop.c (see the Makefile).
;

    ; CP/M: the variables after the window, past the end of the file. (The
    ; classic CRT has no CRT_ORG_BSS of its own: this is what its
    ; crt_section_bss.inc reads.) HDOS (the window is under the program):
    ; CRT_TRIM_BSS puts them after the program, out of the file.
IF !DEFINED_H89_HDOS
    defc    __crt_org_bss = H89_OVL_WIN + H89_OVL_SIZE
ENDIF

    INCLUDE "crt/classic/crt_section_standard.inc"

    SECTION OVL_1
    org     $010000 + H89_OVL_WIN
    SECTION code_ovl_1
    SECTION rodata_ovl_1
    SECTION OVL_1_END

    SECTION OVL_2
    org     $020000 + H89_OVL_WIN
    SECTION code_ovl_2
    SECTION rodata_ovl_2
    SECTION OVL_2_END

    SECTION OVL_3
    org     $030000 + H89_OVL_WIN
    SECTION code_ovl_3
    SECTION rodata_ovl_3
    SECTION OVL_3_END

    SECTION OVL_4
    org     $040000 + H89_OVL_WIN
    SECTION code_ovl_4
    SECTION rodata_ovl_4
    SECTION OVL_4_END

    SECTION OVL_5
    org     $050000 + H89_OVL_WIN
    SECTION code_ovl_5
    SECTION rodata_ovl_5
    SECTION OVL_5_END
