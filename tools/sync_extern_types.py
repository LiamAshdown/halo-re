"""Copy a verified definition's prototype into callers' extern declarations when only the TYPES disagree
(same parameter count): out/prototype_mismatches.json entries of kind "parameter kinds differ" or "return",
for callees the hook generator accepts (their register convention matches the binary: harness/gen/hook_table.c).
The caller's extern statement is replaced by  extern <definition return type> name(<definition parameters>);
and any comment after it on the same line is kept. Callers that then fail to compile, or gain a type warning,
are left for a person (revert them with --verify).
Usage: python tools/sync_extern_types.py [--apply]"""
import os, re, sys, json, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); import gen_hooks as g

def main():
    apply = "--apply" in sys.argv
    mism = json.load(open(os.path.join(ROOT, "out", "prototype_mismatches.json")))
    tab = open(os.path.join(ROOT, "harness", "gen", "hook_table.c")).read()
    adapted = {n for n, mo, s in re.findall(r'"(\w+)", "(\w+)", ([01])\}', tab)}
    defs = {}
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        name = os.path.splitext(os.path.basename(p))[0]
        t = open(p, encoding="utf-8", errors="replace").read(); b = t[:t.rfind("#if 0")] if "#if 0" in t else t
        for m in g.DEF.finditer(b):
            if m.group(2) == name:
                prefix = re.sub(r"\b(static|inline)\b", "", m.group(1)).strip()
                params = re.sub(r"/\*.*?\*/|//[^\n]*", "", m.group(3), flags=re.S)
                params = re.sub(r"\s+", " ", params).strip() or "void"
                defs[name] = f"extern {prefix} {name}({params});"
    changed = {}
    for x in mism:
        prob = " ".join(x["problems"])
        if "parameters declared" in prob or x["callee"] not in adapted or x["callee"] not in defs: continue
        p = os.path.join(ROOT, x["caller"]); t = open(p, encoding="utf-8", errors="replace").read()
        cut = t.rfind("#if 0"); b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        pat = re.compile(r"^[ \t]*extern\s+[^;{}]*?\b" + re.escape(x["callee"]) + r"\s*\([^;{}]*?\)\s*;", re.M)
        m = pat.search(b)
        if not m: continue
        nb = b[:m.start()] + defs[x["callee"]] + b[m.end():]
        if nb != b:
            changed.setdefault(p, 0); changed[p] += 1
            if apply: open(p, "w", encoding="utf-8", newline="").write(nb + tail)
    print(("updated" if apply else "would update"), sum(changed.values()), "declarations in", len(changed), "files")
    if apply: open(os.path.join(ROOT, "build", "sync_extern_changed.txt"), "w").write("\n".join(sorted(changed)))

if __name__ == "__main__": main()
