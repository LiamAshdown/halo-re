"""Who uses which struct field: every member access in src/**/*.c, attributed through clang's AST to the struct that
declares the field (so actor.unknown_6e and prop.unknown_6e are told apart, which grep cannot do).
Parses each file as 32-bit Windows (clang -target i686-w64-windows-gnu, the mingw-w64 headers), so the layout the
static offset checks in types/ assert is the one the game uses.

Usage: python tools/field_uses.py scan [module ...]      (re)build build/field_uses.json (all modules by default)
       python tools/field_uses.py top [N] [--all]         structs by count of accessed unknown_ fields / uses
       python tools/field_uses.py show STRUCT [FIELD]     every use of STRUCT's unknown_ fields (or of one FIELD, any name)
Needs: pip install libclang; mingw-w64 headers (apt install mingw-w64-i686-dev) or HALO_WIN_INCLUDE."""
import glob, json, os, sys
from collections import defaultdict
from multiprocessing import Pool

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE = os.path.join(ROOT, "build", "field_uses.json")
WIN_INCLUDE = os.environ.get("HALO_WIN_INCLUDE", "/usr/i686-w64-mingw32/include")
ARGS = ["-target", "i686-w64-windows-gnu", "-fms-extensions", "-Wno-everything", "-isystem", WIN_INCLUDE,
        "-I", os.path.join(ROOT, "types")]


def record_name(decl):
    """The struct's name: its tag, or the typedef naming an anonymous struct."""
    if decl.spelling and "unnamed" not in decl.spelling and "anonymous" not in decl.spelling:
        return decl.spelling
    t = decl.type.spelling
    return t.replace("struct ", "")


def scan_file(path):
    from clang import cindex
    index = cindex.Index.create()
    try:
        tu = index.parse(path, args=ARGS)
    except cindex.TranslationUnitLoadError:
        return path, []
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    out = []
    main = os.path.abspath(path)

    def walk(node, func):
        for c in node.get_children():
            loc = c.location
            if loc.file is not None and os.path.abspath(loc.file.name) != main:
                continue
            f = func
            if c.kind == cindex.CursorKind.FUNCTION_DECL and c.is_definition():
                f = c.spelling
            if c.kind == cindex.CursorKind.MEMBER_REF_EXPR:
                ref = c.referenced
                if ref is not None and ref.kind == cindex.CursorKind.FIELD_DECL:
                    parent = ref.semantic_parent
                    # fields of anonymous nested structs/unions: name them after the outermost named record
                    names = []
                    while parent is not None and parent.kind in (cindex.CursorKind.STRUCT_DECL, cindex.CursorKind.UNION_DECL):
                        names.append(record_name(parent))
                        if parent.spelling and "unnamed" not in parent.spelling and "anonymous" not in parent.spelling:
                            break
                        parent = parent.semantic_parent
                    line, col = c.location.line, c.location.column
                    out.append([names[0] if names else "?", ref.spelling, f, line,
                                lines[line - 1].strip() if 0 < line <= len(lines) else "", col])
            walk(c, f)

    walk(tu.cursor, None)
    return os.path.relpath(path, ROOT).replace(os.sep, "/"), out


def scan(mods):
    files = []
    for m in mods:
        files += sorted(glob.glob(os.path.join(ROOT, "src", m, "*.c")))
    data = json.load(open(CACHE)) if os.path.exists(CACHE) else {}
    with Pool() as pool:
        for rel, uses in pool.imap_unordered(scan_file, files, chunksize=8):
            data[rel] = uses
    os.makedirs(os.path.dirname(CACHE), exist_ok=True)
    json.dump(data, open(CACHE, "w"), separators=(",", ":"))
    print("scanned %d files, %d member accesses" % (len(files), sum(len(v) for v in data.values())))


def load():
    return json.load(open(CACHE))


def top(n, everything):
    per = defaultdict(lambda: defaultdict(int))
    for rel, uses in load().items():
        seen = set()
        for struct, field, func, line, text, *_ in uses:
            if everything or field.startswith("unknown_"):
                if (struct, field, line) in seen:
                    continue
                seen.add((struct, field, line))
                per[struct][field] += 1
    rows = sorted(per.items(), key=lambda kv: -sum(kv[1].values()))[:n]
    for struct, fields in rows:
        best = sorted(fields.items(), key=lambda kv: -kv[1])[:6]
        print("%-32s %5d uses %4d fields   %s" % (struct, sum(fields.values()), len(fields),
                                                 " ".join("%s:%d" % kv for kv in best)))


def show(struct, field):
    rows = []
    for rel, uses in sorted(load().items()):
        seen = set()
        for s, f, func, line, text, *_ in uses:
            if s == struct and (f == field if field else f.startswith("unknown_")) and (f, line) not in seen:
                seen.add((f, line))
                rows.append((f, rel, line, func, text))
    for f, rel, line, func, text in sorted(rows):
        print("%-14s %s:%d [%s]  %s" % (f, rel, line, func, text[:150]))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    cmd = sys.argv[1]
    if cmd == "scan":
        scan(sys.argv[2:] or sorted(d for d in os.listdir(os.path.join(ROOT, "src"))
                                    if os.path.isdir(os.path.join(ROOT, "src", d))))
    elif cmd == "top":
        rest = [a for a in sys.argv[2:] if a != "--all"]
        top(int(rest[0]) if rest else 30, "--all" in sys.argv)
    elif cmd == "show":
        show(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else None)
    else:
        print(__doc__)


if __name__ == "__main__":
    main()
