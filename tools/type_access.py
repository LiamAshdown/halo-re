"""Rewrites raw-offset accesses through a pointer variable into field accesses of a struct, without changing the
variable's type (so no arithmetic on it changes meaning):
    *(float *)(obj + 0x208)   ->   ((unit_object *)obj)->unit.<field at 0x14>
    *(int16_t *)(obj + 0x5e)  ->   *(int16_t *)&((unit_object *)obj)->base.<field>   (access type differs)
  python tools/type_access.py <struct> <header> <var>[,<var>...] <files or module dirs...> [--pre h1,h2] [--dry]
Nested project structs (object, unit_data, ...) and tag sub-structs are descended into; an offset that lands in a
padding/unknown field, an array, or past the struct is left alone. Check the result with tools/objdiff.py."""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import type_global as tg

_layouts = {}


def header_of(struct):
    for h in glob.glob(os.path.join(ROOT, "types", "*.h")):
        if re.search(r"\} %s;" % re.escape(struct), open(h, encoding="utf-8", errors="replace").read()):
            return os.path.basename(h)
    return None


def layout(struct, pre):
    if struct not in _layouts:
        h = header_of(struct)
        _layouts[struct] = tg.layout(struct, h, pre) if h else []
    return _layouts[struct]


def resolve(struct, off, pre, depth=0):
    fields = layout(struct, pre)
    for i, (o, name, ctype, is_array) in enumerate(fields):
        end = fields[i + 1][0] if i + 1 < len(fields) else None
        if off < o or (end is not None and off >= end) or is_array:
            continue
        if name.startswith(("_pad", "unknown", "pad")):
            return None
        rel = off - o
        base = ctype.replace("struct ", "").strip()
        if rel == 0 and base not in tg.SUB and not layout(base, pre):
            return name, ctype
        for so_, sname, stype in tg.SUB.get(base, []):
            if so_ == rel:
                return "%s.%s" % (name, sname), stype
        if depth < 3 and layout(base, pre):
            r = resolve(base, rel, pre, depth + 1)
            if r:
                return "%s.%s" % (name, r[0]), r[1]
        return None
    return None


def main():
    argv = sys.argv[1:]
    if "--pre" in argv:
        k = argv.index("--pre")
        argv = argv[:k] + argv[k + 2:]
    args = [a for a in argv if not a.startswith("--")]
    if len(args) < 4:
        raise SystemExit(__doc__)
    struct, header, names, targets = args[0], args[1], args[2].split(","), args[3:]
    pre = sys.argv[sys.argv.index("--pre") + 1].split(",") if "--pre" in sys.argv else []
    dry = "--dry" in sys.argv
    layout(struct, pre + [header])
    files = []
    for t in targets:
        files += glob.glob(os.path.join(ROOT, t, "*.c")) if os.path.isdir(os.path.join(ROOT, t)) else [os.path.join(ROOT, t)]
    total, changed = 0, []
    for f in sorted(files):
        text = open(f, encoding="utf-8").read()
        i = text.find("\n#if 0")
        live, rest = (text, "") if i < 0 else (text[:i], text[i:])
        n_file = 0
        for v in names:
            acc = re.compile(r"\*\((?:const )?([\w ]+?\s*\**)\s*\*\)\(\(?(?:\((?:const )?[\w ]+\*\))?%s\)? \+ (0x[0-9a-fA-F]+|\d+)\)"
                             % re.escape(v))

            def rep(m):
                nonlocal n_file
                ctype = re.sub(r"\s*\*", "*", m.group(1).strip())
                r = resolve(struct, int(m.group(2), 0), pre + [header])
                if not r:
                    return m.group(0)
                path, ftype = r
                n_file += 1
                view = "((%s *)%s)->%s" % (struct, v, path)
                if ctype == ftype or (ctype, ftype) in tg.SAME:
                    return view
                base, stars = ctype.rstrip("*").strip(), len(ctype) - len(ctype.rstrip("*"))
                return "*(%s %s)&%s" % (base, "*" * (stars + 1), view)
            live = acc.sub(rep, live)
        if n_file:
            total += n_file
            changed.append((os.path.relpath(f, ROOT), n_file))
            if not dry:
                open(f, "w", encoding="utf-8", newline="\n").write(live + rest)
    for f, n in changed:
        print("%4d  %s" % (n, f))
    print("%d accesses in %d files" % (total, len(changed)))


if __name__ == "__main__":
    main()
