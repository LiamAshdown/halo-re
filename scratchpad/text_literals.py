"""hex literals in C code (not comments, not #if 0) that lie in retail .text 0x401000..0x639596: a code address the
standalone cannot run"""
import glob, re
R = "C:\\Users\\Liam-\\halo-re\\"
for p in sorted(glob.glob(R + "src\\*\\*.c")):
    t = open(p, encoding="utf-8", errors="replace").read()
    t = re.sub(r"^#if 0.*?^#endif", "", t, flags=re.S | re.M)
    t = re.sub(r"/\*.*?\*/", "", t, flags=re.S)
    for n, line in enumerate(t.split("\n"), 1):
        code = re.sub(r"//.*", "", line)
        code = re.sub(r'"(\\.|[^"\\])*"', '""', code)
        for m in re.finditer(r"\b0x0*([4-6][0-9a-fA-F]{5})\b", code):
            v = int(m.group(1), 16)
            if 0x401000 <= v < 0x639596:
                print("%s:%d: %s" % (p[len(R):], n, code.strip()[:140]))
