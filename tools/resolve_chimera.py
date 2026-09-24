"""Resolve Chimera byte signatures against bin/halo.exe -> symbols/chimera.txt (kind=code hints)."""
import json, csv, bisect, struct, re, sys
exe = open("bin/halo.exe", "rb").read()
pe = struct.unpack_from("<I", exe, 0x3c)[0]; nsec = struct.unpack_from("<H", exe, pe+6)[0]; optsz = struct.unpack_from("<H", exe, pe+20)[0]
secs = []
for i in range(nsec):
    o = pe+24+optsz+i*40; vs, va, rs, ro = struct.unpack_from("<IIII", exe, o+8)
    secs.append((va, vs, ro, rs))
def raw2va(off):
    for va, vs, ro, rs in secs:
        if ro <= off < ro+rs: return 0x400000 + va + (off-ro)
sigs = json.load(open("symbols/chimera_sigs.json"))
entries = {}
for row in csv.DictReader(open("out/functions.csv", newline="")): entries[int(row["address"],16)] = int(row["size"])
starts = sorted(entries)
def containing(a):
    i = bisect.bisect_right(starts, a)-1
    return starts[i] if i>=0 and starts[i] <= a < starts[i]+entries[starts[i]] else None
rows=[]; unresolved=[]; multi=0
for s in sigs:
    pat = b"".join(b"." if b is None else re.escape(bytes([b])) for b in s["pattern"])
    hits = [m.start() for m in re.finditer(pat, exe, re.S)]
    if not hits: unresolved.append(s["name"]); continue
    if len(hits) > 1: multi += 1
    idx = min(s["match"], len(hits)-1)
    va = raw2va(hits[idx])
    if va is None: unresolved.append(s["name"]); continue
    c = containing(va)
    rows.append((va, s["name"], "code", "0.9" if len(hits)==1 else "0.6", f"chimera:{s['feature']}" + (f" inside=0x{c:06x}" if c is not None and c != va else (" nofunc" if c is None else " entry"))))
rows.sort()
with open("symbols/chimera.txt","w") as fh:
    fh.write("# addr name kind confidence source\n")
    for r in rows: fh.write("0x%06x %s %s %s %s\n" % r)
print("resolved", len(rows), "unresolved", len(unresolved), "multi-hit", multi, "at-entry", sum(1 for r in rows if r[4].endswith("entry")))
print("unresolved sample:", unresolved[:12])
