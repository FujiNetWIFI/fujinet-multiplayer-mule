import sys
rows=[]
for line in open(sys.argv[1]):
    f=line.rstrip('\n').split(',')
    if f[0]=='frame': hdr=f; continue
    if f[0]=='#mark': rows.append(('mark',int(f[1]),f[2])); continue
    rows.append(('row',dict(zip(hdr,f))))
prev=None;start=None;last=None;marks=[]
def out():
    print('%6d..%6d (%5d f) mode %3s dl %04x chb %3s %s'%(start,last,last-start+1,prev[0],int(prev[1]),prev[2],' '.join(marks)))
for r in rows:
    if r[0]=='mark': marks.append('[%s@%d]'%(r[2],r[1])); continue
    d=r[1]; fr=int(d['frame'])
    k=(d['mode'],d['dlist'],d['chbase'])
    if k!=prev:
        if prev: out(); marks=[]
        prev=k;start=fr
    last=fr
out()
