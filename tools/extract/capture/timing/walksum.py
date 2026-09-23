import sys
rows=[l.rstrip().split(',') for l in open(sys.argv[1])]
h=rows[0]; ix={k:i for i,k in enumerate(h)}
keys=sys.argv[2].split(',') if len(sys.argv)>2 else ['mode','hp0','p0y','dlist']
prev=None; start=None
def fmt(r): return ' '.join('%s=%s'%(k, r[ix[k]]) for k in keys)
for r in rows[1:]:
    if r[0]=='#mark': print('---- mark',r[2],'@',r[1]); continue
    k=tuple(r[ix[x]] for x in keys)+(r[ix['joy0']],r[ix['trig0']])
    if k!=prev:
        print(r[0],'joy',r[ix['joy0']],'trig',r[ix['trig0']],fmt(r),'t',r[ix['t0']],r[ix['t1']],r[ix['t2']],r[ix['t3']])
        prev=k
