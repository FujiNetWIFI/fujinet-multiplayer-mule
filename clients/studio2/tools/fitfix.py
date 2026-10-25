#!/usr/bin/env python3
"""fitfix.py -- set every RFIT in the sources to its routine's size.

    tools/fitfix.py build/mule.b0.lst [build/mule.b1.lst ...]

Measures each RFIT's routine in the listings as fitcheck.py does, then
rewrites `RFIT n` in src/*.inc and mule.asm to the size measured, finding
the RFIT by the label that follows it. A developer's tool: the build only
checks (fitcheck.py); run this when it complains, then build again.
"""
import glob
import re
import sys

LINE = re.compile(r"^(?:\(\d+\))?\s*\d+/\s*([0-9A-F]+) :(.*)$")
RFIT = re.compile(r"\(MACRO\)\[\d+\]\s+RFIT\s+(\d+)")
CODE = re.compile(r"\s*((?:[0-9A-F]{2} )+)")
LABEL = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*):")
SRCLABEL = re.compile(r"\s([A-Za-z_][A-Za-z0-9_]*):")


def measure(path):
    lines = open(path, errors="replace").read().splitlines()
    fits, code, stops = [], [], []
    for i, raw in enumerate(lines):
        if re.search(r"\w+_SEGEND:", raw):
            stops.append(i)
        m = LINE.match(raw)
        if not m:
            continue
        rest = m.group(2)
        r = RFIT.search(rest)
        if r:
            fits.append(i)
        c = CODE.match(rest)
        if c and not rest.lstrip().startswith("=>"):
            code.append((i, int(m.group(1), 16), len(c.group(1).split())))
    sizes = {}
    for k, i in enumerate(fits):
        stop = fits[k + 1] if k + 1 < len(fits) else len(lines)
        stop = min([s for s in stops if s > i] + [stop])
        body = [c for c in code if i < c[0] < stop]
        if not body:
            continue
        name = next((SRCLABEL.search(l).group(1) for l in lines[i + 1:i + 12] if SRCLABEL.search(l)), None)
        if name:
            used = max(a + n for _, a, n in body) - body[0][1]
            sizes[name] = max(sizes.get(name, 0), used)
    return sizes


def main():
    sizes = {}
    for lst in sys.argv[1:]:
        for k, v in measure(lst).items():
            sizes[k] = max(sizes.get(k, 0), v)
    changed = 0
    for path in sorted(glob.glob("src/*.inc")) + ["mule.asm"]:
        src = open(path).read().splitlines()
        out = list(src)
        for i, line in enumerate(src):
            m = re.match(r"^(\s*RFIT\s+)(\d+)(.*)$", line)
            if not m:
                continue
            name = None
            for nxt in src[i + 1:i + 6]:
                lm = LABEL.match(nxt)
                if lm:
                    name = lm.group(1)
                    break
            if name in sizes and sizes[name] != int(m.group(2)):
                if sizes[name] > 256:
                    print("fitfix: %s: %s is %d bytes, more than a page: split it"
                          % (path, name, sizes[name]), file=sys.stderr)
                out[i] = "%s%d%s" % (m.group(1), sizes[name], m.group(3))
                changed += 1
        if out != src:
            open(path, "w").write("\n".join(out) + "\n")
    print("fitfix: %d RFITs set" % changed)


if __name__ == "__main__":
    main()
