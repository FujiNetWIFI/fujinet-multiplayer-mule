; gfx.inc -- the screen and the cart's RAM, as every phase uses them.

; scr_clear: a blank screen: cells, stamps, sprites and the VIEW (CLS).
scr_clear:
        OPN     TOP_CLS
        FRET

; spr_hide: every sprite off.
spr_hide:
        OPN     TOP_SPRHIDE
        FRET

; spec_tag: watching (no seat): WATCH in the top left (mekkogx spec_tag).
spec_tag:
        GETV    SCRNOW
        BZ      spt9
        LDI     B_SPECT
        FCALL   str_get
        AT      00H
        LDR     R8, TXT
        GETV    TXTLEN
        FJMP    putn
spt9:   FRET

; ---- the cart's RAM (pokes) -----------------------------------------------------
; cr_at: the poke pointer to R9, a stream open: cr_put stores D there and
; moves on. A REGSEL anywhere closes it.
        RFIT    58
cr_at:  LDI     FN_REG_PTRLO
        HOT     HI(FN_REGSEL)
        GLO     R9
        HOT     HI(FN_REGDATA)
        LDI     FN_REG_PTRHI
        HOT     HI(FN_REGSEL)
        GHI     R9
        HOT     HI(FN_REGDATA)
        LDI     FN_REG_POKE
        HOT     HI(FN_REGSEL)
        FRET
cr_put: HOT     HI(FN_REGDATA)
        FRET

; cr_puts: the string at R8 (to its NUL, which goes too) to the cart at R9.
        RFIT    24
cr_puts:
        FCALL   cr_at
        LDI     HI(FN_REGDATA)
        PHI     RF
crp1:   LDA     R8
        PLO     RF
        LDN     RF
        GLO     RF
        BNZ     crp1
        LDI     FN_PARK
        PHI     RF
        FRET

; cr_poke: one byte D at R9 in the cart's RAM.
cr_poke:
        PLO     RC
        FCALL   cr_at
        GLO     RC
        FJMP    cr_put
