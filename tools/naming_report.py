"""Naming consistency report over src/ -> out/naming_report.json and a summary on stdout.
Finds:
  1. placeholder file/function names (FUN_xxxxxx, missed_xxxxxx, caseD_, LAB_)
  2. definition collisions: one function name defined at two different addresses
  3. address collisions: one address defined by two files
  4. stale call-site names: an extern prototype annotated with an address (// 0x004xxxxx or 0x5xxxxx) whose
     name differs from the name of the file that defines that address"""
import re, glob, os, json
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
files = glob.glob(os.path.join(ROOT, "src", "*", "*.c"))

defs_by_addr = defaultdict(list)   # addr -> [(name, path)]
defs_by_name = defaultdict(set)    # name -> {addr}
placeholders = []
ADDR_HDR = re.compile(r"address\s+0x0{0,2}([0-9a-f]{6})", re.I)
for p in files:
    name = os.path.splitext(os.path.basename(p))[0]
    head = open(p, encoding="utf-8", errors="replace").read(4000)
    m = ADDR_HDR.search(head)
    if not m: continue
    a = m.group(1).lower()
    defs_by_addr[a].append((name, os.path.relpath(p, ROOT).replace("\\", "/")))
    defs_by_name[name].add(a)
    if re.match(r"(FUN_|missed_|caseD_|LAB_|thunk_FUN)", name): placeholders.append((a, name, os.path.relpath(p, ROOT).replace("\\", "/")))

addr_collisions = {a: v for a, v in defs_by_addr.items() if len(v) > 1}
name_collisions = {n: sorted(v) for n, v in defs_by_name.items() if len(v) > 1}
canonical = {a: v[0][0] for a, v in defs_by_addr.items()}

# extern prototypes with a code address comment
EXTERN = re.compile(r"^\s*extern\s+[^;(]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*;[^\n]*?0x0{0,2}([45][0-9a-f]{5})\b", re.M | re.I)
stale = []
for p in files:
    text = open(p, encoding="utf-8", errors="replace").read()
    body = text.split("#if 0")[0]
    for m in EXTERN.finditer(body):
        name, a = m.group(1), m.group(2).lower()
        want = canonical.get(a)
        if want and want != name:
            stale.append({"file": os.path.relpath(p, ROOT).replace("\\", "/"), "addr": "0x" + a, "declared": name, "defined": want})

report = {"placeholders": placeholders, "address_collisions": addr_collisions, "name_collisions": name_collisions, "stale_externs": stale}
json.dump(report, open(os.path.join(ROOT, "out", "naming_report.json"), "w"), indent=1)
print("files with an address header:", sum(len(v) for v in defs_by_addr.values()))
print("placeholder names:", len(placeholders))
print("address defined by >1 file:", len(addr_collisions))
print("name defined at >1 address:", len(name_collisions))
print("stale extern names at call sites:", len(stale), "in", len({s['file'] for s in stale}), "files")
