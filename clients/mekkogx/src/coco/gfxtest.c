/*
 * gfxtest.c -- the CoCo 3 graphics test (make coco3 GFXTEST=1), in place
 * of the game: screens of what gfx3.c draws, SPACE or a key for the next
 * (the sprite and ship screens run until one).
 */
#if defined(COCO3) && defined(MULE_GFXTEST)
#include <string.h>
#include "mule.h"

/* Wait for the button or a key; the frame count ticks meanwhile (the IRQ). */
static void next(void)
{
    char buf[8];
    for (;;) {
        frame_wait();
        input_poll();
        input_take();
        if (ev_btn || ev_key)
            return;
        if ((uint8_t)frame_count & 7)
            continue;
        buf[fmt_num(buf, frame_count)] = 0;
        gfx_clear_text(34, 23, 6);
        gfx_text(34, 23, buf, C_WHITE);
    }
}

static void bars(void)
{
    uint8_t idx;
    static const char hex[] = "0123456789ABCDEF";
    gfx_clear();
    for (idx = 0; idx < 16; ++idx) {
        gfx_fill(idx * 10, 8, 10, 24, idx);
        gfx_char((uint8_t)((idx * 5 + 1) >> 1), 4, hex[idx], C_WHITE);
    }
    gfx_fill(0, 0, 160, 1, C_WHITE);
    gfx_fill(0, 191, 160, 1, C_WHITE);
    text_center(5, "M.U.L.E. COCO 3  320X192X16", C_WHITE);
    gfx_text(0, 7, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", C_WHITE);
    gfx_text(0, 8, "0123456789 !\"#$%&'()*+,-./", C_WHITE);
    gfx_text(0, 9, ":;<=>?@[\\]^_ abc", C_WHITE);
    gfx_big(0, 10, "BIG TEXT 0123456789", C_WHITE);
    gfx_text(0, 11, "PLAYER 1 BLUE", C_BLUE);
    gfx_text(0, 12, "PLAYER 2 PURPLE", C_PURPLE);
    gfx_text(0, 13, "PLAYER 3 GREEN", C_GREEN);
    gfx_text(0, 14, "PLAYER 4 ORANGE", C_P3);
    gfx_big(18, 11, "RED", C_RED);
    gfx_big(18, 12, "YELLOW", C_YELLOW);
    gfx_big(18, 13, "GREY", C_GREY);
    gfx_big(18, 14, "CYAN", C_CYAN);
    gfx_text(32, 11, "TAN", C_TAN);
    gfx_text(32, 12, "BROWN", C_BROWN);
    gfx_text(32, 13, "LTBLUE", C_LTBLUE);
    gfx_text(32, 14, "PINK", C_PINK);
    gfx_hdots(0, 128, 160, C_WHITE);
    gfx_hdots(1, 130, 158, C_YELLOW);
    for (idx = 0; idx < 4; ++idx)
        gfx_pic(8 + idx * 20, 136, idx, C_WHITE);
    gfx_fill(100, 136, 40, 16, C_BLUE);
    gfx_fill(101, 137, 38, 14, C_GREEN);
    gfx_fill(103, 139, 34, 10, C_ORANGE);
    gfx_text(0, 20, "FRAMES:", C_WHITE);
    gfx_text(28, 23, "FRAME", C_GREY);
    /* the clock: 120000 turns of an 8-cycle loop are 32 frames at 1.79 MHz */
    {
        char buf[8];
        uint16_t start = frame_count;
        for (idx = 0; idx < 2; ++idx) {
            asm {
                ldx     #60000
@spin
                leax    -1,x
                bne     @spin
            }
        }
        buf[fmt_num(buf, frame_count - start)] = 0;
        gfx_text(0, 21, "960K CYCLES IN FRAMES:", C_WHITE);
        gfx_text(23, 21, buf, C_YELLOW);
    }
}

static void plots(void)
{
    PlotLook look;
    uint8_t plot;
    gfx_map_frame();
    for (plot = 0; plot < 45; ++plot) {
        memset(&look, 0, sizeof(look));
        look.store = (plot == 22);
        look.river = (plot % 9 == 4) && !look.store;
        look.mountains = (plot % 4 == 1) ? 1 : (plot % 7 == 2) ? 2 : (plot % 11 == 3) ? 3 : 0;
        look.owned = (plot % 3 == 0) && !look.store;
        look.col = look.owned ? pl_col[(plot / 9) & 3] : C_WHITE;
        look.icon = look.owned ? (uint8_t)(plot % 7) : 0;
        look.dots = look.owned ? (uint8_t)(plot % 9) : 0;
        gfx_plot(plot, &look);
    }
    gfx_fill(MAP_BAR_X, 16, 4, 100, C_RED);
    gfx_text(0, 0, "MAP: PLOTS", C_WHITE);
    gfx_text(28, 23, "FRAME", C_GREY);
}

static void town(void)
{
    gfx_clear();
    gfx_town();
    gfx_text(0, 0, "TOWN", C_WHITE);
    gfx_text(TX(8), 21, "$1234", C_YELLOW);
    gfx_text(TX(20), 21, "MULES 3", C_GREEN);
    gfx_text(28, 23, "FRAME", C_GREY);
}

/* Every kind of sprite over the map at once, crossing each other, the
 * screen edge, a block boundary and text. */

static void sprites(void)
{
    char buf[8];
    int16_t figx[4], cldx = -16;
    uint8_t dir[4], idx, frm, cur = 0;
    uint16_t loop = 0, tfl = 0, start;
    plots();
    frame_count = 0;
    gfx_text(0, 0, "SPRITES   ", C_WHITE);
    for (idx = 0; idx < 4; ++idx) {
        figx[idx] = 20 + idx * 30;
        dir[idx] = idx & 1;
    }
    for (;;) {
        frame_wait();
        input_poll();
        input_take();
        if (ev_btn || ev_key)
            return;
        ++loop;
        frm = (uint8_t)((loop >> 2) & 3);
        for (idx = 0; idx < 4; ++idx) {
            figx[idx] += dir[idx] ? -1 : 1;
            if (figx[idx] <= 0 || figx[idx] >= 150)
                dir[idx] ^= 1;
            spr_set(idx, SH_FIGURE, (uint8_t)(idx * 10 + frm), figx[idx], 52 + idx * 32, pl_col[idx],
                    dir[idx] ? SPR_FLIPX : 0);
        }
        spr_set(5, SH_MULE, (uint8_t)(frm & 1), figx[0] - (dir[0] ? -10 : 10), 48,
                C_WHITE, dir[0] ? SPR_FLIPX : 0);
        if ((loop & 31) == 0)
            cur = (uint8_t)((cur + 1) % 45);
        spr_set(4, SH_CURSOR, 0, PLOT_X(cur), PLOT_Y(cur), C_WHITE, 0);
        if (++cldx > 160)
            cldx = -16;
        spr_set(6, SH_CLOUD, (uint8_t)((loop >> 3) & 3), cldx, 2, C_WHITE, 0);
        /* a hull crossing the left edge, over line 102 (a block boundary) */
        spr_set(7, SH_HULL_BOT, 0, (int16_t)(20 - ((loop >> 2) % 60)), 90, C_BLUE, 0);
        if (loop & 32)
            spr_set(8, SH_WAMPUS, 0, 140, 100, C_PINK, 0);
        else
            spr_hide(8);
        spr_set(9, SH_SPECIES, (uint8_t)(((loop >> 5) & 3) * 11 + (frm & 1)),
                (int16_t)(loop & 127) + 20, 170, pl_col[(loop >> 5) & 3], 0);
        if ((loop & 7) == 0) {         /* loops; frames in the flushes; frames */
            buf[fmt_num(buf, loop)] = 0;
            gfx_clear_text(0, 23, 6);
            gfx_text(0, 23, buf, C_WHITE);
            buf[fmt_num(buf, tfl)] = 0;
            gfx_clear_text(14, 23, 6);
            gfx_text(14, 23, buf, C_YELLOW);
            buf[fmt_num(buf, frame_count)] = 0;
            gfx_clear_text(28, 23, 6);
            gfx_text(28, 23, buf, C_WHITE);
        }
        if ((loop & 63) == 0) {
            PlotLook look;
            memset(&look, 0, sizeof(look));
            look.owned = 1;
            look.col = pl_col[(loop >> 6) & 3];
            look.icon = ICON_MULE;
            look.dots = (uint8_t)((loop >> 6) & 7);
            gfx_plot(cur, &look);
        }
        start = frame_count;
        spr_flush();
        tfl += frame_count - start;
    }
}

extern uint16_t ct_slide, pf_flush;
uint16_t pf_ticks;

/* The GIME timer, every 4 scan lines (455 cycles), counted by a FIRQ:
 * XRoar does not give the count back from $FF94. */
void pf_firq_code(void)
{
    asm {
        bra     @skip
pf_firq
        pshs    a
        lda     $FF93           ; takes the flag down
        inc     _pf_ticks+1
        bne     @done
        inc     _pf_ticks
@done
        puls    a
        rti
@skip
    }
}

static void pf_init(void)
{
    asm {
        orcc    #$50
        lda     #$7E            ; JMP pf_firq at the FIRQ vector jump
        sta     $FEF4
        leax    pf_firq,pcr
        stx     $FEF5
        clr     $FF91           ; the timer clock: the scan line
        lda     #$20
        sta     $FF93           ; the timer on FIRQ
        lda     #$5C
        sta     $FF90           ; FIRQ out
        lda     #3
        sta     $FF95
        clr     $FF94           ; starts it
        andcc   #$AF
    }
}

uint16_t pf_now(void)
{
    return pf_ticks;
}

/* The transport ship's pass (fx.c's fx_land), the heaviest real scene.
 * Over the last 32 loops: row 22, lines slid; row 23, loops, then timer
 * ticks (4 scan lines each) in the flushes. */
static void ship(void)
{
    char buf[8];
    int16_t shipx = 182;
    uint16_t loop = 0;
    pf_init();
    pf_flush = 0;
    spr_hide_all();
    spr_flush();
    plots();
    frame_count = 0;
    gfx_text(0, 0, "SHIP      ", C_WHITE);
    for (;;) {
        frame_wait();
        input_poll();
        input_take();
        if (ev_btn || ev_key)
            return;
        ++loop;
        if (--shipx < -40)
            shipx = 182;
        /* every 128 loops a different set: the ship; the hulls; the top
         * hull; the nose (the title says which) */
        if ((loop & 127) == 1) {
            spr_hide_all();
            gfx_char(5, 0, (char)('0' + ((loop >> 7) & 3)), C_WHITE);
        }
        if (((loop >> 7) & 3) == 0 || ((loop >> 7) & 3) == 3)
            spr_set(0, SH_NOSE, 0, shipx - 8, 84, C_WHITE, 0);
        if (((loop >> 7) & 3) < 3)
            spr_set(1, SH_HULL_TOP, 0, shipx, 74, C_WHITE, 0);
        if (((loop >> 7) & 3) < 2)
            spr_set(2, SH_HULL_BOT, 0, shipx, 90, C_BLUE, 0);
        if ((loop & 31) == 0) {
            gfx_clear_text(0, 22, 40);
            gfx_clear_text(0, 23, 40);
            buf[fmt_num(buf, ct_slide)] = 0;
            gfx_text(20, 22, buf, C_CYAN);
            buf[fmt_num(buf, loop)] = 0;
            gfx_text(0, 23, buf, C_WHITE);
            buf[fmt_num(buf, pf_flush)] = 0;
            gfx_text(6, 23, buf, C_YELLOW);
            ct_slide = 0;
            pf_flush = 0;
        }
        spr_flush();
    }
}

void gfx_test(void)
{
    for (;;) {
        bars();
        next();
        plots();
        next();
        town();
        next();
        sprites();
        ship();
    }
}
#endif
