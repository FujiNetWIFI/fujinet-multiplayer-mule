#!/usr/bin/env python3
"""checkclob.py -- a register read after a call that clobbers it.

    checkclob.py src/*.inc

The cart-RAM writes (CPUT, LPUT, RAMPUT, APUT) load the mailbox's pointer
register into R1 and clobber R0-R2; so does the rest of what a CLOB table
lists. A read of R1 or R2 (LODZ, STRZ from it, COMx/ADDx/... ,R1, an index)
after one, before that register is written again within the same run of
straight-line code, is reported. Heuristic: labels end a run."""
import re
import sys

CLOB = {"CPUT": "012", "LPUT": "012", "RAMPUT": "02", "LPUTR": "012", "APUT": "0123",
        "ZLPUT": "012", "ZRAMPUT": "02", "ZSPRCOL": "0123", "ZUDCQ": "03",
        "ZSAY": "0123", "ZSAYBX": "0123", "ZMUL8": "03", "ZDIV8": "0"}
WRITES = re.compile(r"^\s*(LODI|LODA|LODR|LODZ|STRZ|ADDI|ADDA|ADDZ|SUBI|SUBA|SUBZ|ANDI|ANDA|IORI|IORA|EORI|EORA|RRL|RRR)\b")


def main():
    bad = 0
    for path in sys.argv[1:]:
        dirty = set()
        src = None
        for n, raw in enumerate(open(path), 1):
            line = raw.split(";")[0].rstrip()
            if re.match(r"^[A-Za-z_][A-Za-z0-9_]*:", line):
                dirty = set()
                line = line.split(":", 1)[1]
            s = line.strip()
            if not s:
                continue
            w = s.split()
            op = w[0]
            arg = w[1] if len(w) > 1 else ""
            # what this instruction reads
            reads = set()
            m = re.match(r"^([A-Z]+),R([0-3])$", op)
            if m and m.group(1) not in ("LODI", "LODA", "LODR"):
                reads.add(m.group(2))
            for r in re.findall(r",R([1-3])\b", arg):
                reads.add(r)                    # an index
            if op in ("LODZ", "ADDZ", "SUBZ", "ANDZ", "IORZ", "EORZ", "COMZ") and re.match(r"R[1-3]$", arg):
                reads.add(arg[1])
            hit = reads & dirty
            if hit:
                print("%s:%d: R%s read after %s clobbered it: %s" % (path, n, "/R".join(sorted(hit)), src, s))
                bad = 1
                dirty -= hit
            # what it writes
            if m and WRITES.match(s):
                dirty.discard(m.group(2))
            if op == "STRZ" and re.match(r"R[1-3]$", arg):
                dirty.discard(arg[1])
            name = None
            if op in CLOB:
                name = op
            elif op in ("BSTA,UN", "BSTR,UN", "BCTA,UN", "BCTR,UN") and arg in CLOB:
                name = arg
            elif op == "ZBSR" and arg.lstrip("*") in CLOB:
                name = arg.lstrip("*")
            if name:
                dirty |= set(CLOB[name]) - {"0"}
                src = name
            elif op.startswith(("BSTA", "BSTR", "ZBSR", "BSXA")):
                dirty = set()           # another call: it may return values in them
            if op.startswith(("RETC", "BCTA,UN", "BCTR,UN", "ZBRR", "BXA")):
                dirty = set()
    sys.exit(bad)


if __name__ == "__main__":
    main()
