#!/usr/bin/env python3
"""checkfixed.py -- every bank's assembly must agree outside the window.

    tools/checkfixed.py build/mule.b0.p build/mule.b1.p ...

Each bank is mule.asm assembled again with BANK set; only $C400-$CFFF may
differ. A conditional that leaks into fixed code would make a call from one
bank land somewhere else in another.
"""
import os
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(__file__))
from mkst2 import read_p  # noqa: E402


def fixed(mem):
    return {a: b for a, b in mem.items() if not 0xC400 <= a <= 0xCFFF}


def main():
    base = fixed(read_p(sys.argv[1]))
    bad = 0
    for path in sys.argv[2:]:
        other = fixed(read_p(path))
        diff = sorted(a for a in set(base) | set(other) if base.get(a) != other.get(a))
        if diff:
            bad = 1
            print("checkfixed: %s differs from %s outside the window at %s"
                  % (path, sys.argv[1], " ".join("$%04X" % a for a in diff[:8])), file=sys.stderr)
    if not bad:
        print("checkfixed: %d banks, fixed code the same in each (%d bytes)" % (len(sys.argv) - 1, len(base)))
    return bad


if __name__ == "__main__":
    sys.exit(main())
