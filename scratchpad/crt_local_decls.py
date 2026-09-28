"""in files that include crt.h, local declarations of functions the C runtime headers already declare (found by asking
the compiler: every name crt.h declares)"""
import glob, json, re, shutil, subprocess, sys
R = "C:\\Users\\Liam-\\halo-re\\"
sys.path.insert(0, R + "tools")
import msvc_build as mb
env = mb.msvc_env()
cl = shutil.which("cl", path=env.get("PATH") or env.get("Path"))
open(R + "scratchpad\\crt_pp.c", "w").write('#include "crt.h"\n')
pp = subprocess.run([cl, "/nologo", "/E", "/I", R + "types", R + "scratchpad\\crt_pp.c"], capture_output=True, text=True,
                    env=env).stdout
crt_names = set(re.findall(r"\b([A-Za-z_]\w*)\s*\(", pp))
EXT = re.compile(r"^[ \t]*extern\s+[^;(]*?\b(\w+)\s*\([^;]*?\)\s*;[^\n]*\n", re.M | re.S)
hits = {}
for p in glob.glob(R + "src\\*\\*.c"):
    s = open(p, encoding="utf-8", errors="replace").read().split("\n#if 0")[0]
    if '#include "crt.h"' not in s and not re.search(r"#include <(string|stdio|stdlib|ctype|wchar|time)\.h>", s):
        continue
    names = [m.group(1) for m in EXT.finditer(s) if m.group(1) in crt_names]
    if names:
        hits[p[len(R):]] = names
json.dump(hits, open(R + "scratchpad\\crt_local_decls.json", "w"), indent=1)
print(sum(len(v) for v in hits.values()), "local declarations of runtime functions in", len(hits), "files")
import collections
print(collections.Counter(n for v in hits.values() for n in v).most_common(40))
