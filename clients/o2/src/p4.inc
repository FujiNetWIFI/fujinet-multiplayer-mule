; p4.inc -- page 4: the land grant (mekkogx grant.c), on the colony map.
;
; The cursor sweeps the map a plot every step (12 jiffies), skipping the
; store and owned plots; the button claims the plot under it. The server
; judges the claim as of the press. Between polls the cursor steps on its
; own, never past the plot the state says a computer will stop it on (the
; grant cursor model: header byte 7, grantStop).
;
; The reply window is gone by the next request, and the cursor steps every
; frame, so whether a plot is free is read back from the map's own row
; tables: an owned plot's character is in a seat's colour, and no unowned
; one's ever is.

E_STATE equ 0
E_TICK  equ 1
NPLOTS  equ 45

SPR_WHITE equ C_WHITE<<3        ; a sprite's colour, attribute bits 3-5

    PAGE_HEAD
    db e_state&0FFh, e_tick&0FFh
e_state:
    jmp on_state
e_tick:
    jmp tick

; gr_claim: our claim's answer (grant.c, the claim's part of grant_state).
gr_claim:
    mov r0,#MYSEAT
    movx a,@r0
    jb7 gc_x                    ; watching
    add a,#(ST_GRANT+GR_CLAIMED)&0FFh
    mov r2,a
    mov r3,#(ST_GRANT+GR_CLAIMED)>>8
    call st_byte
    mov r5,a                    ; mine
    add a,#256-NPLOTS
    jc gc_10
    mov a,#1                    ; we have a plot
    mov r0,#GR_CLD
    movx @r0,a
    mov a,#0FFh
    mov r0,#GR_WNT
    movx @r0,a
gc_10:
    mov r0,#GR_WNT
    movx a,@r0
    mov r7,a                    ; j: the plot we wait on
    add a,#256-NPLOTS
    jc gc_x
    mov a,r5
    add a,#256-NPLOTS
    jnc gc_x
    mov r0,#ACTSENT             ; only the state answering the claim judges
    movx a,@r0                  ; it: those before were made before the
    xrl a,#ACT_CLAIM            ; server saw it
    jnz gc_12
    mov r0,#HERR
    movx a,@r0
    jz gc_dn
    mov r0,#GR_SRV              ; turned down as the server's cursor wasn't
    movx a,@r0                  ; there yet (k < j): send it again when it
    xrl a,#0FFh                 ; is; otherwise press again
    jz gc_11
    xrl a,#0FFh
    cpl a
    add a,r7                    ; j + ~k carries: k < j
    jnc gc_dn
gc_11:
    mov a,#1
    mov r0,#GR_PRK
    movx @r0,a
gc_x:
    ret
gc_12:
    mov r0,#GR_PRK
    movx a,@r0
    jz gc_x
    mov r0,#ACT
    movx a,@r0
    jnz gc_x
    mov r0,#GR_SRV              ; it's time once k >= j
    movx a,@r0
    mov r6,a
    xrl a,#0FFh
    jz gc_x
    mov a,r6
    cpl a
    add a,r7
    jc gc_x
    clr a
    mov r0,#GR_PRK
    movx @r0,a
    mov a,r6
    add a,#256-NPLOTS
    jc gc_dn
    mov r0,#ACT                 ; send it again
    mov a,#ACT_CLAIM
    movx @r0,a
    inc r0
    mov a,r7
    movx @r0,a
    inc r0
    clr a
    movx @r0,a
    ret
gc_dn:
    mov a,#SND_ERROR
    FCALL beep
    mov a,#0FFh                 ; press again
    mov r0,#GR_WNT
    movx @r0,a
    clr a
    mov r0,#GR_CLD
    movx @r0,a
    mov r0,#GR_PRK
    movx @r0,a
    ret

    include "sayq.inc"

; gr_dep: the title and message: the ship leaving (round 1, the phase's
; first 700 jiffies: mekkogx gr_depart), then the grant. Said when that
; changes. Clobbers A, r0, r5-r7.
gr_dep:
    call dep_now
    mov r5,a
    mov r0,#GR_DEP
    movx a,@r0
    xrl a,r5
    jz gd_x
    mov a,r5
    movx @r0,a
    jz gd_grant
    SAY T_SHIP, L_MAP, 1, C_WHITE
    SAY M_DEPART, L_MAP+1, 2, C_WHITE
    ret
gd_grant:
    mov a,#SND_ALERT
    FCALL beep
    SAY T_GRANT, L_MAP, 1, C_WHITE
    SAY M_SELECT, L_MAP+1, 2, C_WHITE
gd_x:
    ret

; dep_now: A = 1 while the ship leaves, else 0. Clobbers r0.
dep_now:
    mov r0,#RND
    movx a,@r0
    xrl a,#1
    jnz dn_no
    mov r0,#HPE+1               ; elapsed < 700 ($2BC)
    movx a,@r0
    add a,#256-2
    jnc dn_yes
    jnz dn_no
    mov r0,#HPE
    movx a,@r0
    add a,#256-0BCh
    jc dn_no
dn_yes:
    mov a,#1
    ret
dn_no:
    clr a
    ret

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "stbyte.inc"

; is_free: A = 0 if plot A is owned or the store, else nonzero. Clobbers
; A, r0, r6, r7.
is_free:
    mov r7,a
    xrl a,#STORE_PLOT
    jz if_no
    mov a,r7                    ; row = plot / 9, cell = plot % 9
    mov r6,#0
if_div:
    add a,#256-9
    jnc if_rem
    inc r6
    jmp if_div
if_rem:
    add a,#9                    ; the cell
    add a,#cellpos&0FFh
    movp a,@a                   ; its place in the row's table
    rl a                        ; 2 bytes a place, after Y: the control byte
    add a,#2
    mov r7,a
    mov a,r6                    ; + ROWS + 19 * row
    swap a
    add a,r6
    add a,r6
    add a,r6
    add a,r7
    add a,#ROWS&0FFh
    FCALL fx6_get
    rr a                        ; the colour
    anl a,#7
    add a,#ownedcol&0FFh
    movp a,@a
    ret
if_no:
    clr a
    ret

; next_free: A = the first free plot after plot A, or NPLOTS. Clobbers A,
; r0, r5-r7.
next_free:
    mov r5,a
nf_l:
    inc r5
    mov a,r5
    xrl a,#NPLOTS
    jz nf_x
    mov a,r5
    call is_free
    jz nf_l
nf_x:
    mov a,r5
    ret

    include "cursor.inc"

cellpos: db 0,1,2,3,8,4,5,6,7  ; a cell's place in its row's table
ownedcol:                       ; by colour: nonzero = no one owns it
    db 1, 0, 0, 0, 1, 0, 1, 1   ; grey, red, green, tan, blue, violet, cyan, white

; ===========================================================================
MAPBASE equ 0B00h
    include "map.inc"

; ===========================================================================
    org 0D00h

; on_state: a state has come. A new phase gets the map; then the plots and
; the grant.
on_state:
    mov r0,#PHS
    movx a,@r0
    mov r6,a
    mov r0,#LASTPH
    movx a,@r0
    xrl a,r6
    jz os_same
    mov a,r6
    movx @r0,a
    call map_screen
    call grant_enter
os_same:
    clr a                       ; the M.U.L.E.s' letters
    mov r0,#MAPDIG
    movx @r0,a
    call map_build
    jmp grant_state

; grant_enter: the cursor not yet started.
grant_enter:
    clr a
    mov r0,#G_SHOW
    movx @r0,a
    mov r0,#G_NEXT
    movx @r0,a
    mov r0,#GR_CLD
    movx @r0,a
    mov r0,#GR_PRK
    movx @r0,a
    mov a,#0FFh
    mov r0,#GR_DEP              ; nothing said yet
    movx @r0,a
    mov r0,#G_CUR
    movx @r0,a
    mov r0,#GR_PRS
    movx @r0,a
    mov r0,#GR_WNT
    movx @r0,a
    mov r0,#GR_WAS
    movx @r0,a
    mov r0,#GR_LIM
    movx @r0,a
    ret

; grant_state: the server's cursor, and our claim (grant.c grant_state).
grant_state:
    call gr_dep
    STB ST_GRANT+GR_CURSOR
    mov r0,#GR_SRV
    movx @r0,a
    STB ST_GRANT+GR_STEP
    jnz gs_step
    mov a,#12
gs_step:
    mov r0,#G_STEP
    movx @r0,a
    call gr_claim               ; our claim's answer first
    STB ST_GRANT+GR_NEXT+1      ; w: the countdown, far off: wait for a nearer
    mov r4,#0                   ; state
    jnz gs_w
    STB ST_GRANT+GR_NEXT
    mov r4,a
    mov r0,#RXAGE               ; less the state's age: the step it waited
    movx a,@r0                  ; for may have come since
    cpl a
    add a,r4                    ; w + ~age carries: w > age
    jc gs_sub
    mov r4,#1
    jmp gs_w
gs_sub:
    inc a                       ; (~age + w + 1 = w - age)
    mov r4,a
gs_w:                           ; r4 = w
    mov r0,#GR_WNT              ; our claim is out: the cursor waits on it
    movx a,@r0
    add a,#256-NPLOTS
    jnc gs_lim
    mov r0,#GR_SRV
    movx a,@r0
    mov r6,a                    ; k
    mov r0,#G_CUR
    movx a,@r0
    mov r5,a                    ; cur
    xrl a,r6
    jz gs_next
    mov a,r5                    ; we made the step it waits for a little early:
    add a,#256-NPLOTS           ; no jump back (after a claim it waits
    jc gs_jump                  ; longer, and the cursor goes there)
    mov a,r6
    xrl a,#0FFh
    jz gs_early
    mov a,r6                    ; k < cur?
    cpl a
    add a,r5                    ; cur + ~k carries: k < cur
    jnc gs_jump
gs_early:
    mov a,r4
    jz gs_jump
    mov r0,#G_STEP
    movx a,@r0
    mov r7,a
    cpl a
    add a,r4                    ; w + ~step carries: w > step
    jc gs_jump
    mov a,r7
    add a,r4
    mov r4,a                    ; w + step
    jmp gs_next
gs_jump:
    mov a,#0FFh                 ; a claim (ours, or another's the cursor went
    mov r0,#GR_WAS              ; back for)
    movx @r0,a
    mov a,r6
    mov r0,#G_CUR
    movx @r0,a
gs_next:
    mov a,r4
    mov r0,#G_NEXT
    movx @r0,a
gs_lim:
    STB HDR_GRANTSTOP           ; no further than where a computer stops it
                                ; (an older server doesn't say: one plot past
                                ; its cursor)
    jz gs_old
    dec a
    jmp gs_set
gs_old:
    mov r0,#GR_SRV
    movx a,@r0
    inc a
gs_set:
    mov r0,#GR_LIM
    movx @r0,a
    mov a,#1
    mov r0,#G_SHOW
    movx @r0,a
    ret

; ===========================================================================
    org 0E00h

; tick: every frame. The grant's cursor and the button; then a row of the
; map, while the state's are not all built.
tick:
    call grant_isr
    call grant_tick
    ; fall into map_more

; map_more: the next row of the map, if any (map_build). Clobbers A, r0-r7.
map_more:
    mov r0,#MB_ROW
    movx a,@r0
    add a,#256-5
    jc mm_x
    call map_row
    mov r1,#DIRTY               ; its mountains' right halves are new
    mov a,@r1
    orl a,#DIRTY_STATIC
    mov @r1,a
mm_x:
    ret

; grant_isr: the cursor steps to the next free plot when its countdown runs
; out, and is drawn (grant.c grant_isr).
grant_isr:
    mov r0,#G_SHOW              ; the press: the plot on screen then
    movx a,@r0
    jz gi_2
    mov r0,#G_CUR
    movx a,@r0
    mov r5,a
    add a,#256-NPLOTS
    jc gi_2
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz gi_2
    mov r0,#GR_PRS
    movx a,@r0
    xrl a,#0FFh
    jnz gi_2
    mov a,r5
    movx @r0,a
    mov r0,#G_NEXT              ; the frame after a step: the plot before
    movx a,@r0
    mov r6,a
    mov r0,#G_STEP
    movx a,@r0
    xrl a,r6
    jnz gi_2
    mov r0,#GR_WAS
    movx a,@r0
    mov r6,a
    add a,#256-NPLOTS
    jc gi_2
    mov a,r6
    mov r0,#GR_PRS
    movx @r0,a
gi_2:
    mov r0,#GR_WNT              ; our claim is out: it waits on its plot
    movx a,@r0
    add a,#256-NPLOTS
    jnc gi_4
    mov r0,#G_NEXT              ; its countdown less the tick's frames;
    movx a,@r0                  ; r2 = what it overran by (<= 0)
    jz gi_4
    cpl a
    mov r5,a
    mov r0,#TICKN
    movx a,@r0
    add a,r5                    ; ~next + n carries: n > next, overrun
    cpl a                       ; next - n
    mov r2,a
    mov r0,#G_NEXT
    jc gi_due
    movx @r0,a
    jnz gi_4
gi_due:
    mov r0,#G_CUR
    movx a,@r0
    xrl a,#NPLOTS
    jz gi_4
    xrl a,#NPLOTS
    call next_free
    mov r4,a                    ; n, the next free plot
    mov r0,#GR_LIM              ; a computer stops it before that
    movx a,@r0
    cpl a
    add a,r4                    ; n + ~lim carries: n > lim
    jnc gi_3
    mov a,#1
    mov r0,#G_NEXT
    movx @r0,a
    jmp gi_4
gi_3:
    mov r0,#G_CUR               ; a step, hidden for the frame it is taken
    movx a,@r0
    mov r0,#GR_WAS
    movx @r0,a
    mov a,r4
    mov r0,#G_CUR
    movx @r0,a
    mov r0,#G_STEP              ; the next step: a step on, less the overrun
    movx a,@r0                  ; (1 at least)
    add a,r2
    jz gi_1
    jb7 gi_1
    jmp gi_n
gi_1:
    mov a,#1
gi_n:
    mov r0,#G_NEXT
    movx @r0,a
    jmp cur_hide
gi_4:
    mov r0,#G_CUR
    movx a,@r0
    add a,#256-NPLOTS
    jc gi_h
    mov r0,#G_SHOW
    movx a,@r0
    jz gi_h
    jmp cur_show
gi_h:
    jmp cur_hide

; grant_tick: the button claims the plot it was pressed on.
grant_tick:
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jz gt_x
    mov r0,#GR_PRS              ; the plot the cursor was on at the press
    movx a,@r0
    mov r4,a
    add a,#256-NPLOTS
    jnc gt_2
    mov r0,#G_CUR
    movx a,@r0
    mov r4,a
gt_2:
    mov a,#0FFh
    mov r0,#GR_PRS
    movx @r0,a
    mov r0,#GR_CLD
    movx a,@r0
    jnz gt_x
    mov r0,#MYSEAT
    movx a,@r0
    jb7 gt_x
    mov a,r4
    add a,#256-NPLOTS
    jc gt_x
    mov r0,#ACT                 ; claim it
    mov a,#ACT_CLAIM
    movx @r0,a
    inc r0
    mov a,r4
    movx @r0,a
    inc r0
    clr a
    movx @r0,a
    mov a,#1                    ; hold the cursor there until the server
    mov r0,#GR_CLD              ; answers
    movx @r0,a
    mov a,#SND_SELECT
    FCALL beep

    mov a,r4
    mov r0,#GR_WNT
    movx @r0,a
    mov r0,#G_CUR
    movx @r0,a
    mov a,#0FFh
    mov r0,#GR_WAS
    movx @r0,a
gt_x:
    ret

    PAGE_TAIL
