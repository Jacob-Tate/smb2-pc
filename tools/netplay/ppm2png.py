import sys
from PIL import Image
im=Image.open(sys.argv[1]); s=float(sys.argv[3]) if len(sys.argv)>3 else 1.0
if s!=1.0: im=im.resize((int(im.width*s),int(im.height*s)),Image.LANCZOS)
im.save(sys.argv[2])
