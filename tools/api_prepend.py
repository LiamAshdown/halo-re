"""python tools/api_prepend.py <log> <function> <text> [--only-from-type=T]: for each C2660/C2664 error naming halo::*::<function> in the log, inserts <text>
(e.g. '(ColorARGB *)0, ') as the first argument of the call on the reported line. With --from-type=T only C2664 errors whose argument 1 comes from a type
containing T are touched."""
import re, sys
log = open(sys.argv[1], errors="replace").read()
fn, text = sys.argv[2], sys.argv[3]
ftype = next((a.split("=", 1)[1] for a in sys.argv[4:] if a.startswith("--from-type=")), None)
seen = set()
for m in re.finditer(r"^\s*([A-Za-z]:[^()\n]*?)\((\d+),(\d+)\): error (C2660|C2664): ([^\n]*)", log, re.M):
    path, line, col, code, msg = m.groups()
    if f"::{fn}" not in msg.split(":")[0] + ":" + msg and f"::{fn}'" not in msg:
        continue
    if f"::{fn}" not in msg:
        continue
    if ftype and ("from '" not in msg or ftype not in msg.split("from '")[1]):
        continue
    key = (path, int(line), int(col))
    if key in seen:
        continue
    seen.add(key)
lines_by_file = {}
for path, line, col in seen:
    lines_by_file.setdefault(path, set()).add(line)
for path, lines in lines_by_file.items():
    s = open(path, encoding="utf-8", errors="replace", newline="").read().split("\n")
    for ln in lines:
        l = s[ln - 1]
        pat = re.compile(r"(\b(?:halo::\w+::)?" + re.escape(fn) + r"\()")
        mm = pat.search(l)
        if not mm:
            print("no call on", path, ln, l.strip()[:100])
            continue
        s[ln - 1] = l[:mm.end()] + text + l[mm.end():]
    open(path, "w", encoding="utf-8", newline="").write("\n".join(s))
    print("fixed", path, sorted(lines))
