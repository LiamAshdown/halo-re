"""Check slice 6 of the globals -> C conversion (standalone/data/slice06.c). Stdlib only, reads no retail file.

For every global defined in standalone/data/slice06.c (the "// 0x........" comment is its original address):
  * its original address lies past the initialised part of the image (standalone/image/pieces.json: .data va + size),
    i.e. in the zero-initialised BSS, so the image bytes the old EQU symbol pointed at are all zero;
  * the C definition is zero-initialised: the linked exe (build/<dir>/Release/halo_rebuilt.exe, symbol address from the
    /MAP file) has only zero bytes (or no raw bytes at all) over the object's extent;
  * the EQU / PUBLIC lines are gone from standalone/globals.asm;
  * the objects do not overlap each other and no symbol of the slice is missing from the map.
Usage: python tools/globals_check_slice06.py [build dir, default build/s06]"""
import os, re, sys, json, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import globals_asm_history

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
build = sys.argv[1] if len(sys.argv) > 1 else os.path.join("build", "s06")
rel = os.path.join(ROOT, build, "Release")
exe = os.path.join(rel, "halo_rebuilt.exe")
mapf = os.path.join(rel, "halo_rebuilt.map")
src = os.path.join(ROOT, "standalone", "data", "slice06.c")

defs = []   # (name, original address, size)
for line in open(src):
    m = re.match(r'^(?:__declspec\(align\(\d+\)\) )?([A-Za-z_][\w ]*?[ *])\s*(\w+)(?:\[(\d+)\])?\s*=\s*(?:\{0\}|0);\s*//\s*0x([0-9a-f]+)', line)
    if not m:
        continue
    ctype, name, count, addr = m.group(1).strip(), m.group(2), m.group(3), int(m.group(4), 16)
    sizes = {"uint8_t": 1, "int8_t": 1, "char": 1, "unsigned char": 1, "uint16_t": 2, "int16_t": 2, "uint32_t": 4,
             "int32_t": 4, "float": 4, "void *": 4, "void*": 4}
    es = sizes.get(ctype.replace("  ", " "))
    if es is None:
        print("unknown type", ctype, name); sys.exit(1)
    defs.append((name, addr, es * (int(count) if count else 1)))

pieces = {p["label"]: p for p in json.load(open(os.path.join(ROOT, "standalone", "image", "pieces.json")))}
data_end = pieces["data"]["va"] + pieces["data"]["size"]

bad = []
# PE section table of the exe (to read the bytes at a symbol's address)
blob = open(exe, "rb").read()
pe = struct.unpack_from("<I", blob, 0x3c)[0]
nsec = struct.unpack_from("<H", blob, pe + 6)[0]
optsz = struct.unpack_from("<H", blob, pe + 20)[0]
image_base = struct.unpack_from("<I", blob, pe + 24 + 28)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + optsz + 40 * i
    vsize, va, rawsize, rawptr = struct.unpack_from("<IIII", blob, o + 8)
    secs.append((va, vsize, rawptr, rawsize))

def exe_bytes(va, size):
    rva = va - image_base
    for sva, svs, raw, rs in secs:
        if sva <= rva < sva + max(svs, rs):
            out = bytearray()
            for k in range(size):
                off = rva + k - sva
                out.append(blob[raw + off] if off < rs and raw else 0)
            return bytes(out)
    return None

syms = {}
for line in open(mapf, errors="replace"):
    m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+_(\S+)\s+([0-9a-f]{8})\s", line)
    if m:
        syms.setdefault(m.group(1), int(m.group(2), 16))

asm = globals_asm_history.current()
spans = []
for name, addr, size in defs:
    if addr < data_end:
        bad.append("%s: original address 0x%x is inside the initialised image (ends 0x%x)" % (name, addr, data_end))
    if re.search(r"^_%s EQU" % re.escape(name), asm, re.M):
        bad.append("%s: EQU still in globals.asm" % name)
    va = syms.get(name)
    if va is None:
        bad.append("%s: not in the link map" % name); continue
    b = exe_bytes(va, size)
    if b is None or any(b):
        bad.append("%s: linked bytes at 0x%x are not zero" % (name, va))
    spans.append((va, va + size, name))
spans.sort()
for (a0, a1, n0), (b0, b1, n1) in zip(spans, spans[1:]):
    if b0 < a1:
        bad.append("overlap: %s and %s" % (n0, n1))
print("checked %d globals, %d problems" % (len(defs), len(bad)))
for x in bad:
    print("  ", x)
sys.exit(1 if bad else 0)
