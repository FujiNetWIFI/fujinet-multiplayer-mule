#!/usr/bin/env python3
"""msxsim.py -- drive the MSX build in openMSX from a scenario.

usage: msxsim.py ROM MAPFILE SCENARIO OUTDIR

Runs the ROM (r2r/msxrom/mule.rom, ASCII8) in openMSX with no window, as
fast as it goes, waits for the game's title, then plays the scenario, one
command a line; times are in frames of the game's clock. The same scenario
language as tools/adam/adamsim (and a2sim, cocosim):

    test PHASE [ARG [PEVENT]]   offline test mode (src/test.c)
    wait N                      let N frames pass
    snap NAME                   save OUTDIR/NAME.png (drawn from VRAM by
                                tools/tmsshot.py) and NAME.vram
    hold DIRS                   hold directions: any of u d l r (or 'none')
    press                       tap the button
    key CHAR|NUM                a key (RETURN = 13, ESC = 27)
    peek SYMBOL [N]             print N bytes at a symbol (from the map)
    poke SYMBOL VALUE
    type TEXT                   type on the real keyboard (openMSX's type)

MSXSIM_MACHINE picks the machine (C-BIOS_MSX1 by default). Against a
FujiNet: MSXSIM_FUJINET=1 adds openMSX's FujiNet extension (fujinet-pc-msx
listening on 1985 first); the ROM is then the FujiNet's to run, and ROM
may be "-".
"""
import os
import re
import subprocess
import sys

OPENMSX = os.environ.get("OPENMSX", "openmsx")
HERE = os.path.dirname(os.path.abspath(__file__))
DIRBITS = {"d": 1, "r": 2, "u": 4, "l": 8}


def symbols(mapfile):
    syms = {}
    for name, addr in re.findall(r"^(\S+)\s+=\s+\$([0-9A-Fa-f]{4})", open(mapfile).read(), re.M):
        syms.setdefault(name, int(addr, 16))
    return syms


def main():
    rom, mapfile, scen, out = sys.argv[1:5]
    out = os.path.abspath(out)
    os.makedirs(out, exist_ok=True)
    syms = symbols(mapfile)
    sym = lambda n: syms["_" + n] if "_" + n in syms else syms[n]
    steps = []
    for line in open(scen):
        line = line.split("#")[0].strip()
        if not line:
            continue
        w = line.split()
        c = w[0]
        if c == "test":
            a = [int(x) for x in w[1:]] + [0, 0, 0]
            steps.append("test %d %d %d %d" % (sym("test_hook"), a[0] + 1, a[1], a[2]))
        elif c == "wait":
            steps.append("wait %d" % int(w[1]))
        elif c == "snap":
            steps.append("snap %s" % os.path.join(out, w[1]))
        elif c == "hold":
            bits = 0 if w[1] == "none" else sum(DIRBITS[ch] for ch in w[1])
            steps += ["poke %d 1" % sym("inp_hook"), "poke %d %d" % (sym("inp_hook_dir"), bits)]
        elif c == "press":
            steps += ["poke %d 1" % sym("inp_hook"), "poke %d 1" % sym("inp_hook_btn"), "wait 3",
                      "poke %d 0" % sym("inp_hook_btn")]
        elif c == "key":
            k = int(w[1]) if w[1].isdigit() and len(w[1]) > 1 else ord(w[1][0])
            steps += ["poke %d 1" % sym("inp_hook"), "poke %d %d" % (sym("inp_hook_key"), k), "wait 2"]
        elif c == "peek":
            steps.append("peek %s %d %d" % (w[1], sym(w[1]), int(w[2]) if len(w) > 2 else 1))
        elif c == "poke":
            steps.append("poke %d %d" % (sym(w[1]), int(w[2], 0)))
        elif c == "type":
            steps.append("type {%s}" % line.split(None, 1)[1].replace("\\n", "\r"))
        else:
            sys.exit("unknown command: " + line)

    tcl = """
set steps {%s}
set fc %d
set limit %d
set i 0
set started 0
set asked 0
set wait_until -1
set frames 0
proc vram {path} {
  set f [open $path wb]
  fconfigure $f -translation binary
  puts -nonewline $f [debug read_block VRAM 0 16384]
  close $f
}
proc tick {} {
  global steps fc limit i started asked wait_until frames
  incr frames
  if {$frames > $limit} { puts stderr "MSXSIM timeout"; exit }
  set now [peek16 $fc]
  if {!$started} {
    set s [lindex $steps 0]
    if {[lindex $s 0] eq "test"} {
      set a [lindex $s 1]
      if {$asked && [peek $a] == 0xA5} { set started 1; set i 1
      } elseif {!$asked && [peek $a] == 0x3C && [peek [expr {$a + 1}]] == 0xC3} {
        poke [expr {$a + 1}] [lindex $s 2]; poke [expr {$a + 2}] [lindex $s 3]
        poke [expr {$a + 3}] [lindex $s 4]; poke $a 0x5A
        set asked 1
      }
      after time 0.01 tick; return
    } elseif {$frames > 1000} { set started 1 } else { after time 0.01 tick; return }
  }
  while {$i < [llength $steps]} {
    set s [lindex $steps $i]
    switch -- [lindex $s 0] {
      wait {
        if {$wait_until < 0} { set wait_until [expr {($now + [lindex $s 1]) & 0xFFFF}] }
        if {(($now - $wait_until) & 0xFFFF) < 0x8000} { set wait_until -1; incr i } else { after time 0.01 tick; return }
      }
      poke { poke [lindex $s 1] [lindex $s 2]; incr i }
      snap {
        vram "[lindex $s 1].vram"
        puts stderr "SNAP [lindex $s 1] game frame $now msx frame $frames"
        incr i; after time 0.01 tick; return
      }
      peek {
        set t {}
        for {set k 0} {$k < [lindex $s 3]} {incr k} { lappend t [format %%02X [peek [expr {[lindex $s 2] + $k}]]] }
        puts stderr "PEEK [lindex $s 1] [join $t { }]  (msx frame $frames)"
        incr i
      }
      type { type [lindex $s 1]; incr i }
    }
  }
  exit
}
after time 0.01 tick
""" % ("\n".join("{%s}" % s for s in steps), sym("frame_count"),
       int(os.environ.get("MSXSIM_SECONDS", "600")) * 100)
    tclfile = os.path.join(out, "scenario.tcl")
    open(tclfile, "w").write(tcl)
    cmd = [OPENMSX, "-machine", os.environ.get("MSXSIM_MACHINE", "C-BIOS_MSX1")]
    if os.environ.get("MSXSIM_FUJINET"):
        cmd += ["-ext", "FujiNet"]
    if rom != "-":
        cmd += ["-cart", os.path.abspath(rom), "-romtype", "ASCII8"]
    cmd += ["-command", "set renderer none; set throttle off; set mute on",
            "-script", tclfile]
    if os.environ.get("MSXSIM_FUJINET"):       # a real server: real time
        cmd[cmd.index("-command") + 1] = "set renderer none; set mute on"
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    r = subprocess.run(cmd, capture_output=True, text=True, env=env,
                       timeout=int(os.environ.get("MSXSIM_SECONDS", "600")) + 300)
    if os.environ.get("MSXSIM_VERBOSE"):
        print(r.stdout, r.stderr)
    for line in (r.stdout + r.stderr).splitlines():
        if line.startswith(("PEEK", "SNAP", "MSXSIM")):
            print(line)
            if line.startswith("SNAP"):
                path = line.split()[1]
                subprocess.run([sys.executable, os.path.join(HERE, "..", "..", "tmsshot.py"),
                                path + ".vram", path + ".png"])
                print(path + ".png")


if __name__ == "__main__":
    main()
