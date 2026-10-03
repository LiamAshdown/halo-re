"""Scans the initialised data of the linked exe for dwords that fall in the old retail data image range (0x630000..0x890000).
Every real pointer in our data is relocated to our own image base (0x10000000+); a raw value in the old range is a stale
absolute address (a pointer that was never re-targeted, or a number that happens to look like one). Prints each one with
the nearest preceding map symbol so the definition can be fixed.
Usage: python tools/scan_data_stale_pointers.py [exe] [map]"""
import re, struct, sys, bisect

LO, HI = 0x630000, 0x891000
exe = sys.argv[1] if len(sys.argv) > 1 else "build/cxx/Release/halo_rebuilt.exe"
mp = sys.argv[2] if len(sys.argv) > 2 else "build/cxx/Release/halo_rebuilt.map"
data = open(exe, "rb").read()
pe = struct.unpack_from("<I", data, 0x3c)[0]
nsec = struct.unpack_from("<H", data, pe + 6)[0]
opt = pe + 24
optsize = struct.unpack_from("<H", data, pe + 20)[0]
base = struct.unpack_from("<I", data, opt + 28)[0]
secs = []
for i in range(nsec):
    o = opt + optsize + 40 * i
    name = data[o:o + 8].rstrip(b"\0").decode()
    vsize, va, rsize, raw = struct.unpack_from("<IIII", data, o + 8)
    secs.append((name, va, vsize, raw, rsize))

syms = []
for line in open(mp, errors="replace"):
    m = re.match(r"\s+([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s", line)
    if m:
        syms.append((int(m.group(4), 16), m.group(3)))
syms.sort()
keys = [s[0] for s in syms]


def sym_at(addr):
    i = bisect.bisect_right(keys, addr) - 1
    return (syms[i][1], addr - syms[i][0]) if i >= 0 else ("?", 0)


count = {}
for name, va, vsize, raw, rsize in secs:
    if name in (".text", ".reloc", ".idata", ".didat") or rsize == 0:
        continue
    for off in range(0, min(vsize, rsize) - 3, 4):
        v = struct.unpack_from("<I", data, raw + off)[0]
        if LO <= v < HI:
            s, d = sym_at(base + va + off)
            count.setdefault(s, []).append((d, v))
tot = sum(len(v) for v in count.values())
print("%d dwords in the old range, in %d symbols" % (tot, len(count)))
for s, v in sorted(count.items(), key=lambda kv: -len(kv[1]))[:5000]:
    print("%4d  %s  e.g. +0x%x = 0x%x" % (len(v), s[:90], v[0][0], v[0][1]))
