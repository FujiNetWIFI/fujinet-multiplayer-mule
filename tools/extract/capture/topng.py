import sys
from PIL import Image
import os
pal=open(os.path.join(os.path.dirname(os.path.abspath(__file__)),"..","ntsc.pal"),"rb").read()
for fn in sys.argv[1:]:
    d=open(fn,'rb').read()
    im=Image.new('RGB',(336,224))
    px=[]
    for y in range(8,232):
        row=d[y*384+24:y*384+360]
        px+= [tuple(pal[c*3:c*3+3]) for c in row]
    im.putdata(px)
    im.resize((672,448),Image.NEAREST).save(fn.replace('.scr','.png'))
