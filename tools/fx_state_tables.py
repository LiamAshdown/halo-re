"""Effect state tables: halo.exe's statically linked (2003) D3DX versus d3dx9_43.dll (June 2010).

A compiled effect's pass/sampler states are stored as an operation index into D3DX's own state table (entries of
name, flags, three counts, (class << 24) | D3D state, enum table). halo.exe's table sits at 0x0067ee50; d3dx9_43.dll
carries the same kind of table with one more dword per entry (after the flags). This module reads both and maps each legacy operation to the modern one by name
(and checks the class/state pair agrees). Used by tools/convert_fx.py.
Usage: python tools/fx_state_tables.py   (prints the mapping and any legacy entry without a modern match)"""
import os, struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HALO = os.path.join(ROOT, "bin", "halo.exe")
D3DX = r"C:\Windows\SysWOW64\d3dx9_43.dll"


class Image:
    def __init__(self, path):
        self.d = open(path, "rb").read()
        pe = struct.unpack_from("<I", self.d, 0x3c)[0]
        n = struct.unpack_from("<H", self.d, pe + 6)[0]
        opt = pe + 24
        osz = struct.unpack_from("<H", self.d, pe + 20)[0]
        self.base = struct.unpack_from("<I", self.d, opt + 28)[0]
        self.secs = [struct.unpack_from("<8sIIII", self.d, opt + osz + 40 * i) for i in range(n)]

    def off(self, va):
        for _, vs, v, rs, ro in self.secs:
            if self.base + v <= va < self.base + v + rs:
                return va - self.base - v + ro
        return None

    def va(self, off):
        for _, vs, v, rs, ro in self.secs:
            if ro <= off < ro + rs:
                return self.base + v + off - ro
        return None

    def u32(self, va):
        return struct.unpack_from("<I", self.d, self.off(va))[0]

    def cstr(self, va):
        o = self.off(va)
        if o is None:
            return None
        e = self.d.find(b"\0", o)
        s = self.d[o:e]
        return s.decode("latin1") if 0 < len(s) < 48 and all(32 <= c < 127 for c in s) else None


def read_table(img, start, stride, packed_at):
    """entries from start until a name that is not a string: [(name, class, state)]"""
    out = []
    a = start
    while True:
        name = img.cstr(img.u32(a))
        if not name:
            break
        packed = img.u32(a + packed_at)
        out.append((name, packed >> 24, packed & 0xffffff))
        a += stride
    return out


def find_table(img, packed_at):
    """the table starts at the entry naming ZENABLE (render state class 1, D3DRS_ZENABLE 7)"""
    z = img.va(img.d.find(b"ZENABLE\0"))
    for o in range(0, len(img.d) - 0x20, 4):
        if struct.unpack_from("<I", img.d, o)[0] == z and struct.unpack_from("<I", img.d, o + packed_at)[0] == 0x01000007:
            return img.va(o)
    raise SystemExit("no state table in " + str(img))


def mapping():
    halo, d3dx = Image(HALO), Image(D3DX)
    legacy = read_table(halo, 0x0067ee50, 0x1c, 0x14)             # 2003: 7 dwords per entry
    modern = read_table(d3dx, find_table(d3dx, 0x18), 0x20, 0x18)   # June 2010: 8 dwords per entry
    by_name = {}
    for i, (name, cls, st) in enumerate(modern):
        by_name.setdefault(name, (i, cls, st))
    result, missing, renamed = {}, [], []
    for i, (name, cls, st) in enumerate(legacy):
        m = by_name.get(name)
        # the render and texture-stage states (entries 0..120) agree exactly; after them the class numbering was
        # redone and the order changed, but each legacy name has one modern entry with the same state number (or
        # the constant-register forms, whose state field packs the register differently) -- mapped by name
        if m:
            result[i] = m[0]
            if (m[1], m[2]) != (cls, st):
                renamed.append((i, name, cls, st, m))
        else:
            missing.append((i, name, cls, st, m))
    return legacy, modern, result, missing


if __name__ == "__main__":
    legacy, modern, result, missing = mapping()
    print("legacy entries %d, modern entries %d, mapped %d" % (len(legacy), len(modern), len(result)))
    moved = [(i, result[i], legacy[i][0]) for i in sorted(result) if result[i] != i]
    print("renumbered: %d" % len(moved))
    for x in moved[:12]:
        print("   legacy 0x%x -> modern 0x%x  %s" % x)
    for x in missing:
        print("   NO MATCH legacy 0x%x %s class %d state %d (modern by name: %s)" % x)
