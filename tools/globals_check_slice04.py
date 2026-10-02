#!/usr/bin/env python
"""Check slice 4 of the globals->C conversion (standalone/data/slice04.c).

For every global defined there (each definition is preceded by a marker comment
"/* 0xADDR size N: name */") it compares the bytes the C variable has in the linked exe with the bytes the
committed data image (standalone/image/*.asm) holds at the original address.

  python tools/globals_check_slice04.py [build_dir]       default build/s04/Release (halo_rebuilt.exe + .map)

Pointer dwords that were deliberately re-expressed in C (function names, string literals, &other_global) cannot
be equal; for those the check verifies the target instead: a string/wide string must read the same text as the
image string it replaced, a function pointer must point into the exe's code, a pointer to another converted
object must be that object's new address. Stdlib only; never reads retail files.
"""
import json, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SA = os.path.join(ROOT, "standalone")
OLD_LO, OLD_HI = 0x400000, 0x891000


def load_image():
    """image byte map {addr: byte} and {addr: symbol} for the dword rows of rdata.asm and data.asm"""
    b, sym = {}, {}
    pieces = {p["label"]: p for p in json.load(open(os.path.join(SA, "image", "pieces.json")))}
    for lab in ("rdata", "data"):
        addr = pieces[lab]["va"]
        started = False
        for l in open(os.path.join(SA, "image", lab + ".asm")):
            if "LABEL BYTE" in l:
                started = True
                continue
            if not started:
                continue
            m = re.match(r"\s*; 0x([0-9a-f]+)\s*$", l)
            if m:
                assert int(m.group(1), 16) == addr, (lab, hex(addr), m.group(1))
                continue
            m = re.match(r"\s+dd (.*?)(?:\s*;.*)?$", l.rstrip("\r\n"))
            if not m:
                continue
            for item in re.split(r",\s*", m.group(1).strip()):
                d = re.match(r"(\d+) dup \((.*)\)$", item)
                n, v = (int(d.group(1)), d.group(2)) if d else (1, item)
                for _ in range(n):
                    if re.match(r"[0-9A-F]+h$", v):
                        val = int(v[:-1], 16)
                    elif re.match(r"-?\d+$", v):
                        val = int(v) & 0xFFFFFFFF
                    else:
                        val = None
                        sym[addr] = v
                    for i in range(4):
                        b[addr + i] = 0 if val is None else (val >> (8 * i)) & 255
                    addr += 4
    return b, sym


class PE:
    def __init__(self, path):
        d = open(path, "rb").read()
        self.d = d
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        opt = struct.unpack_from("<H", d, pe + 20)[0]
        self.base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        so = pe + 24 + opt
        self.secs = []
        for i in range(nsec):
            name, vsize, va, rsize, roff = struct.unpack_from("<8sIIII", d, so + 40 * i)
            self.secs.append((name.rstrip(b"\0").decode(), va, vsize, roff, rsize))

    def read(self, vaddr, n):
        rva = vaddr - self.base
        for name, va, vsize, roff, rsize in self.secs:
            if va <= rva < va + max(vsize, rsize):
                out = bytearray()
                for k in range(n):
                    o = rva + k - va
                    out.append(self.d[roff + o] if o < rsize and roff + o < len(self.d) else 0)
                return bytes(out)
        raise KeyError(hex(vaddr))

    def section_of(self, vaddr):
        rva = vaddr - self.base
        for name, va, vsize, roff, rsize in self.secs:
            if va <= rva < va + max(vsize, rsize):
                return name
        return None

    def cstr(self, vaddr, wide=False):
        out, step = [], 2 if wide else 1
        while len(out) < 400:
            c = self.read(vaddr + step * len(out), step)
            c = int.from_bytes(c, "little")
            if c == 0:
                break
            out.append(c)
        return out


def read_map(path):
    syms = {}
    for l in open(path, errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s", l)
        if m:
            syms[m.group(1)] = int(m.group(2), 16)
    return syms


def main():
    bd = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "s04", "Release")
    exe, mp = os.path.join(bd, "halo_rebuilt.exe"), os.path.join(bd, "halo_rebuilt.map")
    pe, syms = PE(exe), read_map(mp)
    img, imgsym = load_image()
    src = open(os.path.join(SA, "data", "slice04.c")).read()
    defs = [(int(m.group(1), 16), int(m.group(2)), m.group(3)) for m in
            re.finditer(r"/\* 0x([0-9a-f]+) size (\d+): (\w+) \*/", src)]
    newaddr = {a: syms.get("_" + n) for a, s, n in defs}
    errors, ptrs, exact = [], 0, 0
    text = (pe.base, 0)
    for addr, size, name in defs:
        va = syms.get("_" + name)
        if va is None:
            errors.append("%s: not in the map" % name)
            continue
        built = pe.read(va, size)
        for off in range(0, size, 4):
            n = min(4, size - off)
            want = bytes(img.get(addr + off + i, 0) for i in range(n))
            got = built[off:off + n]
            a4 = addr + off
            if a4 in imgsym or (n == 4 and OLD_LO <= int.from_bytes(want, "little") < OLD_HI and got != want):
                ptrs += 1
                g = int.from_bytes(got, "little")
                if a4 in imgsym:
                    if pe.section_of(g) is None or not pe.section_of(g).startswith(".text"):
                        errors.append("%s+0x%x: code pointer %08x is not in the exe's code" % (name, off, g))
                    continue
                w = int.from_bytes(want, "little")
                if w in newaddr and newaddr[w] is not None:
                    if g != newaddr[w]:
                        errors.append("%s+0x%x: pointer to converted %x is %08x, want %08x" % (name, off, w, g, newaddr[w]))
                    continue
                # a string pointer: same text (ascii or utf-16) as the image string it replaced
                s1 = []
                while len(s1) < 400 and img.get(w + len(s1), 0):
                    s1.append(img[w + len(s1)])
                w1 = []
                while len(w1) < 400 and (img.get(w + 2 * len(w1), 0) or img.get(w + 2 * len(w1) + 1, 0)):
                    w1.append(img.get(w + 2 * len(w1), 0) | img.get(w + 2 * len(w1) + 1, 0) << 8)
                ok = False
                try:
                    if pe.cstr(g) == s1 and s1 or (not s1 and pe.cstr(g) == []):
                        ok = True
                    if len(w1) > 1 and pe.cstr(g, True) == w1:
                        ok = True
                except KeyError:
                    pass
                if not ok:
                    errors.append("%s+0x%x: pointer %08x -> %08x does not match the image data at %08x" % (name, off, int.from_bytes(want, "little"), g, w))
            elif got != want:
                errors.append("%s+0x%x: C bytes %s != image bytes %s" % (name, off, got.hex(), want.hex()))
            else:
                exact += n
    print("checked %d globals (%d bytes identical, %d pointer dwords re-targeted and verified)" % (len(defs), exact, ptrs))
    for e in errors:
        print("MISMATCH", e)
    print("%d mismatches" % len(errors))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
