; boot.inc -- bank 0: the pictures, the title, the boot (name, table), the
; menu. Entered by BCALL through the entries (equ.inc E_*).

        FJMP    title                   ; E_TITLE
        FJMP    boot                    ; E_BOOT
        FJMP    rechoose                ; E_RECHOOSE
        FJMP    menu                    ; E_MENU

; ---- the title (mekkogx main: M.U.L.E., TOURNAMENT, the harness's hook) ----
        RFIT    141
title:  FCALL   scr_clear
        LDR     RC, logotab             ; the letters and their dots
ti1:    LDA     RC
        XRI     0FFH
        BZ      ti2
        PARS
        DEC     RC
        LDA     RC
        PD
        LDA     RC
        PD
        LDA     RC
        PD
        P       TM_OR
        GOP     TOP_STAMP
        BR      ti1
ti2:    LDI     W_TOURNAMENT
        FCALL   str_get
        LDI     4
        FCALL   text_center
        LDI     0
        PUTV    PH                      ; PH = frames shown
        SETV    TESTHK, 3CH             ; the harness may poke now
        SETV    TESTHK+1, 0C3H
ti3:    FCALL   frame_wait
        FCALL   parade
        GETV    TESTHK
        XRI     5AH
        BZ      ti4
        GETV    PH
        ADI     1
        STR     RA
        SMI     TITLEF
        BNF     ti3
        FJMP    spr_hide
ti4:    SETV    TESTHK, 0A5H            ; test mode: tell the harness
        SETV    TESTM, 1
        FJMP    spr_hide
TITLEF  EQU     150                     ; frames the title shows
logotab:
        DB      IM_LOGO_M, 10, 20, IM_LOGO_DOT, 18, 35
        DB      IM_LOGO_U, 21, 20, IM_LOGO_DOT, 29, 35
        DB      IM_LOGO_L, 32, 20, IM_LOGO_DOT, 40, 35
        DB      IM_LOGO_E, 43, 20, IM_LOGO_DOT, 51, 35
        DB      0FFH

; parade: the eight species walk across under the title, a step every
; other frame (PH counts the frames).
        RFIT    71
parade: LDI     0
        PLO     RC
pa1:    PARS
        GLO     RC                      ; sprite n = species n
        PD
        GETV    PH                      ; the frame: 2 steps, every 8 frames
        ANI     8
        BZ      pa2
        LDI     IM_WALKB_MECHTRON-IM_WALKA_MECHTRON
pa2:    ADI     IM_WALKA_MECHTRON
        STR     R2
        GLO     RC
        ADD
        PD
        GLO     RC                      ; x = frames / 2 - 9n, round the screen
        SHL
        SHL
        SHL
        STR     R2
        GLO     RC
        ADD
        STR     R2
        GETV    PH
        SHR
        SM
        ANI     7FH
        SMI     8
        PD
        P       80
        P       TM_OR
        GOP     TOP_SPRITE
        INC     RC
        GLO     RC
        XRI     8
        BNZ     pa1
        FRET

; ---- the boot (mekkogx boot.c): who we are, which table ---------------------
; The FujiNet lobby hands a game its details in appkeys (creator 1, app 1):
; key 0 the player's name, key 10 the table's URL "http://host/?table=id".
; Without them we ask for a name and list the build's server's tables; the
; table chosen goes back to key 10, so restarting rejoins it.
AK_NAME EQU     0
AK_ROOM EQU     10
AKREAD  EQU     0
AKWRITE EQU     1

        RFIT    76
boot:   SETV    KREPON, 1
        FCALL   ak_load
        SLD     CR_NAME
        BNZ     bo1
        FCALL   name_entry
bo1:    SLD     CR_TABLE
        BNZ     bo2
        FCALL   table_select
bo2:    SETV    KREPON, 0
        FJMP    ak_save_room

; rechoose: the table refused us (main.c choose_table): forget the room,
; choose another, keep it.
rechoose:
        SETV    KREPON, 1
        LDR     R9, CR_TABLE
        LDI     0
        FCALL   cr_poke
        LDI     AK_ROOM
        FCALL   ak_clear
        FCALL   table_select
        SETV    KREPON, 0
        FJMP    ak_save_room

; ---- appkeys (Battleship's akopen: the payload is all six bytes) --------------
; ak_open: OPEN_APPKEY key R9.0, mode D. D = 0 if it worked.
        RFIT    51
ak_open:
        PLO     RC
        GLO     R9
        PLO     RB
        LDI     FNDEVF
        PHI     R9
        LDI     FCOKEY
        PLO     R9
        LDI     0
        FCALL   fn_beg
        LDI     1                       ; creator 1 (LE)
        FCALL   fn_txb
        LDI     0
        FCALL   fn_txb
        LDI     1                       ; app 1
        FCALL   fn_txb
        GLO     RB                      ; the key
        FCALL   fn_txb
        GLO     RC                      ; the mode
        FCALL   fn_txb
        LDI     0
        FCALL   fn_txb
; ak_go: run it; D = 0 if it worked (and the server said yes).
ak_go:  FCALL   fn_go
        BNZ     akg9
        FJMP    fn_ack
akg9:   FRET

; ak_read: READ_APPKEY key D: the reply window holds the u16 length, then the
; value. D = the length (0: none, or no appkeys; 64 at most).
        RFIT    47
ak_read:
        PLO     R9
        LDI     AKREAD
        FCALL   ak_open
        BNZ     akr0
        LDI     FNDEVF
        PHI     R9
        LDI     FCRKEY
        PLO     R9
        LDI     0
        FCALL   fn_beg
        FCALL   ak_go
        BNZ     akr0
        SPTR    FN_REPLY+1              ; the length, at most 64
        LDN     R8
        BNZ     akr1
        DEC     R8
        LDN     R8
        SMI     65
        BDF     akr1
        LDN     R8
        FRET
akr1:   LDI     64
        FRET
akr0:   LDI     0
        FRET

; ak_write: WRITE_APPKEY key R9.0 = TXT (TXTLEN bytes).
        RFIT    53
ak_write:
        LDI     AKWRITE
        FCALL   ak_open
        BNZ     akw9
        LDI     FNDEVF
        PHI     R9
        LDI     FCWKEY
        PLO     R9
        LDI     0
        FCALL   fn_beg
        LDR     R8, TXT
        GETV    TXTLEN
        PLO     RC
akw1:   GLO     RC
        BZ      akw2
        DEC     RC
        LDA     R8
        FCALL   fn_txb
        BR      akw1
akw2:   FJMP    ak_go
akw9:   FRET

; ak_clear: key D written empty (zeros read back as no table).
ak_clear:
        PLO     R9
        SETV    TXTLEN, 0
        FJMP    ak_write

; ak_load: the name (A-Z, 0-9, at most 8, lower case raised) and the room
; (mekkogx appkey_load), into the cart's RAM. Nothing usable: empty.
        RFIT    103
ak_load:
        LDR     R9, CR_NAME
        LDI     0
        FCALL   cr_poke
        LDR     R9, CR_TABLE
        LDI     0
        FCALL   cr_poke
        LDR     R9, CR_ENDPT
        LDI     0
        FCALL   cr_poke
        LDI     AK_NAME
        FCALL   ak_read
        BZ      akl5
        SMI     9
        BNF     akl1
        LDI     8
        BR      akl2
akl1:   ADI     9
akl2:   PLO     RC                      ; RC.0 = characters to take
        SPTR    FN_REPLY+2
        LDR     R9, TXT
akl3:   GLO     RC
        BZ      akl4
        DEC     RC
        LDA     R8
        BZ      akl4
        FCALL   ak_namec
        BZ      akl5                    ; not A-Z, 0-9: no name
        STR     R9
        INC     R9
        BR      akl3
akl4:   LDI     0
        STR     R9
        LDR     R8, TXT
        LDN     R8
        BZ      akl5
        LDR     R9, CR_NAME
        FCALL   cr_puts
akl5:   FJMP    ak_room

; ak_namec: D, raised to upper case, if A-Z or 0-9; else 0. Keeps R8, R9.
        RFIT    36
ak_namec:
        PLO     RB
        SMI     'a'
        BNF     anc1
        SMI     26
        BDF     anc1
        GLO     RB
        SMI     32
        PLO     RB
anc1:   GLO     RB
        SMI     '0'
        BNF     anc9
        SMI     10
        BNF     anc8
        GLO     RB
        SMI     'A'
        BNF     anc9
        SMI     26
        BDF     anc9
anc8:   GLO     RB
        FRET
anc9:   LDI     0
        FRET

; ak_room: key 10, "http://host/?table=id": the endpoint (no trailing '/')
; and the table (letters and digits, at most 8, else none).
        RFIT    143
ak_room:
        LDI     AK_ROOM
        FCALL   ak_read
        SMI     5
        BNF     akm9                    ; too short to be a URL
        SPTR    FN_REPLY+2
        LDN     R8
        XRI     'h'
        BNZ     akm9
        LDR     R9, TXT                 ; the endpoint, to the '?'
akm1:   LDA     R8
        BZ      akm2
        XRI     '?'
        BZ      akm2
        XRI     '?'
        STR     R9
        INC     R9
        GLO     R9
        XRI     64
        BNZ     akm1
akm2:   DEC     R9                      ; no trailing '/'
        LDN     R9
        XRI     '/'
        BZ      akm3
        INC     R9
akm3:   LDI     0
        STR     R9
        DEC     R8                      ; "?table=": the '?' was read
        LDA     R8
        XRI     '?'
        BNZ     akm9
        LDR     R9, u_tablek
akm4:   LDN     R9
        BZ      akm5
        LDA     R8
        STR     R2
        LDA     R9
        XOR
        BNZ     akm8                    ; another key: no table
        BR      akm4
akm5:   LDR     R9, TXT+70H             ; the table, after the endpoint's room
akm6:   LDA     R8
        BZ      akm7
        XRI     '&'
        BZ      akm7
        XRI     '&'
        FCALL   ak_alnum
        BZ      akm8
        STR     R9
        INC     R9
        GLO     R9
        XRI     78H
        BNZ     akm6
akm7:   LDI     0
        STR     R9
        LDR     R8, TXT+70H
        LDR     R9, CR_TABLE
        FCALL   cr_puts
akm8:   LDR     R8, TXT
        LDR     R9, CR_ENDPT
        FJMP    cr_puts
akm9:   FRET
u_tablek: DB    "table=", 0

; ak_alnum: D if a letter or a digit (either case), else 0. Keeps R8, R9.
        RFIT    25
ak_alnum:
        PLO     RB
        SMI     '0'
        BNF     aan9
        SMI     10
        BNF     aan8
        GLO     RB
        ANI     0DFH
        SMI     'A'
        BNF     aan9
        SMI     26
        BDF     aan9
aan8:   GLO     RB
        FRET
aan9:   LDI     0
        FRET

; ak_save_room: key 10 = endpoint "/?table=" table (mekkogx appkey_save_room).
        RFIT    81
ak_save_room:
        LDR     R9, TXT
        SLD     CR_ENDPT
        BZ      asr1
        SPTR    CR_ENDPT
        BR      asr2
asr1:   LDR     R8, UENDPT
asr2:   FCALL   asr_cat
        LDR     R8, u_room
        FCALL   asr_cat
        SPTR    CR_TABLE
        FCALL   asr_cat
        GLO     R9
        SMI     LO(TXT)
        PUTV    TXTLEN
        LDI     AK_ROOM
        PLO     R9
        FJMP    ak_write
asr_cat: LDA    R8
        STR     R9
        BZ      asc9
        INC     R9
        BR      asr_cat
asc9:   FRET
u_room: DB      "/?table=", 0

; ---- the name (Battleship's grid: 2 4 6 8 move, 5 types, 1 rubs out, 3 done) ----
NM_LEN  EQU     PH+0                    ; characters typed
NM_X    EQU     PH+1                    ; the cursor in the grid
NM_Y    EQU     PH+2
NM_BUF  EQU     PH+3                    ; 9: the name being typed

        RFIT    110
name_entry:
        FCALL   scr_clear
        LDI     B_NAME
        FCALL   str_get
        LDI     0
        FCALL   text_center
        AT      80H
        LDR     R8, nm_leg1
        FCALL   d_puts
        AT      90H
        LDR     R8, nm_leg2
        FCALL   d_puts
        SETV    NM_LEN, 0
        SETV    NM_X, 0
        SETV    NM_Y, 0
        SETV    NM_BUF, 0
nm1:    FCALL   nm_draw
nm2:    FCALL   frame_wait
        FCALL   snd_tick
        FCALL   keys
        GETV    KEDGE
        XRI     K_NONE
        BZ      nm2
        SND     SND_SELECT
        GETV    KEDGE
        FCALL   nm_key
        BZ      nm1
        FRET                            ; done

; nm_key: key D. D = 0 to go on, else the name is done (and kept).
        RFIT    146
nm_key: XRI     2
        BNZ     nk1
        GETV    NM_Y                    ; up, round from the top
        BNZ     nk0
        LDI     3
nk0:    SMI     1
        BR      nky
nk1:    XRI     2!8
        BNZ     nk2
        GETV    NM_Y                    ; down
        ADI     1
        XRI     3
        BZ      nky
        XRI     3
nky:    PUTV    NM_Y
        LDI     0
        FRET
nk2:    XRI     8!4
        BNZ     nk3
        GETV    NM_X
        SMI     1
        BR      nkx
nk3:    XRI     4!6
        BNZ     nk4
        GETV    NM_X
        ADI     1
nkx:    ANI     0FH
        PUTV    NM_X
        LDI     0
        FRET
nk4:    XRI     6!1
        BNZ     nk5
        GETV    NM_LEN                  ; 1: rub out
        BZ      nk9
        SMI     1
        STR     RA
        ADI     LO(NM_BUF)
        PLO     RA
        LDI     0
        STR     RA
        BR      nk9
nk5:    XRI     1!3
        BNZ     nk6
        GETV    NM_LEN                  ; 3: done, if there is a name
        BZ      nk9
        FJMP    nm_save
nk6:    XRI     3!5
        BNZ     nk9
        FCALL   nm_char                 ; 5: type it
        XRI     ' '
        BZ      nk9
        GETV    NM_LEN
        XRI     8
        BZ      nk9
        GETV    NM_LEN
        ADI     1
        STR     RA
        ADI     LO(NM_BUF)-1
        PLO     RA
        LDN     R8
        STR     RA
        INC     RA
        LDI     0
        STR     RA
nk9:    LDI     0
        FRET

; nm_char: R8 on the grid's character under the cursor; D = it.
        RFIT    30
nm_char:
        GETV    NM_Y
        SHL
        SHL
        SHL
        SHL
        STR     R2
        GETV    NM_Y
        ADD
        STR     R2                      ; 17 a row
        GETV    NM_X
        ADD
        ADI     LO(nm_grid)
        PLO     R8
        LDI     HI(nm_grid)
        ADCI    0
        PHI     R8
        LDN     R8
        FRET

; nm_draw: the name, the grid, the cursor's cell inverse.
        RFIT    160
nm_draw:
        AT      34H
        LDR     R8, NM_BUF
        FCALL   d_puts
        GETV    NM_LEN
        XRI     8
        BZ      nmd1
        LDI     '_'
        HOT     TX_PUTC
nmd1:   OPN     TOP_CLREOL
        AT      50H
        LDR     R8, nm_grid
        FCALL   d_puts
        AT      60H
        LDR     R8, nm_grid+17
        FCALL   d_puts
        AT      70H
        LDR     R8, nm_grid+34
        FCALL   d_puts
        GETV    NM_Y                    ; the cursor: row 5 + y, column x
        ADI     5
        SHL
        SHL
        SHL
        SHL
        STR     R2
        GETV    NM_X
        OR
        HOT     TX_CURSOR
        OPN     TOP_INV_ON
        FCALL   nm_char
        HOT     TX_PUTC
        OPN     TOP_INV_OFF
        FRET

; nm_save: the name kept (the cart's RAM, and key 0 for every game).
        RFIT    127
nm_save:
        LDR     R8, NM_BUF
        LDR     R9, CR_NAME
        FCALL   cr_puts
        LDR     R8, NM_BUF              ; key 0: the name
        LDR     R9, TXT
nms1:   LDA     R8
        STR     R9
        INC     R9
        BNZ     nms1
        GETV    NM_LEN
        PUTV    TXTLEN
        LDI     AK_NAME
        PLO     R9
        FCALL   ak_write
        LDI     1
        FRET

nm_grid: DB     "ABCDEFGHIJKLMNOP", 0
        DB      "QRSTUVWXYZ012345", 0
        DB      "6789            ", 0
nm_leg1: DB     "2468 MOVE 5 TYPE", 0
nm_leg2: DB     "1 RUB OUT 3 DONE", 0

; ---- the table (mekkogx table_select) ---------------------------------------
TB_N    EQU     PH+0                    ; tables listed (at most 7)
TB_SEL  EQU     PH+1

        RFIT    149
table_select:
        FCALL   scr_clear
        LDI     B_TABLES
        FCALL   str_get
        LDI     0
        FCALL   text_center
        LDI     B_JOINING
        FCALL   str_get
        LDI     9
        FCALL   text_center
        LDI     ACT_TABLES              ; the list, through the network
        PUTV    ACT
        FCALL   net_sync
        XRI     NH_TABLES
        BNZ     tbs0
        SPTR    FN_REPLY                ; the count
        LDN     R8
        BZ      tbs0
        SMI     8
        BNF     tbs1
        LDI     7
        BR      tbs2
tbs1:   ADI     8
tbs2:   PUTV    TB_N
        FCALL   tb_draw
        SETV    TB_SEL, 0
tbs3:   FCALL   tb_mark
        FCALL   frame_wait
        FCALL   snd_tick
        FCALL   keys
        GETV    KEDGE
        XRI     2
        BZ      tbs4
        XRI     2!8
        BZ      tbs5
        XRI     8!5
        BNZ     tbs3
        FJMP    tb_take
tbs4:   GETV    TB_SEL                  ; up
        BZ      tbs3
        SMI     1
        BR      tbs6
tbs5:   GETV    TB_N                    ; down
        STR     R2
        GETV    TB_SEL
        ADI     1
        SD
        BZ      tbs3
        GETV    TB_SEL
        ADI     1
tbs6:   PUTV    TB_SEL
        SND     SND_SELECT
        BR      tbs3
tbs0:   FJMP    tb_none

; tb_none: no tables (or no answer): 5 tries again.
        RFIT    30
tb_none:
        LDI     B_NOTABLES
        FCALL   str_get
        LDI     9
        FCALL   text_center
tbn1:   FCALL   frame_wait
        FCALL   snd_tick
        FCALL   keys
        GETV    KEDGE
        XRI     K_FIRE
        BNZ     tbn1
        FJMP    table_select

; tb_draw: the tables, from the reply window (the cart prints their names):
; rows 2-8, the name (9) from column 1, the players ("n / 4") at 11; the
; keys on row 9.
        RFIT    149
tb_draw:
        LDI     0
        PLO     RC
        LDR     R9, 1+TBL_NAME          ; reply offset of the name
tbd1:   GLO     RC
        ADI     2
        SHL
        SHL
        SHL
        SHL
        ORI     1
        HOT     TX_CURSOR
        PARS
        GLO     R9
        PD
        GHI     R9
        PD
        P       9
        GOP     TOP_PRINT_PAD
        GLO     RC
        ADI     2
        SHL
        SHL
        SHL
        SHL
        ORI     11
        HOT     TX_CURSOR
        PARS
        GLO     R9
        ADI     TBL_PLAYERS-TBL_NAME
        PD
        GHI     R9
        ADCI    0
        PD
        P       5
        GOP     TOP_PRINT
        GLO     R9                      ; the next entry, 33 on
        ADI     TBL_SIZE
        PLO     R9
        GHI     R9
        ADCI    0
        PHI     R9
        INC     RC
        GLO     RC
        STR     R2
        GETV    TB_N
        XOR
        BNZ     tbd1
        AT      90H
        LDR     R8, tb_keys
        FCALL   d_puts
        OPN     TOP_CLREOL
        FRET
tb_keys: DB     "2 8 MOVE  5 JOIN", 0

; tb_mark: '>' at the selection, blanks at the others.
        RFIT    52
tb_mark:
        LDI     0
        PLO     RC
tbm1:   GLO     RC
        ADI     2
        SHL
        SHL
        SHL
        SHL
        HOT     TX_CURSOR
        GETV    TB_SEL
        STR     R2
        GLO     RC
        XOR
        BZ      tbm2
        LDI     ' '
        BR      tbm3
tbm2:   LDI     '>'
tbm3:   HOT     TX_PUTC
        INC     RC
        GLO     RC
        STR     R2
        GETV    TB_N
        XOR
        BNZ     tbm1
        FRET

; tb_take: the chosen table's id (8, NUL-padded) to the cart's RAM.
        RFIT    64
tb_take:
        SND     SND_SELECT
        GETV    TB_SEL                  ; entry: 1 + 33 * sel
        PLO     RC
        LDR     R8, FN_REPLY+1+TBL_ID
tbt1:   GLO     RC
        BZ      tbt2
        DEC     RC
        GLO     R8
        ADI     TBL_SIZE
        PLO     R8
        GHI     R8
        ADCI    0
        PHI     R8
        BR      tbt1
tbt2:   LDR     R9, TXT                 ; (8 at most, then a NUL)
        LDI     8
        PLO     RC
tbt3:   LDA     R8
        STR     R9
        INC     R9
        DEC     RC
        GLO     RC
        BNZ     tbt3
        LDI     0
        STR     R9
        LDR     R8, TXT
        LDR     R9, CR_TABLE
        FJMP    cr_puts

; ---- the menu (0): 2 leaves the table (asked first when seated in a game),
; 3 the sound, 4 ends our turn; any other key carries on. The game stands
; still meanwhile (mekkogx leave_ask). It is a message: the one shown
; before is gone.
        RFIT    146
menu:   GETV    PHASE
        XRI     PHASE_DEVELOP
        BNZ     mnu1
        SLD     TN_SEAT
        STR     R2
        GETV    MYSEAT
        XOR
        BNZ     mnu1
        LDI     B_MENU2                 ; our turn: 4 too
        BR      mnu2
mnu1:    LDI     B_MENU
mnu2:    FCALL   msg_show
        FCALL   mn_key
        XRI     2
        BZ      mn_lv
        XRI     2!3
        BZ      mn_snd
        XRI     3!4
        BNZ     mnu9
        GETV    PHASE                   ; 4: our turn ends
        XRI     PHASE_DEVELOP
        BNZ     mnu9
        LDI     0
        PLO     R9
        PHI     R9
        LDI     ACT_END
        FCALL   act_send
mnu9:    FJMP    msg_clear
mn_snd: GETV    MUTE
        XRI     1
        STR     RA
        BNZ     mnu9
        LDI     SND_SELECT
        FCALL   snd_play
        BR      mnu9
mn_lv:  GETV    MYSEAT                  ; watching, or the lobby: no question
        SMI     4
        BDF     mn_go
        GETV    PHASE
        BZ      mn_go
        LDI     M_LEAVEASK
        FCALL   msg_show
        FCALL   mn_key
        XRI     2
        BNZ     mnu9
mn_go:  GETV    TESTM
        BNZ     mnu9
        FCALL   msg_clear
        FJMP    leave_table
; mn_key: D = the next key pressed (the sound and a scrolling message go on).
mn_key: FCALL   frame_wait
        FCALL   snd_tick
        FCALL   msg_tick
        FCALL   keys
        GETV    KEDGE
        XRI     K_NONE
        BZ      mn_key
        GETV    KEDGE
        FRET

; leave_table: the server gives the computer our seat; then another table
; (mekkogx leave_table).
        RFIT    30
leave_table:
        SETV    ANIM, AN_NONE
        FCALL   fx_stop_all
        FCALL   spr_hide
        LDI     0
        PLO     R9
        PHI     R9
        LDI     ACT_LEAVE
        FCALL   act_send
        FCALL   net_sync
        FCALL   rechoose
        FJMP    game_start

