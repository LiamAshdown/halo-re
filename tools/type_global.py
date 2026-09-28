"""Gives a global its struct type in the files that still declare it as a raw pointer, turning each raw-offset
access into the field at that offset. Refuses (leaves untouched) any file where the global would still be used in
byte arithmetic, since that silently becomes struct-pointer arithmetic once the global is typed.
  python tools/type_global.py <global> <struct> <header> [--dry]
e.g. python tools/type_global.py global_structure_bsp ScenarioStructureBSP tags.h
Field offsets come from the compiler (tools/struct_offsets.py); tag sub-structs (TagReflexive, TagDependency,
TagDataOffset, TagID) and real_point3d / real_vector3d fields are descended into. An access whose type differs from
the field's is kept as *(type *)&G->field, so the generated code does not change (check with tools/objdiff.py)."""
import glob, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import struct_offsets as so

SUB = {
    "TagReflexive": [(0, "count", "uint32_t"), (4, "pointer", "uint32_t"), (8, "definition", "uint32_t")],
    "TagDependency": [(0, "tag_fourcc", "uint32_t"), (4, "path_pointer", "uint32_t"), (8, "path_size", "uint32_t"),
                      (0xc, "tag_id", "TagID")],
    "TagDataOffset": [(0, "size", "uint32_t"), (4, "flags", "uint32_t"), (8, "file_offset", "uint32_t"),
                      (0xc, "pointer", "uint32_t"), (0x10, "definition", "uint32_t")],
    "real_point3d": [(0, "x", "float"), (4, "y", "float"), (8, "z", "float")],
    "Point3D": [(0, "x", "float"), (4, "y", "float"), (8, "z", "float")],
    "real_vector3d": [(0, "i", "float"), (4, "j", "float"), (8, "k", "float")],
    "Vector3D": [(0, "i", "float"), (4, "j", "float"), (8, "k", "float")],
}
SAME = {("float", "real"), ("real", "float")}


def field_types(header, name):
    t = open(os.path.join(ROOT, "types", header), encoding="utf-8", errors="replace").read()
    m = re.search(r"typedef struct %s \{(.*?)\n\} %s;" % (re.escape(name), re.escape(name)), t, re.S)
    body = re.sub(r"//[^\n]*|/\*.*?\*/", "", m.group(1), flags=re.S)
    out = {}
    for decl in body.split(";"):
        mm = re.match(r"\s*(?:struct\s+)?([\w ]+?)\s*(\*?)\s*(\w+)\s*(\[[^\]]*\])?\s*$", decl)
        if mm:
            out[mm.group(3)] = (mm.group(1) + (" *" if mm.group(2) else ""), bool(mm.group(4)))
    return out


def layout(struct, header, extra):
    old = sys.argv
    sys.argv = ["struct_offsets.py", struct, header] + extra
    import io, contextlib
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        so.main()
    sys.argv = old
    types = field_types(header, struct)
    fields = []
    for line in buf.getvalue().splitlines():
        m = re.match(r"0x([0-9a-f]+)\s+(\w+)", line)
        if m:
            fields.append((int(m.group(1), 16), m.group(2)) + types.get(m.group(2), ("?", False)))
    return fields


def resolve(fields, off):
    """(path, ctype) of the scalar at off, or None"""
    for i, (o, name, ctype, is_array) in enumerate(fields):
        end = fields[i + 1][0] if i + 1 < len(fields) else o + 0x10000
        if not (o <= off < end) or is_array or name.startswith("_pad") or name.startswith("unknown"):
            continue
        rel = off - o
        if rel == 0 and ctype not in SUB:
            return name, ctype
        for so_, sname, stype in SUB.get(ctype, []):
            if so_ == rel:
                return "%s.%s" % (name, sname), stype
    return None


def main():
    if len(sys.argv) < 4:
        raise SystemExit(__doc__)
    g, struct, header = sys.argv[1:4]
    dry = "--dry" in sys.argv
    extra = [a for a in sys.argv[4:] if a != "--dry"]
    fields = layout(struct, header, extra)
    decl = re.compile(r"^extern\s+(?:const\s+)?(?:uint8_t|void|int32_t|uint32_t|char)\s*\*\s*%s\s*;[^\n]*$" % g, re.M)
    access = re.compile(r"\*\((?:const )?([\w ]+?\s*\**)\s*\*\)\(\(?(?:\((?:const )?[\w ]+\*\))?%s\)? \+ (0x[0-9a-fA-F]+|\d+)\)" % g)
    changed, refused = [], []
    for f in sorted(glob.glob(os.path.join(ROOT, "src", "*", "*.c"))):
        t = open(f, encoding="utf-8").read()
        i = t.find("\n#if 0")
        live, rest = (t, "") if i < 0 else (t[:i], t[i:])
        if not decl.search(live):
            continue
        new = decl.sub("extern %s *%s;" % (struct, g), live, count=1)

        def rep(m):
            ctype, off = re.sub(r"\s*\*", "*", m.group(1).strip()), int(m.group(2), 0)
            base, stars = ctype.rstrip("*").strip(), len(ctype) - len(ctype.rstrip("*"))
            r = resolve(fields, off)
            if not r:
                return m.group(0)
            path, ftype = r
            after = m.string[m.end():m.end() + 4]
            lvalue = re.match(r"\s*(=[^=]|\+\+|--|[-+*/|&^]=|<<=|>>=)", after) is not None
            if (ctype == ftype or (ctype, ftype) in SAME) and not stars:
                return "%s->%s" % (g, path)
            if stars and ftype == "uint32_t" and not lvalue:
                # a pointer stored in a tag field: a value cast, the same load
                return "(%s %s)%s->%s" % (base, "*" * stars, g, path)
            return "*(%s %s)&%s->%s" % (base, "*" * (stars + 1), g, path)
        new = access.sub(rep, new)
        code = re.sub(r"//[^\n]*|/\*.*?\*/", "", new, flags=re.S)
        left = [l.strip() for l in code.splitlines()
                if re.search(r"\b%s\b\)?\s*(\+|-(?!>)|\[)" % g, l) or re.search(r"\(\s*(?:\([\w ]+\*\))?%s\s*\+" % g, l)]
        if left:
            refused.append((os.path.relpath(f, ROOT), left[:3]))
            continue
        if new != live:
            changed.append(os.path.relpath(f, ROOT))
            if not dry:
                open(f, "w", encoding="utf-8", newline="\n").write(new + rest)
    for f in changed:
        print("typed", f)
    for f, left in refused:
        print("REFUSED", f, left)
    mods = sorted({c.split(os.sep)[1] for c in changed})
    print("%d typed, %d refused; modules: %s" % (len(changed), len(refused), " ".join(mods)))


if __name__ == "__main__":
    main()
