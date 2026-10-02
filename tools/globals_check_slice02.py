#!/usr/bin/env python3
"""Check the slice 2 globals (standalone/data/slice02.c, 0x0066e674..0x0068941d) against the data image.

For every global defined in standalone/data/slice02.c the linked exe (build/<dir>/Release/halo_rebuilt.exe, address
from halo_rebuilt.map) must hold the same initial bytes as the committed image standalone/image/*.asm at the global's
original address (the address is the /* 0x... */ comment above each definition in
slice02.c). Pointer-valued dwords are skipped: a dword that is a code address (`dd halo_code_*`) or a number that
points into the image (0x400000..0x8a0000) is deliberately re-targeted (function name, &global, string literal);
they are counted, not compared. Also checks that the names defined here are gone from globals.asm and that every
alias pragma target exists. stdlib only; never reads the retail binary.

usage: python tools/globals_check_slice02.py [build_dir]     (default build/s02)
"""
import os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PIECES = [(0x63a000, "rdata"), (0x676000, "data")]


def load_image():
    mem = {}  # address -> ("n", value) | ("s", symbol)
    for base, label in PIECES:
        a, started = base, False
        for line in open(os.path.join(ROOT, "standalone", "image", label + ".asm")):
            if "LABEL BYTE" in line:
                started = True
                continue
            if not started:
                continue
            code = line.split(";")[0].strip()
            if not code.startswith("dd "):
                continue
            for item in (x.strip() for x in code[3:].split(",")):
                m = re.match(r"(\d+) dup \((\w+)\)$", item)
                if m:
                    v = m.group(2)
                    v = int(v[:-1], 16) if v[-1] in "hH" else int(v)
                    for _ in range(int(m.group(1))):
                        mem[a] = ("n", v)
                        a += 4
                    continue
                if re.fullmatch(r"[0-9A-Fa-f]+[hH]", item):
                    mem[a] = ("n", int(item[:-1], 16))
                elif re.fullmatch(r"\d+", item):
                    mem[a] = ("n", int(item))
                else:
                    mem[a] = ("s", item)
                a += 4
    return mem


def parse_definitions():
    """[(address, name, byte size, type text)] from the definitions in slice02.c"""
    sizes = {"float": 4, "double": 8, "uint32_t": 4, "uint8_t": 1, "uint16_t": 2, "int16_t": 2, "char": 1, "void *": 4}
    out = []
    lines = open(os.path.join(ROOT, "standalone", "data", "slice02.c"), encoding="utf8").read().split("\n")
    for i, l in enumerate(lines):
        m = re.match(r"/\* 0x([0-9a-f]{8}) \*/$", l)
        if not m:
            continue
        addr = int(m.group(1), 16)
        d = re.match(r"(float|double|uint32_t|uint8_t|uint16_t|int16_t|char|void \*)\s*(\w+)(?:\[(\d+)\])?\s*=", lines[i + 1])
        assert d, lines[i + 1]
        out.append((addr, d.group(2), sizes[d.group(1)] * int(d.group(3) or 1), d.group(1)))
    return out


class PE:
    def __init__(self, path):
        self.d = open(path, "rb").read()
        pe = struct.unpack_from("<I", self.d, 0x3C)[0]
        n = struct.unpack_from("<H", self.d, pe + 6)[0]
        opt = struct.unpack_from("<H", self.d, pe + 20)[0]
        self.base = struct.unpack_from("<I", self.d, pe + 24 + 28)[0]
        self.secs = []
        s = pe + 24 + opt
        for k in range(n):
            vsz, va, rsz, raw = struct.unpack_from("<IIII", self.d, s + 40 * k + 8)
            self.secs.append((va, max(vsz, rsz), raw, rsz))

    def read(self, vaddr, size):
        rva = vaddr - self.base
        for va, vsz, raw, rsz in self.secs:
            if va <= rva < va + vsz:
                off = rva - va
                b = self.d[raw + off: raw + min(off + size, rsz)] if off < rsz else b""
                return b + b"\0" * (size - len(b))
        return None


def main():
    bdir = os.path.join(ROOT, sys.argv[1] if len(sys.argv) > 1 else "build/s02")
    exe = map_path = None
    for r, _, fs in os.walk(bdir):
        for f in fs:
            if f == "halo_rebuilt.exe":
                exe = os.path.join(r, f)
            if f == "halo_rebuilt.map":
                map_path = os.path.join(r, f)
    if not exe or not map_path:
        sys.exit("no halo_rebuilt.exe / .map under %s (build first)" % bdir)
    syms = {}
    for l in open(map_path, errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+(?:f\s+)?\S*slice02\.obj", l)
        if m:
            syms[m.group(1)] = int(m.group(2), 16)
    mem, defs, pe = load_image(), parse_definitions(), PE(exe)
    asm = open(os.path.join(ROOT, "standalone", "globals.asm")).read()
    bad = 0
    retarget = 0
    for addr, name, size, _ in defs:
        if re.search(r"^_%s EQU" % re.escape(name), asm, re.M):
            print("STILL EQU in globals.asm:", name)
            bad += 1
        va = syms.get("_" + name)
        if va is None:
            print("MISSING in map:", name)
            bad += 1
            continue
        got = pe.read(va, size)
        for off in range(0, size):
            w = (addr + off) & ~3
            if w not in mem:
                print("%s: no image data at %x" % (name, w))
                bad += 1
                break
            kind, v = mem[w]
            if (addr + off) % 4 == 0 and (off + 4 <= size) and (kind == "s" or 0x400000 <= v < 0x8a0000):
                if off % 4 == 0:
                    retarget += 1
                continue
            if kind == "s":
                continue
            exp = (v >> (8 * ((addr + off) & 3))) & 255
            if got[off] != exp:
                print("MISMATCH %s +0x%x (0x%08x): exe %02x image %02x" % (name, off, addr + off, got[off], exp))
                bad += 1
                break
    for line in re.findall(r"alternatename:_(\w+)=_(\w+)", open(os.path.join(ROOT, "standalone", "data", "slice02.c")).read()):
        if "_" + line[1] not in syms:
            print("alias target missing:", line)
            bad += 1
    print("%d globals checked, %d pointer dwords re-targeted (not compared), %d problems" % (len(defs), retarget, bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
