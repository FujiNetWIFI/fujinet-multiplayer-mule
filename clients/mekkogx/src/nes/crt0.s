;
; crt0.s -- M.U.L.E. on the NES: the iNES header, start-up, the frame's
; interrupts and the PPU queue (src/nes/neshw.h).
;
; The screen is a bitmap of 768 cells, each its own tile, as the TMS9918A's
; Graphics II: a third of the screen (8 rows, 256 tiles) to each of three
; 4K pattern tables in CHR-RAM, rows 2-25 of the name table. A table is
; two of MMC3's 2K banks, R0 (tiles 0-127: a third's first four rows) and
; R1 (128-255: its last four), and each is switched while the rows on the
; screen use only the other: anywhere in a 32-line window, so neither the
; interrupt's lateness nor a line more or less in MMC3's count (the
; hardware and MesenCE count the pre-render line, MAME's cart doesn't)
; can put a wrong tile on the screen. The margins (rows 0-1: tile 128;
; rows 26-29: tile 0) show CHR bank 12's first tile, kept blank (the
; sprites' pair 0, src/nes/gfx.c), while R1 or R0 holds bank 12:
;
;   NMI        R0 the first table's first half, R1 bank 12 (the top
;              margin), everything on
;   line  32   R1 the first table's second half    (after row 1, before 6)
;         64   R0 the second table's first half    (after row 5, before 10)
;         96   R1 its second half                  (after row 9, before 14)
;        128   R0 the third table's first half     (after row 13, before 18)
;        160   R1 its second half                  (after row 17, before 22)
;        192   R0 bank 12, the bottom margin       (after row 21, before 26)
;        210   everything off: from here to the end of the vertical blank
;              the PPU is free, and the queue is drained (then again in
;              the NMI, before the sprites' DMA)
;
; Rendering goes off mid-screen, so it goes off at a dot that leaves OAM
; alone: ~150, in the next line's sprite evaluation (dots 65-256), with no
; sprite on that line (gfx.c keeps them higher up). Its second OAM address
; is then 0; switched off where the interrupt lands (dot ~5-30, as that
; OAM is being cleared) it was left part way, and at the pre-render line
; a 2C02 copies OAM row 0 over the row it points at (MesenCE does too:
; a different pair of the river's sprites gone each frame). The DMA comes
; last before rendering is back on: OAM left alone with the display off
; decays.
;
; The NMI counts the frames, plays the sound effect's next frame and puts
; the display back for the next frame (the banks, the scroll, the counter).
; The count is the server's 60 Hz, not the NES's 60.0988: one frame in
; CLK_EVERY isn't counted. (Counting them all, the server's clock fell
; behind ours a jiffy every ten seconds, and src/main.c's clock_sync took
; that for replies coming ever later: every state looked up to 30 jiffies
; old, and the grant's cursor stepped as states came, not on its own.)
;
; Game code never touches the PPU while it shows a picture: drawing goes
; into the queue (ppuq) and the interrupts drain it, a budget's worth a
; frame. While a whole screen is drawn, ppu_direct is set: the NMI turns
; the display off and leaves the PPU alone, and the game drains the queue
; itself as it fills (src/nes/gfx.c).
;
; The sprites have two pages (oam, oam2): the game builds the one not
; shown and names it in oam_next, and the NMI sends that one from then on,
; never a page half made.
;
; MMC3's bank select register is shared: the game sets R6 for its overlays
; and its art (prg_r6), the interrupts R0/R1 for the pattern tables. The
; game's choice is kept in mmc3_sel (written before the register), and
; whatever the interrupts change they put back, so an interrupt between the
; game's two writes is harmless.
;
        .export         __STARTUP__ : absolute = 1
        .export         _exit
        .export         _ppu_drain, _prg_r6, _snd_frame_nmi
        .export         oam_send        ; (tools/nessim's ppuwatch)
        .exportzp       _qhead, _qtail, _ppu_direct, _mmc3_sel, _ppu_off, _spr_ovf
        .exportzp       _oam_show, _oam_next
        .export         _frame_count, _ppuq, _oam, _oam2
        .import         initlib, donelib, callmain, zerobss, copydata
        .import         _snd_f, _snd_nf, _snd_n
        .include        "zeropage.inc"

PPUCTRL   = $2000
PPUMASK   = $2001
PPUSTATUS = $2002
OAMADDR   = $2003
PPUSCROLL = $2005
PPUADDR   = $2006
PPUDATA   = $2007
OAMDMA    = $4014

CTRL_ON   = $A0                 ; NMI, 8 x 16 sprites, background at $0000
MASK_ALL  = $1E                 ; background and sprites, no clipping

; The first interrupt: the counter is reloaded with the latch at the first
; line it is clocked on (the pre-render line, or MAME's line 0), and goes
; off when it has counted down to 0; each interrupt then sets the next.
IRQ_FIRST = 32
NBANK     = 12                  ; the 1K bank with the blank tile (R0/R1: 12-13)

; Drain budgets, in units of 16 cycles, counted from the code below at
; its slowest (a switch of the third's banks included): a copy of n bytes
; is 15n + 196 cycles, a fill 9n + 230, a paint 1110 and a row of n cells
; 185n + 215. No command may cost more than the IRQ's budget (gfx.c keeps
; them smaller), or it would never go.
Q_COPY    = 1
Q_FILL    = 2
Q_PAINT   = 3
Q_ROW     = 4
Q_BASE    = 13
PAINT_COST = 71
; The IRQ's drain starts by line 212 (rendering off at dot ~150 of 211)
; and the display must be back on by the pre-render line's dot 280 (or it
; isn't scrolled to the top): 49 lines for the drain, the NMI meanwhile,
; the sprites' DMA and show. Past line 241 the NMI leaves those to the IRQ
; (nmi_late). The NMI's own drain has the vertical blank before its DMA:
; 2366 cycles in all, about 680 of them the NMI's own work. (A budget is a
; byte, 255 meaning no limit.)
IRQ_BUDGET = 247
NMI_BUDGET = 90
OFF_DELAY  = 8                  ; the IRQ's wait before rendering goes off
CLK_EVERY  = 608                ; 60.0988 * 607 / 608 = 60.000 Hz

; ---------------------------------------------------------------------------
; The header: NES 2.0, mapper 4, 128K PRG, 16K CHR-RAM, 8K PRG-RAM.
.segment "HEADER"
        .byte   "NES", $1A
        .byte   8               ; PRG: 8 x 16K
        .byte   0               ; CHR-ROM: none (RAM)
        .byte   $41             ; mapper 4 (low), vertical mirroring
        .byte   $08             ; NES 2.0
        .byte   0, 0            ; mapper high, submapper; ROM size high
        .byte   $07             ; PRG-RAM 64 << 7 = 8K
        .byte   $08             ; CHR-RAM 64 << 8 = 16K
        .byte   0, 0, 0, 0

; ---------------------------------------------------------------------------
.segment "ZEROPAGE"
_qhead:      .res 1             ; the queue: where the game writes next
_qtail:      .res 1             ; where the drain reads next
_ppu_direct: .res 1
_mmc3_sel:   .res 1             ; the game's bank select (R6)
budget:      .res 1
cur_third:   .res 1             ; the table R0/R1 hold
irq_step:    .res 1
in_drain:    .res 1             ; the IRQ's drain is going
nmi_late:    .res 1             ; and the NMI came meanwhile
op:          .res 1
tmp:         .res 1
ahi:         .res 1
alo:         .res 1
alo8:        .res 1
cnt:         .res 1
cost:        .res 1
_ppu_off:    .res 1             ; the NMI has turned the display off
_spr_ovf:    .res 1             ; a line had more than 8 sprites
_oam_show:   .res 1             ; the sprites' page the NMI sends
_oam_next:   .res 1             ; the page to send from now on (0: none)
clk_n:       .res 2             ; frames to the next one not counted
pbuf:        .res 16            ; a cell being painted

.segment "BSS"
_frame_count: .res 2            ; (cleared by zerobss)

.segment "PPUQ"
_ppuq:       .res 256

.segment "OAM"
_oam:        .res 256

.segment "OAM2"
_oam2:       .res 256

; ---------------------------------------------------------------------------
.segment "STARTUP"

start:
        sei
        cld
        ldx     #$FF
        txs
        inx
        stx     PPUCTRL         ; NMI off
        stx     PPUMASK
        stx     $4010           ; no DMC interrupt
        lda     #$40
        sta     $4017           ; no APU frame interrupt
        stx     $E000           ; no MMC3 interrupt
        bit     PPUSTATUS
@v1:    bit     PPUSTATUS
        bpl     @v1

        ; MMC3: PRG mode 0 ($8000 R6, $A000 R7, $C000 fixed), CHR mode 0
        ldy     #0
@mm:    sty     $8000
        lda     mmc3_init,y
        sta     $8001
        iny
        cpy     #8
        bne     @mm
        lda     #6
        sta     _mmc3_sel
        sta     $8000
        lda     #$80
        sta     $A001           ; WRAM on, writable
        lda     #0
        sta     $A000           ; vertical mirroring

        ; RAM: all of it zero (the stack's page too: nothing is on it yet)
        lda     #0
        tax
@clr:   sta     $00,x
        sta     $0100,x
        sta     $0200,x
        sta     $0300,x
        sta     $0400,x
        sta     $0500,x
        sta     $0600,x
        sta     $0700,x
        inx
        bne     @clr
        lda     #6
        sta     _mmc3_sel

        ; sprites all off the screen
        lda     #$F8
@ofs:   sta     _oam,x
        sta     _oam2,x
        inx
        bne     @ofs
        lda     #>_oam
        sta     _oam_show

@v2:    bit     PPUSTATUS
        bpl     @v2

        jsr     zerobss
        jsr     copydata
        lda     #<$0800         ; the C stack: $0500-$07FF
        ldx     #>$0800
        sta     c_sp
        stx     c_sp+1
        lda     #1
        sta     _ppu_direct     ; the display stays off until gfx_init
        lda     #CTRL_ON
        sta     PPUCTRL         ; the frame clock
        cli                     ; (the scanline interrupts)
        jsr     initlib
        jsr     callmain
_exit:  jsr     donelib
        jmp     start

mmc3_init:
        .byte   0, 2, 12, 13, 14, 15    ; CHR: table 0; the sprites' 4K
        .byte   0, 7                    ; PRG: overlay 1; R7's bank

; ---------------------------------------------------------------------------
.segment "CODE"

; prg_r6(bank): the overlay window ($8000) gets PRG bank A.
_prg_r6:
        ldx     #6
        stx     _mmc3_sel
        stx     $8000
        sta     $8001
        rts

; ---------------------------------------------------------------------------
; The display for the next frame: the first table's first half and the
; blank bank, scroll 0 (the address too: should this ever come after the
; pre-render line, which copies the scroll into it, the picture is only
; lower, not from wherever the drain left it), and the scanline counter.
show:
        lda     #0
        sta     $8000
        sta     $8001
        lda     #1
        sta     $8000
        lda     #NBANK
        sta     $8001
        lda     _mmc3_sel
        sta     $8000
        lda     #$FE            ; R0/R1 hold no one table
        sta     cur_third
        lda     #0
        sta     irq_step
        bit     PPUSTATUS
        lda     #CTRL_ON
        sta     PPUCTRL
        lda     #0
        sta     PPUSCROLL
        sta     PPUSCROLL
        sta     PPUADDR
        sta     PPUADDR
        lda     #MASK_ALL
        sta     PPUMASK
        sta     $E000
        lda     #IRQ_FIRST
        sta     $C000
        sta     $C001
        sta     $E001
        rts

; The sprites' DMA, from the page the game last finished.
oam_send:
        lda     _oam_next
        beq     @dma
        sta     _oam_show
        lda     #0
        sta     _oam_next
@dma:   lda     #0
        sta     OAMADDR
        lda     _oam_show
        sta     OAMDMA
        rts

nmi:
        pha
        txa
        pha
        tya
        pha
        lda     clk_n           ; (see the top: one frame in CLK_EVERY
        bne     @cl             ; isn't counted)
        lda     clk_n+1
        bne     @ch
        lda     #<(CLK_EVERY - 1)
        sta     clk_n
        lda     #>(CLK_EVERY - 1)
        sta     clk_n+1
        jmp     @c
@ch:    dec     clk_n+1
@cl:    dec     clk_n
        inc     _frame_count
        bne     @c
        inc     _frame_count+1
@c:     lda     _ppu_direct
        bne     @direct
        lda     PPUSTATUS       ; bit 5: a line had a ninth sprite
        and     #$20
        ora     _spr_ovf
        sta     _spr_ovf
        lda     in_drain
        bne     @late
        lda     #NMI_BUDGET
        jsr     _ppu_drain
        jsr     oam_send
        jsr     show
        jmp     @snd
@late:  lda     #1              ; the IRQ's drain shows it when done
        sta     nmi_late
        jmp     @snd
@direct:
        lda     #0
        sta     PPUMASK
        sta     $E000
        lda     #1
        sta     _ppu_off
@snd:   jsr     _snd_frame_nmi
        pla
        tay
        pla
        tax
        pla
        rti

; The scanline interrupt's steps (see the top): a bank register, its bank,
; and the latch for the next (lines to it, less one); then IRQ_STEPS, off.
IRQ_STEPS = 6
irq_reg:  .byte 1, 0, 1, 0, 1, 0
irq_bank: .byte 2, 4, 6, 8, 10, NBANK
irq_next: .byte 31, 31, 31, 31, 31, 17

irq:
        pha
        txa
        pha
        ldx     irq_step
        cpx     #IRQ_STEPS
        beq     @off
        lda     irq_reg,x
        sta     $8000
        lda     irq_bank,x
        sta     $8001
        lda     _mmc3_sel
        sta     $8000
        sta     $E000
        lda     irq_next,x
        sta     $C000
        sta     $C001
        sta     $E001
        inc     irq_step
        pla
        tax
        pla
        rti
        ; line 210: off at dot ~150 of the next (see the top), and the
        ; queue. 38 + 5 * OFF_DELAY cycles from the interrupt (at dot ~260)
        ; to the write.
@off:   tya
        pha
        ldy     #OFF_DELAY
@dl:    dey
        bne     @dl
        lda     #0
        sta     PPUMASK
        sta     $E000
        lda     #1
        sta     in_drain
        lda     #IRQ_BUDGET
        jsr     _ppu_drain
        lda     #0
        sta     in_drain
        lda     nmi_late
        beq     @ret
        lda     #0
        sta     nmi_late
        jsr     oam_send
        jsr     show
@ret:   pla
        tay
        pla
        tax
        pla
        rti

; ---------------------------------------------------------------------------
; ppu_drain(budget): run queued commands while the budget lasts (255: all
; of them). Only with the PPU free: from the interrupts, or the game while
; ppu_direct is set.
_ppu_drain:
        sta     budget
@next:  ldx     _qtail
        cpx     _qhead
        bne     @have
        rts
@have:  lda     _ppuq,x
        cmp     #Q_PAINT
        bcs     @cpaint
        sta     tmp
        txa                     ; n: after op, third, address
        clc
        adc     #4
        tay
        lda     _ppuq,y
        ldy     tmp
        cpy     #Q_FILL
        bne     @cc
        sta     tmp             ; a fill: 9/16 of a unit a byte
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        sta     cost
        lda     tmp
        lsr     a
        clc
        adc     cost
        adc     #3              ; (its own work is more than a copy's)
@cc:    clc
        adc     #Q_BASE
        bcc     @chk
        lda     #255
        bne     @chk
@cpaint:
        cmp     #Q_ROW
        beq     @crow
        lda     #PAINT_COST
        bne     @chk
@crow:  txa                     ; n cells (16 at most): 12 units each
        clc
        adc     #4
        tay
        lda     _ppuq,y
        sta     tmp
        asl     a
        adc     tmp
        asl     a
        asl     a
        adc     #Q_BASE+1
@chk:   ldy     budget
        cpy     #255
        beq     @go
        cmp     budget
        beq     @take
        bcc     @take
        rts                     ; not this frame
@take:  sta     tmp
        lda     budget
        sec
        sbc     tmp
        sta     budget
@go:    lda     _ppuq,x
        sta     op
        inx
        lda     _ppuq,x         ; the third
        inx
        cmp     #$FF
        beq     @addr
        cmp     cur_third
        beq     @addr
        sta     cur_third
        asl     a
        asl     a
        sta     tmp
        lda     #0
        sta     $8000
        lda     tmp
        sta     $8001
        lda     #1
        sta     $8000
        lda     tmp
        ora     #2
        sta     $8001
        lda     _mmc3_sel
        sta     $8000
@addr:  bit     PPUSTATUS
        lda     _ppuq,x
        sta     ahi
        sta     PPUADDR
        inx
        lda     _ppuq,x
        sta     alo
        sta     PPUADDR
        inx
        lda     op
        cmp     #Q_COPY
        bne     @nc
        jmp     @copy
@nc:    cmp     #Q_FILL
        bne     @nf
        jmp     @fill
@nf:    cmp     #Q_ROW
        beq     @row
        jmp     @paint
@row:

        ; row: one line of n cells, each read, the masked pixels replaced
        ; (the address is plane 0's line; plane 1's is 8 on)
        lda     _ppuq,x
        sta     cnt
        inx
@rc:    lda     alo
        ora     #8
        sta     alo8
        bit     PPUSTATUS
        lda     ahi
        sta     PPUADDR
        lda     alo
        sta     PPUADDR
        lda     PPUDATA
        lda     PPUDATA
        sta     pbuf
        bit     PPUSTATUS
        lda     ahi
        sta     PPUADDR
        lda     alo8
        sta     PPUADDR
        lda     PPUDATA
        lda     PPUDATA
        sta     pbuf+1
        lda     _ppuq,x         ; mask
        eor     #$FF
        sta     tmp
        and     pbuf
        inx
        ora     _ppuq,x
        sta     pbuf
        inx
        lda     pbuf+1
        and     tmp
        ora     _ppuq,x
        sta     pbuf+1
        inx
        bit     PPUSTATUS
        lda     ahi
        sta     PPUADDR
        lda     alo
        sta     PPUADDR
        lda     pbuf
        sta     PPUDATA
        bit     PPUSTATUS
        lda     ahi
        sta     PPUADDR
        lda     alo8
        sta     PPUADDR
        lda     pbuf+1
        sta     PPUDATA
        lda     alo
        clc
        adc     #16
        sta     alo
        bcc     @rn
        inc     ahi
@rn:    dec     cnt
        beq     @rd1
        jmp     @rc
@rd1:   stx     _qtail
        jmp     @next

@paint: ; paint: read the cell's 16 bytes, replace the masked pixels
        lda     PPUDATA         ; (the first read is the old buffer)
        ldy     #0
@rd:    lda     PPUDATA
        sta     pbuf,y
        iny
        cpy     #16
        bne     @rd
        ldy     #0
@pl:    lda     _ppuq,x         ; mask
        eor     #$FF
        sta     tmp
        and     pbuf,y
        inx
        ora     _ppuq,x         ; plane 0's new bits
        sta     pbuf,y
        inx
        lda     pbuf+8,y
        and     tmp
        ora     _ppuq,x         ; plane 1's
        sta     pbuf+8,y
        inx
        iny
        cpy     #8
        bne     @pl
        bit     PPUSTATUS
        lda     ahi
        sta     PPUADDR
        lda     alo
        sta     PPUADDR
        ldy     #0
@wr:    lda     pbuf,y
        sta     PPUDATA
        iny
        cpy     #16
        bne     @wr
        stx     _qtail
        jmp     @next

@copy:  ldy     _ppuq,x
        inx
@cl:    lda     _ppuq,x
        sta     PPUDATA
        inx
        dey
        bne     @cl
        stx     _qtail
        lda     ahi             ; a palette's: the address off it (with the
        cmp     #$3F            ; display off, the colour it points at is
        bne     @cn             ; what the screen shows)
        sty     PPUADDR
        sty     PPUADDR
@cn:    jmp     @next

@fill:  ldy     _ppuq,x
        inx
        lda     _ppuq,x
        inx
@fl:    sta     PPUDATA
        dey
        bne     @fl
        stx     _qtail
        jmp     @next

; ---------------------------------------------------------------------------
; A frame of the sound effect (src/nes/sound.c): the tone's frames run
; out, then the second tone (if any) starts, then quiet.
_snd_frame_nmi:
        lda     _snd_f
        beq     @r
        dec     _snd_f
        bne     @r
        lda     _snd_nf
        beq     @q
        sta     _snd_f
        lda     #0
        sta     _snd_nf
        lda     _snd_n+2
        sta     $4000
        lda     _snd_n
        sta     $4002
        lda     _snd_n+1
        sta     $4003
@r:     rts
@q:     lda     #$30
        sta     $4000
        rts

; ---------------------------------------------------------------------------
.segment "VECTORS"
        .word   nmi
        .word   start
        .word   irq

.segment "CLAIM"
        .byte   "FUJI"
