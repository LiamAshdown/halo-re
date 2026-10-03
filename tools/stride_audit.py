"""Find typed-pointer arithmetic with hex offsets, where retyping a pointer silently changes the stride.
Usage: python tools/stride_audit.py [--all] [paths...]   (default: src/)
Flags (1) `name + 0xNN` / `name[0xNN]` where name is declared as a non-byte pointer in the same file,
(2) `((T *)expr)[0xNN]` and `(T *)expr + 0xNN` with T wider than a byte. Byte-typed pointers are fine.
Hits reviewed against retail/struct layouts are listed in tools/stride_audit_ok.txt (`path|hit`); --all prints them too."""
import re, sys, os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BYTE = {"char", "uint8_t", "int8_t", "byte", "void", "BYTE", "uchar", "u8"}
HEX = r"0[xX][0-9a-fA-F]+"
decl = re.compile(r"\b((?:const\s+)?(?:struct\s+|enum\s+)?[A-Za-z_][\w:]*)\s*\*+\s*(?:const\s+)?([A-Za-z_]\w*)\s*(?=[=;,)\[])")
cast_idx = re.compile(r"\(\s*\(\s*(?:const\s+)?([\w: ]+?)\s*\*+\s*\)[^;\n]*?\)\s*\[\s*(" + HEX + r")\s*\]")
cast_add = re.compile(r"\(\s*(?:const\s+)?([\w: ]+?)\s*\*+\s*\)\s*[\w.>-]+\s*\+\s*(" + HEX + r")")
bytecast = re.compile(r"\(\s*(?:const\s+)?(?:" + "|".join(BYTE) + r")\s*\*\s*\)\s*$")
_ok = os.path.join(ROOT, "tools", "stride_audit_ok.txt")
ALLOW = set(l.strip() for l in open(_ok)) if os.path.exists(_ok) else set()
# member types of halo::<m>::Globals (include/halo/<m>/*.hpp): pointer members whose element type is not a byte
GMEM = {}
_gm = re.compile(r"^\s*(?:const\s+)?(?:struct\s+)?([A-Za-z_][\w:]*)\s*\*\s*&\s*(\w+)\s*;")
_inc = os.path.join(ROOT, "include", "halo")
for _m in os.listdir(_inc) if os.path.isdir(_inc) else []:
    _d = os.path.join(_inc, _m)
    for _f in os.listdir(_d) if os.path.isdir(_d) else []:
        if not _f.endswith(".hpp"):
            continue
        _b = re.search(r"struct Globals \{(.*?)\n\};", open(os.path.join(_d, _f), encoding="utf-8", errors="replace").read(), re.S)
        for _l in _b.group(1).splitlines() if _b else []:
            _x = _gm.match(_l)
            if _x and _x.group(1).split("::")[-1] not in BYTE:
                GMEM[(_m, _x.group(2))] = _x.group(1)
gacc = re.compile(r"halo::(\w+)::globals\(\)\.(\w+)\s*(?:\+\s*(" + HEX + r")|\[\s*(" + HEX + r")\s*\])")
galias = re.compile(r"(\w+)\s*=\s*halo::(\w+)::globals\(\)\.(\w+)\s*;")
lref = re.compile(r"\bauto\s*&\s*(\w+)\s*=\s*halo::link::ref<\s*(?:const\s+)?([\w: ]+?)\s*\*\s*>")
SKIP = {"return", "sizeof", "else", "case", "delete", "new"}


def scan(path, show_ok):
    out = []
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    names = {}
    for l in lines:
        for m in decl.finditer(l):
            t, n = re.sub(r"^(const|struct|enum)\s+", "", m.group(1).strip()), m.group(2)
            if t in SKIP:
                continue
            names.setdefault(n, set()).add(t)
    for l in lines:
        for m in lref.finditer(l):
            if m.group(2).split()[-1].split("::")[-1] not in BYTE:
                names.setdefault(m.group(1), set()).add(m.group(2))
        for m in galias.finditer(l):
            if GMEM.get((m.group(2), m.group(3))):
                names.setdefault(m.group(1), set()).add(GMEM[(m.group(2), m.group(3))])
    wide = {n for n, ts in names.items() if not any(t in BYTE for t in ts)}
    use = None
    if wide:
        use = re.compile(r"(?<![\w.>])(" + "|".join(map(re.escape, sorted(wide))) + r")\s*(?:\+\s*(" + HEX + r")|\[\s*(" + HEX + r")\s*\])")
    for i, l in enumerate(lines, 1):
        if re.match(r"\s*(\*(?![\w(&*])|/\*|//)", l):
            continue
        s = l.split("//")[0]
        ok = False
        hits = []
        for m in cast_idx.finditer(s):
            if m.group(1).split()[-1] not in BYTE:
                hits.append(f"(({m.group(1)}*)..)[{m.group(2)}]")
        for m in cast_add.finditer(s):
            if m.group(1).split()[-1] not in BYTE:
                hits.append(f"({m.group(1)}*)x + {m.group(2)}")
        if use:
            for m in use.finditer(s):
                if bytecast.search(s[:m.start()]):
                    continue
                off = "+ " + m.group(2) if m.group(2) else "[" + m.group(3) + "]"
                hits.append(f"{m.group(1)} ({'/'.join(sorted(names[m.group(1)]))}*) {off}")
        for m in gacc.finditer(s):
            t = GMEM.get((m.group(1), m.group(2)))
            if t and not bytecast.search(s[:m.start()]):
                off = "+ " + m.group(3) if m.group(3) else "[" + m.group(4) + "]"
                hits.append(f"{m.group(1)}::globals().{m.group(2)} ({t}*) {off}")
        for h in hits:
            rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
            ok = f"{rel}|{h}" in ALLOW
            if ok and not show_ok:
                continue
            out.append(f"{rel}:{i}: {h}{' [ok]' if ok else ''}")
    return out


def main():
    args = [x for x in sys.argv[1:] if not x.startswith("--")]
    show = "--all" in sys.argv
    n = 0
    for root in args or [os.path.join(ROOT, "src")]:
        files = [root] if os.path.isfile(root) else [os.path.join(d, f) for d, _, fs in os.walk(root) for f in fs if f.endswith((".cpp", ".hpp", ".h", ".c"))]
        for f in sorted(files):
            for h in scan(f, show):
                print(h)
                n += 1
    print(f"{n} hit(s)", file=sys.stderr)


if __name__ == "__main__":
    main()
