"""Standalone build generator, stage 1: extract what a halo.exe built from src/ needs from the original image.

The hybrid standalone plan: our exe is linked away from 0x400000 (it runs no original code), and at start-up a small
loader maps halo.exe's .rdata/.data/.bss back at their original addresses, so every global our C references by
absolute address (gen_link's EQU symbols) stays valid. This stage writes:
  build/standalone/halo_image.bin      .rdata, the initialised part of .data, .tls and .rsrc, back to back
  build/standalone/layout.json         where each piece goes (virtual address, raw size, virtual size)
  build/standalone/code_pointers.json  every dword in .rdata/.data equal to a known function start, with the C
                                       symbol that replaces it (or why there is none yet)
  build/standalone/imports.json        IAT slots (normal and delay-load) the loader fills: slot, dll, name/ordinal
  build/standalone/report.txt          what still ties a standalone build to the original code
halo.exe has no relocation table (IMAGE_FILE_RELOCS_STRIPPED), so code pointers are found by exact match against
function starts (out/functions.json plus src/ headers); a coincidental integer equal to a function address would be
a false positive, which is why the report lists every target by name for review.
Usage: python tools/gen_standalone.py"""
import os, re, sys, json, glob, struct, collections

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "build", "standalone")
EXE = os.path.join(ROOT, "bin", "halo.exe")


def pe_layout(exe):
    pe = struct.unpack_from("<I", exe, 0x3c)[0]
    nsec = struct.unpack_from("<H", exe, pe + 6)[0]
    opt = pe + 24
    optsize = struct.unpack_from("<H", exe, pe + 20)[0]
    base = struct.unpack_from("<I", exe, opt + 28)[0]
    dirs = [struct.unpack_from("<II", exe, opt + 96 + 8 * i) for i in range(16)]
    sections = []
    for i in range(nsec):
        o = opt + optsize + 40 * i
        name = exe[o:o + 8].rstrip(b"\0").decode()
        vsize, va, rsize, raw = struct.unpack_from("<IIII", exe, o + 8)
        sections.append({"name": name, "va": base + va, "vsize": vsize, "raw": raw, "rsize": rsize})
    return base, dirs, sections


def reader(exe, base, sections):
    def off(va):
        for s in sections:
            if s["va"] <= va < s["va"] + max(s["vsize"], s["rsize"]):
                d = va - s["va"]
                return s["raw"] + d if d < s["rsize"] else None
        return None
    def u32(va):
        o = off(va)
        return None if o is None else struct.unpack_from("<I", exe, o)[0]
    def cstr(va):
        o = off(va)
        return exe[o:exe.index(b"\0", o)].decode("latin-1")
    return off, u32, cstr


def imports(exe, base, dirs, u32, cstr):
    slots = []
    va, size = dirs[1]
    d = base + va
    while True:
        oft, _, _, name, ft = [u32(d + 4 * k) for k in range(5)]
        if not name:
            break
        dll = cstr(base + name)
        lookup, iat = base + (oft or ft), base + ft
        k = 0
        while True:
            e = u32(lookup + 4 * k)
            if not e:
                break
            sym = ("#%d" % (e & 0xffff)) if e & 0x80000000 else cstr(base + e + 2)
            slots.append({"slot": iat + 4 * k, "dll": dll, "name": sym, "delay": False})
            k += 1
        d += 20
    va, size = dirs[13]                         # delay-load directory (VC7 layout: attributes bit 0 = RVAs)
    d = base + va
    while True:
        attrs, name, hmod, iat, int_, bound, unload, stamp = [u32(d + 4 * k) for k in range(8)]
        if not name:
            break
        rel = base if attrs & 1 else 0
        dll = cstr(rel + name)
        k = 0
        while True:
            e = u32(rel + int_ + 4 * k)
            if not e:
                break
            sym = ("#%d" % (e & 0xffff)) if e & 0x80000000 else cstr(rel + e + 2)
            slots.append({"slot": rel + iat + 4 * k, "dll": dll, "name": sym, "delay": True,
                          "module_handle_slot": rel + hmod})
            k += 1
        d += 32
    return slots


def rewritten_functions():
    by_addr = {}
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(p, encoding="utf-8", errors="replace").read()
        m = re.search(r"address\s+0x0*([0-9a-f]{6}),\s*size\s+(\d+)", t[:4000], re.I)
        if m:
            name = os.path.splitext(os.path.basename(p))[0]
            by_addr[int(m.group(1), 16)] = {"name": name, "module": os.path.basename(os.path.dirname(p))}
    return by_addr


def main():
    os.makedirs(OUT, exist_ok=True)
    exe = open(EXE, "rb").read()
    base, dirs, sections = pe_layout(exe)
    off, u32, cstr = reader(exe, base, sections)
    sec = {s["name"]: s for s in sections}

    # ---- data image: .rdata then .data (initialised part); .bss is the zero tail of .data's virtual size
    rdata, data = sec[".rdata"], sec[".data"]
    blob = b""
    pieces = []
    # .text goes in too, but only as data: the loader maps it without execute permission, so tables the compiler put in
    # the code section (jump tables, handler arrays) read correctly and any jump into original code faults (DEP) at an
    # address that names the original function
    for name in (".text", ".rdata", ".data", ".tls", ".rsrc"):
        if name not in sec:
            continue
        s = sec[name]
        pieces.append({"name": name, "va": s["va"], "blob_offset": len(blob), "raw": s["rsize"], "virtual": s["vsize"]})
        blob += exe[s["raw"]:s["raw"] + s["rsize"]]
    open(os.path.join(OUT, "halo_image.bin"), "wb").write(blob)
    layout = {"image_base": base, "entry": base + struct.unpack_from("<I", exe, struct.unpack_from("<I", exe, 0x3c)[0] + 40)[0],
              "pieces": pieces,
              "tls_directory": base + dirs[9][0] if dirs[9][1] else None,
              "tls_section": {"va": sec[".tls"]["va"], "virtual": sec[".tls"]["vsize"]} if ".tls" in sec else None,
              "resources": {"va": sec[".rsrc"]["va"], "size": sec[".rsrc"]["vsize"]} if ".rsrc" in sec else None,
              "reserve": {"from": rdata["va"], "to": max(s["va"] + s["vsize"] for s in sections)}}
    json.dump(layout, open(os.path.join(OUT, "layout.json"), "w"), indent=1)

    # ---- import slots the loader fills
    slots = imports(exe, base, dirs, u32, cstr)
    json.dump(slots, open(os.path.join(OUT, "imports.json"), "w"), indent=1)
    slot_set = {s["slot"] for s in slots}

    # ---- code pointers stored in data
    funcs = {int(f["addr"], 16): f for f in json.load(open(os.path.join(ROOT, "out", "functions.json")))}
    mods = json.load(open(os.path.join(ROOT, "modules.json")))
    rewritten = rewritten_functions()
    text = sec[".text"]
    pointers = []
    for s in (rdata, data):
        for o in range(0, s["rsize"] - 3, 4):
            va = s["va"] + o
            if va in slot_set:
                continue
            v = struct.unpack_from("<I", exe, s["raw"] + o)[0]
            if not (text["va"] <= v < text["va"] + text["vsize"]):
                continue
            f = funcs.get(v)
            r = rewritten.get(v)
            if not f and not r:
                continue
            m = mods.get("%x" % v, {})
            module = m.get("module", "?") if isinstance(m, dict) else m
            if f and (f.get("lib") or f.get("fid")):
                module = "lib:crt" if module in ("?", "") else module
            entry = {"slot": va, "target": v, "name": (r or f)["name"], "module": module}
            if r:
                entry["c_symbol"] = r["name"]
            pointers.append(entry)
    json.dump(pointers, open(os.path.join(OUT, "code_pointers.json"), "w"), indent=1)

    # ---- report
    by_kind = collections.Counter()
    missing = collections.defaultdict(list)
    for p in pointers:
        if "c_symbol" in p:
            by_kind["C rewrite"] += 1
        elif p["module"].startswith("lib"):
            by_kind[p["module"]] += 1
            missing[p["module"]].append(p)
        else:
            by_kind["game code without C"] += 1
            missing["game"].append(p)
    link_log = os.path.join(ROOT, "harness", "build", "link.log")
    lines = ["Standalone generator stage 1 report", "",
             "data image: .rdata %d bytes at 0x%x, .data %d of %d bytes at 0x%x (rest zero), reserve 0x%x..0x%x" % (
                 rdata["rsize"], rdata["va"], data["rsize"], data["vsize"], data["va"],
                 layout["reserve"]["from"], layout["reserve"]["to"]),
             "import slots to fill: %d (%d delay-load) from %d DLLs" % (
                 len(slots), sum(s["delay"] for s in slots), len({s["dll"].lower() for s in slots})),
             "code pointers in data: %d -> %s" % (len(pointers), dict(by_kind)), ""]
    for k in sorted(missing):
        lines.append("== pointers to %s (%d): no C target yet" % (k, len(missing[k])))
        for name, n in collections.Counter(p["name"] for p in missing[k]).most_common():
            lines.append("   %-50s x%d" % (name, n))
    lines += ["", "DLLs: " + ", ".join(sorted({s["dll"] for s in slots}, key=str.lower))]
    open(os.path.join(OUT, "report.txt"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines[:6]))


if __name__ == "__main__":
    main()
