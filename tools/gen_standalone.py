"""Standalone build generator, stage 1: the build-time tables for a halo.exe built from src/.

The standalone exe is linked away from 0x400000 (it runs no original code), and at start-up its loader copies the data
image back to the original addresses, so every global our C references by
absolute address (gen_link's EQU symbols) stays valid. This stage reads only committed files -- standalone/frozen/
and the src/ headers; it never reads the retail binary (retail_guard.forbid_retail) -- and writes:
  build/standalone/layout.json         where each piece goes (virtual address, raw size, virtual size)
  build/standalone/code_pointers.json  every dword in .rdata/.data equal to a function start, with the C symbol that
                                       replaces it (or why there is none)
  build/standalone/code_entries.json   every rewritten function by its original address
  build/standalone/imports.json        IAT slots (normal and delay-load) the loader fills: slot, dll, name/ordinal
  build/standalone/report.txt          what still ties a standalone build to the original code
The frozen inputs are regenerated from the retail binary only by the maintenance tool tools/freeze_retail_inputs.py.
Usage: python tools/gen_standalone.py"""
import os, re, sys, json, glob, collections

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import retail_guard as rg   # the build reads standalone/frozen/, never the retail binary

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "build", "standalone")


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
    rg.forbid_retail()
    os.makedirs(OUT, exist_ok=True)
    layout = rg.read_frozen("layout.json")
    slots = rg.read_frozen("imports.json")
    json.dump(layout, open(os.path.join(OUT, "layout.json"), "w"), indent=1)
    json.dump(slots, open(os.path.join(OUT, "imports.json"), "w"), indent=1)
    rewritten = rewritten_functions()
    pointers = []
    for p in rg.read_frozen("code_pointer_slots.json"):
        entry = dict(p)
        r = rewritten.get(p["target"])
        if r:
            entry["name"] = r["name"]
            entry["c_symbol"] = r["name"]
        pointers.append(entry)
    pointers.sort(key=lambda p: p["slot"])
    json.dump(pointers, open(os.path.join(OUT, "code_pointers.json"), "w"), indent=1)
    # every rewritten function by its original address: the loader redirects a jump into original .text (a constant
    # function address a stable rewrite still passes, e.g. structure_picked_polygon_draw's callbacks) to its C.
    entries = [{"addr": a, "c_symbol": r["name"], "module": r["module"]} for a, r in sorted(rewritten.items())]
    json.dump(entries, open(os.path.join(OUT, "code_entries.json"), "w"), indent=1)
    piece = {p["name"]: p for p in layout["pieces"]}
    rdata = {"rsize": piece[".rdata"]["raw"], "va": piece[".rdata"]["va"]}
    data = {"rsize": piece[".data"]["raw"], "vsize": piece[".data"]["virtual"], "va": piece[".data"]["va"]}

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
