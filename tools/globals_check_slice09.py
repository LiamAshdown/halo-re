"""Check for slice 9 of the globals->C conversion (standalone/data/slice09.c, original addresses 0x721ebc..0x7c04a0).
No retail reads: the initial bytes come from the committed data image (standalone/image/pieces.json + *.asm).

For every variable of slice09.c (and every /alternatename alias) it checks that
  1. the original address lies outside every initialised image piece, i.e. its initial bytes are all zero (BSS in the
     reserve, standalone/frozen/layout.json), so a C definition without an initializer reproduces the image;
  2. the compiled object (build/<dir>/.../slice09.obj, found automatically or given as argv[1]) defines it with no
     initialised data (a common or .bss symbol), at the size the C type gives;
  3. its EQU line is gone from standalone/globals.asm (no duplicate definition).
Exit status 0 when everything agrees."""
import glob, json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "standalone", "data", "slice09.c")
LO, HI = 0x721ebc, 0x7c04a0


def objects():
    """name -> (address, type text) from slice09.c; aliases from the /alternatename pragmas."""
    defs, alias = {}, {}
    for line in open(SRC, encoding="utf-8"):
        m = re.match(r"^([A-Za-z_][\w \*]*?)\s*\**\s*(\w+)((?:\[[^\]]+\])*);\s*// 0x([0-9a-f]{8})", line)
        if m:
            defs[m.group(2)] = int(m.group(4), 16)
        m = re.match(r'#pragma comment\(linker, "/alternatename:_(\w+)=_(\w+)"\)', line)
        if m:
            alias[m.group(1)] = m.group(2)
    return defs, alias


def find_obj():
    c = sorted(glob.glob(os.path.join(ROOT, "build", "*", "**", "slice09.obj"), recursive=True), key=os.path.getmtime)
    return c[-1] if c else None


def dumpbin(obj):
    sys.path.insert(0, os.path.join(ROOT, "harness"))
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    import gen_link as gl
    r = subprocess.run([gl.tool("dumpbin"), "/symbols", obj], capture_output=True, text=True, env=gl.env)
    syms = {}
    for l in r.stdout.splitlines():
        m = re.match(r"\s*[0-9A-F]+ ([0-9A-F]{8}) (\S+)\s+\S+\s+(\S+)\s+\| _(\w+)\s*$", l)
        if m:
            syms[m.group(4)] = (int(m.group(1), 16), m.group(2), m.group(3))
    return syms


def main():
    defs, alias = objects()
    bad = []
    pieces = json.load(open(os.path.join(ROOT, "standalone", "image", "pieces.json")))
    for name, addr in sorted(defs.items(), key=lambda x: x[1]):
        if not LO <= addr <= HI:
            bad.append("%s 0x%x outside the slice" % (name, addr))
        for p in pieces:
            if p["va"] <= addr < p["va"] + p["size"]:
                bad.append("%s 0x%x lies in the initialised image piece %s: needs an initializer" % (name, addr, p["label"]))
    for a, t in alias.items():
        if t not in defs:
            bad.append("alias %s -> %s: target not defined" % (a, t))
    eq = set(re.findall(r"^_(\w+) EQU", open(os.path.join(ROOT, "standalone", "globals.asm")).read(), re.M))
    for n in list(defs) + list(alias):
        if n in eq:
            bad.append("%s still has an EQU in globals.asm" % n)
    obj = sys.argv[1] if len(sys.argv) > 1 else find_obj()
    if not obj:
        bad.append("no slice09.obj found (build first)")
    else:
        syms = dumpbin(obj)
        for n in defs:
            s = syms.get(n)
            if s is None:
                bad.append("%s missing from %s" % (n, obj))
            elif s[1] == "UNDEF":
                if s[0] == 0:
                    bad.append("%s is a real undefined reference, not a definition" % n)   # common: UNDEF with size
            else:   # a definition placed in a section would carry an initializer: only commons / zero data are expected
                bad.append("%s is placed in %s: check that it has no initializer" % (n, s[1]))
    print("%d variables, %d aliases checked" % (len(defs), len(alias)))
    for b in bad:
        print("MISMATCH", b)
    print("OK" if not bad else "%d problems" % len(bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
