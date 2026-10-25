#!/usr/bin/env python3
"""fixreach.py -- rebuild until AS stops reporting "jump distance too big",
turning each such relative branch (BCTR BCFR BSTR BSFR BRNR BDRR BIRR) into
its absolute form. A development aid; the result is ordinary source."""
import re
import subprocess
import sys

REL = {"BCTR": "BCTA", "BCFR": "BCFA", "BSTR": "BSTA", "BSFR": "BSFA",
       "BRNR": "BRNA", "BDRR": "BDRA", "BIRR": "BIRA", "BSNR": "BSNA"}
for _ in range(200):
    r = subprocess.run(["./build.sh"], capture_output=True, text=True)
    out = r.stdout + r.stderr
    hits = re.findall(r"(\S+\.inc|\S+\.asm)\((\d+)\): error: jump distance too big", out)
    if not hits:
        print(out)
        sys.exit(r.returncode)
    for f, ln in set(hits):
        path = f if "/" in f else ("src/" + f if not f.endswith(".asm") else f)
        lines = open(path).read().split("\n")
        i = int(ln) - 1
        for k, v in REL.items():
            if re.search(r"\b%s\b" % k, lines[i]):
                lines[i] = re.sub(r"\b%s\b" % k, v, lines[i])
                print("fixreach: %s:%s %s -> %s" % (path, ln, k, v))
                break
        else:
            print("fixreach: %s:%s: nothing to widen: %s" % (path, ln, lines[i]))
            sys.exit(1)
        open(path, "w").write("\n".join(lines))
