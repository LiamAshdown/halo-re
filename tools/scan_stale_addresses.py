"""Finds code that still depends on the retail data image (the old fixed addresses 0x400000..0x890000).

Two scans (the standalone data image is going away, so nothing may read an old address):
  1. raw address literals in code (not comments): `(void *)0x7c3168`, `*(int *)0x6a32e8`, ...
  2. extern declarations in src/include whose shape disagrees with the definition in standalone/data/*.c: a pointer
     declared for something defined as an array or struct (the code then reads the object's bytes as a pointer value,
     which happened to be the old address of the data when the image was mapped).
Usage: python tools/scan_stale_addresses.py [--literals] [--types]"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ADDR = re.compile(r"(?<![\w])0x0{0,2}([4-8][0-9a-fA-F]{5})\b")


def code_lines(path):
    in_block = False
    for i, line in enumerate(open(path, encoding="utf-8", errors="replace"), 1):
        s = line
        out = ""
        j = 0
        while j < len(s):
            if in_block:
                k = s.find("*/", j)
                if k < 0:
                    j = len(s)
                else:
                    in_block = False
                    j = k + 2
            else:
                if s.startswith("//", j):
                    break
                if s.startswith("/*", j):
                    in_block = True
                    j += 2
                    continue
                out += s[j]
                j += 1
        yield i, out


def source_files():
    files = []
    for pat in ("src/*/*.cpp", "src/*/*.c", "include/halo/*/*.hpp", "include/halo/*/*.h", "standalone/*.c", "standalone/*.cpp"):
        files += glob.glob(os.path.join(ROOT, pat))
    return [f for f in files if os.sep + "gamespy" + os.sep not in f]


def scan_literals():
    n = 0
    for f in source_files():
        for i, code in code_lines(f):
            if code.lstrip().startswith("#"):
                continue
            for m in ADDR.finditer(code):
                print("%s:%d: %s" % (os.path.relpath(f, ROOT).replace("\\", "/"), i, code.strip()[:150]))
                n += 1
    print("%d raw address literals" % n)


DEF = re.compile(r"^(?:const\s+)?([A-Za-z_][\w \*]*?)\s*(\*+)?\s*\b([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*(?:=[^;]*)?;", re.M)
EXT = re.compile(r"^\s*extern\s+(?:const\s+)?([A-Za-z_][\w ]*?)\s*(\*+)?\s*\b([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*;", re.M)


def scan_types():
    defs = {}
    for f in glob.glob(os.path.join(ROOT, "standalone", "data", "*.c")):
        text = open(f, encoding="utf-8", errors="replace").read()
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        text = re.sub(r"//[^\n]*", "", text)
        for m in DEF.finditer(text):
            base, stars, name, arr = m.groups()
            if base.strip().startswith(("extern", "typedef", "return", "static", "#")):
                continue
            defs.setdefault(name, (len(stars or ""), arr is not None, f))
    bad = 0
    for f in source_files():
        text = open(f, encoding="utf-8", errors="replace").read()
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        text = re.sub(r"//[^\n]*", "", text)
        for m in EXT.finditer(text):
            base, stars, name, arr = m.groups()
            if name not in defs:
                continue
            dstars, darr, df = defs[name]
            estars, earr = len(stars or ""), arr is not None
            # a pointer declared for an array/value, or an array/value declared for a pointer, reads the wrong thing
            if (estars > 0 and not earr) != (dstars > 0 and not darr) or (earr != darr and (estars > 0 or dstars > 0)):
                bad += 1
                print("%s: extern %s%s %s%s  vs definition (%d stars%s) in %s" % (
                    os.path.relpath(f, ROOT).replace("\\", "/"), base.strip(), "*" * estars, name, arr or "", dstars,
                    ", array" if darr else "", os.path.basename(df)))
    print("%d pointer/array/value mismatches" % bad)


if __name__ == "__main__":
    if "--types" not in sys.argv:
        scan_literals()
    if "--literals" not in sys.argv:
        scan_types()
