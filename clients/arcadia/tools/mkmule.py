#!/usr/bin/env python3
"""mkmule.py -- link the per-page assemblies into build/mule.bin.

usage: mkmule.py BUILD NPAGES OUT

BUILD/mule_pN.flat is page N's assembly, CPU $0000-$2FFF. Every one must
hold the same fixed halves -- fixed A and RAM-L ($0000-$07FF), fixed B and
RAM-H ($2000-$2AFF) -- byte for byte: a difference means fixed code named
something only some pages have. None may hold anything in $1000-$1FFF
(console RAM) or $2B00-$2FFF (the mailbox), and page N's id byte at $0FFF
must be N.

The image (fujinet-firmware pico/arcadia fuji_mailbox.h, protocol 2):
page 0's flat folded to 8K (image 0x0000-0x0FFF = CPU $0000-$0FFF,
0x1000-0x1FFF = $2000-$2FFF), "FUJI" at 0x1CFC, then each page N >= 1's
$0800-$0FFF.
"""
import sys

FIXED = [(0x0000, 0x0800, "fixed A + RAM-L"), (0x2000, 0x2B00, "fixed B + RAM-H")]


def last_used(b, lo, hi):
    for i in range(hi - 1, lo - 1, -1):
        if b[i]:
            return i + 1
    return lo


def main():
    build, npages, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    flats = []
    for n in range(npages):
        f = open("%s/mule_p%d.flat" % (build, n), "rb").read().ljust(0x3000, b"\0")
        if len(f) > 0x3000:
            sys.exit("mkmule: page %d runs past $2FFF" % n)
        if f[0x1000:0x2000].strip(b"\0"):
            sys.exit("mkmule: page %d puts something in console RAM ($1000-$1FFF)" % n)
        if f[0x2B00:0x3000].strip(b"\0"):
            sys.exit("mkmule: page %d puts something in the mailbox ($2B00-$2FFF)" % n)
        if f[0x0FFF] != n:
            sys.exit("mkmule: page %d's id byte is %d" % (n, f[0x0FFF]))
        flats.append(f)
    for lo, hi, what in FIXED:
        for n in range(1, npages):
            if flats[n][lo:hi] != flats[0][lo:hi]:
                at = next(i for i in range(lo, hi) if flats[n][i] != flats[0][i])
                sys.exit("mkmule: page %d's %s differs from page 0's at $%04X" % (n, what, at))
    f0 = flats[0]
    image = bytearray(f0[0x0000:0x1000] + f0[0x2000:0x3000])
    image[0x1CFC:0x1D00] = b"FUJI"
    for n in range(1, npages):
        image += flats[n][0x0800:0x1000]
    open(out, "wb").write(image)
    print("mkmule: %s, %d bytes: fixed A $0000-$%04X (of $05FF), fixed B $2000-$%04X (of $28FF)"
          % (out, len(image), last_used(f0, 0, 0x600) - 1, last_used(f0, 0x2000, 0x2900) - 1))
    for n in range(npages):
        used = last_used(flats[n], 0x0800, 0x0FFF) - 0x0800
        print("  page %2d: %4d bytes" % (n, used))


if __name__ == "__main__":
    main()
