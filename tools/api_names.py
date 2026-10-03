"""python tools/api_names.py <module> name1 name2 ...  (step B helper for functions defined outside <module>_api.cpp, e.g. variadic ones)

The functions must already be declared in include/halo/<module>/api.hpp. Replaces calls in every other file by halo::<module>::name, deletes the
callers' extern declarations (plain or inside a one-line extern "C" block) and includes the api header."""
import glob, os, re, sys

os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
mod = sys.argv[1]
names = sys.argv[2:]
alt = "|".join(map(re.escape, names))
decl = re.compile(r'^[ \t]*(?:extern\s+"C"\s*\{\s*)?extern\s+(?:"C"\s+)?[^;{}()]*?\b(?:' + alt + r')\s*\((?:[^;{}()]|\([^()]*\))*\)\s*;[ \t]*\r?\n', re.M)
decl2 = re.compile(r'^[ \t]*extern\s+"C"\s*\{\s*[^;{}()]*?\b(?:' + alt + r')\s*\((?:[^;{}()]|\([^()]*\))*\)\s*;\s*\}[ \t]*\r?\n', re.M)
call = re.compile(r'(?<![\w:.>&"\'])(' + alt + r')(?=\s*\()')
TOK = re.compile(r'"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'|//[^\n]*|/\*.*?\*/', re.S)


def code_sub(text, fn):
    out, last = [], 0
    for m in TOK.finditer(text):
        out.append(fn(text[last:m.start()]))
        out.append(m.group(0))
        last = m.end()
    out.append(fn(text[last:]))
    return "".join(out)


inc = f'#include "halo/{mod}/api.hpp"'
for f in glob.glob("src/**/*.cpp", recursive=True) + glob.glob("src/**/*.hpp", recursive=True) + glob.glob("include/**/*.hpp", recursive=True) + glob.glob("standalone/**/*.cpp", recursive=True):
    f = f.replace("\\", "/")
    if f == f"include/halo/{mod}/api.hpp":
        continue
    s = open(f, encoding="utf-8", errors="replace", newline="").read()
    if not re.search(r'\b(?:' + alt + r')\b', s):
        continue
    o = s
    s = decl2.sub("", decl.sub("", s))
    s = code_sub(s, lambda t: call.sub(lambda m: f"halo::{mod}::{m.group(1)}", t))
    if s != o and f"halo::{mod}::" in s and inc not in s:
        incs = list(re.finditer(r"^#include[^\n]*\n", s, re.M))
        p = incs[-1].end() if incs else 0
        s = s[:p] + inc + "\n" + s[p:]
    if s != o:
        open(f, "w", encoding="utf-8", newline="").write(s)
        print("edited", f)
