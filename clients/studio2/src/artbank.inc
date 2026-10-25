; art.inc -- bank 2: the pictures, and their upload.

        FJMP    gfx_load                ; E_GFXLOAD

; gfx_load: every picture (build/artdata.inc) into the cart's image store.
        RFIT    44
gfx_load:
        LDR     R8, IMGTAB
gl1:    LDN     R8
        XRI     0FFH
        BZ      gl9
        PARS
        LDA     R8                      ; id
        PD
        LDA     R8                      ; width
        PD
        LDA     R8                      ; height: the rows that follow
        PLO     R9
        PD
gl2:    LDA     R8
        PD
        DEC     R9
        GLO     R9
        BNZ     gl2
        GOP     TOP_IMAGE
        BR      gl1
gl9:    FRET

IMGTAB_SEGEND:                          ; (data from here: no page rules)
        INCLUDE "artdata.inc"
