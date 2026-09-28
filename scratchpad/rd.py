"""rd.py VA COUNT [fmt]: read COUNT dwords (or 's' string) from halo.exe at VA."""
import struct, sys
d=open(r'C:/Program Files (x86)/Microsoft Games/Halo/halo.exe','rb').read()
pe=struct.unpack_from('<I',d,0x3c)[0]; n=struct.unpack_from('<H',d,pe+6)[0]; so=pe+24+struct.unpack_from('<H',d,pe+20)[0]
def off(va):
  for i in range(n):
    name,vs,sva,rs,ra=struct.unpack_from('<8sIIII',d,so+40*i)
    if sva<=va-0x400000<sva+vs: return va-0x400000-sva+ra
va=int(sys.argv[1],16); k=int(sys.argv[2]); o=off(va)
if len(sys.argv)>3 and sys.argv[3]=='s': print(d[o:o+k])
else: print(' '.join('%x'%struct.unpack_from('<I',d,o+4*i)[0] for i in range(k)))
