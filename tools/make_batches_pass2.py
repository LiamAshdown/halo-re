"""Naming pass 2 inputs: out/phase2/p2_<module>/NN.md for modules NOT yet rewritten, covering only functions that are
still FUN_/sig__ in Ghidra or whose pass-1 name is parked below threshold. Each pack carries the parked proposal
(name, confidence, evidence) as an explicit hint. Same pack format as pass 1 so the naming workflow can run unchanged
with module name "p2_<module>".
Usage: python tools/make_batches_pass2.py [functions_per_batch=60] [max_c_lines=120]"""
import json, os, sys, shutil
from collections import defaultdict
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pack as P

ROOT = P.ROOT
N = int(sys.argv[1]) if len(sys.argv) > 1 else 60
MAXC = int(sys.argv[2]) if len(sys.argv) > 2 else 120
REWRITTEN = {d for d in os.listdir(os.path.join(ROOT, "src")) if os.path.isdir(os.path.join(ROOT, "src", d))}
mods = json.load(open(os.path.join(ROOT, "modules.json")))
meta = P.meta()

parked = {}
for fn in ("review_queue.txt", "review_queue_mech.txt"):
    p = os.path.join(ROOT, "symbols", fn)
    if not os.path.exists(p): continue
    for line in open(p, encoding="utf-8", errors="replace"):
        if line.startswith("#") or not line.strip(): continue
        t = line.split(None, 3)
        a = int(t[0], 16)
        if a not in parked or float(t[2]) > parked[a][1]: parked[a] = (t[1], float(t[2]), t[3].strip() if len(t) > 3 else "")

def truncated_pack(addr):
    text = P.pack(addr)
    head, sep, c = text.partition("## decompiled C (Ghidra)\n```c\n")
    hint = ""
    if addr in parked:
        n, conf, ev = parked[addr]
        hint = "\n## pass-1 proposal (parked, verify): %s  conf=%.2f\n%s\n" % (n, conf, ev[:400])
    lines = c.split("\n")
    if len(lines) > MAXC:
        c = "\n".join(lines[:MAXC]) + "\n// ... truncated (%d more lines; run python tools/pack.py 0x%06x for all)\n```\n" % (len(lines) - MAXC, addr)
    return head + hint + sep + c

by_mod = defaultdict(list)
for a, v in mods.items():
    m = v["module"]
    if m.startswith("lib") or m in REWRITTEN: continue
    ai = int(a, 16); f = meta.get(ai)
    if f is None: continue
    unnamed = f["name"].startswith(("FUN_", "thunk_FUN", "sig__"))
    if unnamed or ai in parked: by_mod[m].append(ai)

index = []
for m, addrs in sorted(by_mod.items()):
    addrs.sort()
    d = os.path.join(ROOT, "out", "phase2", "p2_" + m)
    if os.path.isdir(d): shutil.rmtree(d)
    os.makedirs(d)
    for i in range(0, len(addrs), N):
        batch = addrs[i:i + N]
        fn = os.path.join(d, "%02d.md" % (i // N))
        with open(fn, "w", encoding="utf-8") as fh:
            fh.write("# module %s (pass 2) batch %02d: %d functions 0x%06x..0x%06x\n\n" % (m, i // N, len(batch), batch[0], batch[-1]))
            for a in batch: fh.write(truncated_pack(a)); fh.write("\n---\n\n")
        index.append({"module": "p2_" + m, "batch": i // N, "file": os.path.relpath(fn, ROOT).replace("\\", "/"), "addrs": ["0x%06x" % a for a in batch], "bytes": os.path.getsize(fn)})
json.dump(index, open(os.path.join(ROOT, "out", "phase2", "index_pass2.json"), "w"), indent=0)
print("pass-2 batches:", len(index), "functions:", sum(len(b["addrs"]) for b in index), "modules:", len(by_mod))
for m in sorted(by_mod, key=lambda m: -len(by_mod[m])): print("  %-12s %4d" % (m, len(by_mod[m])))
