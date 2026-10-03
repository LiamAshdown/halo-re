"""python tools/api_globals.py <module> <mapping...>  where mapping is  c_name=member  : replace uses of the module-owned global c_name in other
modules by halo::<module>::globals().member and drop their extern declarations. Files that define a local/member of the same
name must be excluded with  -x path.
"""
import glob, os, re, sys

os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
mod = sys.argv[1]
maps = dict(a.split("=") for a in sys.argv[2:] if "=" in a)
excl = [a[3:] for a in sys.argv[2:] if a.startswith("-x=")]
STRRE = re.compile(r'"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'|//[^\n]*|/\*.*?\*/', re.S)


def code_sub(text, fn):
    out, last = [], 0
    for m in STRRE.finditer(text):
        out.append(fn(text[last:m.start()]))
        out.append(m.group(0))
        last = m.end()
    out.append(fn(text[last:]))
    return "".join(out)


names = list(maps)
nre = re.compile(r"(?<![\w:.>])(" + "|".join(map(re.escape, names)) + r")\b")
inc = f'#include "halo/{mod}/api.hpp"'
for f in glob.glob("src/**/*.cpp", recursive=True) + glob.glob("include/**/*.hpp", recursive=True):
    f = f.replace("\\", "/")
    if f.startswith(f"src/{mod}/") or f.startswith(f"include/halo/{mod}/") or f in excl:
        continue
    s = open(f, encoding="utf-8", errors="replace", newline="").read()
    if not nre.search(code_sub(s, lambda t: t)):
        continue
    o = s
    for n in names:
        s = re.sub(r'^[ \t]*extern\s+"C"\s*\{\s*extern\s+[^;{}()]*?\b' + re.escape(n) + r'(\[[^\]]*\])?\s*;\s*\}[ \t]*\r?\n', "", s, flags=re.M)
        s = re.sub(r"^[ \t]*extern\s+[^;{}()]*?\b" + re.escape(n) + r"(\[[^\]]*\])?\s*;[ \t]*\r?\n", "", s, flags=re.M)
    s = code_sub(s, lambda t: nre.sub(lambda m: f"halo::{mod}::globals().{maps[m.group(1)]}", t))
    if s != o and inc not in s:
        incs = list(re.finditer(r"^#include[^\n]*\n", s, re.M))
        p = incs[-1].end() if incs else 0
        s = s[:p] + inc + "\n" + s[p:]
    if s != o:
        open(f, "w", encoding="utf-8", newline="").write(s)
        print("edited", f)
