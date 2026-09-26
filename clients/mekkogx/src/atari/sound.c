/*
 * sound.c -- the game's sounds on POKEY, the original's (pokey.s plays
 * them; gen/atari/snd.s holds the tunes and scripts, tools/mksnd_atari.py).
 *
 * The original has no sound for a few of the game's cues; those get its
 * nearest: the player blip, the message beep. The tunes and scripts live in
 * the overlays that use them (the tunes in the status one, the colony events
 * in production's, the jingles in develop's): they play only while theirs
 * is in (bank.c stops them when it goes).
 */
#if defined(__ATARI__) || defined(__LYNX__)   /* the Lynx: src/lynx/sound.c */

#include <string.h>
#include "mule.h"
#include "bank.h"

extern uint8_t snd_mute, mus_desc[43];
extern const uint8_t *seq_ptr;
void mus_start(void);
void mus_stop(void);
void __fastcall__ seq_start(const uint8_t *s);
void seq_stop(void);
void __fastcall__ v4_play(uint8_t id);
void __fastcall__ tm_set(uint8_t left);
void __fastcall__ step_play(uint16_t fc);

extern const uint8_t step_tab[16], tune_title_desc[43], tune_round_desc[43];
extern const uint16_t tune_round_len[4];
extern const uint8_t snx_ship2[], snx_ship3[], snx_good[], snx_bad[], snx_chirps[], snx_outfit[];
extern const uint8_t snx_pest[], snx_pirate[], snx_rain[], snx_quake[];
extern const uint8_t snx_meteor[], snx_radiation[], snx_fire[];

uint8_t snd_arg;

/* The voice-4 effect for each cue: 0-3 the player blip (a pitch each), 4
 * the message beep (repeats in bits 4-7), 255 none (the extra ids do it). */
static const uint8_t cue[SND_COUNT] = {
    0,          /* select */
    0,          /* sample (as installing a M.U.L.E.) */
    4,          /* error: the message beep */
    0,          /* buy */
    0,          /* my turn */
    0,          /* declare */
    4 | 0x30,   /* auction over: four beeps */
    0,          /* trade: a unit */
    255,        /* tick: the timer (SNX_TIMER) */
    255,        /* event: SNX_EVENT */
    4 | 0x10,   /* founder: two beeps */
    255,        /* boot */
    255,        /* step: below */
};

void snd_init(void)
{
    mus_stop();
    seq_stop();
}

void snd_play(uint8_t id)
{
    uint8_t c, s;
    if (snd_mute || id >= SND_COUNT)
        return;
    if (id == SND_STEP) {
        s = PL(st.turn.seat & 3).species & 7;
        step_play(step_tab[s] | step_tab[s + 8] << 8);
        return;
    }
    c = cue[id];
    if (c == 255)
        return;
    if (!(c & 4))
        c = my_seat < 4 ? my_seat : 0;  /* your blip */
    v4_play(c);
}

void snd_extra(uint8_t id)
{
    uint8_t ov = bank_current(), r;
    const uint8_t *s = 0;
    switch (id) {
    case SNX_TITLE:
    case SNX_TUNE:
        if (ov != OV_STATUS)
            return;
        mus_stop();
        memcpy(mus_desc, id == SNX_TITLE ? tune_title_desc : tune_round_desc, sizeof(mus_desc));
        if (id == SNX_TUNE) {
            /* from round 12 the whole tune; before it, from 2 bytes on
             * (then 4), longer every 4 rounds ($2527) */
            r = snd_arg >= 12 ? 0 : 2;
            *(uint16_t *)(mus_desc + 0) += r;       /* voices 1-3 start */
            *(uint16_t *)(mus_desc + 4) += r;
            *(uint16_t *)(mus_desc + 10) += r;
            *(uint16_t *)(mus_desc + 2) = *(uint16_t *)mus_desc + tune_round_len[(snd_arg >> 2) & 3];
            mus_desc[42] = r != 0;
        }
        if (!snd_mute)
            mus_start();
        return;
    case SNX_SHIP:
        if (ov == OV_STATUS)
            s = snx_ship2;
        else if (ov == OV_GRANT)
            s = snx_ship3;
        break;
    case SNX_EVENT:
        if (ov != OV_GRANT)
            return;
        switch (snd_arg) {
        case CE_PEST:        s = snx_pest; break;
        case CE_PIRATES:     s = snx_pirate; break;
        case CE_ACIDRAIN:    s = snx_rain; break;
        case CE_QUAKE:       s = snx_quake; break;
        case CE_METEORITE:   s = snx_meteor; break;
        case CE_RADIATION:   s = snx_radiation; break;
        case CE_FIRE:        s = snx_fire; break;
        case CE_SHIPRETURNS: s = snx_ship3; break;
        }
        break;
    case SNX_NEWS:
        if (ov == OV_DEVELOP)
            s = snd_arg <= 13 ? snx_good : snx_bad; /* events 1-13 are good news */
        break;
    case SNX_CHIRPS:
        if (ov == OV_GRANT)
            s = snx_chirps;
        break;
    case SNX_OUTFIT:
        if (ov == OV_DEVELOP)
            s = snx_outfit;
        break;
    case SNX_TIMER:
        tm_set(snd_mute ? 0 : snd_arg);
        return;
    }
    if (s && !snd_mute)
        seq_start(s);
}

/* An overlay is going: its tunes and scripts with it. */
void snd_overlay(void)
{
    mus_stop();
    seq_stop();
}

void snd_tick(void)
{
}

void snd_toggle_mute(void)
{
    snd_mute ^= 1;
    if (snd_mute) {
        mus_stop();
        seq_stop();
        tm_set(0);
    }
}

#endif
