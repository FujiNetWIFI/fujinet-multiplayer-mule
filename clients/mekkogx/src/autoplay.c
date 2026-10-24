/*
 * autoplay.c -- plays by itself, for soak tests and filmed games against a
 * real server (make AUTOPLAY=1; the INTV client's tools/autoplay.py does
 * the same from outside). It only does what a player would, through the
 * stick and the button, a frame at a time, so the screen shows a real
 * game: ready once four players sit, claim land, walk a turn (corral,
 * outfitter, out to our plot, install, the pub), declare and trade. Each
 * phase's part lives with the phase (ap_status, ap_grant, ap_auction,
 * ap_develop), in its overlay. Not in normal builds.
 */
#ifdef MULE_AUTOPLAY

#include "mule.h"

static uint8_t ap_ph = 255;
static uint8_t ap_pf;           /* the frame (low byte) of the last press */
uint16_t ap_t0;                 /* the frame this phase began (for the parts) */

/* 1 at most every n frames: a press the server may not have seen yet
 * isn't repeated at once */
uint8_t ap_every(uint8_t n)
{
    if ((uint8_t)((uint8_t)frame_count - ap_pf) < n)
        return 0;
    ap_pf = (uint8_t)frame_count;
    return 1;
}

void autoplay(void)
{
    if (!st_ok || my_seat > 3)
        return;
    if (phase != ap_ph) {
        ap_ph = phase;
        ap_t0 = frame_count;
    }
    inp_dir = 0;
    switch (phase) {
    case PHASE_LOBBY:
    case PHASE_SUMMARY:
        ap_status();
        break;
    case PHASE_GRANT:
        ap_grant();
        break;
    case PHASE_LANDAUCTION:
    case PHASE_DECLARE:
    case PHASE_TRADING:
        ap_auction();
        break;
    case PHASE_DEVELOP:
        ap_develop();
        break;
    }
}

#endif
