;
; crt0.s -- M.U.L.E. on the NES: the iNES header, start-up, the frame's
; interrupts and the PPU queue (src/nes/neshw.h).
;
; The screen is a bitmap of 768 cells, each its own tile, as the TMS9918A's
; Graphics II: a third of the screen (8 rows, 256 tiles) to each of three
; 4K pattern tables in CHR-RAM. MMC3's scanline counter interrupts at the
; lines where they change:
;
;   line  15  the background on (above it, rows 0-1, only sprites)
;         79  rows 10-17: the second table
;        143  rows 18-25: the third
;        207  everything off: from here to the end of the vertical blank
;             the PPU is free, and the queue is drained (then again in the
;             NMI, after the sprites' DMA)
;
; The NMI counts the frames, plays the sound effect's next frame and puts
; the display back for the next frame (the first table, the scroll, the
; counter).
;
; Game code never touches the PPU while it shows a picture: drawing goes
; into the queue (ppuq) and the interrupts drain it, a budget's worth a
; frame. While a whole screen is drawn, ppu_direct is set: the NMI turns
; the display off and leaves the PPU alone, and the game drains the queue
; itself as it fills (src/nes/gfx.c).
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
        .exportzp       _qhead, _qtail, _ppu_direct, _mmc3_sel, _ppu_off, _spr_ovf
        .export         _frame_count, _ppuq, _oam
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
MASK_SPR  = $14                 ; sprites only (the top margin)
MASK_ALL  = $1E                 ; background and sprites, no clipping

; The first interrupt: the counter is reloaded with the latch at the first
; rendered line it sees, then counts down a line at a time. MAME's cart
; (pico/nes/emu) clocks it from line 0; the hardware also clocks it on the
; pre-render line, a line earlier: IRQ_EARLY 1 for that.
.ifndef IRQ_EARLY
IRQ_EARLY = 0
.endif
IRQ_FIRST = 15 - IRQ_EARLY
IRQ_NEXT  = 63                  ; then every 64 lines

; Drain budgets, in units of about 16 cycles (a byte copied is one, a byte
; filled half). No command may cost more than the IRQ's budget (gfx.c keeps
; them smaller), or it would never go.
Q_COPY    = 1
Q_FILL    = 2
Q_PAINT   = 3
Q_ROW     = 4
PAINT_COST = 56
IRQ_BUDGET = 190                ; lines 208-240
NMI_BUDGET = 85                 ; the vertical blank, after the DMA

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
_ppu_off:    .res 1             ; the NMI has turned the display off
_spr_ovf:    .res 1             ; a line had more than 8 sprites

.segment "BSS"
_frame_count: .res 2            ; (cleared by zerobss)

.segment "PPUQ"
_ppuq:       .res 256

.segment "OAM"
_oam:        .res 256

.segment "PBUF"
pbuf:        .res 16            ; a cell being painted

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
        inx
        bne     @ofs

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
; The display for the next frame: the first table, scroll 0, the top margin
; showing sprites only, and the scanline counter set for line 15.
show:
        lda     #0
        sta     $8000
        sta     $8001
        lda     #1
        sta     $8000
        lda     #2
        sta     $8001
        lda     _mmc3_sel
        sta     $8000
        lda     #0
        sta     cur_third
        sta     irq_step
        bit     PPUSTATUS
        lda     #CTRL_ON
        sta     PPUCTRL
        lda     #0
        sta     PPUSCROLL
        sta     PPUSCROLL
        lda     #MASK_SPR
        sta     PPUMASK
        sta     $E000
        lda     #IRQ_FIRST
        sta     $C000
        sta     $C001
        sta     $E001
        rts

nmi:
        pha
        txa
        pha
        tya
        pha
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
        lda     #0
        sta     OAMADDR
        lda     #>_oam
        sta     OAMDMA
        lda     #NMI_BUDGET
        jsr     _ppu_drain
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

irq:
        pha
        lda     irq_step
        beq     @s0
        cmp     #1
        beq     @s1
        cmp     #2
        beq     @s2
        ; line 207: off, and the queue
        lda     #0
        sta     PPUMASK
        sta     $E000
        txa
        pha
        tya
        pha
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
        lda     #0
        sta     OAMADDR
        lda     #>_oam
        sta     OAMDMA
        jsr     show
@ret:   pla
        tay
        pla
        tax
        pla
        rti
@s1:    lda     #0              ; R0 first: row 10's first tiles are in it
        sta     $8000
        lda     #4
        sta     $8001
        lda     #1
        sta     $8000
        lda     #6
        sta     $8001
        lda     #1
        sta     cur_third
        bne     @next
@s2:    lda     #0
        sta     $8000
        lda     #8
        sta     $8001
        lda     #1
        sta     $8000
        lda     #10
        sta     $8001
        lda     #2
        sta     cur_third
        bne     @next
@s0:    lda     #MASK_ALL
        sta     PPUMASK
@next:  lda     _mmc3_sel
        sta     $8000
        sta     $E000
        lda     #IRQ_NEXT
        sta     $C000
        sta     $C001
        sta     $E001
        inc     irq_step
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
        lsr     a               ; a fill: half a unit a byte
@cc:    clc
        adc     #4
        bcc     @chk
        lda     #255
        bne     @chk
@cpaint:
        cmp     #Q_ROW
        beq     @crow
        lda     #PAINT_COST
        bne     @chk
@crow:  txa                     ; n cells: 10 units each
        clc
        adc     #4
        tay
        lda     _ppuq,y
        sta     tmp
        asl     a
        asl     a
        adc     tmp
        asl     a
        adc     #5
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
        jmp     @next

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
