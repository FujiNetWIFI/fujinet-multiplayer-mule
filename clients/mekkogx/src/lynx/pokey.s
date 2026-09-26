;
; pokey.s -- the Atari's sound driver (src/atari/pokey.s), unchanged: it
; plays the original's tunes and effects into a POKEY shadow, which
; mikey.s turns into Mikey's four channels. Run by the VBL (irq.s).
;
        .include "../atari/pokey.s"
