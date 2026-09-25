; mule.asm -- the MSX's memory map: an ASCII8 megaROM (tools/msx/mkrom.py)
;
;   4000h-9FFFh     banks 0-2, fixed: the program (code and constants)
;   A000h-BFFBh     the window: bank 3 + n is overlay n (a phase's code),
;                   bank 8 the art (gen/tms9918/gfx.c), bank 9 the strings;
;                   BFFCh-BFFFh are the FujiNet's registers in every bank
;   C000h ..        RAM: the variables, then the stack down from HIMEM

        PUBLIC  WIN_SIZE
        defc    WIN_ORG = $A000
        defc    WIN_SIZE = $1FFC

        SECTION OV1
        org     WIN_ORG
        SECTION OV2
        org     WIN_ORG
        SECTION OV3
        org     WIN_ORG
        SECTION OV4
        org     WIN_ORG
        SECTION OV5
        org     WIN_ORG
        SECTION ART
        org     WIN_ORG
        SECTION STR
        org     WIN_ORG
