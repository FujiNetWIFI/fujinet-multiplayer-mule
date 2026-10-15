#!/usr/bin/env python3
"""checkacc.py -- the F8 trap that bit three times: PI and JMP destroy the
accumulator (they move the address through it), and ENTER's PUSHK does too.

So a label reached by PI, JMP or a long-branch macro (JZ JNZ JC JNC, which
end in JMP) must not read A before it writes it, and nor may the first
instruction after an ENTER. Reading A means: arithmetic and logic on it,
ST, LR x,A, OUTS, shifts, INC, COM, LNK, ADC, STB. Flags do survive PI and
JMP, so a branch first is fine. And since LEAVE (the BIOS POPK) does change
the flags, no caller may branch on flags right after an ENTER'd routine.

usage: checkacc.py FILE... (the sources: src/*.inc, mule.asm)
"""
import re
import sys

READS = {"AI", "CI", "NI", "OI", "XI", "AS", "ASD", "XS", "NS", "AM", "AMD",
         "NM", "OM", "XM", "CM", "ST", "OUTS", "OUT", "SL", "SR", "INC",
         "COM", "LNK", "ADC", "STB"}
WRITES = {"LI", "LIS", "CLR", "LM", "LDB", "LDW", "SETB", "INCB", "DECB",
          "PI", "JMP", "BR", "POP", "PK", "TAIL", "FAR", "CALLPH", "STW",
          "INS", "IN", "PRTXT", "LISU", "LISL", "DS", "TICKRD"}
NEUTRAL = {"DCI", "XDC", "NOP", "DI", "EI"}   # look past these
BRANCH = re.compile(r"^(BZ|BNZ|BC|BNC|BP|BM|BNO|BT|BF|BR7|BNOV|BEQI|BNEI|BLTI|BGEI)$")


def parse(files):
    lines, labels = [], {}
    for f in files:
        for n, raw in enumerate(open(f, encoding="latin-1"), 1):
            text = raw.split(";")[0].rstrip()
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):\s*(.*)$", text)
            if m:
                labels.setdefault(m.group(1).upper(), len(lines))
                text = " " + m.group(2)
            if not text.strip():
                if m:
                    lines.append((f, n, "", ""))
                continue
            if not text[0].isspace():        # EQU, MACRO and the like
                continue
            w = text.split(None, 1)
            lines.append((f, n, w[0].upper(), w[1].strip() if len(w) > 1 else ""))
    return lines, labels


def reads_a(op, args):
    if op == "LR":
        dst, _, src = args.replace(" ", "").upper().partition(",")
        if src == "A":
            return True
        return False                      # LR A,x writes it; LR K,P, LR DC,Q ... don't care
    return op in READS


def first_use(lines, i):
    """'read', 'write' or None for the code from line i on."""
    after_enter = False
    for f, n, op, args in lines[i:i + 12]:
        if not op:
            continue
        if op == "ENTER":
            after_enter = True
            continue
        if op in NEUTRAL:
            continue
        if BRANCH.match(op):
            return None
        if reads_a(op, args):
            return ("read", f, n, op, args, after_enter)
        if op == "LR" or op in WRITES:
            return None
        return None                       # unknown macro: assume it loads A
    return None


def main():
    lines, labels = parse(sys.argv[1:])
    bad = 0
    seen = set()
    for f, n, op, args in lines:
        target = None
        if op in ("PI", "JMP", "JZ", "JNZ", "JC", "JNC"):
            target = args.split(",")[0].strip().upper()
        if not target or target not in labels or (target, op == "PI") in seen:
            continue
        seen.add((target, op == "PI"))
        r = first_use(lines, labels[target])
        if r:
            _, tf, tn, top, targs, enter = r
            why = "after ENTER" if enter else "reached by " + op
            print("%s:%d: %s reads A (%s %s) %s, which destroys it (from %s:%d)"
                  % (tf, tn, target, top, targs, why, f, n))
            bad += 1
    # an ENTER'd routine reached only by fall-through still loses A
    for i, (f, n, op, args) in enumerate(lines):
        if op == "ENTER":
            r = first_use(lines, i + 1)
            if r and (f, n) not in seen:
                _, tf, tn, top, targs, _ = r
                print("%s:%d: reads A (%s %s) right after ENTER, which destroys it"
                      % (tf, tn, top, targs))
                bad += 1
                seen.add((f, n))
    # LEAVE runs the BIOS POPK, which changes the flags: a caller can't
    # branch on flags an ENTER'd routine set (return a value in A instead)
    def entered(lbl):
        i = labels.get(lbl)
        if i is None:
            return False
        for _, _, op, _ in lines[i:i + 3]:
            if op:
                return op == "ENTER"
        return False
    for i, (f, n, op, args) in enumerate(lines):
        if op != "PI":
            continue
        target = args.split()[0].upper()
        for _, _, op2, args2 in lines[i + 1:i + 3]:
            if not op2:
                continue
            if (BRANCH.match(op2) or op2 in ("JZ", "JNZ", "JC", "JNC")) and entered(target):
                print("%s:%d: branches (%s) on flags from %s, whose LEAVE changed them"
                      % (f, n, op2, target))
                bad += 1
            break
    if bad:
        sys.exit("checkacc: %d place(s) read A or flags that PI/JMP/ENTER/LEAVE destroyed" % bad)
    print("checkacc: ok")


if __name__ == "__main__":
    main()
