import json, re
R = "C:\\Users\\Liam-\\halo-re\\"
hits = json.load(open(R + "scratchpad\\crt_local_decls.json"))
n_files = n_decls = 0
for rel, names in hits.items():
    names = [n for n in names if n not in ("void", "__declspec")]
    if not names:
        continue
    p = R + rel
    s = open(p, encoding="utf-8").read()
    cut = s.find("\n#if 0")
    head, tail = (s, "") if cut < 0 else (s[:cut], s[cut:])
    pat = re.compile(r"^[ \t]*extern\s+[^;(]*?\b(?:%s)\s*\([^;]*?\)\s*;[^\n]*\n" % "|".join(map(re.escape, names)), re.M | re.S)
    head, k = pat.subn("", head)
    if '#include "crt.h"' not in head:
        i = head.find("#include")
        head = head[:i] + '#include "crt.h"\n' + head[i:]
    open(p, "w", encoding="utf-8", newline="\n").write(head + tail)
    n_files += 1
    n_decls += k
print(n_decls, "declarations removed in", n_files, "files")
