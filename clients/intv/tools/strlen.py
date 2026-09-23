#!/usr/bin/env python3
"""Check (or with --fix, correct) the length prefixes of `DATA n, "text"`
strings in IntyBASIC sources. Strings must not contain double quotes and
must fit the Intellivision GROM (ASCII 32..95)."""
import re
import sys

PAT = re.compile(r'^(\s*DATA\s+)(\d+)(\s*,\s*")([^"]*)(".*)$')


def main():
    fix = "--fix" in sys.argv
    bad = 0
    for fn in [a for a in sys.argv[1:] if not a.startswith("--")]:
        lines = open(fn).read().split("\n")
        for n, ln in enumerate(lines):
            m = PAT.match(ln)
            if not m:
                continue
            text = m.group(4)
            for ch in text:
                if not 32 <= ord(ch) <= 95:
                    print("%s:%d: character %r is not in the GROM" % (fn, n + 1, ch))
                    bad += 1
            if int(m.group(2)) != len(text):
                if fix:
                    lines[n] = m.group(1) + str(len(text)) + m.group(3) + text + m.group(5)
                else:
                    print("%s:%d: length %s, text is %d" % (fn, n + 1, m.group(2), len(text)))
                    bad += 1
        if fix:
            open(fn, "w").write("\n".join(lines))
    sys.exit(1 if bad else 0)


main()
