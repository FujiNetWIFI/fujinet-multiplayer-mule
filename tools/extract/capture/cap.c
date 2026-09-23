/* Scripted capture and timing harness for libatari800 (atari800's library build).
 *
 * usage: cap disk.atr script.txt outdir
 *
 * Script commands (one per line, '#' comments):
 *   wait N                 run N frames (jiffies)
 *   joy0 V / trig0 V       joystick 0 bits (up=1 down=2 left=4 right=8, active high) / trigger (1=pressed)
 *   joy1 V / trig1 V       same for joystick 1
 *   start V / select V / option V
 *   key C / nokey
 *   shot NAME              write NAME.scr (384x240 colour indices), NAME.mem (64K RAM)
 *   autoshot 1|0           shot fNNNNNN whenever the game mode ($C1) or display list changes
 *   autopress N            press trigger 0 whenever mode+display list has not changed for N frames (0=off)
 *   memdump FILE [N]       append (int32 frame, 64K RAM) every N frames; 'memdump off' stops
 *   trace FILE [addrs]     start a per-frame CSV trace (extra hex RAM addresses, comma separated)
 *   trace off              stop tracing
 *   mark LABEL             write a marker row into the trace
 *   until ADDR VAL [MAX]   run frames until RAM[ADDR]==VAL (hex), at most MAX frames (default 36000)
 *   untilne ADDR VAL [MAX] run frames until RAM[ADDR]!=VAL
 *   untildl ADDR [MAX]     run until the ANTIC display list pointer equals ADDR
 *   untilframe F           run until the emulator frame counter reaches F
 *   savestate NAME / loadstate NAME
 *
 * Trace columns: frame,joy0,trig0,rtclok,mode($C1),t0..t3($C3-$C6),dli($92),
 *   hposp0..3,p0y..p3y,chbase,dlist,screen_crc,[extra addrs...]
 * p0y..p3y are the first non-empty line of each player in PMG memory (-1 if empty).
 *
 * GPLv3.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libatari800.h"
#include "colours.h"
#include "gtia.h"
#include "antic.h"

static input_template_t in;
static FILE *tracef;
static int nextra;
static unsigned extra[64];
static const char *outdir;
static int autoshot, lastkey = -1;
static int autopress, stable, pressing, akey = -1;
static FILE *memf;
static int memevery;
static void shot(const char *name);

static UBYTE *mem(void) { return libatari800_get_main_memory_ptr(); }

static unsigned crc32(const UBYTE *p, int n) {
	unsigned c = 0xFFFFFFFF;
	for (int i = 0; i < n; i++) {
		c ^= p[i];
		for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320 & -(c & 1));
	}
	return ~c;
}

static int ptop(int player) {
	/* single-line resolution: players at PMBASE+$400 + player*$100 */
	const UBYTE *m = mem() + ANTIC_PMBASE * 256 + 0x400 + player * 0x100;
	for (int y = 0; y < 256; y++) if (m[y]) return y;
	return -1;
}

static void trace_row(const char *label) {
	if (!tracef) return;
	UBYTE *m = mem();
	if (label) { fprintf(tracef, "#mark,%d,%s\n", libatari800_get_frame_number(), label); return; }
	fprintf(tracef, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%08x",
		libatari800_get_frame_number(), in.joy0, in.trig0, m[0x14], m[0xC1], m[0xC3], m[0xC4], m[0xC5], m[0xC6], m[0x92],
		GTIA_HPOSP0, GTIA_HPOSP1, GTIA_HPOSP2, GTIA_HPOSP3, ptop(0), ptop(1), ptop(2), ptop(3), ANTIC_CHBASE, ANTIC_dlist,
		crc32(libatari800_get_screen_ptr(), 384 * 240));
	for (int i = 0; i < nextra; i++) fprintf(tracef, ",%d", m[extra[i]]);
	fputc('\n', tracef);
}

static void frame(void) {
	libatari800_next_frame(&in);
	trace_row(NULL);
	if (memf && libatari800_get_frame_number() % memevery == 0) {
		int fr = libatari800_get_frame_number();
		fwrite(&fr, 4, 1, memf);
		fwrite(mem(), 1, 65536, memf);
	}
	if (autopress) {
		int k = mem()[0xC1] << 16 | ANTIC_dlist;
		if (k != akey) { akey = k; stable = 0; } else stable++;
		if (pressing && --pressing == 0) in.trig0 = 0;
		if (stable == autopress) { in.trig0 = 1; pressing = 6; stable = 0; trace_row("autopress"); }
	}
	if (autoshot) {
		int key = mem()[0xC1] << 16 | ANTIC_dlist;
		if (key != lastkey) {
			char n[32];
			lastkey = key;
			snprintf(n, sizeof n, "f%06d", libatari800_get_frame_number());
			shot(n);
		}
	}
}

static void shot(const char *name) {
	char fn[512];
	snprintf(fn, sizeof fn, "%s/%s.scr", outdir, name);
	FILE *f = fopen(fn, "wb"); fwrite(libatari800_get_screen_ptr(), 1, 384 * 240, f); fclose(f);
	snprintf(fn, sizeof fn, "%s/%s.mem", outdir, name);
	f = fopen(fn, "wb"); fwrite(mem(), 1, 65536, f); fclose(f);
}

static void state_io(const char *name, int save) {
	static emulator_state_t st;
	char fn[512];
	snprintf(fn, sizeof fn, "%s/%s.a8s", outdir, name);
	FILE *f = fopen(fn, save ? "wb" : "rb");
	if (!f) { fprintf(stderr, "cannot open %s\n", fn); exit(1); }
	if (save) { libatari800_get_current_state(&st); fwrite(&st, 1, sizeof st, f); }
	else { fread(&st, 1, sizeof st, f); libatari800_restore_state(&st); }
	fclose(f);
}

int main(int argc, char **argv) {
	if (argc < 4) { fprintf(stderr, "usage: cap disk.atr script.txt outdir\n"); return 1; }
	char *args[] = {"atari800", "-config", "/dev/null", "-atari", "-800-rev", "altirra", "-nobasic", "-ntsc", argv[1], NULL};
	outdir = argv[3];
	if (!libatari800_init(9, args)) { fprintf(stderr, "init failed\n"); return 1; }
	FILE *pf = fopen("palette.bin", "wb");
	for (int i = 0; i < 256; i++) { int c = Colours_table[i]; fputc((c >> 16) & 255, pf); fputc((c >> 8) & 255, pf); fputc(c & 255, pf); }
	fclose(pf);
	libatari800_clear_input_array(&in);
	FILE *sf = fopen(argv[2], "r");
	if (!sf) { perror(argv[2]); return 1; }
	char line[512], cmd[32], a1[256], a2[64], a3[64];
	while (fgets(line, sizeof line, sf)) {
		a1[0] = a2[0] = a3[0] = 0;
		if (sscanf(line, "%31s %255s %63s %63s", cmd, a1, a2, a3) < 1 || cmd[0] == '#') continue;
		int v = atoi(a1);
		if (!strcmp(cmd, "wait")) for (int i = 0; i < v; i++) frame();
		else if (!strcmp(cmd, "joy0")) in.joy0 = v;
		else if (!strcmp(cmd, "trig0")) in.trig0 = v;
		else if (!strcmp(cmd, "joy1")) in.joy1 = v;
		else if (!strcmp(cmd, "trig1")) in.trig1 = v;
		else if (!strcmp(cmd, "start")) in.start = v;
		else if (!strcmp(cmd, "select")) in.select = v;
		else if (!strcmp(cmd, "option")) in.option = v;
		else if (!strcmp(cmd, "key")) in.keychar = a1[0];
		else if (!strcmp(cmd, "nokey")) in.keychar = 0;
		else if (!strcmp(cmd, "shot")) shot(a1);
		else if (!strcmp(cmd, "autoshot")) autoshot = v;
		else if (!strcmp(cmd, "memdump")) {
			if (memf) { fclose(memf); memf = NULL; }
			if (strcmp(a1, "off")) {
				char fn[512];
				snprintf(fn, sizeof fn, "%s/%s", outdir, a1);
				memf = fopen(fn, "wb");
				memevery = a2[0] ? atoi(a2) : 1;
			}
		}
		else if (!strcmp(cmd, "autopress")) { autopress = v; stable = 0; }
		else if (!strcmp(cmd, "mark")) trace_row(a1);
		else if (!strcmp(cmd, "savestate")) state_io(a1, 1);
		else if (!strcmp(cmd, "loadstate")) state_io(a1, 0);
		else if (!strcmp(cmd, "trace")) {
			if (tracef) { fclose(tracef); tracef = NULL; }
			if (strcmp(a1, "off")) {
				char fn[512];
				snprintf(fn, sizeof fn, "%s/%s", outdir, a1);
				tracef = fopen(fn, "w");
				nextra = 0;
				for (char *t = strtok(a2, ","); t && nextra < 64; t = strtok(NULL, ",")) extra[nextra++] = strtoul(t, NULL, 16);
				fprintf(tracef, "frame,joy0,trig0,rtclok,mode,t0,t1,t2,t3,dli,hp0,hp1,hp2,hp3,p0y,p1y,p2y,p3y,chbase,dlist,crc");
				for (int i = 0; i < nextra; i++) fprintf(tracef, ",m%04x", extra[i]);
				fputc('\n', tracef);
			}
		} else if (!strcmp(cmd, "untildl")) {
			unsigned dl = strtoul(a1, NULL, 16);
			int maxf = a2[0] ? atoi(a2) : 36000, n = 0;
			while (n++ < maxf && ANTIC_dlist != dl) frame();
			fprintf(stderr, "untildl %04x: %d frames (now frame %d)\n", dl, n - 1, libatari800_get_frame_number());
		} else if (!strcmp(cmd, "untilframe")) {
			while (libatari800_get_frame_number() < v) frame();
		} else if (!strcmp(cmd, "until") || !strcmp(cmd, "untilne")) {
			unsigned addr = strtoul(a1, NULL, 16), val = strtoul(a2, NULL, 16);
			int maxf = a3[0] ? atoi(a3) : 36000, ne = cmd[5] == 'n', n = 0;
			while (n++ < maxf && ((mem()[addr] == val) == ne)) frame();
			fprintf(stderr, "%s %04x %02x: %d frames (now frame %d)\n", cmd, addr, val, n - 1, libatari800_get_frame_number());
		}
		else fprintf(stderr, "unknown command: %s\n", cmd);
	}
	if (tracef) fclose(tracef);
	if (memf) fclose(memf);
	libatari800_exit();
	return 0;
}
