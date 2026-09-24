"""halocea/src/blam -> symbols/candidates_cea.json
{name: {module, file, args, strings[]}}  (names only; no code is copied)"""
import re, glob, os, json, sys
from collections import Counter

root = sys.argv[1] if len(sys.argv) > 1 else "vendor/halocea/src/blam"
out = sys.argv[2] if len(sys.argv) > 2 else "symbols/candidates_cea.json"

defre = re.compile(
    r"^(?:static\s+|inline\s+|__forceinline\s+)*"
    r"(?:[A-Za-z_][A-Za-z0-9_:<>]*\s*[*&]*\s+)+[*]*([A-Za-z_][A-Za-z0-9_]*)\s*\(([^;{]*?)\)\s*(?:const\s*)?\{",
    re.M)
strre = re.compile(r'"((?:[^"\\]|\\.){3,})"')
KW = {"if", "for", "while", "switch", "return", "sizeof", "else", "do"}

funcs = {}
per_mod = Counter()
for f in glob.glob(root + "/**/*.c*", recursive=True):
    rel = os.path.relpath(f, root).replace("\\", "/")
    mod = rel.split("/")[0]
    t = open(f, encoding="utf-8", errors="ignore").read()
    for m in defre.finditer(t):
        name = m.group(1)
        if name in KW:
            continue
        i, depth = m.end(), 1
        while i < len(t) and depth:
            depth += (t[i] == "{") - (t[i] == "}")
            i += 1
        body = t[m.end():i]
        lits = sorted(set(strre.findall(body)))[:20]
        if name not in funcs:
            funcs[name] = {"module": mod, "file": rel, "args": " ".join(m.group(2).split()), "strings": lits}
            per_mod[mod] += 1

json.dump(funcs, open(out, "w"), indent=0)
print(len(funcs), "CEA functions;", sum(1 for v in funcs.values() if v["strings"]), "with string literals")
print(per_mod.most_common(40))
