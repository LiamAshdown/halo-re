"""Build step (CMakeLists.txt, HALO_REGENERATE): runs between compiling src/ and linking halo_rebuilt, so adding or
renaming a function or using a new engine global needs no manual step:
  1. tools/gen_link_sources.py: standalone/generated/code_entries.c, image_bindings.c, standalone/image/pieces.c
     (it tells a function from a fragment file by the compiled objects: here the build's own objects)
  2. standalone/globals.asm: every data symbol the objects reference but none defines that a declaration in src/ gives
     an address comment for (what tools/update_globals.py adds after a failed link) is added before the link
The committed files are rewritten only when their content changes; the stamp is touched only then too, so an
unchanged result recompiles nothing (Ninja restat).
  python tools/cmake_regen.py <file listing the objects, one per line> <stamp file>"""
import os, re, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_link_sources as gls
import update_globals as ug


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


def coff_externals(path):
    """(defined, undefined) external symbol names of a COFF object"""
    data = open(path, "rb").read()
    machine, nsections, _, symtab, nsyms = struct.unpack_from("<HHIII", data, 0)
    strtab = symtab + nsyms * 18
    defined, undefined = set(), set()
    i = 0
    while i < nsyms:
        off = symtab + i * 18
        raw = data[off:off + 8]
        value, section, _, storage, naux = struct.unpack_from("<IhHBB", data, off + 8)
        if storage == 2:  # IMAGE_SYM_CLASS_EXTERNAL
            if raw[:4] == b"\0\0\0\0":
                start = strtab + struct.unpack_from("<I", raw, 4)[0]
                name = data[start:data.index(b"\0", start)].decode("latin-1")
            else:
                name = raw.rstrip(b"\0").decode("latin-1")
            if section == 0 and value == 0:
                undefined.add(name)
            elif section != 0:
                defined.add(name)
        i += 1 + naux
    return defined, undefined


# data the link gets from a library, not from the data image (dinput8.lib)
LIBRARY_DATA = {"_c_dfDIKeyboard", "_c_dfDIMouse2"}
# the symbols standalone/bridges.cpp defines through /alternatename (src/ declares them extern)
BRIDGE_SYMBOLS = set(re.findall(r"/alternatename:(\w+)=", open(os.path.join(ug.ROOT, "standalone", "bridges.cpp"), encoding="utf-8").read()))


def other_link_definitions():
    """PUBLIC symbols of the link's own assembly (bridges.cpp, the data image), which src/ may declare too"""
    out = set(LIBRARY_DATA) | BRIDGE_SYMBOLS
    for d, _, files in os.walk(os.path.join(ug.ROOT, "standalone")):
        for f in files:
            p = os.path.join(d, f)
            if f.endswith(".asm") and os.path.abspath(p) != os.path.abspath(ug.GLOBALS):
                out |= set(re.findall(r"^\s*PUBLIC\s+(\S+)", open(p, encoding="utf-8", errors="replace").read(), re.M))
    return out


def missing_globals(objects):
    defined, undefined = other_link_definitions(), set()
    for p in objects.values():
        d, u = coff_externals(p)
        defined |= d
        undefined |= u
    have = ug.read_equ(ug.GLOBALS)
    sys.path.insert(0, os.path.join(ug.ROOT, "harness"))
    import gen_link as gl
    addr, kind = gl.extern_map()
    out = {}
    for s in sorted(undefined - defined - set(have)):
        n = s[1:]
        m = re.fullmatch(r"(?:PTR_)?DAT_([0-9a-fA-F]{8})", n)
        if m:
            out[s] = int(m.group(1), 16)
        elif n in addr and kind.get(n) == "data" and len(addr[n]) == 1:
            out[s] = next(iter(addr[n]))
        elif n in addr and kind.get(n) == "data" and len(addr[n]) > 1:
            print("cmake_regen: %s: address comments disagree (%s); fix the C" %
                  (s, ", ".join("0x%x" % a for a in sorted(addr[n]))))
    return out


def main():
    list_file, stamp = sys.argv[1], sys.argv[2]
    objects = object_map(list_file)
    watched = [os.path.join(gls.GEN, "code_entries.c"), os.path.join(gls.GEN, "image_bindings.c"),
               os.path.join(gls.SA, "image", "pieces.c"), ug.GLOBALS]
    before = [open(p, "rb").read() if os.path.exists(p) else None for p in watched]

    gls.defines_function = lambda e: _defines(objects, e)
    gls.main()

    new = missing_globals(objects)
    if new:
        have = ug.read_equ(ug.GLOBALS)
        have.update(new)
        ug.write(have)
        print("cmake_regen: globals.asm +%d: %s" % (len(new), ", ".join(sorted(new))))

    after = [open(p, "rb").read() if os.path.exists(p) else None for p in watched]
    if before != after or not os.path.exists(stamp):
        with open(stamp, "w") as f:
            f.write("\n".join(os.path.relpath(p, ug.ROOT) for p, a, b in zip(watched, before, after) if a != b) + "\n")


def _defines(objects, e):
    p = objects.get(e["c_symbol"])
    if p is None or not os.path.exists(p):
        return False
    data = open(p, "rb").read()
    name = e["c_symbol"].encode()
    return any(q + name + s in data for q in (b"_", b"@") for s in (b"\0", b"@"))


if __name__ == "__main__":
    main()
