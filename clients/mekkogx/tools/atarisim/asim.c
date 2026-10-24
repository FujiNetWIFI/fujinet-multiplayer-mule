/* asim -- a scripted libatari800 driver for the Atari client (and for
 * studying the original disk). It grew out of tools/extract/capture/cap.c.
 *
 * usage: asim [-xl] [-netsio PORT] disk.atr|- script.txt outdir
 *   -xl      an 800XL with 64K (default: an 800 with 48K, no BASIC)
 *
 * Script commands (one per line, '#' comments; numbers in hex where noted):
 *   wait N                 run N frames
 *   joy0 V / trig0 V       joystick bits (up=1 down=2 left=4 right=8) / trigger (1 = pressed)
 *   start V / select V / option V
 *   key C / keycode N / nokey   press a character / an AKEY code (decimal) / release
 *   shot NAME              NAME.scr (384x240 colour indices), NAME.mem (64K), NAME.regs
 *   poke ADDR VAL          (hex) write RAM
 *   peek ADDR [N]          (hex) print N bytes of RAM
 *   until ADDR VAL [MAX]   run until RAM[ADDR] == VAL (hex); untilne: until !=
 *   untildl ADDR [MAX]     run until the display list pointer is ADDR (hex)
 *   pokey FILE | off       log POKEY writes: frame,scanline,pc,reg,value
 *   gtia FILE | off        log GTIA/ANTIC colour, position and size writes likewise
 *   wav FILE | off         record the sound (16-bit mono)
 *   trace FILE [ADDRS] | off   per-frame CSV of the given RAM addresses (hex, comma separated)
 *   film FILE | off        append every frame's screen (384x240 colour indices) to FILE
 *   filmlog FILE [ADDRS] | off  per frame: frame, wall clock and monotonic ns, the RAM bytes
 *   untilfile PATH [MAX]   run until PATH exists (checked every 60 frames), or SIGTERM
 *   cpu                    print the 6502's registers (and the stack)
 *   brkcont 1|0            carry on at a BRK (else libatari800 stops there)
 *   coldstart              power off and on (boot again: FujiNet is there now)
 *   savestate NAME / loadstate NAME
 *
 * A JAM or a display list error prints CRASH (code, frame, PC).
 *
 * Link with -Wl,--wrap=POKEY_PutByte,--wrap=GTIA_PutByte,--wrap=ANTIC_PutByte
 * (see Makefile). GPLv3.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "libatari800.h"
#include "colours.h"
#include "gtia.h"
#include "antic.h"
#include "cpu.h"
#include "pokey.h"
#include "atari.h"

static input_template_t in;
static const char *outdir;
static FILE *pokeyf, *gtiaf, *wavf, *tracef, *filmf, *flogf;
static long wavbytes;
static int nextra, nflog;
static unsigned extra[64], flog[64];
static volatile sig_atomic_t stop;      /* SIGTERM: wind up (the WAV's header) */

static void on_term(int sig) { (void)sig; stop = 1; }

static UBYTE *mem(void) { return libatari800_get_main_memory_ptr(); }

static void outpath(char *fn, size_t n, const char *name) { snprintf(fn, n, "%s/%s", outdir, name); }

void __real_POKEY_PutByte(UWORD addr, UBYTE byte);
void __wrap_POKEY_PutByte(UWORD addr, UBYTE byte)
{
	if (pokeyf && (addr & 0x0f) <= 8)
		fprintf(pokeyf, "%d,%d,%04x,%x,%02x\n", libatari800_get_frame_number(), ANTIC_ypos, CPU_regPC, addr & 0x0f, byte);
	__real_POKEY_PutByte(addr, byte);
}

void __real_GTIA_PutByte(UWORD addr, UBYTE byte);
void __wrap_GTIA_PutByte(UWORD addr, UBYTE byte)
{
	if (gtiaf) fprintf(gtiaf, "%d,%d,%04x,G%02x,%02x\n", libatari800_get_frame_number(), ANTIC_ypos, CPU_regPC, addr & 0x1f, byte);
	__real_GTIA_PutByte(addr, byte);
}

void __real_ANTIC_PutByte(UWORD addr, UBYTE byte);
void __wrap_ANTIC_PutByte(UWORD addr, UBYTE byte)
{
	if (gtiaf && (addr & 0x0f) != 0x0a) /* not WSYNC */
		fprintf(gtiaf, "%d,%d,%04x,A%x,%02x\n", libatari800_get_frame_number(), ANTIC_ypos, CPU_regPC, addr & 0x0f, byte);
	__real_ANTIC_PutByte(addr, byte);
}

static void put16(FILE *f, unsigned v) { fputc(v & 255, f); fputc(v >> 8, f); }
static void put32(FILE *f, unsigned long v) { put16(f, v & 0xffff); put16(f, v >> 16); }

static void wav_header(FILE *f, long bytes)
{
	int rate = libatari800_get_sound_frequency(), ch = libatari800_get_num_sound_channels();
	int ss = libatari800_get_sound_sample_size();
	fwrite("RIFF", 1, 4, f); put32(f, 36 + bytes); fwrite("WAVEfmt ", 1, 8, f);
	put32(f, 16); put16(f, 1); put16(f, ch); put32(f, rate); put32(f, rate * ch * ss);
	put16(f, ch * ss); put16(f, ss * 8); fwrite("data", 1, 4, f); put32(f, bytes);
}

static int realtime;             /* NetSIO: keep to 60 frames a second */

static void frame(void)
{
	static int frames, said = -600;
	++frames;
	if (!libatari800_next_frame(&in) && frames > 60 && frames - said >= 600) {
		/* a JAM (2), a display list error (4): at most every 10 s (the
		 * power-on frames have no display list yet) */
		said = frames;
		printf("CRASH %d frame %d PC %04X\n", libatari800_error_code, frames, CPU_regPC);
		fflush(stdout);
	}
	if (realtime) {
		static struct timespec next;
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		if (!next.tv_sec) next = now;
		next.tv_nsec += 16683333;
		if (next.tv_nsec >= 1000000000) { next.tv_nsec -= 1000000000; next.tv_sec++; }
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
	}
	if (wavf) {
		int n = libatari800_get_sound_buffer_len();
		fwrite(libatari800_get_sound_buffer(), 1, n, wavf);
		wavbytes += n;
	}
	if (filmf)
		fwrite(libatari800_get_screen_ptr(), 1, 384 * 240, filmf);
	if (flogf) {
		struct timespec rt, mt;
		clock_gettime(CLOCK_REALTIME, &rt);
		clock_gettime(CLOCK_MONOTONIC, &mt);
		fprintf(flogf, "%d %lld %lld", libatari800_get_frame_number(),
			(long long)rt.tv_sec * 1000000000LL + rt.tv_nsec, (long long)mt.tv_sec * 1000000000LL + mt.tv_nsec);
		for (int i = 0; i < nflog; i++) fprintf(flogf, " %d", mem()[flog[i]]);
		fputc('\n', flogf);
	}
	if (tracef) {
		fprintf(tracef, "%d,%04x", libatari800_get_frame_number(), ANTIC_dlist);
		for (int i = 0; i < nextra; i++) fprintf(tracef, ",%02x", mem()[extra[i]]);
		fputc('\n', tracef);
	}
}

/* The hardware registers a screen was drawn with. */
static void regs(FILE *f)
{
	fprintf(f, "nmien %02x vdslst %04x\n", ANTIC_NMIEN, mem()[0x200] | mem()[0x201] << 8);
	fprintf(f, "dlist %04x chbase %02x pmbase %02x dmactl %02x chactl %02x prior %02x gractl %02x\n",
		ANTIC_dlist, ANTIC_CHBASE, ANTIC_PMBASE, ANTIC_DMACTL, ANTIC_CHACTL, GTIA_PRIOR, GTIA_GRACTL);
	fprintf(f, "colpm %02x %02x %02x %02x colpf %02x %02x %02x %02x colbk %02x\n",
		GTIA_COLPM0, GTIA_COLPM1, GTIA_COLPM2, GTIA_COLPM3,
		GTIA_COLPF0, GTIA_COLPF1, GTIA_COLPF2, GTIA_COLPF3, GTIA_COLBK);
	fprintf(f, "hposp %02x %02x %02x %02x hposm %02x %02x %02x %02x sizep %02x %02x %02x %02x sizem %02x\n",
		GTIA_HPOSP0, GTIA_HPOSP1, GTIA_HPOSP2, GTIA_HPOSP3, GTIA_HPOSM0, GTIA_HPOSM1, GTIA_HPOSM2, GTIA_HPOSM3,
		GTIA_SIZEP0, GTIA_SIZEP1, GTIA_SIZEP2, GTIA_SIZEP3, GTIA_SIZEM);
	fprintf(f, "audf %02x %02x %02x %02x audc %02x %02x %02x %02x audctl %02x\n",
		POKEY_AUDF[0], POKEY_AUDF[1], POKEY_AUDF[2], POKEY_AUDF[3],
		POKEY_AUDC[0], POKEY_AUDC[1], POKEY_AUDC[2], POKEY_AUDC[3], POKEY_AUDCTL[0]);
}

static void shot(const char *name)
{
	char fn[512], n2[300];
	snprintf(n2, sizeof n2, "%s.scr", name); outpath(fn, sizeof fn, n2);
	FILE *f = fopen(fn, "wb"); fwrite(libatari800_get_screen_ptr(), 1, 384 * 240, f); fclose(f);
	snprintf(n2, sizeof n2, "%s.mem", name); outpath(fn, sizeof fn, n2);
	f = fopen(fn, "wb"); fwrite(mem(), 1, 65536, f); fclose(f);
	snprintf(n2, sizeof n2, "%s.regs", name); outpath(fn, sizeof fn, n2);
	f = fopen(fn, "w"); regs(f); fclose(f);
}

static void state_io(const char *name, int save)
{
	static emulator_state_t st;
	char fn[512], n2[300];
	snprintf(n2, sizeof n2, "%s.a8s", name); outpath(fn, sizeof fn, n2);
	FILE *f = fopen(fn, save ? "wb" : "rb");
	if (!f) { fprintf(stderr, "cannot open %s\n", fn); exit(1); }
	if (save) { libatari800_get_current_state(&st); fwrite(&st, 1, sizeof st, f); }
	else { if (fread(&st, 1, sizeof st, f) != sizeof st) exit(1); libatari800_restore_state(&st); }
	fclose(f);
}

static FILE *reopen(FILE *old, const char *name, const char *mode)
{
	char fn[512];
	if (old) fclose(old);
	if (!strcmp(name, "off")) return NULL;
	outpath(fn, sizeof fn, name);
	FILE *f = fopen(fn, mode);
	if (!f) { perror(fn); exit(1); }
	return f;
}

int main(int argc, char **argv)
{
	int xl = 0, ai = 1;
	const char *netsio = NULL;
	while (ai < argc && argv[ai][0] == '-' && argv[ai][1]) {
		if (!strcmp(argv[ai], "-xl")) xl = 1;
		else if (!strcmp(argv[ai], "-netsio") && ai + 1 < argc) netsio = argv[++ai];
		ai++;
	}
	if (argc - ai < 3) { fprintf(stderr, "usage: asim [-xl] [-netsio PORT] disk.atr script.txt outdir\n"); return 1; }
	char *args[16];
	int na = 0;
	args[na++] = "atari800"; args[na++] = "-config"; args[na++] = "/dev/null";
	if (xl) { args[na++] = "-xl"; args[na++] = "-xl-rev"; args[na++] = "altirra"; }
	else { args[na++] = "-atari"; args[na++] = "-800-rev"; args[na++] = "altirra"; }
	args[na++] = "-nobasic"; args[na++] = "-ntsc";
	realtime = netsio != NULL;
	if (netsio) { args[na++] = "-netsio"; args[na++] = (char *)netsio; args[na++] = "-nopatchall"; }
	if (strcmp(argv[ai], "-")) args[na++] = argv[ai];     /* "-": no disk (FujiNet boots it) */
	args[na] = NULL;
	outdir = argv[ai + 2];
	if (!libatari800_init(na, args)) { fprintf(stderr, "init failed: %s\n", libatari800_error_message()); return 1; }
	libatari800_clear_input_array(&in);
	{	/* the palette, for topng.py */
		char fn[512];
		outpath(fn, sizeof fn, "palette.bin");
		FILE *pf = fopen(fn, "wb");
		for (int i = 0; i < 256; i++) { int c = Colours_table[i]; fputc((c >> 16) & 255, pf); fputc((c >> 8) & 255, pf); fputc(c & 255, pf); }
		fclose(pf);
	}
	signal(SIGTERM, on_term);
	signal(SIGINT, on_term);
	FILE *sf = fopen(argv[ai + 1], "r");
	if (!sf) { perror(argv[ai + 1]); return 1; }
	char line[512], cmd[32], a1[256], a2[256], a3[64];
	while (!stop && fgets(line, sizeof line, sf)) {
		a1[0] = a2[0] = a3[0] = 0;
		if (sscanf(line, "%31s %255s %255s %63s", cmd, a1, a2, a3) < 1 || cmd[0] == '#') continue;
		int v = atoi(a1);
		if (!strcmp(cmd, "wait")) for (int i = 0; i < v && !stop; i++) frame();
		else if (!strcmp(cmd, "joy0")) in.joy0 = v;
		else if (!strcmp(cmd, "trig0")) in.trig0 = v;
		else if (!strcmp(cmd, "start")) in.start = v;
		else if (!strcmp(cmd, "select")) in.select = v;
		else if (!strcmp(cmd, "option")) in.option = v;
		else if (!strcmp(cmd, "key")) in.keychar = !strcmp(a1, "space") ? ' ' : a1[0];
		else if (!strcmp(cmd, "keycode")) in.keycode = v;
		else if (!strcmp(cmd, "nokey")) { in.keychar = 0; in.keycode = 0; }
		else if (!strcmp(cmd, "shot")) shot(a1);
		else if (!strcmp(cmd, "poke")) mem()[strtoul(a1, NULL, 16) & 0xffff] = strtoul(a2, NULL, 16);
		else if (!strcmp(cmd, "peek")) {
			unsigned a = strtoul(a1, NULL, 16), n = a2[0] ? strtoul(a2, NULL, 16) : 1;
			printf("%04x:", a);
			for (unsigned i = 0; i < n; i++) printf(" %02x", mem()[(a + i) & 0xffff]);
			printf("\n");
		}
		else if (!strcmp(cmd, "pokey")) pokeyf = reopen(pokeyf, a1, "w");
		else if (!strcmp(cmd, "gtia")) gtiaf = reopen(gtiaf, a1, "w");
		else if (!strcmp(cmd, "wav")) {
			if (wavf) { fseek(wavf, 0, SEEK_SET); wav_header(wavf, wavbytes); }
			wavf = reopen(wavf, a1, "wb");
			wavbytes = 0;
			if (wavf) wav_header(wavf, 0);
		}
		else if (!strcmp(cmd, "trace")) {
			tracef = reopen(tracef, a1, "w");
			nextra = 0;
			for (char *t = strtok(a2, ","); t && nextra < 64; t = strtok(NULL, ",")) extra[nextra++] = strtoul(t, NULL, 16);
		}
		else if (!strcmp(cmd, "film")) filmf = reopen(filmf, a1, "wb");
		else if (!strcmp(cmd, "filmlog")) {
			flogf = reopen(flogf, a1, "w");
			nflog = 0;
			for (char *t = strtok(a2, ","); t && nflog < 64; t = strtok(NULL, ",")) flog[nflog++] = strtoul(t, NULL, 16);
		}
		else if (!strcmp(cmd, "untilfile")) {
			long maxf = a2[0] ? atol(a2) : 0x7fffffffL, n = 0;
			while (!stop && n < maxf && (n % 60 || access(a1, F_OK))) { frame(); n++; }
			fprintf(stderr, "untilfile %s: %ld frames (frame %d)\n", a1, n, libatari800_get_frame_number());
		}
		else if (!strcmp(cmd, "coldstart")) Atari800_Coldstart();
		else if (!strcmp(cmd, "brkcont")) libatari800_continue_emulation_on_brk(v);
		else if (!strcmp(cmd, "cpu")) {
			printf("frame %d PC %04x A %02x X %02x Y %02x S %02x P %02x stack:", libatari800_get_frame_number(),
				CPU_regPC, CPU_regA, CPU_regX, CPU_regY, CPU_regS, CPU_regP);
			for (int i = CPU_regS + 1; i < 256 && i < CPU_regS + 13; i++) printf(" %02x", mem()[0x100 + i]);
			printf("\n");
		}
		else if (!strcmp(cmd, "savestate")) state_io(a1, 1);
		else if (!strcmp(cmd, "loadstate")) state_io(a1, 0);
		else if (!strcmp(cmd, "untildl")) {
			unsigned dl = strtoul(a1, NULL, 16);
			int maxf = a2[0] ? atoi(a2) : 36000, n = 0;
			while (n++ < maxf && ANTIC_dlist != dl) frame();
			fprintf(stderr, "untildl %04x: %d frames (frame %d)\n", dl, n - 1, libatari800_get_frame_number());
		} else if (!strcmp(cmd, "until") || !strcmp(cmd, "untilne")) {
			unsigned addr = strtoul(a1, NULL, 16), val = strtoul(a2, NULL, 16);
			int maxf = a3[0] ? atoi(a3) : 36000, ne = cmd[5] == 'n', n = 0;
			while (n++ < maxf && ((mem()[addr] == val) == ne)) frame();
			fprintf(stderr, "%s %04x %02x: %d frames (frame %d)\n", cmd, addr, val, n - 1, libatari800_get_frame_number());
		}
		else fprintf(stderr, "unknown command: %s\n", cmd);
	}
	if (wavf) { fseek(wavf, 0, SEEK_SET); wav_header(wavf, wavbytes); fclose(wavf); }
	if (pokeyf) fclose(pokeyf);
	if (gtiaf) fclose(gtiaf);
	if (tracef) fclose(tracef);
	if (filmf) fclose(filmf);
	if (flogf) fclose(flogf);
	libatari800_exit();
	return 0;
}
