"""the decompiler's names for C runtime functions -> the standard names, their local declarations removed, crt.h in"""
import glob, json, re
R = "C:\\Users\\Liam-\\halo-re\\"
MAP = json.load(open(R + "scratchpad\\crt_map.json"))
std = set(MAP.values())
ren = re.compile(r"\b(%s)\b" % "|".join(sorted(MAP, key=len, reverse=True)))
decl = re.compile(r"^[ \t]*extern\s+[^;(]*?\b(?:%s)\s*\([^;]*?\)\s*;[^\n]*\n" % "|".join(sorted(std, key=len, reverse=True)),
                  re.M | re.S)
files = renamed = removed = 0
for p in glob.glob(R + "src\\*\\*.c"):
    s = open(p, encoding="utf-8", errors="replace").read()
    cut = s.find("\n#if 0")
    head, tail = (s, "") if cut < 0 else (s[:cut], s[cut:])
    new, n = ren.subn(lambda m: MAP[m.group(1)], head)
    if n == 0:
        continue
    new, k = decl.subn("", new)
    if '#include "crt.h"' not in new:
        i = new.find("#include")
        new = new[:i] + '#include "crt.h"\n' + new[i:]
    open(p, "w", encoding="utf-8", newline="\n").write(new + tail)
    files += 1
    renamed += n
    removed += k
print(renamed, "uses renamed,", removed, "local runtime declarations removed,", files, "files include crt.h")
