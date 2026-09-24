"""Coverage audit: does every engine function have exactly one rewritten file?
Engine functions = modules.json entries whose module is not lib* (library code: CRT, D3DX, GameSpy, Bink, Vorbis, ...)
plus symbols/missing_functions.txt (the functions Ghidra's auto-analysis missed).
Each one must be defined by exactly one src/<module>/*.c (its 'address 0x...' header), or be explained: an address that
no file defines is accepted when some src file names it as a fragment / folded / not-a-function piece of another function.
Also lists src files whose address is not in the engine set, and files filed under a module other than modules.json's.
Writes out/coverage_audit.json and prints a summary.   Usage: python tools/coverage_audit.py"""
import re, glob, os, json
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
mods = json.load(open(os.path.join(ROOT, "modules.json")))
engine = {a.lower().lstrip("0"): v["module"] for a, v in mods.items() if not v["module"].startswith("lib")}
library = {a.lower().lstrip("0"): v["module"] for a, v in mods.items() if v["module"].startswith("lib")}
for line in open(os.path.join(ROOT, "symbols", "missing_functions.txt"), encoding="utf-8", errors="replace"):
    m = re.match(r"\s*0x0*([0-9a-f]+)", line, re.I)
    if m: engine.setdefault(m.group(1).lower(), "missed")

ADDR_HDR = re.compile(r"address\s+0x0*([0-9a-f]{6}),\s*size\s+(\d+)", re.I)
ADDR_ONLY = re.compile(r"address\s+0x0*([0-9a-f]{6})", re.I)
defined = defaultdict(list)   # addr -> [file]
spans = []                    # (start, end, file) of every rewritten function
ghidra = {}
fj = os.path.join(ROOT, "out", "functions.json")   # ExportMeta output (gitignored); used only to classify gaps
if os.path.exists(fj): ghidra = {x["addr"].lower().lstrip("0"): x for x in json.load(open(fj))}
alltext = {}
for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
    rel = os.path.relpath(p, ROOT).replace("\\", "/")
    text = open(p, encoding="utf-8", errors="replace").read()
    alltext[rel] = text
    m = ADDR_HDR.search(text[:4000]) or ADDR_ONLY.search(text[:4000])
    if m:
        defined[m.group(1).lower()].append(rel)
        if m.lastindex == 2: spans.append((int(m.group(1), 16), int(m.group(1), 16) + int(m.group(2)), rel))

FRAGMENT = re.compile(r"fragment|folded|outlined|not a (real |separate )?function|tail of|part of|inside", re.I)
def explained(addr):
    pat = re.compile(r"0x0*" + addr + r"\b", re.I)
    for rel, text in alltext.items():
        head = text[:text.rfind("#if 0")] if "#if 0" in text else text
        for m in pat.finditer(head):
            window = head[max(0, m.start() - 300): m.end() + 300]
            if FRAGMENT.search(window): return rel
    return None

def classify(a):
    """why an address with no file of its own needs none, or None for a real orphan"""
    g = ghidra.get(a, {}); name = g.get("name", ""); n = int(a, 16)
    for s, e, rel in spans:
        if s < n < e: return "inside " + rel            # a label or hook point inside a rewritten function
    if name.startswith("Catch@"): return "C++ EH catch funclet (part of its parent function)"
    if n >= 0x6b0000: return "not code: import/data address"
    if 0 < g.get("size", 99) <= 8: return "trivial stub (%d bytes)" % g["size"]
    return None

report = {"covered": 0, "duplicates": {}, "explained": {}, "missing": [], "not_engine": {}, "library_rewritten": {}, "module_mismatch": []}
for a, mod in sorted(engine.items()):
    files = defined.get(a, [])
    if len(files) == 1:
        report["covered"] += 1
        d = files[0].split("/")[1]
        if mod not in ("missed", "unknown") and d != mod: report["module_mismatch"].append({"addr": "0x" + a, "modules.json": mod, "file": files[0]})
    elif len(files) > 1: report["duplicates"]["0x" + a] = files
    else:
        why = classify(a) or explained(a)
        if why: report["explained"]["0x" + a] = why
        else: report["missing"].append({"addr": "0x" + a, "module": mod, "name": ghidra.get(a, {}).get("name", ""), "size": ghidra.get(a, {}).get("size", 0)})
for a, files in defined.items():
    if a not in engine:
        (report["library_rewritten"] if a in library else report["not_engine"])["0x" + a] = files
json.dump(report, open(os.path.join(ROOT, "out", "coverage_audit.json"), "w"), indent=1)
print("engine functions:", len(engine), "| exactly one file:", report["covered"], "| duplicates:", len(report["duplicates"]),
      "| explained fragments:", len(report["explained"]), "| missing:", len(report["missing"]))
print("files for addresses outside the engine set:", len(report["not_engine"]), "| library functions rewritten:",
      len(report["library_rewritten"]), "| filed under another module:", len(report["module_mismatch"]))
from collections import Counter
print("orphans by module:", dict(Counter(x["module"] for x in report["missing"]).most_common()), "bytes:", sum(x["size"] for x in report["missing"]))
print("explained by kind:", dict(Counter(v.split(" ")[0] for v in report["explained"].values())))
