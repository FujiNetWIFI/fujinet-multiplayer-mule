#!/usr/bin/env python3
# Fixes up the HDOS disk image appmake +hdos makes for h89-hdos.mk.
#
# appmake writes a file's last-sector index (directory byte 18) counting from
# 0, where HDOS counts from 1, so HDOS sees the file a sector short -- or a
# whole group short when the count is even. Set it from the file itself:
#
#   hdos-dir-fixup.py IMAGE NAME.EXT FILE
#
# The label is sector 9: the first directory block at byte 3, sectors a
# group at byte 7. A block is two sectors of 22 23-byte entries.
import sys

img, name, src = sys.argv[1:4]
nam, _, ext = name.upper().partition('.')
want = nam.encode().ljust(8, b'\0') + ext.encode().ljust(3, b'\0')
sectors = (len(open(src, 'rb').read()) + 255) // 256

with open(img, 'r+b') as f:
    f.seek(9 * 256)
    lab = f.read(256)
    blk, spg = lab[3] | lab[4] << 8, lab[7]
    while blk:
        f.seek(blk * 256)
        d = bytearray(f.read(512))
        for e in range(0, 22 * 23, 23):
            if d[e] == 0xFE:
                break
            if bytes(d[e:e + 11]) == want:
                d[e + 18] = (sectors - 1) % spg + 1
                f.seek(blk * 256)
                f.write(d)
                sys.exit(0)
        blk = d[510] | d[511] << 8
sys.exit('hdos-dir-fixup: no %s in %s' % (name, img))
