#!/usr/bin/env python3
"""checkdepth.py -- the deepest subroutine chain of each page's assembly.

    checkdepth.py [--max N] [--show] build/mule_p*.lst

The 2650's return-address stack is 8 entries on chip, and there is no RAM
stack: a ninth call silently overwrites the oldest return, and the program
later "returns" somewhere it never came from. So every chain from the main
loop must stay within 8.

Works on the AS listing (macros expanded: a CPUT's BSTA RAMPUT counts).
Each label is a node. CALL edges: BSTA, BSTR, ZBSR (through the zero-page
vector, resolved from its DWBE), and BSXA. FLOW edges, free: conditional
and unconditional branches, ZBRR, BXA, and falling into the next label.
PGCALL's BXA WIN,R3 flows to every entry of the page in this assembly;
PGFAR always calls the core page, so it costs one call plus the deepest
core entry (from the core page's own listing). The depth of a node
is the most calls on any path from it; the roots are START and MAINLP.
A call inside a cycle is recursion and fails the build.
"""
import re
import sys as _s
_s.setrecursionlimit(20000)
import sys

LINE = re.compile(r"^\(\d+\)\s+\d+/\s*([0-9A-F]+) : ((?:[0-9A-F]{2} )*)\s*(.*)$")
LABEL = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*):")
OPND = re.compile(r"^\s*([A-Z]+)(?:,([A-Z0-9]+))?\s+(\*?)([A-Za-z_$][A-Za-z0-9_$+]*)")
CALLS = {"BSTA", "BSTR", "BSFA", "BSFR", "BSNA", "BSNR"}
FLOWS = {"BCTA", "BCTR", "BCFA", "BCFR", "BDRA", "BDRR", "BIRA", "BIRR", "BRNA", "BRNR", "BXA"}
ENDS = {"RETC,UN", "RETE,UN", "BCTA,UN", "BCTR,UN", "ZBRR", "BXA"}


def parse(path):
    nodes, order, edges = {}, [], {}
    zvec = {}           # vector label -> target
    cur = None
    ended = False
    entries = []
    r1 = r3 = None      # the page and entry a PGCALL/PGFAR is about to take
    for raw in open(path, errors="replace"):
        m = LINE.match(raw)
        if not m:
            continue
        addr, byts, src = int(m.group(1), 16), m.group(2).strip(), m.group(3)
        src = src.split(";")[0].rstrip()
        if "=>FALSE" in raw or "=>TRUE" in raw:
            continue
        src = re.sub(r"^\(MACRO\)\[\d+\]\s*", "", src)     # a macro's call line
        if 0x0800 <= addr < 0x0840 and byts and src.split()[:1] == ["BCTA,UN"] \
                and len(entries) < 12:
            entries.append((addr - 0x0800, src.split()[1]))     # the page's entry table
        lm = LABEL.match(src)
        if lm:
            lab = lm.group(1)
            if cur is not None and not ended:
                edges.setdefault(cur, []).append(("flow", lab))
            cur = lab
            r1 = r3 = None
            if lab not in nodes:
                nodes[lab] = addr
                order.append(lab)
            ended = False
            src = src[lm.end():]
            if src.strip().startswith("DWBE"):
                t = src.split()[1]
                zvec[lab] = t
        if byts and ended and not lm:
            cur = None          # unlabelled after a jump or return: not reached
        if not byts or cur is None:
            continue
        s = src.strip()
        if not s:
            continue
        w = s.split()
        op = w[0]
        mn = op.split(",")[0]
        tgt = None
        if len(w) > 1:
            tgt = w[1].lstrip("*").split(",")[0]
        b = byts.split()
        if op == "LODI,R1":
            r1 = int(b[1], 16)
        elif op == "LODI,R3":
            r3 = int(b[1], 16)
        elif ",R1" in op and mn not in ("STRA", "STRR", "COMI", "COMA", "COMR", "TMI") \
                or s.replace(" ", "") == "STRZR1":
            r1 = None
        elif ",R3" in op and mn not in ("STRA", "STRR", "COMI", "COMA", "COMR", "TMI") \
                or s.replace(" ", "") == "STRZR3":
            r3 = None
        if mn in ("BSTA", "BSTR", "BCTA", "BCTR") and tgt in ("PGCALL", "PGFAR"):
            kind = "call" if mn.startswith("BS") else "flow"
            edges.setdefault(cur, []).append((kind, "%s:%s:%s" % (tgt, r1, r3)))
        elif mn in CALLS and tgt:
            edges.setdefault(cur, []).append(("call", tgt))
        elif mn == "ZBSR" and tgt:
            edges.setdefault(cur, []).append(("zcall", tgt))
        elif mn == "ZBRR" and tgt:
            edges.setdefault(cur, []).append(("zflow", tgt))
        elif mn == "BSXA":
            edges.setdefault(cur, []).append(("dispatch", None))
        elif mn == "BXA" and tgt == "WIN":
            edges.setdefault(cur, []).append(("dispatchflow", None))
        elif mn in FLOWS and tgt:
            edges.setdefault(cur, []).append(("flow", tgt))
        ended = op in ENDS or mn in ("ZBRR", "BXA") or op.startswith("RETC,UN") \
            or op == "BCTA,UN" or op == "BCTR,UN"
    return nodes, edges, zvec, entries


def resolve(edges, zvec, entries, nodes, page, phase, core, dcore):
    """Page-call sites become nodes "PGCALL:r1:r3": they call (or flow to)
    entry r3 of this page when r1 is this page (an unknown r1, PHPAGE's, is
    any phase page), and nothing otherwise: that page's own assembly counts
    them. PGFAR is a call into the core page: one more, and its entry's
    depth (dcore, from the core listing)."""
    g = {}
    for n, es in edges.items():
        out = []
        for kind, t in es:
            if t and t.startswith(("PGCALL:", "PGFAR:")):
                fn, a1, a3 = t.split(":")
                a1 = None if a1 == "None" else int(a1)
                a3 = None if a3 == "None" else int(a3)
                if fn == "PGFAR":
                    ents = dcore.get(a1, {}) if a1 is not None else {}
                    d = max(ents.values() or [0]) if a3 is None else ents.get(a3, 0)
                    g.setdefault(t, [("call", "__CORE%d__" % d)])
                    out.append((kind, t))
                    continue
                if a1 == page or (a1 is None and page in phase):
                    tg = [("flow", e) for off, e in entries if a3 is None or off == a3]
                    g.setdefault(t, tg)
                    out.append((kind, t))
                continue
            if kind in ("dispatch", "dispatchflow"):
                for off, e in entries:
                    out.append(("call" if kind == "dispatch" else "flow", e))
                continue
            if kind in ("zcall", "zflow"):
                t = zvec.get(t)
                kind = "call" if kind == "zcall" else "flow"
            if kind in ("dispatch", "dispatchflow"):
                for e in entries:
                    out.append(("call" if kind == "dispatch" else "flow", e))
                continue
            if t in nodes:
                out.append((kind, t))
        g[n] = out
    return g


def depth(g, root, memo, stack):
    """Most calls on a path from root; cycles through flow edges are fine,
    a call back into the stack is recursion."""
    if root in memo:
        return memo[root][0]
    if root in stack:
        return 0           # a flow cycle: no more calls than once round
    stack.add(root)
    best, via = 0, None
    for kind, t in g.get(root, []):
        d = depth(g, t, memo, stack)
        if kind == "call":
            d += 1
        if d > best:
            best, via = d, (kind, t)
    stack.discard(root)
    memo[root] = (best, via)
    return best


def chain(g, memo, root):
    out = [root]
    seen = set()
    while root in memo and memo[root][1] and root not in seen:
        seen.add(root)
        kind, t = memo[root][1]
        out.append(("+" if kind == "call" else "") + t)
        root = t
    return out


def pages():
    import os
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "pages.inc")
    pg = {}
    for ln in open(path):
        m = re.match(r"\s*(PG_[A-Z0-9_]+)\s+EQU\s+(\d+)", ln)
        if m:
            pg[m.group(1)] = int(m.group(2))
    return pg


def graph(path, phase, core, dcore):
    m = re.search(r"_p(\d+)\.lst$", path)
    page = int(m.group(1)) if m else None
    nodes, edges, zvec, entries = parse(path)
    return nodes, resolve(edges, zvec, entries, nodes, page, phase, core, dcore), entries


def main():
    args = sys.argv[1:]
    mx, show = 8, False
    if args and args[0] == "--max":
        mx = int(args[1])
        args = args[2:]
    if args and args[0] == "--show":
        show = True
        args = args[1:]
    bad = 0
    pg = pages()
    phase = set(range(pg["PG_LOBBY"], pg["PG_PROD"] + 1))   # what PHPAGE can be
    # every page's entries, as far-called (PGFAR) from another: their depth,
    # settled over a few passes (a far-called page may far-call the core)
    dcore = {}
    for _ in range(3):
        nxt = {}
        for path in args:
            m = re.search(r"_p(\d+)\.lst$", path)
            if not m:
                continue
            nodes, g, entries = graph(path, phase, None, dcore)
            memo = {"__CORE%d__" % d: (d, None) for d in range(0, 20)}
            nxt[int(m.group(1))] = {off: depth(g, e, dict(memo), set()) for off, e in entries}
        dcore = nxt
    if show:
        print("checkdepth: far entries: %s" % {k: v for k, v in dcore.items() if v})
    seeds = {"__CORE%d__" % d: (d, None) for d in range(0, 20)}
    for path in args:
        nodes, g, entries = graph(path, phase, None, dcore)
        for root in ("MAINLP", "MAININIT"):
            if root not in nodes:
                continue
            memo = dict(seeds)
            d = depth(g, root, memo, set())
            if d > mx or show:
                print("checkdepth: %s: %s %d: %s" % (path, root, d,
                      " ".join(chain(g, memo, root))))
            if d > mx:
                bad = 1
    sys.exit(bad)


if __name__ == "__main__":
    main()
