"""python tools/api_globals2.py <module> c_name=member@type ...

Like api_globals.py but typed: a file's uses of c_name are replaced by halo::<module>::globals().member only when that file declares
c_name with exactly `type` (other declarations stay), and the Globals struct + globals() accessor are generated into
include/halo/<module>/api.hpp and src/<module>/<module>_api.cpp (type is a scalar/pointer type; spelled as in the extern declaration).
"""
import glob, os, re, sys

os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
mod = sys.argv[1]
specs = {}
for a in sys.argv[2:]:
    name, rest = a.split("=", 1)
    member, typ = rest.split("@", 1)
    specs[name] = (member, typ.strip())

STRRE = re.compile(r'"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'|//[^\n]*|/\*.*?\*/', re.S)


def code_sub(text, fn):
    out, last = [], 0
    for m in STRRE.finditer(text):
        out.append(fn(text[last:m.start()]))
        out.append(m.group(0))
        last = m.end()
    out.append(fn(text[last:]))
    return "".join(out)


def decl_re(n, typ):
    t = re.escape(typ).replace(r"\ ", r"\s*")
    one = r"extern\s+\"C\"\s*\{\s*extern\s+" + t + r"\s*\b" + re.escape(n) + r"\s*;\s*\}"
    two = r"extern\s+" + t + r"\s*\b" + re.escape(n) + r"\s*;"
    return re.compile(r"^[ \t]*(?:" + one + "|" + two + r")[ \t]*\r?\n", re.M)


def other_decls(s, n, typ):
    allm = re.findall(r"^[ \t]*(?:extern\s+\"C\"\s*\{\s*)?extern\s+([^;(]*?)\b" + re.escape(n) + r"\s*(?:\[[^\]]*\])?\s*;", s, re.M)
    norm = lambda x: re.sub(r"\s+", "", x)
    return any(norm(x) != norm(typ) for x in allm)


inc = f'#include "halo/{mod}/api.hpp"'
used = set()
for f in glob.glob("src/**/*.cpp", recursive=True) + glob.glob("include/**/*.hpp", recursive=True):
    f = f.replace("\\", "/")
    if f.startswith(f"src/{mod}/") or f.startswith(f"include/halo/{mod}/"):
        continue
    s = open(f, encoding="utf-8", errors="replace", newline="").read()
    o = s
    mine = []
    for n, (member, typ) in specs.items():
        d = decl_re(n, typ)
        if d.search(s) and not other_decls(s, n, typ):
            mine.append(n)
    if not mine:
        continue
    for n in mine:
        s = decl_re(n, specs[n][1]).sub("", s)
    nre = re.compile(r"(?<![\w:.>])(" + "|".join(map(re.escape, mine)) + r")\b")
    s = code_sub(s, lambda t: nre.sub(lambda m: f"halo::{mod}::globals().{specs[m.group(1)][0]}", t))
    used |= set(mine)
    if inc not in s:
        incs = list(re.finditer(r"^#include[^\n]*\n", s, re.M))
        p = incs[-1].end() if incs else 0
        s = s[:p] + inc + "\n" + s[p:]
    open(f, "w", encoding="utf-8", newline="").write(s)
    print("edited", f, sorted(mine))

# emit
sys.path.insert(0, "tools")
src = open("tools/api_convert.py").read()
exec(src[src.index("_types_text = None"):src.index("DECL = lambda")])
members = [(specs[n][0], specs[n][1], n) for n in specs if n in used]
decl_text = "\n".join(f"{t} x;" for _, t, _ in members)
incs, fwds, tds = forward_block(decl_text)
hp = f"include/halo/{mod}/api.hpp"
h = open(hp, newline="").read()
fb = "\n".join(x for x in incs + fwds + tds if x not in h)
body = "".join(f"    {t}{'' if t.endswith('*') else ' '}&{m};\n" for m, t, _ in members)
struct = f'''
/**
 * The engine globals the {mod} module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {{
{body}}};

Globals &globals();
'''
if "struct Globals" in h:
    h = h.replace("};\n\nGlobals &globals();", body + "};\n\nGlobals &globals();", 1) if False else h
    raise SystemExit("api.hpp already has Globals; extend by hand")
h = h.replace("#include <stdint.h>\n", "#include <stdint.h>\n\n" + fb + "\n", 1)
h = h.replace(f"namespace halo::{mod} {{\n", f"namespace halo::{mod} {{\n" + struct, 1)
open(hp, "w", newline="").write(h)

cp = f"src/{mod}/{mod}_api.cpp"
c = open(cp, newline="").read()
externs = "".join(f"extern {t}{'' if t.endswith('*') else ' '}{n};\n" for _, t, n in members)
inits = ", ".join(f"::{n}" for _, _, n in members)
c = c.replace(f"namespace halo::{mod} {{\n", f'extern "C" {{\n{externs}}}\n\nnamespace halo::{mod} {{\n\nGlobals &globals()\n{{\n    static Globals instance{{{inits}}};\n    return instance;\n}}\n', 1)
open(cp, "w", newline="").write(c)
print("emitted", len(members), "globals")
