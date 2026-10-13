#!/usr/bin/env python3
# trimcom.py MAP COM -- cut a z88dk CP/M .COM short at its BSS.
#
# z88dk's +cpm writes the zeros of the uninitialised variables into the
# .COM (the CRT clears them anyway, CRT_INITIALIZE_BSS=1): 5K more for the
# H17 to read at every start. Everything after __BSS_head must be BSS
# (__BSS_END_tail = __tail), or this says so and leaves the file.
import re
import sys

syms = {}
for line in open(sys.argv[1]):
    m = re.match(r'(\S+)\s+= \$([0-9A-F]+) ;', line)
    if m:
        syms.setdefault(m.group(1), int(m.group(2), 16))
head, end, tail = syms['__BSS_head'], syms['__BSS_END_tail'], syms['__tail']
if end != tail:
    sys.exit('trimcom.py: something follows the BSS: left as it is')
data = open(sys.argv[2], 'rb').read()
keep = head - 0x100
if any(data[keep:]):
    sys.exit('trimcom.py: the BSS in the file is not all zero: left as it is')
open(sys.argv[2], 'wb').write(data[:keep])
print('trimcom.py: %d bytes (%d of BSS cut)' % (keep, len(data) - keep))
