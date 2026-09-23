import sys
m=open(sys.argv[1],'rb').read()
w=lambda a:m[a]|m[a+1]<<8
dl=w(0x230)
print('DL %04x CHBAS %02x PMBASE? colors 2c0..2c8:'%(dl,m[0x2f4]),' '.join('%02x'%m[0x2c0+i] for i in range(9)), 'VDSLST %04x'%w(0x200))
a=dl; line=0
for _ in range(120):
    b=m[a]; mode=b&15
    if mode==0:
        print('%04x blank %d%s'%(a,((b>>4)&7)+1,' DLI' if b&0x80 else '')); a+=1; line+=((b>>4)&7)+1; continue
    if mode==1:
        print('%04x JMP%s %04x'%(a,'VB' if b&0x40 else '',w(a+1))); break
    s='%04x mode %X'%(a,mode)
    if b&0x40: s+=' LMS %04x'%w(a+1); a+=3
    else: a+=1
    if b&0x80: s+=' DLI'
    if b&0x20: s+=' VS'
    if b&0x10: s+=' HS'
    print(s)
