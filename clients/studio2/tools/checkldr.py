#!/usr/bin/env python3
"""checkldr.py -- D given a value, then lost to a pointer load before its use.

    tools/checkldr.py src/*.inc

LDR and SPTR load their register through D (LDI, PHI, LDI, PLO), so
    GETV  SEAT
    LDR   R9, FIELD
    FCALL pl_byte          ; D is LO(FIELD), not the seat
hands the routine the wrong value. Flags a line that leaves a value in D
followed at once by one of them and then by a call, jump or store.
"""
import re
import sys

SETS_D = re.compile(r"^\s*(?:\w+:)?\s*(GETV|GLO|GHI|LDN|LDA|LDX|LDXA|LDI|ADI|ADCI|SMI|SMBI|SDI|ANI|ORI|XRI|"
                    r"SHL|SHR|SHLC|SHRC|ADD|ADC|SM|SMB|SD|AND|OR|XOR|PUTV)\b", re.I)
CLOBBER = re.compile(r"^\s*(LDR|SPTR)\b", re.I)
USE = re.compile(r"^\s*(FCALL|FJMP|PD|PUTV|STR|STXD|PLO|PHI|BCALL)\b", re.I)


def code(line):
    return line.split(";")[0].rstrip()


def main():
    bad = 0
    for path in sys.argv[1:]:
        lines = [code(l) for l in open(path)]
        for i in range(len(lines) - 2):
            if SETS_D.match(lines[i]) and CLOBBER.match(lines[i + 1]) and USE.match(lines[i + 2]):
                if lines[i + 2].split()[0].upper() in ("PLO", "PHI") and lines[i + 1].split()[0].upper() == "SLD":
                    continue
                print("checkldr: %s:%d: D set, then %s before it is used" % (path, i + 1, lines[i + 1].split()[0]),
                      file=sys.stderr)
                bad = 1
    if not bad:
        print("checkldr: no value lost to a pointer load")
    return bad


if __name__ == "__main__":
    sys.exit(main())
