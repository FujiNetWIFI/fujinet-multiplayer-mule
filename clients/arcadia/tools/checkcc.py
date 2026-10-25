#!/usr/bin/env python3
"""checkcc.py -- a conditional branch right after a LODI (or DCOL, one).

    checkcc.py src/*.inc

LODI sets the condition code from its constant, so a conditional branch
(or RETC) straight after one tests the constant, never the compare before
it: always the slip of a LODI put between a COM and its branch."""
import re
import sys

COND = re.compile(r"^(BC[TF][AR]|RETC|BS[TF][AR]|BR[NI][AR]),(EQ|GT|LT)\b")


def main():
    bad = 0
    for path in sys.argv[1:]:
        prev = None
        for n, raw in enumerate(open(path), 1):
            line = raw.split(";")[0].rstrip()
            lab = re.match(r"^[A-Za-z_][A-Za-z0-9_]*:", line)
            if lab:
                prev = None                 # a label: reached from elsewhere too
                line = line[lab.end():]
            s = line.strip()
            if not s:
                continue
            op = s.split()[0]
            if COND.match(op) and prev and (prev.startswith("LODI,") or prev == "DCOL"):
                print("%s:%d: %s tests the constant of the LODI before it" % (path, n, s))
                bad = 1
            prev = op
    sys.exit(bad)


if __name__ == "__main__":
    main()
