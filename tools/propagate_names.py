"""Propagate canonical function names to call sites.
Canonical name for an address = the file name of the src/<module>/<name>.c whose header says 'address 0xADDR'.
In every src file, above the '#if 0' block only, each extern prototype carrying an address comment whose name differs
from the canonical name is renamed, together with every whole-word use of the old name in that same region.
If the file already declares the canonical name, the stale prototype line is dropped instead of duplicated.
Usage: python tools/propagate_names.py [--apply]   (default is a dry run)"""
import re, glob, os, sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APPLY = "--apply" in sys.argv
files = glob.glob(os.path.join(ROOT, "src", "*", "*.c"))
ADDR_HDR = re.compile(r"address\s+0x0{0,2}([0-9a-f]{6})", re.I)
canonical = {}
for p in files:
    m = ADDR_HDR.search(open(p, encoding="utf-8", errors="replace").read(4000))
    if m: canonical[m.group(1).lower()] = os.path.splitext(os.path.basename(p))[0]

EXTERN_LINE = re.compile(r"^(\s*extern\s+[^;(]*?\b)([A-Za-z_][A-Za-z0-9_]*)(\s*\((?!\s*\*)[^;]*\)\s*;[^\n]*?0x0{0,2}([45][0-9a-f]{5})\b[^\n]*)$", re.M | re.I)
stats = Counter(); changed_files = []
SKIP = {'actor_movement_test_obstacle_ray.c', 'ai_search_evaluate_edge_cost.c'}  # deliberate per-call-shape aliases; resolve in reconciliation
for p in files:
    if os.path.basename(p) in SKIP: continue
    text = open(p, encoding="utf-8", errors="replace").read()
    cut = text.rfind("#if 0")
    body, tail = (text, "") if cut < 0 else (text[:cut], text[cut:])
    own = os.path.splitext(os.path.basename(p))[0]
    renames = {}
    for m in EXTERN_LINE.finditer(body):
        old, a = m.group(2), m.group(4).lower()
        new = canonical.get(a)
        if new and new != old and old != own and old not in {'void','int','char','short','long','float','double','unsigned','signed','struct','const'} and not old.endswith('_t'): renames[old] = new
    if not renames: continue
    new_body = body
    for old, new in renames.items():
        declared_new = re.search(r"^\s*extern\s[^;(]*\b" + re.escape(new) + r"\s*\(", new_body, re.M)
        if declared_new:
            # drop the stale prototype line, then rename remaining uses
            new_body = re.sub(r"^\s*extern\s[^;(]*\b" + re.escape(old) + r"\s*\([^;]*\)\s*;[^\n]*\n", "", new_body, count=1, flags=re.M)
            stats["duplicate-prototype-dropped"] += 1
        new_body = re.sub(r"\b" + re.escape(old) + r"\b", new, new_body)
        stats["renamed"] += 1
    if new_body != body:
        changed_files.append(os.path.relpath(p, ROOT))
        if APPLY:
            open(p, "w", encoding="utf-8", newline="").write(new_body + tail)
print(("APPLIED" if APPLY else "DRY RUN"), dict(stats), "files:", len(changed_files))
