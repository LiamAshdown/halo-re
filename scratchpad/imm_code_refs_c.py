"""imm_code_refs.py restricted to references that lie inside a function we have C for (by header address/size)"""
import bisect, contextlib, glob, io, os, re
R = "C:\\Users\\Liam-\\halo-re\\"
c_rng = []
for p in glob.glob(R + "src\\*\\*.c"):
    t = open(p, encoding="utf-8", errors="replace").read(3000)
    m = re.search(r"address\s+0x0*([0-9a-f]{6}),\s*size\s+(\d+)", t)
    if m:
        c_rng.append((int(m.group(1), 16), int(m.group(2)), os.path.splitext(os.path.basename(p))[0]))
c_rng.sort()
c_starts = [r[0] for r in c_rng]


def inside(a):
    i = bisect.bisect_right(c_starts, a) - 1
    if i >= 0 and c_rng[i][0] <= a < c_rng[i][0] + c_rng[i][1]:
        return c_rng[i][2]
    return None


buf = io.StringIO()
with contextlib.redirect_stdout(buf):
    exec(open(R + "scratchpad\\imm_code_refs.py").read())
n = 0
for line in buf.getvalue().split("\n"):
    refs = re.findall(r"([0-9a-f]{6})\(", line)
    ins = [(r, inside(int(r, 16))) for r in refs if inside(int(r, 16))]
    if ins:
        n += 1
        print(line[:62].rstrip(), "| in C:", ins)
print(n, "targets referenced from C-covered code")
