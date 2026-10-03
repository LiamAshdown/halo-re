"""Checker for globals->C slice 1 (standalone/data/slice01.c): the C definitions must hold the bytes the data image
(standalone/image/rdata.asm, committed) holds at each global's original address.

For every definition in standalone/data/slice01.c it compares the initial bytes of the linked variable (read from
build/<dir>/Release/halo_rebuilt.exe at the address the link map gives) with the image bytes at the original address.
Dwords that are pointers are not equal by design (the target moved); each is checked by what it points at:
  - a code pointer (halo_code_<addr> in the image): must equal the map address of halo_code_<addr> (that is, of the C
    function image_bindings.c binds it to), or the plain number when the image dword was only a number that looked
    like a code address;
  - a pointer to another global: must equal the map address of that global (+ offset into it);
  - a pointer to a string: the string it points at in the exe must equal the string at the original address;
  - anything else is reported as an unresolved raw pointer and must be equal to the original value.
It also checks that the AI-table cluster (the items in section .gdat01) keeps the original layout: each item's offset
from the first one equals its original address difference. Reads no retail file, only committed sources and the build.

Usage: python tools/globals_check_slice01.py [path\\to\\halo_rebuilt.map]   (default: build/s01/Release, then build/standalone)
Exit status 0 when every global matches."""
import os, re, struct, sys, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import globals_asm_history

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SA = os.path.join(ROOT, "standalone")


def parse_image(label):
    va = {p["label"]: p["va"] for p in json.load(open(os.path.join(SA, "image", "pieces.json")))}[label]
    dws, started = [], False
    for l in open(os.path.join(SA, "image", label + ".asm"), encoding="utf-8"):
        if l.startswith("_halo_image_"):
            started = True
            continue
        if not started:
            continue
        m = re.match(r"\s*dd\s+(.*)$", l.split(";")[0])
        if not m:
            continue
        ops, depth, cur = [], 0, ""
        for c in m.group(1):
            depth += (c == "(") - (c == ")")
            if c == "," and depth == 0:
                ops.append(cur); cur = ""
            else:
                cur += c
        ops.append(cur)
        for t in ops:
            t = t.strip()
            md = re.match(r"(\d+)\s+dup\s*\((.*)\)$", t)
            n, v = (int(md.group(1)), md.group(2).strip()) if md else (1, t)
            if re.match(r"^[0-9][0-9A-Fa-f]*h$", v): v = int(v[:-1], 16)
            elif re.match(r"^-?\d+$", v): v = int(v) & 0xffffffff
            dws.extend([v] * n)
    return va, dws


class Image:
    def __init__(self):
        self.va, self.d = parse_image("rdata")

    def dword(self, a):
        return self.d[(a - self.va) // 4]

    def num(self, a):
        w = self.dword(a)
        return int(w[len("halo_code_"):], 16) if isinstance(w, str) else w

    def bytes(self, a, n):
        return bytes((self.num(i - i % 4) >> (8 * (i % 4))) & 255 for i in range(a, a + n))

    def cstring(self, a, wide=False):
        out, step = b"", 2 if wide else 1
        while len(out) < 0x400:
            b = self.bytes(a + len(out), step)
            if b == b"\0" * step: break
            out += b
        return out


class Exe:
    def __init__(self, path):
        self.data = open(path, "rb").read()
        pe = struct.unpack_from("<I", self.data, 0x3c)[0]
        nsec = struct.unpack_from("<H", self.data, pe + 6)[0]
        opt = struct.unpack_from("<H", self.data, pe + 20)[0]
        self.base = struct.unpack_from("<I", self.data, pe + 24 + 28)[0]
        self.secs = []
        o = pe + 24 + opt
        for i in range(nsec):
            name = self.data[o:o + 8].rstrip(b"\0").decode("latin1")
            vsize, rva, rsize, raw = struct.unpack_from("<IIII", self.data, o + 8)
            self.secs.append((name, self.base + rva, max(vsize, rsize), raw, rsize))
            o += 40

    def off(self, va):
        for name, a, size, raw, rsize in self.secs:
            if a <= va < a + size:
                if va - a >= rsize: return None            # zero-initialised tail
                return raw + va - a
        raise KeyError("address 0x%x is not in the exe" % va)

    def bytes(self, va, n):
        out = bytearray()
        for i in range(n):
            o = self.off(va + i)
            out.append(0 if o is None else self.data[o])
        return bytes(out)

    def dword(self, va):
        return struct.unpack("<I", self.bytes(va, 4))[0]

    def cstring(self, va, wide=False):
        out, step = b"", 2 if wide else 1
        while len(out) < 0x400:
            b = self.bytes(va + len(out), step)
            if b == b"\0" * step: break
            out += b
        return out


def find_map(argv):
    if len(argv) > 1: return argv[1]
    for p in ("build/s01/Release/halo_rebuilt.map", "build/standalone/halo_rebuilt.map", "build/cmake/Release/halo_rebuilt.map"):
        if os.path.exists(os.path.join(ROOT, p)): return os.path.join(ROOT, p)
    raise SystemExit("no link map: build first (cmake --build build/s01 --config Release)")


def parse_map(path):
    syms = {}
    for l in open(path, encoding="latin1"):
        m = re.match(r"^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\b", l)
        if m: syms.setdefault(m.group(1), int(m.group(2), 16))
    return syms


def parse_c():
    """(name, original address, size, in_cluster) of every definition in standalone/data/slice01.c"""
    out, pend = [], None
    for l in open(os.path.join(SA, "data", "slice01.c"), encoding="utf-8"):
        m = re.match(r"/\* 0x([0-9a-f]{8})\b", l)
        if m: pend = int(m.group(1), 16); cl = False
        if ".gdat01" in l and "__declspec(allocate" in l: cl = True
        m = re.match(r"SLICE01_SIZE_CHECK\((\w+), (\d+)\);", l)
        if m:
            out.append((m.group(1), pend, int(m.group(2)), cl))
    return out


def main():
    mp = find_map(sys.argv)
    exe = Exe(os.path.splitext(mp)[0] + ".exe")
    syms = parse_map(mp)
    img = Image()
    defs = parse_c()
    # original address -> global (every global, converted or still an absolute EQU), for pointer targets
    by_addr = {}
    for l in globals_asm_history.original().splitlines():
        m = re.match(r"(\S+)\s+EQU\s+([0-9A-Fa-f]+)h", l)
        if m: by_addr[int(m.group(2), 16)] = m.group(1)
    mine = {}
    for n, a, size, cl in defs:
        by_addr[a] = "_" + n
        mine[n] = (a, size)
    bad, nptr, ndw = [], {"code": 0, "global": 0, "string": 0, "own": 0, "raw": 0, "num": 0}, 0
    for n, a, size, cl in defs:
        s = syms.get("_" + n)
        if s is None:
            bad.append("%s: not in the link map" % n); continue
        for o in range(0, size, 4 if a % 4 == 0 else size):
            span = min(4, size - o) if a % 4 == 0 else size
            want = img.bytes(a + o, span); got = exe.bytes(s + o, span)
            ndw += 1
            if want == got: continue
            iw = img.dword(a + o) if a % 4 == 0 and span == 4 else None
            gv = struct.unpack("<I", got)[0] if span == 4 else None
            if iw is None:
                bad.append("%s+0x%x: bytes differ: image %s exe %s" % (n, o, want.hex(), got.hex())); continue
            v = img.num(a + o)
            ok = False
            if isinstance(iw, str):                                   # a code address in the image
                t = syms.get(iw)
                if t == gv: ok = True; nptr["code"] += 1
            else:
                # pointer to another global / into one of ours
                if v in by_addr:
                    t = syms.get(by_addr[v])
                    if t == gv: ok = True; nptr["global"] += 1
                if not ok:
                    for m2, (ma, msz) in mine.items():
                        if ma < v < ma + msz and syms.get("_" + m2) is not None and syms["_" + m2] + v - ma == gv:
                            ok = True; nptr["own"] += 1; break
                if not ok and img.va <= v < img.va + len(img.d) * 4:
                    want_s = img.cstring(v)
                    if want_s and gv is not None:
                        try:
                            if exe.cstring(gv) == want_s: ok = True; nptr["string"] += 1
                        except KeyError:
                            pass
            if not ok:
                bad.append("%s+0x%x: dword image %s exe 0x%08x not a recognised retarget" % (
                    n, o, ("0x%08x" % iw) if isinstance(iw, int) else iw, gv))
    # layout of the cluster
    cl = [(n, a) for n, a, size, c in defs if c]
    for n, a in cl[1:]:
        if syms["_" + n] - syms["_" + cl[0][0]] != a - cl[0][1]:
            bad.append("cluster: %s is at +0x%x, original +0x%x" % (n, syms["_" + n] - syms["_" + cl[0][0]], a - cl[0][1]))
    # the EQU lines are gone
    eq = globals_asm_history.current()
    for n, a, size, c in defs:
        if re.search(r"^_%s EQU " % re.escape(n), eq, re.M):
            bad.append("%s: still an EQU in globals.asm" % n)
    print("checked %d globals (%d dwords/chunks); retargeted pointers by kind: %s" % (len(defs), ndw, nptr))
    for b in bad: print("MISMATCH", b)
    print("OK" if not bad else "%d mismatches" % len(bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
