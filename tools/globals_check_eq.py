"""Check standalone/data/eq_data.c and eq_bss.c, the C definitions of the globals that were the last absolute EQU
symbols of standalone/globals.asm (original addresses from the last committed globals.asm, tools/globals_asm_history).

Stdlib only, reads no retail file. From the compiled objects (default build/x1/halo_data.dir/Release/eq_*.obj, or the
paths given as arguments) it checks:
  1. every EQU name of the original globals.asm is defined in one ".geq$<address>v" / ".g08$0000_<address>v" section
     whose address is the name's original address, or is a /alternatename alias of a name defined at that address
     (in eq_*.c or a slice file);
  2. the sections, laid out the way the linker does (group sorted by name, each aligned to its own alignment), keep
     every object at original address - cluster start from the cluster's start, and each object's size reaches the
     next object of its cluster: a cluster is gap-free, so overruns and sweeps touch the original neighbours;
  3. no symbol is defined twice across standalone/data/*.obj next to the eq objects;
  4. eq_bss initial bytes are zero; eq_data bytes equal the data image (standalone/image/*.asm) at the original address
     wherever the image holds a plain number and the object has no relocation.
With build/x1/Release/halo_rebuilt.map the linked addresses are compared too. Exit status 1 on any mismatch.
Usage: python tools/globals_check_eq.py [eq_data.obj eq_bss.obj]"""
import collections, glob, json, os, re, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import globals_asm_history

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "standalone", "data")
OBJDIR = os.path.join(ROOT, "build", "x1", "halo_data.dir", "Release")
SEC = re.compile(r"^\.(?:geq\$|g08\$0000_)([0-9a-f]{8})(v?)$")


def read_obj(path):
    d = open(path, "rb").read()
    _, nsec, _, symptr, nsym, optsz, _ = struct.unpack_from("<HHIIIHH", d, 0)
    strtab = symptr + 18 * nsym

    def string_at(o):
        return d[strtab + o: d.index(b"\0", strtab + o)].decode()
    secs = []
    for i in range(nsec):
        name, _, _, rawsz, rawptr, relptr, _, nrel, _, chars = struct.unpack_from("<8sIIIIIIHHI", d, 20 + optsz + 40 * i)
        name = name.rstrip(b"\0").decode()
        if name.startswith("/"):
            name = string_at(int(name[1:]))
        al = (chars >> 20) & 0xF
        relocs = {struct.unpack_from("<I", d, relptr + 10 * k)[0] for k in range(nrel)}
        secs.append((name, rawsz, rawptr, 1 << (al - 1) if al else 1, relocs, chars))
    syms, i = {}, 0
    while i < nsym:
        nm, value, sec, _, cls, naux = struct.unpack_from("<8sIhHBB", d, symptr + 18 * i)
        name = string_at(struct.unpack("<I", nm[4:])[0]) if nm[:4] == b"\0\0\0\0" else nm.rstrip(b"\0").decode()
        if cls == 2 and sec > 0:
            syms[name] = (sec - 1, value)
        i += 1 + naux
    return d, secs, syms


def image():
    mem = {}
    for p in json.load(open(os.path.join(ROOT, "standalone", "image", "pieces.json"))):
        if p["label"] not in ("rdata", "data"):
            continue
        a, on = p["va"], False
        for line in open(os.path.join(ROOT, "standalone", "image", p["label"] + ".asm")):
            if "LABEL BYTE" in line:
                on = True
                continue
            code = line.split(";")[0].strip()
            if not on or not code.startswith("dd "):
                continue
            for it in code[3:].split(","):
                it = it.strip()
                num = lambda t: int(t[:-1], 16) if t[-1:] in "hH" else int(t)
                m = re.match(r"(\w+) dup \((\w+)\)", it)
                if m:
                    for _ in range(num(m.group(1))):
                        mem[a] = num(m.group(2)); a += 4
                else:
                    mem[a] = None if it.startswith("halo_code_") else num(it); a += 4
    return mem


def main():
    objs = sys.argv[1:] or [os.path.join(OBJDIR, "eq_data.obj"), os.path.join(OBJDIR, "eq_bss.obj")]
    for o in objs:
        if not os.path.exists(o):
            raise SystemExit("%s missing: build first" % o)
    eq = {n: int(a, 16) for n, a in re.findall(r"^_(\w+) EQU 0?([0-9A-Fa-f]+)h", globals_asm_history.original(), re.M)}
    src = "".join(open(p, encoding="utf-8").read() for p in glob.glob(os.path.join(DATA, "eq_*.c")))
    aliases = dict(re.findall(r"/alternatename:_(\w+)=_(\w+)", src))
    bad = []
    mem = image()
    # every section of the groups, from all objects, with the object it is in
    items = []   # (section name, address, is_object, size, align, symbol, obj path, data, rawptr, relocs, bss)
    defined = {}
    for o in objs:
        d, secs, syms = read_obj(o)
        bysec = collections.defaultdict(list)
        for n, (si, v) in syms.items():
            bysec[si].append((v, n))
        for si, (name, rawsz, rawptr, al, relocs, chars) in enumerate(secs):
            m = SEC.match(name)
            if not m:
                continue
            ss = bysec.get(si, [])
            if len(ss) != 1 or ss[0][0] != 0:
                bad.append("%s: section %s holds %s, not one object" % (os.path.basename(o), name, ss))
                continue
            sym = ss[0][1][1:]
            items.append((name, int(m.group(1), 16), bool(m.group(2)), rawsz, al, sym, o, d, rawptr, relocs,
                          "eq_bss" in os.path.basename(o)))
            defined[sym] = int(m.group(1), 16) if m.group(2) else None
    items.sort()
    # linker layout of each group
    pos, layout = {}, {}
    for group in (".geq$", ".g08$0000_"):
        p = 0
        for it in [x for x in items if x[0].startswith(group)]:
            p = (p + it[4] - 1) // it[4] * it[4]
            layout[it[0]] = p
            p += it[3]
    # clusters: a pad (or a 16-aligned first object) opens one; check offsets and gap-free extents
    start = None
    for k, it in enumerate(items):
        name, addr, isobj, size, al, sym = it[:6]
        if not isobj or start is None or name.split("$")[0] != items[k - 1][0].split("$")[0] or \
                layout[name] != layout[items[k - 1][0]] + items[k - 1][3]:
            start = (layout[name], addr if isobj else addr & ~15)
            if isobj and (addr - start[1]) % 16:
                bad.append("%s: cluster start 0x%x not congruent modulo 16" % (sym, addr))
        elif layout[name] - start[0] != addr - start[1]:
            bad.append("%s: offset 0x%x in its run, original 0x%x" % (sym, layout[name] - start[0], addr - start[1]))
        nxt = items[k + 1] if k + 1 < len(items) else None
        if isobj and nxt and nxt[2] and nxt[0][:5] == name[:5] and nxt[1] > addr and addr + size > nxt[1]:
            bad.append("%s: size 0x%x runs past the next object at 0x%x" % (sym, size, nxt[1]))
        # initial bytes
        d, rawptr, relocs, bss = it[7], it[8], it[9], it[10]
        body = d[rawptr: rawptr + size] if rawptr else bytes(size)
        if bss or not isobj:
            if any(body):
                bad.append("%s: non-zero initial bytes" % sym)
        else:
            for off in range(0, size - size % 4, 4):
                v = mem.get(addr + off)
                if off in relocs or v is None:
                    continue
                if struct.unpack_from("<I", body, off)[0] != v:
                    bad.append("%s+0x%x: 0x%08x, image 0x%08x" % (sym, off, struct.unpack_from("<I", body, off)[0], v))
            if size % 4:
                for off in range(size):
                    q = (addr + off) & ~3
                    v = mem.get(q)
                    if v is not None and body[off] != (v >> (8 * (addr + off - q))) & 0xff:
                        bad.append("%s+0x%x: byte differs from the image" % (sym, off)); break
    # every EQU name: defined at its address, or an alias of a name defined at its address
    slice_defs = set()
    for o in glob.glob(os.path.join(os.path.dirname(objs[0]), "slice*.obj")):
        slice_defs |= {n[1:] for n in read_obj(o)[2]}
    for n, a in sorted(eq.items(), key=lambda t: t[1]):
        if n in defined:
            if defined[n] != a:
                bad.append("%s: section address 0x%x, original 0x%x" % (n, defined[n], a))
        elif n in aliases:
            t = aliases[n]
            if t in defined and defined[t] != a:
                bad.append("alias %s -> %s: target at 0x%x, original 0x%x" % (n, t, defined[t], a))
            elif t not in defined and t not in slice_defs:
                bad.append("alias %s -> %s: target not defined" % (n, t))
        else:
            bad.append("%s: not converted" % n)
    # duplicates across the data objects
    seen = collections.Counter()
    for o in glob.glob(os.path.join(os.path.dirname(objs[0]), "*.obj")):
        seen.update(read_obj(o)[2].keys())
    for n, c in seen.items():
        if c > 1:
            bad.append("%s defined %d times in standalone/data" % (n, c))
    for n in aliases:
        if "_" + n in seen:
            bad.append("alias %s is also defined" % n)
    # linked map
    mp = os.path.join(ROOT, "build", "x1", "Release", "halo_rebuilt.map")
    nmap = 0
    if os.path.exists(mp) and os.path.getsize(mp):
        at = {}
        for l in open(mp, errors="replace"):
            m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+_(\w+)\s+([0-9a-f]{8})\s", l)
            if m:
                at[m.group(1)] = int(m.group(2), 16)
        objs_by_addr = sorted((it[1], it[5]) for it in items if it[2] and it[5] in at)
        if "main_globals_data" in at and "g08_base_marker" in at and at["g08_base_marker"] - at["main_globals_data"] != 0x70:
            bad.append("map: slice08's run does not continue main_globals at +0x70")
        for (a1, n1), (a2, n2) in zip(objs_by_addr, objs_by_addr[1:]):
            if a2 - a1 < 0x1000 and at[n2] - at[n1] not in (a2 - a1,) and at[n2] > at[n1] and at[n2] - at[n1] < a2 - a1:
                bad.append("map: %s..%s 0x%x apart, original 0x%x" % (n1, n2, at[n2] - at[n1], a2 - a1))
            nmap += 1
    print("%d EQU names (%d objects, %d aliases), %d sections in %d objects, map pairs compared: %d" % (
        len(eq), sum(1 for x in items if x[2]), len(aliases), len(items), len(objs), nmap))
    for b in bad:
        print("MISMATCH", b)
    print("OK" if not bad else "%d mismatches" % len(bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
