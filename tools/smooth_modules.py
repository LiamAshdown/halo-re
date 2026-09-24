"""Contiguity smoothing: a run of <= MAXLEN functions flanked on both sides by the same module is
reassigned to that module unless its own confidence is >= KEEP. Evidence is recorded; nothing is deleted."""
import json, sys
MAXLEN = int(sys.argv[1]) if len(sys.argv) > 1 else 2
KEEP = 0.85
mods = json.load(open("modules.json"))
order = sorted(mods.items())
runs = []
for a, v in order:
    if runs and runs[-1]["m"] == v["module"]: runs[-1]["items"].append(a)
    else: runs.append({"m": v["module"], "items": [a]})
changed = 0
for i in range(1, len(runs) - 1):
    r = runs[i]
    if len(r["items"]) > MAXLEN or r["m"].startswith("lib"): continue
    left, right = runs[i - 1]["m"], runs[i + 1]["m"]
    if left != right or left.startswith("lib") or left == "unknown": continue
    for a in r["items"]:
        v = mods[a]
        if v["confidence"] >= KEEP: continue
        mods[a] = {"module": left, "confidence": min(0.7, max(v["confidence"], 0.5)), "source": "smooth",
                   "evidence": "contiguity: lone %s inside %s run (was %.2f: %s)" % (v["module"], left, v["confidence"], v["evidence"][:100])}
        changed += 1
json.dump(dict(sorted(mods.items())), open("modules.json", "w"), indent=0)
print("smoothed", changed)
