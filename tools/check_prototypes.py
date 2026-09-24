"""Cross-file prototype check: every extern declaration of a rewritten function must agree with its definition.
C does not check this across translation units, so a caller that declares (out, in, m) for a function defined as
(in, m, out) compiles cleanly and passes the wrong arguments. Compares, per parameter position, the kind of each
parameter (pointer / float / integer / 8-byte), the parameter count, and the return kind.
Writes out/prototype_mismatches.json; prints a summary.   Usage: python tools/check_prototypes.py [module ...]"""
import os, re, glob, json, sys, collections

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEF = re.compile(r"^(?!extern|static|typedef|#|//|\s)([A-Za-z_][\w \t\*]*?)\b([A-Za-z_]\w*)\s*\(([^;{]*?)\)\s*(?://[^\n]*)?\s*\{", re.M)
EXT = re.compile(r"^[ \t]*extern\s+([^;(]*?)\b([A-Za-z_]\w*)\s*\(([^;]*?)\)\s*;", re.M)

def split_params(s):
    out, depth, cur = [], 0, ""
    s = re.sub(r"/\*.*?\*/|//[^\n]*", "", s)
    for ch in s:
        if ch == "(": depth += 1
        elif ch == ")": depth -= 1
        if ch == "," and depth == 0: out.append(cur.strip()); cur = ""
        else: cur += ch
    if cur.strip(): out.append(cur.strip())
    return [p for p in out if p not in ("void", "")]

def kind(t):
    t = re.sub(r"\b(const|volatile|struct|enum|register|unsigned|signed)\b", " ", t)
    if "*" in t or "[" in t or re.search(r"\(\s*\*", t): return "p"
    if re.search(r"\b(double|int64_t|uint64_t|__int64)\b", t): return "d"
    if re.search(r"\b(float|real|angle)\b", t): return "f"
    return "i"

def ret_kind(prefix):
    prefix = re.sub(r"\b(extern|static|__cdecl|__stdcall|__fastcall|__thiscall|inline)\b", " ", prefix).strip()
    if re.fullmatch(r"void", prefix): return "v"
    return kind(prefix)

def body(t): return t[:t.rfind("#if 0")] if "#if 0" in t else t

mods = sys.argv[1:]
files = [p for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")) if not mods or os.path.basename(os.path.dirname(p)) in mods]
defs = {}
for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
    name = os.path.splitext(os.path.basename(p))[0]
    for m in DEF.finditer(body(open(p, encoding="utf-8", errors="replace").read())):
        if m.group(2) == name:
            defs[name] = (ret_kind(m.group(1)), [kind(x) for x in split_params(m.group(3))], os.path.relpath(p, ROOT))
mism = []
for p in files:
    t = body(open(p, encoding="utf-8", errors="replace").read())
    for m in EXT.finditer(t):
        n = m.group(2)
        if n not in defs or "(*" in m.group(0).split(n)[0][-3:]: continue
        dr, dp, dfile = defs[n]
        er, ep = ret_kind(m.group(1)), [kind(x) for x in split_params(m.group(3))]
        problems = []
        if len(ep) != len(dp): problems.append(f"{len(ep)} parameters declared, {len(dp)} defined")
        else:
            diffs = [i for i, (a, b) in enumerate(zip(ep, dp)) if a != b]
            if diffs: problems.append("parameter kinds differ at " + ", ".join(f"#{i + 1} ({ep[i]} vs {dp[i]})" for i in diffs))
        if er != dr and not (er in "iv" and dr in "iv"): problems.append(f"return {er} vs {dr}")
        if problems:
            mism.append({"caller": os.path.relpath(p, ROOT), "callee": n, "definition": dfile, "declared": m.group(0).strip()[:200], "problems": problems})
json.dump(mism, open(os.path.join(ROOT, "out", "prototype_mismatches.json"), "w"), indent=1)
by = collections.Counter(m["caller"].split(os.sep)[1] for m in mism)
print("extern declarations that disagree with the definition:", len(mism), "in", len({m['caller'] for m in mism}), "files;", dict(by.most_common()))
