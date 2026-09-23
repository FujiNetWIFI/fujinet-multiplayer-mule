import sys
rows=[l.rstrip().split(',') for l in open(sys.argv[1])]
h=rows[0]; ix={k:i for i,k in enumerate(h)}
var=sys.argv[2]; show=sys.argv[3].split(',') if len(sys.argv)>3 else []
prev=None;pf=None
for r in rows[1:]:
    if r[0]=='#mark': print('--',r[2],r[1]); continue
    v=r[ix[var]]; f=int(r[0])
    if v!=prev:
        print(f,var,v,'dt',(f-pf) if pf else '', ' '.join('%s=%s'%(k,r[ix[k]]) for k in show))
        prev=v;pf=f
