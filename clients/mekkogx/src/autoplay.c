/*
 * autoplay.c -- plays by itself, for soak tests against a real server
 * (make AUTOPLAY=1; the INTV client's tools/autoplay.py does the same from
 * outside). It only presses what a player would: ready, claims, turns,
 * continue. Not in normal builds.
 */
#ifdef MULE_AUTOPLAY

#include "mule.h"

static uint16_t ap_last;

void autoplay(void)
{
    uint16_t now = frame_count;
    if ((uint16_t)(now - ap_last) < 45 || !st_ok || my_seat > 3)
        return;
    ap_last = now;
    switch (phase) {
    case PHASE_LOBBY:
        if (!(PL(my_seat).flags & PF_READY))
            ev_btn = 1;
        break;
    case PHASE_SUMMARY:
    case PHASE_GRANT:
        ev_btn = 1;
        break;
    case PHASE_DEVELOP:
        if (dv_state == DV_PROMPT)
            ev_btn = 1;
        else if (dv_state == DV_WALK)
            ev_key = KEY_END;
        break;
    case PHASE_DECLARE:
        ev_key = KEY_SITOUT;
        break;
    }
}

#endif
