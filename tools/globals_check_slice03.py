"""Checks the slice-3 globals (standalone/data/slice03.c) against the data image they replaced.

For every definition in slice03.c (`T name... = ...; /* 0xADDR, N bytes */`) it finds the variable in the linked exe
(build/<dir>/.../halo_rebuilt.map + the PE file) and compares its bytes with the bytes the committed data image
(standalone/image/*.asm) holds at the original address. Pointer-valued dwords are allowed to differ, because the C
initializer deliberately re-targets them; for each one it verifies that
  * a code pointer (`dd halo_code_X` in the image) now points into the exe's code,
  * any other pointer now points into the exe and that the bytes at the new target equal the bytes at the old
    target in the image (strings, constants, tables) as far as the image defines them.
Stdlib only; reads no retail file (the image .asm and the exe built from it are the only inputs).

  python tools/globals_check_slice03.py [build/s03]
"""
import os, re, struct, sys, json, glob

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, sys.argv[1] if len(sys.argv) > 1 else "build/s03")
SRC = os.path.join(ROOT, "standalone", "data", "slice03.c")
MANIFEST = os.path.join(ROOT, "tools", "globals_check_slice03.json")   # the pointer dwords slice03.c re-targets
IMG = os.path.join(ROOT, "standalone", "image")
BSS_START = 0x6a0088     # end of the initialised .data piece: everything above is zero in the original image


def num(tok):
    tok = tok.strip()
    if tok[-1:] in "hH":
        return int(tok[:-1], 16)
    return int(tok)


def load_image():
    pieces = {p["label"]: p for p in json.load(open(os.path.join(IMG, "pieces.json")))}
    mem = {}      # address -> dword value
    code = {}     # address -> original code address (dd halo_code_X)
    for label, p in pieces.items():
        addr = p["va"]
        for raw in open(os.path.join(IMG, label + ".asm")):
            line = raw.split(";")[0].strip()
            m = re.match(r"dd\s+(.*)$", line)
            if not m:
                continue
            for item in m.group(1).split(","):
                item = item.strip()
                md = re.match(r"(\d+) dup \((\w+)\)$", item)
                if md:
                    v = num(md.group(2))
                    for _ in range(int(md.group(1))):
                        mem[addr] = v
                        addr += 4
                    continue
                mc = re.match(r"halo_code_([0-9a-f]+)$", item)
                if mc:
                    code[addr] = int(mc.group(1), 16)
                    mem[addr] = None
                else:
                    mem[addr] = num(item)
                addr += 4
    return mem, code


def img_byte(mem, a):
    d = mem.get(a & ~3, "missing")
    if d == "missing":
        return 0 if a >= BSS_START else None
    if d is None:
        return None
    return (d >> (8 * (a & 3))) & 0xFF


class PE:
    def __init__(self, path):
        self.data = open(path, "rb").read()
        d = self.data
        pe = struct.unpack_from("<I", d, 0x3c)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        opt = struct.unpack_from("<H", d, pe + 20)[0]
        self.base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        s = pe + 24 + opt
        self.secs = []
        for i in range(nsec):
            name, vsz, va, rsz, raw = struct.unpack_from("<8sIIII", d, s + 40 * i)[:5]
            ch = struct.unpack_from("<I", d, s + 40 * i + 36)[0]
            self.secs.append((name.rstrip(b"\0").decode(), self.base + va, vsz, raw, rsz, ch))

    def byte(self, a):
        for name, va, vsz, raw, rsz, ch in self.secs:
            if va <= a < va + max(vsz, rsz):
                off = a - va
                return self.data[raw + off] if off < rsz and raw else 0
        return None

    def in_exe(self, a):
        return any(va <= a < va + max(vsz, rsz) for name, va, vsz, raw, rsz, ch in self.secs)

    def in_code(self, a):
        return any(ch & 0x20 and va <= a < va + vsz for name, va, vsz, raw, rsz, ch in self.secs)


def main():
    mems, code = load_image()
    maps = glob.glob(os.path.join(BUILD, "**", "halo_rebuilt.map"), recursive=True)
    exes = glob.glob(os.path.join(BUILD, "**", "halo_rebuilt.exe"), recursive=True)
    if not maps or not exes:
        raise SystemExit("no halo_rebuilt.map / .exe under %s: build first" % BUILD)
    maps.sort(key=os.path.getmtime)
    exes.sort(key=os.path.getmtime)
    syms = {}
    for line in open(maps[-1], errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s", line)
        if m:
            syms[m.group(1)] = int(m.group(2), 16)
    pe = PE(exes[-1])

    defs = []
    for line in open(SRC):
        m = re.match(r"^[A-Za-z_][\w \*]*?[ \*](\w+)(\[\d+\])? = .*; /\* 0x([0-9a-f]{8}), (\d+) bytes \*/$", line.rstrip())
        if m:
            defs.append((m.group(1), int(m.group(3), 16), int(m.group(4))))
    if not defs:
        raise SystemExit("no definitions parsed from slice03.c")

    # the pointer dwords the C initializers re-target (generated with slice03.c): (object, offset, old value, kind, a, b)
    manifest = {}
    for obj, off, old, kind, a, b in json.load(open(MANIFEST)):
        manifest[(obj, off)] = (old, kind, a, b)

    def cstr_at(addr, wide):
        out = bytearray()
        step = 2 if wide else 1
        while True:
            chunk = [pe.byte(addr + len(out) + i) for i in range(step)]
            if None in chunk:
                return None
            if not any(chunk):
                return bytes(out)
            out += bytes(chunk)

    bad = 0
    retargeted = 0
    checked_bytes = 0
    kinds = {}
    for name, old, size in defs:
        new = syms.get("_" + name)
        if new is None:
            print("MISSING in map: %s" % name)
            bad += 1
            continue
        a = 0
        while a < size:
            dw_old = old + a
            if dw_old % 4 == 0 and a + 4 <= size:
                nb = [pe.byte(new + a + i) for i in range(4)]
                if None in nb:
                    print("%s+%#x: not in exe" % (name, a))
                    bad += 1
                    a += 4
                    continue
                nv = int.from_bytes(bytes(nb), "little")
                entry = mems.get(dw_old, "missing")
                ov = 0 if entry == "missing" else entry          # None: a code symbol in the image
                man = manifest.get((name, a))
                if man is None:
                    if entry is None or nv != ov:
                        print("%s+%#x: dword %#x differs from the image (%s) and is not a declared re-target" %
                              (name, a, nv, "code pointer" if entry is None else "%#x" % ov))
                        bad += 1
                    else:
                        checked_bytes += 4
                    a += 4
                    continue
                oldv, kind, x, y = man
                retargeted += 1
                kinds[kind] = kinds.get(kind, 0) + 1
                ok = False
                if kind == "code":
                    tgt = syms.get("_" + x)
                    ok = pe.in_code(nv) and (tgt is None or tgt == nv)
                elif kind == "sym":
                    tgt = syms.get("_" + x)
                    ok = (nv == tgt + y) if tgt is not None else (nv == oldv)   # still-EQU globals keep their address
                    if tgt is None and nv != oldv:
                        print("%s+%#x: target %s is neither in the map nor at its original address" % (name, a, x))
                elif kind in ("str", "wstr"):
                    want = x.encode("utf-16-le" if kind == "wstr" else "latin-1")
                    ok = cstr_at(nv, kind == "wstr") == want
                    # and the original pointee (image bytes) must be the same text
                    img = bytes(img_byte(mems, oldv + i) or 0 for i in range(len(want)))
                    if img != want:
                        ok = False
                else:
                    ok = False
                if not ok:
                    print("%s+%#x: re-targeted pointer %#x -> %#x (%s %r) does not hold" % (name, a, oldv, nv, kind, x))
                    bad += 1
                a += 4
            else:
                ob = img_byte(mems, dw_old)
                nbv = pe.byte(new + a)
                if ob is None:
                    a += 1      # inside a symbolic dword (the unused lead bytes of a table based mid-dword)
                    continue
                if ob != nbv:
                    print("%s+%#x: byte %#x != image %#x" % (name, a, nbv or 0, ob))
                    bad += 1
                else:
                    checked_bytes += 1
                a += 1
    print("%d definitions, %d bytes compared equal, %d pointer dwords re-targeted %s, %d mismatches" %
          (len(defs), checked_bytes, retargeted, kinds, bad))
    raise SystemExit(1 if bad else 0)


if __name__ == "__main__":
    main()
