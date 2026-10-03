"""Of the stale old-range dwords in the linked exe's data (tools/scan_data_stale_pointers.py), lists those that point at (or into)
a global that now lives elsewhere: the original address of every converted global comes from the old standalone/globals.asm
in git history (the commit before the slice conversions). A hit means the pointer reads a dead copy: fix the definition so it
points at `&global`.
Usage: python tools/scan_stale_into_globals.py [exe] [map]"""
import re, struct, subprocess, sys, bisect

exe = sys.argv[1] if len(sys.argv) > 1 else "build/cxx/Release/halo_rebuilt.exe"
mp = sys.argv[2] if len(sys.argv) > 2 else "build/cxx/Release/halo_rebuilt.map"
LO, HI = 0x630000, 0x891000

old = subprocess.run(["git", "show", "8252d572:standalone/globals.asm"], capture_output=True, text=True,
                     encoding="utf-8").stdout
orig = {}
for m in re.finditer(r"^(\S+) EQU 0([0-9A-Fa-f]+)h", old, re.M):
    orig[m.group(1)] = int(m.group(2), 16)
by_addr = sorted((a, n) for n, a in orig.items() if a >= 0x600000)
addrs = [a for a, n in by_addr]

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


def sym_at(a):
    i = bisect.bisect_right(keys, a) - 1
    return (syms[i][1], a - syms[i][0]) if i >= 0 else ("?", 0)


hits = {}
for name, va, vsize, raw, rsize in secs:
    if name in (".text", ".reloc", ".idata", ".didat") or rsize == 0:
        continue
    for off in range(0, min(vsize, rsize) - 3, 4):
        v = struct.unpack_from("<I", data, raw + off)[0]
        if not (LO <= v < HI):
            continue
        s, d = sym_at(base + va + off)
        if s.startswith(("_halo_image_", "??_C@", "$SG", ".idata", "___", "__NULL")) or "??" in s:
            continue
        i = bisect.bisect_right(addrs, v) - 1
        if i < 0:
            continue
        a, gname = by_addr[i]
        if v - a < 0x1000 and v >= 0x676000:   # in the old .data/.bss, near a named global
            hits.setdefault(s, []).append((d, v, gname, v - a))
print("%d stale pointers into old .data/.bss near named globals, in %d symbols" % (sum(len(v) for v in hits.values()), len(hits)))
for s, v in sorted(hits.items(), key=lambda kv: -len(kv[1]))[:80]:
    d, val, g, delta = v[0]
    print("%4d  %-40s e.g. +0x%x = 0x%x -> _%s+0x%x" % (len(v), s[:40], d, val, g.lstrip("_"), delta))
