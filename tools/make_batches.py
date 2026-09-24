"""Phase 2 inputs: out/phase2/<module>/<NN>.md, batches of engine functions grouped by module in address order.
Each batch is a concatenation of context packs (tools/pack.py) with the decompiled C truncated per function so a
batch fits one agent context; agents can run pack.py for the full text of any function.
Usage: python tools/make_batches.py [functions_per_batch=30] [max_c_lines=120] [module ...]"""
import json, os, sys, shutil
from collections import defaultdict
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pack as P

ROOT = P.ROOT
N = int(sys.argv[1]) if len(sys.argv) > 1 else 30
MAXC = int(sys.argv[2]) if len(sys.argv) > 2 else 120
only = set(sys.argv[3:])
mods = json.load(open(os.path.join(ROOT, "modules.json")))
meta = P.meta()
dec = P.decomp()

def truncated_pack(addr):
    text = P.pack(addr)
    head, sep, c = text.partition("## decompiled C (Ghidra)\n```c\n")
    lines = c.split("\n")
    if len(lines) > MAXC:
        c = "\n".join(lines[:MAXC]) + "\n// ... truncated (%d more lines; run python tools/pack.py 0x%06x for all)\n```\n" % (len(lines) - MAXC, addr)
    return head + sep + c

by_mod = defaultdict(list)
for a, v in mods.items():
    m = v["module"]
    if m.startswith("lib") or (only and m not in only): continue
    by_mod[m].append(int(a, 16))

outroot = os.path.join(ROOT, "out", "phase2")
index = []
for m, addrs in sorted(by_mod.items()):
    addrs.sort()
    d = os.path.join(outroot, m)
    if os.path.isdir(d) and not only: shutil.rmtree(d)
    os.makedirs(d, exist_ok=True)
    for i in range(0, len(addrs), N):
        batch = addrs[i:i + N]
        fn = os.path.join(d, "%02d.md" % (i // N))
        with open(fn, "w", encoding="utf-8") as fh:
            fh.write("# module %s batch %02d: %d functions 0x%06x..0x%06x\n\n" % (m, i // N, len(batch), batch[0], batch[-1]))
            for a in batch:
                fh.write(truncated_pack(a)); fh.write("\n---\n\n")
        index.append({"module": m, "batch": i // N, "file": os.path.relpath(fn, ROOT).replace("\\", "/"), "addrs": ["0x%06x" % a for a in batch], "bytes": os.path.getsize(fn)})
json.dump(index, open(os.path.join(outroot, "index.json"), "w"), indent=0)
print("batches:", len(index), "functions:", sum(len(b["addrs"]) for b in index), "modules:", len(by_mod))
print("largest batch bytes:", max(b["bytes"] for b in index), " avg:", sum(b["bytes"] for b in index) // len(index))
