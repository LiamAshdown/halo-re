"""OpenSauce Halo1_CE 1.10 client constants -> symbols/opensauce.txt
Line format: 0xADDR name kind confidence source[ inside=0x...]
kind: func (function entry), code (address inside code: hook/call/ref site), data (global)."""
import re, glob, os, sys, csv, bisect
from collections import Counter
root, out, funcs_csv = sys.argv[1:4]           # Halo1_CE dir, output, out/functions.csv
consts = {}
for f in glob.glob(os.path.join(root, "Memory/1.10/Pointers/HaloCE_110_Runtime*.inl")):
    for m in re.finditer(r"#define\s+(K_[A-Z0-9_]+)\s+(0x[0-9A-Fa-f]+)", open(f, encoding="utf-8", errors="ignore").read()):
        consts[m.group(1)] = int(m.group(2), 16)
macro_of = {}
for f in glob.glob(root + "/**/*.*", recursive=True):
    if not f.endswith((".inl", ".cpp", ".hpp", ".h", ".c")): continue
    t = open(f, encoding="utf-8", errors="ignore").read()
    for m in re.finditer(r"\b(FUNC_PTR|ENGINE_PTR|ENGINE_DPTR|DATA_PTR|CAST_PTR|GET_FUNC_PTR|GET_PTR|GET_DPTR)\s*\(([^;{]*?)\)", t, re.S):
        for k in re.findall(r"\bK_[A-Z0-9_]+\b", m.group(2)):
            macro_of.setdefault(k, set()).add(m.group(1))
entries = {}
with open(funcs_csv, newline="") as fh:
    for row in csv.DictReader(fh):
        entries[int(row["address"], 16)] = int(row["size"])
starts = sorted(entries)
def containing(a):
    i = bisect.bisect_right(starts, a) - 1
    if i >= 0 and starts[i] <= a < starts[i] + entries[starts[i]]: return starts[i]
    return None
SITE = re.compile(r"_(HOOK|RETN|RET|CALL|JMP|REF|REFERENCE|OFFSET)(_\d+|_16BIT_\d+)?$|_(CALL|REF|REFERENCE|COUNT)_\d+$")
rows = []
for k, addr in sorted(consts.items(), key=lambda kv: kv[1]):
    name = k[2:].lower(); ms = macro_of.get(k, set()); src = "opensauce"
    if "FUNC_PTR" in ms or "GET_FUNC_PTR" in ms:
        kind, conf = ("code", "0.9") if SITE.search(k) else ("func", "1.0")
    elif ms & {"ENGINE_PTR", "ENGINE_DPTR", "DATA_PTR", "GET_PTR", "GET_DPTR"}:
        kind, conf = "data", "1.0"
    elif "CAST_PTR" in ms:
        kind, conf = "code", "0.9"
    else:
        src = "opensauce-unref"
        kind, conf = ("code", "0.7") if SITE.search(k) else ("func" if addr in entries else "data", "0.6")
    extra = ""
    if kind in ("func", "code"):
        c = containing(addr)
        if c is not None and c != addr: extra = f" inside=0x{c:06x}"
        elif c is None and kind == "func": extra = " nofunc"
    rows.append((addr, name, kind, conf, src + extra))
with open(out, "w") as fh:
    fh.write("# addr name kind confidence source\n")
    for r in rows: fh.write("0x%06x %s %s %s %s\n" % r)
c = Counter(r[2] for r in rows)
print("consts", len(consts), dict(c), "func-with-ghidra-entry", sum(1 for r in rows if r[2]=="func" and r[0] in entries),
      "func-nofunc", sum(1 for r in rows if "nofunc" in r[4]), "func-inside", sum(1 for r in rows if r[2]=="func" and "inside" in r[4]))
