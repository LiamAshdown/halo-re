"""Rename identifiers in one file's live code: never inside #if 0 blocks, extern declarations (single or multi-line),
comments, or string/char literals. Refuses a new name already used in that code.
  python scratchpad/rename_in.py <file> old=new [old=new ...]"""
import re, sys

path, pairs = sys.argv[1], [a.split("=", 1) for a in sys.argv[2:]]
t = open(path, encoding="utf-8").read()
parts = re.split(r"(\n#if 0\b.*?\n#endif[^\n]*)", t, flags=re.S)
TOKEN = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'|^[ \t]*extern\b[^;]*;|\w+',
                   re.S | re.M)


def code_idents(s):
    return {m.group(0) for m in TOKEN.finditer(s) if re.fullmatch(r"\w+", m.group(0))}


for i in range(0, len(parts), 2):
    used = code_idents(parts[i])
    for old, new in pairs:
        if old in used and new in used:
            sys.exit("REFUSED: %s already used in %s" % (new, path))

    def rep(m):
        s = m.group(0)
        return dict(pairs).get(s, s) if re.fullmatch(r"\w+", s) else s
    parts[i] = TOKEN.sub(rep, parts[i])
open(path, "w", encoding="utf-8", newline="\n").write("".join(parts))
print("renamed", path, " ".join("%s->%s" % tuple(p) for p in pairs))
