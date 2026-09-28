"""src files with an address header whose object does not define the file's own function"""
import glob, os, re
R = "C:\\Users\\Liam-\\halo-re\\"
out = []
for p in glob.glob(R + "src\\*\\*.c"):
    name = os.path.splitext(os.path.basename(p))[0]
    t = open(p, encoding="utf-8", errors="replace").read()
    if not re.search(r"address\s+0x0*[0-9a-f]{6},\s*size\s+\d+", t[:4000], re.I):
        continue
    body = re.sub(r"^#if 0.*?^#endif", "", t, flags=re.S | re.M)
    if not re.search(r"\b%s\s*\([^;{]*\)\s*\{" % re.escape(name), body, re.S):
        out.append(p[len(R):])
print(len(out))
for p in out:
    print(p)
