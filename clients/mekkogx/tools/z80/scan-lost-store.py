#!/usr/bin/env python3
# scan-lost-store.py FILE.asm... -- look for zsdcc -SO3's lost store.
#
# zsdcc 4.x at -SO3 can turn `v |= n;` on a local kept in the frame into
# `ld a,(ix+d)` / `or a,n` with no `ld (ix+d),a` after it (src/sms/input.c,
# src/h89/gfx.c found it the hard way): the result is dropped at the next
# label or when A is overwritten. This prints every load-op-immediate on a
# frame byte that isn't stored back before A is used up, for a person to
# judge.
import re
import sys

LOAD = re.compile(r"\s+ld\s+a,\((i[xy][+-]\d+)\)")
OP = re.compile(r"\s+(or|and|xor|add|adc)\s+a,\s*(0x[0-9a-fA-F]+|\d+)\s*$")
bad = 0
for path in sys.argv[1:]:
    lines = open(path).read().split("\n")
    for i in range(len(lines) - 2):
        m = LOAD.match(lines[i])
        if not m or not OP.match(lines[i + 1]):
            continue
        slot = m.group(1)
        nxt = lines[i + 2].strip()
        stored = nxt.replace(" ", "").startswith("ld(%s),a" % slot.replace(" ", ""))
        # (a compare's flags going to a conditional jump are its use)
        killed = re.match(r"(ld\s+a,|xor\s+a,\s*a|jp\s+[^,]*$|jr\s+[^,]*$|ret\s*$|call)", nxt)
        if not stored and (nxt.endswith(":") or killed):
            bad += 1
            print("%s:%d: %s / %s / %s" % (path, i + 1, lines[i].strip(), lines[i + 1].strip(), nxt))
sys.exit(1 if bad else 0)
