; hgrpage.s -- HGR page 1 ($2000-$3FFF), kept clear of code and data by the
; linker config (the file carries zeros here, so loading blanks the screen).
        .segment "HGRPAGE"
        .res    $2000
