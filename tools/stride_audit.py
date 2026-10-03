"""Find typed-pointer arithmetic with hex offsets, where retyping a pointer silently changes the stride.
Usage: python tools/stride_audit.py [--all] [paths...]   (default: src/)
Flags (1) `name + 0xNN` / `name[0xNN]` where name is declared as a non-byte pointer in the same file,
(2) `((T *)expr)[0xNN]` and `(T *)expr + 0xNN` with T wider than a byte. Byte-typed pointers are fine.
Add `// stride-ok` on a line to silence a verified hit; --all prints silenced hits too."""
import re, sys, os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BYTE = {"char", "uint8_t", "int8_t", "byte", "void", "BYTE", "uchar", "u8"}
HEX = r"0[xX][0-9a-fA-F]+"
decl = re.compile(r"\b((?:const\s+)?(?:struct\s+|enum\s+)?[A-Za-z_][\w:]*)\s*\*+\s*(?:const\s+)?([A-Za-z_]\w*)\s*(?=[=;,)\[])")
cast_idx = re.compile(r"\(\s*\(\s*(?:const\s+)?([\w: ]+?)\s*\*+\s*\)[^;\n]*?\)\s*\[\s*(" + HEX + r")\s*\]")
cast_add = re.compile(r"\(\s*(?:const\s+)?([\w: ]+?)\s*\*+\s*\)\s*[\w.>-]+\s*\+\s*(" + HEX + r")")
bytecast = re.compile(r"\(\s*(?:const\s+)?(?:" + "|".join(BYTE) + r")\s*\*\s*\)\s*$")
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
    wide = {n for n, ts in names.items() if not any(t in BYTE for t in ts)}
    use = None
    if wide:
        use = re.compile(r"(?<![\w.>])(" + "|".join(map(re.escape, sorted(wide))) + r")\s*(?:\+\s*(" + HEX + r")|\[\s*(" + HEX + r")\s*\])")
    for i, l in enumerate(lines, 1):
        s = l.split("//")[0]
        ok = "stride-ok" in l
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
        for h in hits:
            if ok and not show_ok:
                continue
            out.append(f"{os.path.relpath(path, ROOT)}:{i}: {h}{' [ok]' if ok else ''}")
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
