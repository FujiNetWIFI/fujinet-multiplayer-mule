#!/usr/bin/env python3
"""checkcalls.py -- the paging rules, from the sources.

Every page is assembled at $4800, so a call from one page to a label of
another assembles fine and runs whatever the mapped page has there. This
fails the build when:

  - code in a page (src/p<n>*.inc) calls or jumps (PI, JMP, the J* macros,
    TAIL) to a label defined in another page;
  - anything but SELPG and PGPUSH (start.inc) stores to FNBANK.

Fixed code may call into a page (the phase tables, the ISRs): the page is
mapped then, which is the loop's business, not this check's.

usage: checkcalls.py [LISTING]   (the listing is ignored; the sources are read)
"""
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(HERE)


def page_of(path):
    m = re.match(r"p(\d)", os.path.basename(path))
    return int(m.group(1)) if m else None


def main():
    files = sorted(glob.glob(os.path.join(TOP, "src", "*.inc")))
    labels = {}                 # name -> page (None: fixed or data)
    for f in files:
        pg = page_of(f)
        for line in open(f, encoding="latin-1"):
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):", line)
            if m:
                labels.setdefault(m.group(1).upper(), pg)
    bad = 0
    for f in files:
        pg = page_of(f)
        routine = None
        for n, line in enumerate(open(f, encoding="latin-1"), 1):
            code = line.split(";")[0]
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):", code)
            if m:
                routine = m.group(1).upper()
                code = code[m.end():]
            w = code.split()
            if not w:
                continue
            op = w[0].upper()
            if op in ("PI", "JMP", "JZ", "JNZ", "JC", "JNC", "TAIL") and len(w) > 1:
                t = w[1].split(",")[0].upper()
                tp = labels.get(t, "?")
                if pg is not None and tp not in (None, pg, "?"):
                    print("%s:%d: page %d %s %s, a label of page %d"
                          % (os.path.relpath(f, TOP), n, pg, op, t, tp))
                    bad += 1
            if op in ("DCI",) and len(w) > 1 and w[1].upper().startswith("FNBANK"):
                if routine not in ("SELPG", "PGPUSH"):
                    print("%s:%d: %s points DC0 at FNBANK (only SELPG/PGPUSH may)"
                          % (os.path.relpath(f, TOP), n, routine))
                    bad += 1
    if bad:
        sys.exit("checkcalls: %d paging error(s)" % bad)
    print("checkcalls: ok")


if __name__ == "__main__":
    main()
