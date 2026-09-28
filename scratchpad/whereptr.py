"""whereptr.py ADDR... -- find where each code address is stored in halo.exe data (VA of each dword)."""
import struct, sys
EXE = r'C:\Program Files (x86)\Microsoft Games\Halo\halo.exe'
d = open(EXE, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]
opt = struct.unpack_from('<H', d, pe + 20)[0]
base = struct.unpack_from('<I', d, pe + 52)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + opt + 40 * i
    va, rsz, rptr = struct.unpack_from('<I', d, o + 12)[0], struct.unpack_from('<I', d, o + 16)[0], struct.unpack_from('<I', d, o + 20)[0]
    secs.append((va, rsz, rptr))
def va_of(off):
    for va, rsz, rptr in secs:
        if rptr <= off < rptr + rsz:
            return base + va + off - rptr
def rd(v):
    for va, rsz, rptr in secs:
        if base + va <= v < base + va + rsz:
            return struct.unpack_from('<I', d, rptr + v - base - va)[0]
for a in sys.argv[1:]:
    a = int(a, 16)
    pat = struct.pack('<I', a)
    hits, i = [], d.find(pat)
    while i >= 0:
        hits.append(va_of(i))
        i = d.find(pat, i + 1)
    out = []
    for h in hits:
        if h is None:
            continue
        if 0x6927d0 <= h < 0x6927d0 + 4 * 400:
            out.append('%x=event[%d]' % (h, (h - 0x6927d0) // 4))
        else:
            out.append('%x' % h)
    print('%x: %s' % (a, ' '.join(out)))
