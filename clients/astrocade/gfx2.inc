; gfx2.inc -- more resident wrappers.

; SHTX: string kind A, id B, number HL, centred on row D in colour C.
SHTX:   CALL    FARCALL
        DB      PGSTRA
        DW      XSHTX
        RET
