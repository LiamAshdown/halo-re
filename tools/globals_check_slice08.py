"""Check slice 8 of the globals->C conversion (standalone/data/slice08.c, engine globals 0x00719772..0x00721eb8).

Stdlib only; reads no retail file. For every global slice08.c defines (the `// 0x%08x` annotation on each definition is the
original fixed address) it checks, in the compiled object (default build/s08/halo_rebuilt.dir/Release/slice08.obj, or
build/standalone/data_slice08.obj; pass another path as argv[1]):
  1. the symbol exists and lives in a ".g08$NNNN" section;
  2. its offset from the first object, after laying the sections out the way the linker does (the "$" group sorted by
     name, each section aligned to its own alignment), equals original address - first address: the run keeps the
     original layout, so every block clear / overrun in the engine code (default_profile_data's 0x1001-dword clear,
     the 0x2c7-dword block savegame_index_file..last_multiplayer_map_path, variant_write_request_state's 0x29-dword
     clear, network_bandwidth_graph_globals + 0x23e0 label buffer, ...) touches the same neighbours as in the original;
  3. the initial bytes are all zero, and the original address lies past the end of the initialised .data piece of the
     committed image (standalone/image/pieces.json), i.e. it is BSS in the original too, so zero IS the image's value
     and no pointer value has to be reproduced;
  4. the name is gone from standalone/globals.asm (no EQU left) and every alias (two names at one address) resolves
     through /alternatename to a defined object.
If a linked map exists (build/s08/Release/halo_rebuilt.map with the public symbols) the same offsets are also compared
there. Exit status 1 on any mismatch."""
import os, re, sys, json, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import globals_asm_history

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "standalone", "data", "slice08.c")


def read_obj(path):
    d = open(path, "rb").read()
    machine, nsec, _, symptr, nsym, optsz, _ = struct.unpack_from("<HHIIIHH", d, 0)
    strtab = symptr + 18 * nsym

    def string_at(o):
        return d[strtab + o: d.index(b"\0", strtab + o)].decode()
    secs = []
    off = 20 + optsz
    for i in range(nsec):
        name, vsz, va, rawsz, rawptr = struct.unpack_from("<8sIIII", d, off + 40 * i)
        chars = struct.unpack_from("<I", d, off + 40 * i + 36)[0]
        name = name.rstrip(b"\0").decode()
        if name.startswith("/"):
            name = string_at(int(name[1:]))
        al = (chars >> 20) & 0xF
        secs.append((name, rawsz, rawptr, 1 << (al - 1) if al else 1))
    syms = {}
    i = 0
    while i < nsym:
        e = d[symptr + 18 * i: symptr + 18 * i + 18]
        nm, value, sec, typ, cls, naux = struct.unpack("<8sIhHBB", e)
        if nm[:4] == b"\0\0\0\0":
            name = string_at(struct.unpack("<I", nm[4:])[0])
        else:
            name = nm.rstrip(b"\0").decode()
        if cls == 2 and sec > 0:
            syms[name] = (sec - 1, value)
        i += 1 + naux
    return d, secs, syms


def main():
    objs = [sys.argv[1]] if len(sys.argv) > 1 else [
        os.path.join(ROOT, "build", "s08", "halo_rebuilt.dir", "Release", "slice08.obj"),
        os.path.join(ROOT, "build", "standalone", "data_slice08.obj")]
    obj = next((o for o in objs if os.path.exists(o)), None)
    if not obj:
        raise SystemExit("no slice08 object found; build first")
    d, secs, syms = read_obj(obj)
    grp = sorted((i for i, s in enumerate(secs) if s[0].startswith(".g08$")), key=lambda i: secs[i][0])
    if not grp:
        raise SystemExit(".g08$ sections missing")
    start = {}
    pos = 0
    nonzero = 0
    for i in grp:
        name, rawsz, rawptr, al = secs[i]
        pos = (pos + al - 1) // al * al
        start[i] = pos
        nonzero += sum(1 for b in d[rawptr: rawptr + rawsz] if b)
        pos += rawsz
    total = pos
    text = open(SRC, encoding="utf-8").read()
    defs = [(m.group(1), int(m.group(2), 16)) for m in
            re.finditer(r"^__declspec\(allocate\([^)]*\)\) [^=\n]*?\b(\w+)(?:\[[^\]]*\])?\s*=\s*\{0\};\s*// 0x([0-9a-f]{8})\b",
                        text, re.M)]
    aliases = re.findall(r"/alternatename:_(\w+)=_(\w+)", text)
    pieces = json.load(open(os.path.join(ROOT, "standalone", "image", "pieces.json")))
    data = next(p for p in pieces if p["label"] == "data")
    data_end = data["va"] + data["size"]
    asm = globals_asm_history.current()
    bad = []

    def where(n):
        s = syms.get("_" + n)
        return None if not s or s[0] not in start else start[s[0]] + s[1]
    base_name, base_addr = defs[0]
    base_off = where(base_name)
    mapoff = {}
    mp = os.path.join(ROOT, "build", "s08", "Release", "halo_rebuilt.map")
    if os.path.exists(mp):
        for l in open(mp, errors="replace"):
            m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+_(\w+)\s+([0-9a-f]{8})\s", l)
            if m:
                mapoff[m.group(1)] = int(m.group(2), 16)
    for n, a in defs:
        off = where(n)
        if off is None:
            bad.append("%s: not defined in a .g08$ section" % n); continue
        if off - base_off != a - base_addr:
            bad.append("%s: offset %#x, original %#x" % (n, off - base_off, a - base_addr))
        if mapoff and n in mapoff and mapoff[n] - mapoff[base_name] != a - base_addr:
            bad.append("%s: map offset %#x, original %#x" % (n, mapoff[n] - mapoff[base_name], a - base_addr))
        if a < data_end:
            bad.append("%s: 0x%x lies in the initialised .data piece (ends 0x%x)" % (n, a, data_end))
        if re.search(r"^_%s EQU" % re.escape(n), asm, re.M):
            bad.append("%s: EQU still in globals.asm" % n)
    if nonzero:
        bad.append("%d non-zero initial bytes" % nonzero)
    for a_, b_ in aliases:
        if where(b_) is None:
            bad.append("alias target %s missing" % b_)
        if re.search(r"^_%s EQU" % re.escape(a_), asm, re.M):
            bad.append("alias %s: EQU still in globals.asm" % a_)
    print("%d globals + %d aliases checked in %s (%d sections, %d bytes laid out, initial bytes all zero: %s, map symbols "
          "compared: %d)" % (len(defs), len(aliases), os.path.relpath(obj, ROOT), len(grp), total, not nonzero, len(mapoff)))
    for b in bad:
        print("MISMATCH", b)
    print("OK" if not bad else "%d mismatches" % len(bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
