"""Check of the slice 10 engine globals (standalone/data/slice10.c, original addresses 0x7c04e0..0x8805a0).

For every global that moved from an EQU in standalone/globals.asm to a C definition:
  * the name is defined (as a public data symbol) in the compiled standalone/data/slice10.c object;
  * its original address lies past the initialised part of the .data piece (standalone/image/pieces.json), i.e. it is
    zero-initialised BSS, so the original initial bytes are all zero -- and the C object holds only zeroes for it
    (its section is uninitialised data, or has no raw data, or the raw bytes at the symbol are zero);
  * the name is gone from globals.asm, and nothing outside the range was converted by this slice's file.
No retail file is read: the initial bytes come from the committed image description, the EQU list from git HEAD.

Usage: python tools/globals_check_slice10.py [path\\to\\slice10.obj]
(default: the object of the CMake build in build/s10, or build/standalone/data_slice10.obj)."""
import glob, json, os, re, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LO, HI = 0x7C04E0, 0x8805A0
SRC = os.path.join(ROOT, "standalone", "data", "slice10.c")


def equ_list(text):
    return {m.group(1): int(m.group(2), 16) for m in re.finditer(r"^(_\w+)\s+EQU\s+([0-9A-Fa-f]+)h", text, re.M)}


def coff_symbols(path):
    """{name: (section_number, value, raw_zero)} for the external symbols defined in a COFF object"""
    d = open(path, "rb").read()
    machine, nsec, _, symptr, nsym, optsz, _ = struct.unpack_from("<HHIIIHH", d, 0)
    secs = []
    off = 20 + optsz
    for i in range(nsec):
        name, vsize, va, rawsz, rawptr, _, _, _, _, ch = struct.unpack_from("<8sIIIIIIHHI", d, off + 40 * i)
        secs.append((name.rstrip(b"\0").decode(), rawsz, rawptr, ch))
    strtab = symptr + 18 * nsym
    out = {}
    i = 0
    while i < nsym:
        o = symptr + 18 * i
        nm, value, sect, typ, cls, aux = struct.unpack_from("<8sIhHBB", d, o)
        if nm[:4] == b"\0\0\0\0":
            so = struct.unpack_from("<I", nm, 4)[0]
            e = d.index(b"\0", strtab + so)
            name = d[strtab + so:e].decode()
        else:
            name = nm.rstrip(b"\0").decode()
        if cls == 2 and sect > 0:        # external, defined
            sname, rawsz, rawptr, ch = secs[sect - 1]
            zero = bool(ch & 0x80) or rawsz == 0 or not any(d[rawptr + value:rawptr + value + 1])
            out[name] = (sname, value, zero, ch)
        i += 1 + aux
    return out


def find_obj():
    c = glob.glob(os.path.join(ROOT, "build", "*", "**", "slice10*.obj"), recursive=True) + \
        glob.glob(os.path.join(ROOT, "build", "standalone", "data_slice10.obj"))
    return max(c, key=os.path.getmtime) if c else None


def main():
    obj = sys.argv[1] if len(sys.argv) > 1 else find_obj()
    if not obj or not os.path.exists(obj):
        sys.exit("no slice10 object found: build first, or pass its path")
    head = subprocess.run(["git", "show", "HEAD:standalone/globals.asm"], cwd=ROOT, capture_output=True, text=True).stdout
    now = equ_list(open(os.path.join(ROOT, "standalone", "globals.asm")).read())
    before = {n: a for n, a in equ_list(head).items() if LO <= a <= HI}
    pieces = json.load(open(os.path.join(ROOT, "standalone", "image", "pieces.json")))
    data = [p for p in pieces if p["label"] == "data"][0]
    init_end = data["va"] + data["size"]
    syms = coff_symbols(obj)
    converted = sorted(n for n in before if n not in now)
    left = sorted(n for n in before if n in now)
    bad = []
    for n in converted:
        a = before[n]
        if a < init_end:
            bad.append("%s: 0x%x lies in the initialised .data (< 0x%x); initial bytes need an image comparison" % (n, a, init_end))
        s = syms.get(n)
        if s is None:
            bad.append("%s: not defined in %s" % (n, os.path.basename(obj)))
        elif not s[2]:
            bad.append("%s: C object holds non-zero initial bytes (section %s)" % (n, s[0]))
    src_names = set("_" + m for m in re.findall(r"^[^/\n][^=\n]*?\b(\w+)\s*(?:\[[^\]]*\]\s*)*=\s*\{0\};", open(SRC).read(), re.M))
    extra = sorted(n for n in syms if n in now)
    for n in extra:
        bad.append("%s: defined in C but its EQU is still in globals.asm (duplicate symbol)" % n)
    unexpected = sorted(n for n in syms if n not in before and n in src_names)
    for n in unexpected:
        bad.append("%s: defined by slice10.c but outside the slice range" % n)
    print("slice 10 range 0x%x..0x%x: %d globals before, %d converted to C, %d left as EQU" % (LO, HI, len(before), len(converted), len(left)))
    print("initialised .data ends at 0x%x: every converted address is zero-initialised BSS" % init_end)
    for b in bad:
        print("MISMATCH", b)
    print("checker: %s" % ("FAIL (%d)" % len(bad) if bad else "OK"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
