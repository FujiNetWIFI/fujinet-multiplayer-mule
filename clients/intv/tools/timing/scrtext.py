#!/usr/bin/env python3
"""Print the text lines of an Atari M.U.L.E. memory dump (.mem from cap.c).

Walks the ANTIC display list and decodes the character modes (2, 6, 7) to
ASCII. Used to find which screen / event the original game is showing.
usage: scrtext.py dump.mem [...]
"""
import sys

WIDTH = {2: 40, 3: 40, 4: 40, 5: 40, 6: 20, 7: 20}


def text(m):
    w = lambda a: m[a] | m[a + 1] << 8
    a = w(0x230)
    addr = 0
    out = []
    for _ in range(200):
        b = m[a]
        mode = b & 15
        if mode == 0:
            a += 1
            continue
        if mode == 1:
            break
        if b & 0x40:
            addr = w(a + 1)
            a += 3
        else:
            a += 1
        if mode in (2, 6, 7):
            n = WIDTH[mode]
            s = ''
            for c in m[addr:addr + n]:
                c &= 0x7F if mode == 2 else 0x3F
                if c < 64:
                    s += chr(c + 32)
                elif c >= 96:
                    s += chr(c)
                else:
                    s += '.'
            s = s.rstrip()
            if s.strip():
                out.append(s)
        addr += WIDTH.get(mode, 40)
    return out


if __name__ == '__main__':
    for fn in sys.argv[1:]:
        m = open(fn, 'rb').read()
        print(fn, '|', ' / '.join(x.strip() for x in text(m)))
