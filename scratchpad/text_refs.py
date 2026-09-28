"""which parts of retail .text does the standalone still read as data?"""
import json, re, struct, os, collections
R = r"C:\Users\Liam-\halo-re"
exe = open(os.path.join(R, "bin", "halo.exe"), "rb").read()
L = json.load(open(os.path.join(R, "standalone", "frozen", "layout.json")))
P = {p["name"]: p for p in L["pieces"]}
T0, T1 = P[".text"]["va"], P[".text"]["va"] + P[".text"]["virtual"]
# raw offsets: sections from the pe
pe = struct.unpack_from("<I", exe, 0x3c)[0]
nsec = struct.unpack_from("<H", exe, pe + 6)[0]
optsize = struct.unpack_from("<H", exe, pe + 20)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + optsize + 40 * i
    vs, va, rs, raw = struct.unpack_from("<IIII", exe, o + 8)
    secs.append((exe[o:o+8].rstrip(b"\0").decode(), 0x400000 + va, vs, raw, rs))
ptr_slots = {p["slot"] for p in json.load(open(os.path.join(R, "standalone", "frozen", "code_pointer_slots.json")))}
imp_slots = {s["slot"] for s in json.load(open(os.path.join(R, "standalone", "frozen", "imports.json")))}
refs = collections.defaultdict(list)
for name, va, vs, raw, rs in secs:
    if name not in (".rdata", ".data"):
        continue
    for o in range(0, rs - 3, 4):
        v = struct.unpack_from("<I", exe, raw + o)[0]
        if T0 <= v < T1 and va + o not in ptr_slots and va + o not in imp_slots:
            refs[v].append(va + o)
print("data dwords pointing into .text that are not code-pointer slots:", sum(map(len, refs.values())), "targets", len(refs))
for v in [x for x in sorted(refs) if x >= 0x590000]:
    print(" %06x <- %s" % (v, " ".join("%06x" % s for s in refs[v][:4])))
# absolute EQUs into .text from resolve.asm
eq = []
for line in open(os.path.join(R, "build", "standalone", "resolve.asm")):
    m = re.match(r"\s*(\S+)\s+EQU\s+0*([0-9A-Fa-f]+)h", line)
    if m:
        a = int(m.group(2), 16)
        if T0 <= a < T1:
            eq.append((a, m.group(1)))
print("EQU symbols into .text:", len(eq))
for a, n in sorted(eq)[:60]:
    print(" %06x %s" % (a, n))
