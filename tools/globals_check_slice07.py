"""globals->C slice 7 check (stdlib only, reads no retail file): standalone/data/slice07.c versus the data image.

For every global defined in standalone/data/slice07.c it checks
  - the definition has no initializer (so its initial bytes are all zero) and its byte size,
  - the image bytes (standalone/image/*.asm, decoded from the dd rows) at the original address range are all zero or lie
    past the end of the initialised .data piece (zero-initialised BSS in the reserve), i.e. identical to the C object,
  - the name is gone from globals.asm and, with a linked map (build/s07/Release/halo_rebuilt.map or argv[1]), the
    symbol is a real object (address outside the original range 0x710426..0x71976e), not an absolute EQU.
Pointer-valued dwords: none (everything is zero), reported as 0 re-targeted.
Usage: python tools/globals_check_slice07.py [map file]"""
import os, re, sys, json

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SIZES = {"uint8_t": 1, "int8_t": 1, "char": 1, "uint16_t": 2, "int16_t": 2, "wchar_t": 2, "uint32_t": 4, "int32_t": 4,
         "float": 4, "void *": 4}


def decode(path):
    out, addr = {}, None
    for l in open(path):
        l = l.rstrip("\n")
        m = re.match(r"; 0x([0-9a-f]+)\s*$", l)
        if m:
            addr = int(m.group(1), 16)
            continue
        m = re.match(r"\s+dd (.*?)(\s*;.*)?$", l)
        if not m:
            continue
        for it in re.split(r",\s*", m.group(1)):
            it = it.strip()
            d = re.match(r"(\d+) dup \((\w+)\)$", it)
            if d:
                v = d.group(2)
                v = int(v[:-1], 16) if v.endswith("h") else int(v)
                for _ in range(int(d.group(1))):
                    out[addr] = v
                    addr += 4
                continue
            out[addr] = int(it[:-1], 16) if re.match(r"[0-9A-Fa-f]+h$", it) else (int(it) if it.isdigit() else "sym:" + it)
            addr += 4
    return out


def main():
    src = open(os.path.join(ROOT, "standalone", "data", "slice07.c")).read()
    equ = {}
    for l in open(os.path.join(ROOT, "standalone", "globals.asm")):
        m = re.match(r"(_\w+) EQU ([0-9A-Fa-f]+)h", l)
        if m:
            equ[m.group(1)[1:]] = int(m.group(2), 16)
    image = {}
    for p in ("rdata", "data", "tls", "rsrc"):
        image.update(decode(os.path.join(ROOT, "standalone", "image", p + ".asm")))
    mapsyms = {}
    mp = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "s07", "Release", "halo_rebuilt.map")
    if os.path.exists(mp):
        for l in open(mp, errors="replace"):
            m = re.match(r"\s+[0-9a-f]{4}:[0-9a-f]{8}\s+_(\w+)\s+([0-9a-f]{8})\s", l)
            if m:
                mapsyms[m.group(1)] = int(m.group(2), 16)
    # original addresses come from the address comment on each definition line
    bad, n = [], 0
    for l in src.split("\n"):
        m = re.match(r"^(void \*|[A-Za-z_0-9]+ )\s*(\w+)((?:\[[^\]]+\])*);\s*// 0x([0-9a-f]{8})$", l)
        if not m:
            if l and not l.startswith(("/*", "   ", "#")) and "=" in l:
                bad.append("initializer or unparsed line: " + l)
            continue
        ctype, name, dims, addr = m.group(1).strip() if m.group(1) != "void *" else "void *", m.group(2), m.group(3), int(m.group(4), 16)
        n += 1
        size = SIZES[ctype]
        for d in re.findall(r"\[([^\]]+)\]", dims):
            size *= int(d, 0)
        if name in equ:
            bad.append("%s is still an EQU in globals.asm" % name)
        nz = [a for a in range(addr & ~3, addr + size, 4) if image.get(a, 0) != 0]
        if nz:
            bad.append("%s: image has nonzero/pointer dword(s) at %s (C object is zero-initialised)" % (name, [hex(a) for a in nz[:3]]))
        if mapsyms:
            if name not in mapsyms:
                bad.append("%s missing from the link map" % name)
            elif 0x710426 <= mapsyms[name] <= 0x71976e:
                bad.append("%s still linked at its original address" % name)
    print("%d globals checked, 0 pointer dwords re-targeted, %d problem(s)" % (n, len(bad)))
    for b in bad:
        print("  " + b)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
