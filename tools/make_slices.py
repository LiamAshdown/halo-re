"""Build Phase 1 inputs: out/slices/NN.md, one per contiguous address range of non-lib functions.
MSVC links object files contiguously, so module boundaries fall along the address axis; each slice
lists functions in address order with the evidence an agent needs to assign a module.
Usage: python tools/make_slices.py [functions_per_slice]"""
import json, os, sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
N = int(sys.argv[1]) if len(sys.argv) > 1 else 300
funcs = json.load(open(os.path.join(ROOT, "out", "functions.json")))
mods = json.load(open(os.path.join(ROOT, "modules.json"))) if os.path.exists(os.path.join(ROOT, "modules.json")) else {}
cea = json.load(open(os.path.join(ROOT, "symbols", "candidates_cea.json")))
cea_by_string = {}
for name, v in cea.items():
    for s in v.get("strings", []): cea_by_string.setdefault(s, []).append((name, v["module"]))
byaddr = {f["addr"]: f for f in funcs}

def short(f):
    a = f["addr"][-6:]
    m = mods.get(a)
    tag = ("[%s %.1f]" % (m["module"], m["confidence"])) if m else "[?]"
    callees = []
    for c in f["callees"]:
        ca, cn = c.split(":", 1)
        cm = mods.get(ca[-6:])
        callees.append(cn if not cn.startswith("FUN_") else ("FUN_" + ca[-6:] + ("(" + cm["module"] + ")" if cm else "")))
    strs = [s[:60] for s in f["strings"][:6]]
    hints = Counter()
    for s in f["strings"]:
        for name, module in cea_by_string.get(s.strip(), []): hints[module] += 1
    line = "0x%s %s %s size=%d callers=%d" % (a, tag, f["name"], f["size"], f["callers"])
    if callees: line += "\n    calls: " + ", ".join(callees[:25])
    if strs: line += "\n    strings: " + " | ".join(json.dumps(s) for s in strs)
    if hints: line += "\n    cea-string-hint: " + ", ".join("%s(%d)" % kv for kv in hints.most_common(3))
    return line

work = [f for f in funcs if not (mods.get(f["addr"][-6:], {}).get("module") == "lib")]
work.sort(key=lambda f: f["addr"])
outdir = os.path.join(ROOT, "out", "slices"); os.makedirs(outdir, exist_ok=True)
for old in os.listdir(outdir): os.remove(os.path.join(outdir, old))
slices = [work[i:i + N] for i in range(0, len(work), N)]
for i, sl in enumerate(slices):
    with open(os.path.join(outdir, "%02d.md" % i), "w", encoding="utf-8") as fh:
        fh.write("# slice %02d: 0x%s .. 0x%s (%d functions)\n\n" % (i, sl[0]["addr"][-6:], sl[-1]["addr"][-6:], len(sl)))
        fh.write("Format: addr [seed-module confidence] name size callers, then callees / strings / hints.\n\n")
        for f in sl: fh.write(short(f) + "\n")
print("slices:", len(slices), "functions:", len(work), "lib skipped:", len(funcs) - len(work))
print("seeded:", sum(1 for f in work if f["addr"][-6:] in mods))
