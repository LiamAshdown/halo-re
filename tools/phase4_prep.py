"""Prepare Phase 3/4 inputs for one module: out/phase4/<module>_functions.md and a suggested split address.
Usage: python tools/phase4_prep.py <module>"""
import json, glob, os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
P = lambda *a: os.path.join(ROOT, *a)
mod = sys.argv[1]
fs = {f["addr"][-6:]: f for f in json.load(open(P("out", "functions.json")))}
mods = json.load(open(P("modules.json")))
addrs = sorted(a for a, v in mods.items() if v["module"] == mod)
res = {}
for p in glob.glob(P("out", "phase2", "results", mod + "_*.json")):
    d = json.load(open(p, encoding="utf-8"))
    for r in (d if isinstance(d, list) else d.get("results", [])):
        res[r["addr"].lower().replace("0x", "")[-6:]] = r
os.makedirs(P("out", "phase4"), exist_ok=True)
with open(P("out", "phase4", mod + "_functions.md"), "w", encoding="utf-8") as fh:
    fh.write("# %s module: %d functions (address, current Ghidra name, size, agent confidence, summary)\n\n" % (mod, len(addrs)))
    for a in addrs:
        f = fs[a]; r = res.get(a, {})
        fh.write("- 0x%s %s size=%d conf=%s :: %s\n" % (a, f["name"], f["size"], r.get("confidence", "?"), (r.get("summary") or "")[:200]))
batches = sorted(os.path.basename(p) for p in glob.glob(P("out", "phase2", mod, "*.md")))
named = sum(1 for a in addrs if not fs[a]["name"].startswith("FUN_"))
total = sum(fs[a]["size"] for a in addrs)
# split by cumulative size so the two rewrite agents get similar work
acc = 0; split = addrs[len(addrs) // 2]
for a in addrs:
    acc += fs[a]["size"]
    if acc >= total / 2: split = a; break
print(json.dumps({"module": mod, "functions": len(addrs), "named": named, "first": addrs[0], "last": addrs[-1], "split": split,
                  "batches": ["out/phase2/%s/%s" % (mod, b) for b in batches], "bytes": total}))
