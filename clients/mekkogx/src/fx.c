/*
 * fx.c -- the actor engine (clients/intv/fx.bas). See fx.h.
 */
#include "mule.h"

Actor fx[4];
uint8_t fx_on, fx_flag;

static void hide(Actor *a)
{
    spr_hide(a->slot);
    if (a->kind == FK_SHIP) {
        spr_hide(a->slot + 1);
        spr_hide(a->slot + 2);
    }
}

static void draw(Actor *a)
{
    if (a->hidden)
        return;
    if (a->kind == FK_SHIP) {
        /* a nose (8 x 10) in front of a 32 x 32 hull, light over dark */
        spr_set(a->slot, SH_NOSE, a->frame, a->x - 8, a->y + 10, a->col, 0);
        spr_set(a->slot + 1, SH_HULL_TOP, a->frame, a->x, a->y, a->col, 0);
        spr_set(a->slot + 2, SH_HULL_BOT, a->frame, a->x, a->y + 16, a->col2, 0);
        return;
    }
    if (a->x < -32 || a->x > 168 || a->y < -32 || a->y > 192) {
        spr_hide(a->slot);
        return;
    }
    spr_set(a->slot, a->shape, a->fbase + a->frame + (a->alt ? a->d2 : 0), a->x, a->y, a->col, a->flags);
}

void fx_start(uint8_t i, const uint8_t *script)
{
    Actor *a = &fx[i];
    a->st = 0;
    a->pc = script;
    a->wait = 1;
    a->n = 0;
    a->rep = 0;
    a->alt = 0;
    a->hidden = 0;
    a->st = 1;
    fx_on = 1;
}

void fx_stop_all(void)
{
    uint8_t i;
    fx_on = 0;
    for (i = 0; i < 4; ++i) {
        if (fx[i].st)
            hide(&fx[i]);
        fx[i].st = 0;
    }
}

static void step(Actor *a)
{
    const uint8_t *pc;
    uint8_t op, c;

    if (a->wait > 1) {
        --a->wait;
        return;
    }
    if (a->n) {
        a->x += a->dx;
        a->y += a->dy;
        --a->n;
        a->wait = a->per;
        a->alt ^= 1;
        draw(a);
        return;
    }
    /* fetch until something takes time */
    a->wait = 1;
    pc = a->pc;
    for (;;) {
        op = *pc++;
        switch (op) {
        case FX_END:
            a->st = 0;
            a->pc = pc;
            return;
        case FX_POS:
            a->hidden = 0;
            a->x = (int16_t)pc[0] - 64;
            a->y = (int16_t)pc[1] - 64;
            pc += 2;
            draw(a);
            break;
        case FX_MOVE:
            a->dx = (int8_t)pc[0];
            a->dy = (int8_t)pc[1];
            a->per = pc[2];
            a->n = pc[3];
            a->wait = pc[2];
            a->pc = pc + 4;
            return;
        case FX_WAIT:
            a->wait = pc[0];
            a->pc = pc + 1;
            return;
        case FX_LOOK:
            a->kind = pc[0];
            a->slot = pc[1];
            a->shape = pc[2];
            a->frame = pc[3];
            a->d2 = pc[4];
            a->col = pc[5];
            a->col2 = pc[6];
            a->flags = 0;
            a->fbase = 0;
            a->hidden = 0;
            pc += 7;
            break;
        case FX_FRAME:
            a->hidden = 0;
            a->frame = *pc++;
            draw(a);
            break;
        case FX_FLAGS:
            a->flags = *pc++;
            draw(a);
            break;
        case FX_HIDE:
            hide(a);
            a->hidden = 1;
            break;
        case FX_COL:
            a->col = *pc++;
            draw(a);
            break;
        case FX_SIG:
            fx_flag = *pc++;
            break;
        case FX_LOOP:
            c = a->rep;
            if (c == 0)
                c = pc[0] + 1;
            a->rep = --c;
            if (c)
                pc = pc - 1 - pc[1];
            else
                pc += 2;
            break;
        default:
            a->st = 0;
            return;
        }
    }
}

void fx_run(void)
{
    uint8_t i;
    if (!fx_on)
        return;
    for (i = 0; i < 4; ++i)
        if (fx[i].st) {
            step(&fx[i]);
            frame_tick();
        }
}

/* ---------------------------------------------------------------------------
 * The transport ship (clients/intv/events.bas), used by the summary, the
 * grant and production.
 * ------------------------------------------------------------------------- */
#ifdef __ATARI__
/* The hull's halves on whole 8-line rows (9-10, 11-12), so they share a
 * player and the nose (rows 10-11) gets the other: only two are free on
 * the map (sprite.s), and at 74 all three wanted row 11. */
#define SHIP_Y   72
#else
#define SHIP_Y   74             /* hull top (Atari line 103) */
#endif
#define DOCK_X   76
#define DOCK_Y   82

/* Transport ship landing (round 1 arrival, round 12 "the ship is back"). */
const uint8_t fx_land[] = {
    FX_LOOK, FK_SHIP, 5, SH_NOSE, 0, 0, C_WHITE, C_BLUE,
    FX_POS, XB(182), XB(SHIP_Y),
    FX_WAIT, 36,
    FX_MOVE, SB(-1), 0, 1, 230,         /* fast pass right to left, 1 cc a frame */
    FX_HIDE, FX_WAIT, 18,
    FX_POS, XB(-40), XB(SHIP_Y),
    FX_MOVE, 1, 0, 6, 78,               /* slow return, 1 cc every 6 frames */
    FX_MOVE, 5, 0, 6, 9,                /* settling onto the store */
    FX_HIDE,
    FX_LOOK, FK_SINGLE, 8, SH_DOCK, 0, 0, C_LTBLUE, 0,     /* sinking into it */
    FX_POS, XB(DOCK_X), XB(DOCK_Y), FX_WAIT, 8,
    FX_FRAME, 1, FX_WAIT, 8, FX_FRAME, 2, FX_WAIT, 8,
    FX_FRAME, 3, FX_WAIT, 8, FX_HIDE,
    FX_SIG, 1,
    FX_END
};

/* Transport ship leaving (after round 1's summary: "back in 12 months"). */
const uint8_t fx_depart[] = {
    FX_WAIT, 45,
    FX_LOOK, FK_SINGLE, 8, SH_DOCK, 3, 0, C_LTBLUE, 0,     /* rising out of the store */
    FX_POS, XB(DOCK_X), XB(DOCK_Y), FX_WAIT, 8,
    FX_FRAME, 2, FX_WAIT, 8, FX_FRAME, 1, FX_WAIT, 8,
    FX_FRAME, 0, FX_WAIT, 8, FX_HIDE,
    FX_LOOK, FK_SHIP, 5, SH_NOSE, 0, 0, C_WHITE, C_BLUE,
    FX_POS, XB(85), XB(SHIP_Y),
    FX_MOVE, SB(-5), 0, 8, 9,
    FX_MOVE, SB(-1), 0, 8, 90,          /* off to the left, 1 cc every 8 frames */
    FX_HIDE,
    FX_SIG, 1,
    FX_END
};

