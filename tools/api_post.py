"""Step B post-processing for tools/api_convert.py: python tools/api_post.py <module> name1 name2 ...  (names read from api.hpp when omitted)

Removes the module's names from standalone/data/code_refs.hpp and includes the module's api.hpp in the data tables that use it."""
import glob, os, re, sys

os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
mod = sys.argv[1]
api = open(f"include/halo/{mod}/api.hpp").read()
names = set(re.findall(r"\b(\w+)\([^()]*\);", api))
names |= set(sys.argv[2:])
refs = open("standalone/data/code_refs.hpp").read()
out = []
removed = 0
for l in refs.split("\n"):
    m = re.match(r"extern \w[\w \*]* \**(\w+)\(\);", l)
    if m and m.group(1) in names:
        removed += 1
        continue
    out.append(l)
open("standalone/data/code_refs.hpp", "w", newline="").write("\n".join(out))
inc = f'#include "halo/{mod}/api.hpp"'
for f in glob.glob("standalone/data/*.cpp"):
    s = open(f, encoding="utf-8", errors="replace").read()
    if f"halo::{mod}::" in s and inc not in s:
        s = s.replace('#include "code_refs.hpp"', '#include "code_refs.hpp"\n' + inc, 1)
        open(f, "w", encoding="utf-8", newline="").write(s)
        print("included in", f)
print("removed", removed, "declarations from code_refs.hpp")
