; core.inc -- the start, the frame loop (mekkogx main.c), and what every
; phase leans on: the frame clock, the keys, the phases' dispatch.

; ---- start --------------------------------------------------------------------
        RFIT    35
main:   LDI     HI(RAMVARS)
        PHI     RA
        LDR     R8, V_APP               ; our RAM: zeros
mn1:    LDI     0
        STR     R8
        INC     R8
        GHI     R8
        XRI     HI(TXT)
        BNZ     mn1
mn2:    LDI     0
        STR     R8
        INC     R8
        GLO     R8
        XRI     LO(STKLOW)
        BNZ     mn2
        LDI     0
        FCALL   bksel
        FJMP    start

        RFIT    121
start:  PEEK    FN_MAGIC                ; a FujiNet cart, protocol 2?
        XRI     'F'
        BNZ     st_nocart
        PEEK    FN_MAGIC+1
        XRI     'N'
        BNZ     st_nocart
        PEEK    FN_PROTOVER
        SMI     2
        BNF     st_old
        SETV    LASTPH, 0FFH
        SETV    LASTRD, 0FFH
        SETV    KLAST, K_NONE
        SETV    MSGROW, 8
        BCALL   BK_ART, E_GFXLOAD      ; every picture into the cart
        BCALL   BK_BOOT, E_TITLE        ; (the harness may ask for test mode)
        GETV    TESTM
        BZ      st1
        BCALL   BK_TEST, E_TSTART
        BR      st2
st1:    BCALL   BK_BOOT, E_BOOT
st2:    FCALL   game_start
        FJMP    mainlp
st_nocart:
        LDI     B_NOCART
        BR      st3
st_old: LDI     B_OLDCART
st3:    FCALL   say_center4             ; on row 4, centred
st4:    BR      st4

; game_start: a new game, or a new table (mekkogx game_start).
        RFIT    44
game_start:
        SETV    LASTPH, 0FFH
        SETV    LASTRD, 0FFH
        SETV    SCRNOW, SCR_NONE
        SETV    STOK, 0
        SETV    POLLW, 0
        SETV    ACT, ACT_NONE
        FCALL   scr_clear
        LDI     B_JOINING
        FJMP    say_center4             ; (tail)

; Registers: R8, R9, RB, RC are anyone's across a call (the runtime's
; routines leave RB and RC alone); RA.1 stays $08 (RA.0 is anyone's). A
; loop that calls keeps its count in RAM.

; ---- the frame loop (mekkogx main.c main) ----------------------------------------
        RFIT    109
mainlp: FCALL   frame_wait
        FCALL   snd_tick
        FCALL   keys
        GETV    STOK                    ; watching: only the menu key counts
        BZ      ml1
        GETV    MYSEAT
        SMI     4
        BNF     ml1
        GETV    KEDGE
        BZ      ml1                     ; (0 is the menu key)
        SETV    KEDGE, K_NONE
        SETV    KEY, K_NONE
ml1:
        IF      AUTOPLAY
        GETV    STOK
        BZ      ml1a
        BCALL   BK_AP, E_AUTO
ml1a:
        ENDIF
        FCALL   catchup                 ; what moves by itself
        FCALL   net_tick                ; one step of the network
        GETV    STOK
        BZ      mainlp
        GETV    KEDGE                   ; the menu (0): leave, sound, end turn
        BNZ     ml2
        BCALL   BK_BOOT, E_MENU
        FCALL   done_now
        BR      mainlp
ml2:    GETV    PHASE
        FCALL   ph_tick                 ; the phase's frame
        FCALL   map_service             ; the map, a few plots a frame
        GETV    MYSEAT                  ; watching: the tag, every 32 frames
        SMI     4
        BNF     mainlp
        GETV    FCL
        ANI     0E0H
        STR     R2
        GETV    SPECAT
        XOR
        BZ      mainlp
        LDN     R2
        STR     RA
        FCALL   spec_tag
        BR      mainlp

; catchup: once for every frame since the last loop (mekkogx main.c: more
; than two behind, half now and the rest after; more than 8, let go).
        RFIT    130
catchup:
        GETV    DONEL                   ; n = FC - DONE (8 bits is plenty)
        STR     R2
        GETV    FCL
        SM
        PLO     RC
        GETV    DONEH
        STR     R2
        GETV    FCH
        SMB
        BNZ     cu0                     ; far behind: let go
        GLO     RC
        SMI     9
        BNF     cu1
cu0:    GETV    FCL                     ; DONE = FC - 8, n = 8
        SMI     8
        PUTV    DONEL
        GETV    FCH
        SMBI    0
        PUTV    DONEH
        LDI     8
        PLO     RC
cu1:    GLO     RC
        SMI     3
        BNF     cu2
        GLO     RC                      ; n > 2: (n + 1) / 2
        ADI     1
        SHR
        PLO     RC
cu2:    GLO     RC                      ; DONE += n
        STR     R2
        GETV    DONEL
        ADD
        STR     RA
        GETV    DONEH
        ADCI    0
        STR     RA
        GLO     RC
        PUTV    CUN
cu3:    GETV    CUN
        BZ      cu4
        SMI     1
        STR     RA
        GETV    ANIM
        FCALL   an_tick
        FCALL   fx_run
        BR      cu3
cu4:    FRET

; done_now: the self-moving things are up to date (after a stall we caused).
done_now:
        GETV    FCL
        PUTV    DONEL
        GETV    FCH
        PUTV    DONEH
        FRET

; ---- the frame clock -------------------------------------------------------------
; frame_wait: until RE moves (the ISR counts frames), then FC = RE + FADJ.
        RFIT    53
frame_wait:
        VAR     FWLAST
fw1:    GLO     RE
        SEX     RA
        XOR
        SEX     R2
        BZ      fw1
fclock: GHI     RE                      ; RE.1, RE.0, RE.1 again: no tear
        PHI     R9
        GLO     RE
        PLO     R9
        GHI     RE
        STR     R2
        GHI     R9
        XOR
        BNZ     fclock
        GLO     R9
        PUTV    FWLAST
        GETV    FADJL
        STR     R2
        GLO     R9
        ADD
        PUTV    FCL
        GETV    FADJH
        STR     R2
        GHI     R9
        ADC
        PUTV    FCH
        FRET

; frame_lost: D frames were lost (the server's clock says): our clock on.
        RFIT    17
frame_lost:
        STR     R2
        GETV    FADJL
        ADD
        STR     RA
        GETV    FADJH
        ADCI    0
        STR     RA
        FJMP    fclock

; ---- the keys --------------------------------------------------------------------
; keys: KEY = the key held on either pad (0-9) or K_NONE; KEDGE = a key that
; went down this frame, or (KREPON) one held long enough to repeat.
        RFIT    104
keys:   FCALL   in_scan
        BZ      ky0
        ANI     0FH
        BR      ky1
ky0:    LDI     K_NONE
ky1:    PUTV    KEY
        PLO     R9
        SETV    KEDGE, K_NONE
        GETV    KLAST
        STR     R2
        GLO     R9
        STR     RA                      ; KLAST = KEY
        XOR
        BZ      ky3                     ; held (or still none)
        GLO     R9
        XRI     K_NONE
        BZ      ky9                     ; let go
        GLO     R9                      ; down this frame
        PUTV    KEDGE
        GETV    FCL
        ADI     KEY_DELAY
        PUTV    KREPT
        FRET
ky3:    GETV    KREPON
        BZ      ky9
        GLO     R9
        XRI     K_NONE
        BZ      ky9
        GETV    KREPT                   ; time to repeat?
        STR     R2
        GETV    FCL
        SM
        ANI     80H
        BNZ     ky9
        GETV    FCL
        ADI     KEY_RATE
        PUTV    KREPT
        GLO     R9
        PUTV    KEDGE
ky9:    FRET

; ---- the phases --------------------------------------------------------------------
; ph_enter / ph_state / ph_tick: the phase D's (mekkogx enters, states,
; ticks); an_tick: the self-moving thing D's frame (AN_*). The routine takes
; over the call: it returns to our caller.
        RFIT    118
ph_enter:
        PLO     R9
        LDR     R8, tab_enter
        BR      phx
ph_state:
        PLO     R9
        LDR     R8, tab_state
        BR      phx
ph_tick:
        PLO     R9
        LDR     R8, tab_tick
        BR      phx
an_tick:
        PLO     R9
        LDR     R8, tab_anim
phx:    GLO     R9
        SHL
        STR     R2
        GLO     R8
        ADD
        PLO     R8
        GHI     R8
        ADCI    0
        PHI     R8
        LDA     R8
        PHI     R9
        LDN     R8
        PLO     R9
        LDR     R8, jmp_r9
        SEP     R8

tab_enter:
        DW      lobby_enter, grant_enter, landauc_enter, develop_enter, prod_enter
        DW      goods_enter, goods_enter, summary_enter, gameover_enter
tab_state:
        DW      lobby_state, grant_state, auction_state, develop_state, no_op
        DW      auction_state, auction_state, summary_state, no_op
tab_tick:
        DW      lobby_tick, grant_tick, auction_tick, develop_tick, prod_tick
        DW      auction_tick, auction_tick, summary_tick, gameover_tick
tab_anim:
        DW      no_op, walk_isr, grant_isr, auction_isr

no_op:  FRET
