; mule.asm -- the Adam's memory map:
;
;   0000h ..        the program: code, constants, data (loaded at boot by
;                   z88dk's bootstrap, a 1K block at a time), then the
;                   variables (not loaded: the CRT clears them)
;   OVL_ORG ..      the overlay window: a phase's code (bank.c), each of
;                   OV1 .. OV5 linked here and read in whole blocks
;   .. D390h        the stack, down from EOS's own memory
;
; The bootstrap sits at C800h while it loads, so the program must end
; below it; tools/adam/mkimage.py checks that, that the variables end
; below the window, and that each overlay fits it.

        defc    OVL_ORG = $A000
        defc    OVL_SIZE = $1C00        ; 7K: whole blocks

        PUBLIC  _ovl_window, OVL_SIZE
        defc    _ovl_window = OVL_ORG

        SECTION OV1
        org     OVL_ORG
        SECTION OV2
        org     OVL_ORG
        SECTION OV3
        org     OVL_ORG
        SECTION OV4
        org     OVL_ORG
        SECTION OV5
        org     OVL_ORG
