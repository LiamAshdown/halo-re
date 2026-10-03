"""Build step (CMakeLists.txt, HALO_REGENERATE): runs between compiling src/ and linking halo_rebuilt, so adding or
renaming a function needs no manual step: tools/gen_link_sources.py rewrites standalone/generated/code_entries.c,
image_bindings.c and standalone/image/pieces.c (it tells a function from a fragment file by the compiled objects: here
the build's own objects). Engine globals are C definitions in standalone/data/*.c; a new one is added there by hand
(tools/update_globals.py lists what a link left unresolved).
The committed files are rewritten only when their content changes; the stamp is touched only then too, so an
unchanged result recompiles nothing (Ninja restat).
  python tools/cmake_regen.py <file listing the objects, one per line> <stamp file>"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_link_sources as gls

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def object_map(list_file):
    out = {}
    for line in open(list_file, encoding="utf-8").read().splitlines():
        p = line.strip()
        if not p:
            continue
        base = os.path.basename(p)
        stem = base[:-6] if base.endswith(".c.obj") else os.path.splitext(base)[0]
        out[stem] = p
    return out


def main():
    list_file, stamp = sys.argv[1], sys.argv[2]
    objects = object_map(list_file)
    watched = [os.path.join(gls.GEN, "code_entries.c"), os.path.join(gls.GEN, "image_bindings.c"),
               os.path.join(gls.SA, "image", "pieces.c")]
    before = [open(p, "rb").read() if os.path.exists(p) else None for p in watched]

    gls.defines_function = lambda e: _defines(objects, e)
    gls.main()

    after = [open(p, "rb").read() if os.path.exists(p) else None for p in watched]
    if before != after or not os.path.exists(stamp):
        with open(stamp, "w") as f:
            f.write("\n".join(os.path.relpath(p, ROOT) for p, a, b in zip(watched, before, after) if a != b) + "\n")


def _defines(objects, e):
    p = objects.get(e["c_symbol"])
    if p is None or not os.path.exists(p):
        return False
    data = open(p, "rb").read()
    name = e["c_symbol"].encode()
    return any(q + name + s in data for q in (b"_", b"@") for s in (b"\0", b"@"))


if __name__ == "__main__":
    main()
