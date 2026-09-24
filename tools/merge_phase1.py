"""Merge Phase 1 agent outputs (out/phase1/*.json) into modules.json.
Precedence per address: resolve (tie-break) > assign; big20 overrides only when its confidence is higher.
Existing modules.json entries win when their confidence is strictly higher (lib=1.0 from FID always wins).
Also reports coverage and run structure so the result can be sanity-checked before Phase 2."""
import json, glob, os, re, sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
P = lambda *a: os.path.join(ROOT, *a)
VALID = set("""ai bitmaps cache camera cseries cutscene devices dialogs effects game hs input interface items main math
memory models networking objects physics rasterizer render saved_games scenario shaders shell sound structures tag_files
text units lib:d3dx lib:crt lib:gamespy lib:bink lib:vorbis lib:other unknown lib""".split())

funcs = json.load(open(P("out", "functions.json")))
addrs = {f["addr"][-6:] for f in funcs}
mods = json.load(open(P("modules.json"))) if os.path.exists(P("modules.json")) else {}

def norm_addr(a):
    a = a.lower().replace("0x", "")
    return a[-6:].rjust(6, "0")

def put(a, module, conf, ev, src):
    a = norm_addr(a)
    if a not in addrs:
        bad.append((src, a)); return
    module = module.strip()
    if module not in VALID:
        badmod[module] += 1; module = "unknown"; conf = min(conf, 0.3)
    cur = mods.get(a)
    if cur is None or cur["confidence"] < conf or (src.startswith("resolve") and cur.get("source", "").startswith("assign")):
        mods[a] = {"module": module, "confidence": round(float(conf), 2), "evidence": (ev or "")[:200], "source": src}

bad, badmod = [], Counter()
for f in sorted(glob.glob(P("out", "phase1", "assign_*.json"))):
    d = json.load(open(f, encoding="utf-8"))
    for x in d.get("assignments", []): put(x["addr"], x["module"], x["confidence"], x.get("evidence"), "assign:" + os.path.basename(f)[7:9])
for f in sorted(glob.glob(P("out", "phase1", "resolve_*.json"))):
    d = json.load(open(f, encoding="utf-8"))
    for x in d.get("decisions", []): put(x["addr"], x["module"], x["confidence"], x.get("evidence"), "resolve:" + os.path.basename(f)[8:10])
if os.path.exists(P("out", "phase1", "big20.json")):
    big = json.load(open(P("out", "phase1", "big20.json"), encoding="utf-8"))
    for x in (big if isinstance(big, list) else big.get("functions", [])):
        put(x["addr"], x["module"], x["confidence"], x.get("suggested_name", "") + ": " + x.get("what_it_does", ""), "big20")

json.dump(dict(sorted(mods.items())), open(P("modules.json"), "w"), indent=0)

# report
by_mod = Counter(v["module"] for v in mods.values())
unassigned = sorted(addrs - set(mods))
low = sum(1 for v in mods.values() if v["confidence"] < 0.5)
print("modules.json: %d / %d functions assigned; unassigned %d; low-confidence(<0.5) %d; invalid-addr %d; invalid-module %s"
      % (len(mods), len(addrs), len(unassigned), low, len(bad), dict(badmod) or 0))
print(by_mod.most_common())
# run structure: count contiguous runs per module in address order (many tiny runs = suspicious)
order = sorted(mods.items())
runs = Counter(); prev = None
for a, v in order:
    if v["module"] != prev: runs[v["module"]] += 1; prev = v["module"]
print("runs per module (fewer is more plausible):", sorted(runs.items(), key=lambda kv: -kv[1])[:15])
if unassigned: print("unassigned sample:", unassigned[:20])
