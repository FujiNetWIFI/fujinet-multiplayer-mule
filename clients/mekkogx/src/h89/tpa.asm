;
; The memory check, made before BSS is cleared (HDOS build, H89_HDOS).
;
; BSS is not in MULE.ABS (CRT_TRIM_BSS), so HDOS checks only the image when
; it loads the program, and every driver it has loaded since boot -- FN:,
; LP: -- came down from the top of memory. The program fits under HDOS's
; overlay area with FN: in (tools/h89/ovl.py fails the link otherwise);
; with more, BSS could run into it, and clearing BSS would take HDOS with
; it. So the link turns the startup's clear off (CRT_INITIALIZE_BSS=0) and
; this runs in its place, in code_crt_init: it wants the end of BSS under
; S.SYSM - S.OMAX, says what it found if not, and otherwise tells HDOS the
; program's real top (.SETTOP) and clears BSS.
;
; It also fills the stack, 2280H up to near where it is now, with E5H, so a
; memory dump after a run shows how deep the stack went.
;
; M.U.L.E.: the screen's cells (H89_CELLS, src/h89/plat.c) go after BSS, so
; they are part of what it wants. The overlays come from this file, past the
; image (src/h89/bank.c); the BSS is not in it.
;

IFDEF H89_HDOS

    EXTERN  __BSS_head
    EXTERN  __BSS_END_tail
    EXTERN  __restore_sp_onexit

    defc    S_SYSM = 0x20D0         ; the first byte of resident HDOS
    defc    S_OMAX = 0x20D4         ; the size of its overlay area, under that
    defc    USER = 0x2280           ; where programs load, the stack under us
    defc    SETTOP = 0x2A
    defc    SCOUT = 0x02

    SECTION code_crt_init

    ld      hl,(S_SYSM)
    ld      de,(S_OMAX)
    and     a
    sbc     hl,de
    ld      de,__BSS_END_tail + H89_CELLS
    and     a
    sbc     hl,de
    jr      c,tpa_short
    ld      hl,__BSS_END_tail + H89_CELLS - 1
    rst     0x38
    defb    SETTOP
    jr      nc,tpa_fits

tpa_short:
    ld      hl,tpa_msg1
    call    tpa_puts
    ld      hl,__BSS_END_tail + H89_CELLS
    call    tpa_hex
    ld      hl,tpa_msg2
    call    tpa_puts
    ld      hl,(S_SYSM)
    ld      de,(S_OMAX)
    and     a
    sbc     hl,de
    call    tpa_hex
    ld      hl,tpa_msg3
    call    tpa_puts
    jp      __restore_sp_onexit

tpa_fits:
    ld      hl,0
    add     hl,sp
    ld      de,USER + 16
    and     a
    sbc     hl,de
    ld      b,h
    ld      c,l
    ld      hl,USER
    ld      de,USER + 1
    ld      (hl),0xE5
    ldir

    xor     a
    ld      hl,__BSS_head
    ld      de,__BSS_head + 1
    ld      bc,__BSS_END_tail - __BSS_head - 1
    ld      (hl),a
    ldir

    SECTION code_user

tpa_puts:
    ld      a,(hl)
    or      a
    ret     z
    push    hl
    rst     0x38
    defb    SCOUT
    pop     hl
    inc     hl
    jr      tpa_puts

; HL as four hex digits.
tpa_hex:
    ld      a,h
    call    tpa_hex2
    ld      a,l
tpa_hex2:
    push    af
    rrca
    rrca
    rrca
    rrca
    call    tpa_hex1
    pop     af
tpa_hex1:
    and     15
    add     a,0x90
    daa
    adc     a,0x40
    daa
    push    hl
    rst     0x38
    defb    SCOUT
    pop     hl
    ret

    SECTION rodata_user

; One line: HDOS would wrap a longer one.
tpa_msg1:
    defm    "MULE needs memory to ", 0
tpa_msg2:
    defm    "H, HDOS has it to ", 0
tpa_msg3:
    defm    "H: load fewer drivers.", 10, 0

ENDIF
