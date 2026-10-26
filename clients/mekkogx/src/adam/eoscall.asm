; eoscall.asm -- the few EOS calls the game makes, straight into its jump
; table. (z88dk's eos.lib is built with sccz80, whose arguments come in the
; other order from zsdcc's.) Arguments are in variables, not on the stack;
; IX and IY, which EOS may change, are kept.

        SECTION code_user

        PUBLIC  _eos_blk, _eos_dst, _eos_dev, _joy_ctl
        PUBLIC  _eos_read_blk, _eos_kbd_start, _eos_kbd_end, _joy_read

        defc    READ_BLOCK   = $FCF3    ; A dev, HL to, BCDE block: Z ok, else A error
        defc    START_RD_KBD = $FCA8
        defc    END_RD_KBD   = $FC4B    ; C finished (then Z: A the character)

; uint8_t eos_read_blk(void): block eos_blk of eos_dev to eos_dst; 0 or an error
_eos_read_blk:
        push    ix
        push    iy
        ld      a,(_eos_dev)
        ld      hl,(_eos_dst)
        ld      bc,0
        ld      de,(_eos_blk)
        call    READ_BLOCK
        ld      l,0
        jr      z,rb_ok
        ld      l,a
        or      a
        jr      nz,rb_ok
        inc     l               ; (an error with A = 0)
rb_ok:
        pop     iy
        pop     ix
        ret

; void eos_kbd_start(void): ask the keyboard for a character
_eos_kbd_start:
        push    ix
        push    iy
        call    START_RD_KBD
        pop     iy
        pop     ix
        ret

; uint8_t eos_kbd_end(void): 0 none yet, 1 failed (start again), else the character
_eos_kbd_end:
        push    ix
        push    iy
        call    END_RD_KBD
        ld      l,0
        jr      nc,kb_out       ; not yet
        ld      l,a
        jr      z,kb_out        ; the character
        ld      l,1
kb_out:
        pop     iy
        pop     ix
        ret

; void joy_read(void): controller 1 straight from its port (not EOS's POLLER,
; which left the stick dead here) into joy_ctl: stick (1 north, 2 east, 4
; south, 8 west), left fire, right fire, keypad (the raw code: 09h is *, 0Ah
; is 0, 0Fh none). OUT $C0 strobes the stick, OUT $80 the keypad; IN $FC
; reads controller 1, active low.
_joy_read:
        out     ($C0),a
        in      a,($FC)
        ld      l,a
        cpl
        and     $0F
        ld      (_joy_ctl),a
        ld      a,l
        cpl
        and     $40
        ld      (_joy_ctl+1),a  ; left fire
        out     ($80),a
        in      a,($FC)
        ld      l,a
        cpl
        and     $40
        ld      (_joy_ctl+2),a  ; right fire
        ld      a,l
        and     $0F
        ld      (_joy_ctl+3),a
        ret

        SECTION bss_user
_eos_blk:       defs    2
_eos_dst:       defs    2
_eos_dev:       defs    1
_joy_ctl:       defs    4
