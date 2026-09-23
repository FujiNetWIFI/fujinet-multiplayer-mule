import numpy as np, sys
fn=sys.argv[1]; f0=int(sys.argv[2]); f1=int(sys.argv[3])
raw=np.fromfile(fn,dtype=np.uint8).reshape(-1,65540)
frames=raw[:,:4].copy().view(np.int32).ravel()
m=raw[:,4:]
sel=(frames>=f0)&(frames<=f1)
fr=frames[sel]; mm=m[sel].astype(int)
d=np.diff(mm,axis=0)
dec=(d==-1).sum(0); inc=(d==1).sum(0); other=((d!=0)&(abs(d)!=1)).sum(0)
cands=[a for a in range(0x80,0xB000) if dec[a]>=5 and inc[a]==0 and other[a]<=1]
for a in cands[:60]:
    col=mm[:,a]; ch=np.nonzero(np.diff(col))[0]
    times=fr[ch+1]
    iv=np.diff(times)
    print('%04x start %3d end %3d decs %3d intervals %s'%(a,col[0],col[-1],dec[a],list(iv[:12])))
