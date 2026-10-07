/*
 * machine.h -- which PC this is (set by plat.c).
 *
 * MULEJR.EXE is built with PLAT_JRONLY: the PCjr in its 16-colour mode
 * and nothing else. These are constants there, so the CGA, EGA/VGA and
 * PC keyboard and speaker paths fold away (gfxa.asm's are IFNDEF JRONLY),
 * which leaves the 128K PCjr more room for the overlays. Any other
 * machine, or a PCjr without room for the 16-colour screen, is handed to
 * MULE.EXE (plat.c).
 */
#ifndef MACHINE_H
#define MACHINE_H
#include <stdint.h>

#ifdef PLAT_JRONLY
#define plat_jr   1
#define plat_pal  2
#define plat_pcjr 1
#else
extern uint8_t plat_jr;         /* the PCjr's 16-colour mode is on */
extern uint8_t plat_pal;        /* palette registers: 0 no (CGA), 1 EGA/VGA, 2 PCjr */
extern uint8_t plat_pcjr;       /* this is a PCjr */
#endif

#endif
