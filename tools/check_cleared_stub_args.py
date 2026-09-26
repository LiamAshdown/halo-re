"""For every stub harness/gen_hooks.py cleared as a plain cdecl call (harness/build/stubs_cleared.txt), compare the
number of stack arguments the original reads with every C declaration of it; print the mismatches (candidates for
harness/stub_unsafe.txt). Usage: python tools/check_cleared_stub_args.py"""
import re, glob, os, sys, json
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); sys.path.insert(0, os.path.join(ROOT, "tools"))
import gen_hooks as g
from fix_extra_regs import stack_arg_count
from fix_register_arg_callers import split_args
res = open(os.path.join(ROOT, "harness", "gen", "resolve.asm")).read()
addr = dict((n, int(a, 16)) for n, a in re.findall(r"^PUBLIC (\S+)\n\S+:\n    push 0([0-9A-F]+)h\n    ret", res, re.M))
fsize = {int(x["addr"], 16): int(x.get("size") or 0) for x in json.load(open(os.path.join(ROOT, "out", "functions.json")))}
srcs = {}
for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
    t = open(p, encoding="utf-8", errors="replace").read(); srcs[p] = t[:t.rfind("#if 0")] if "#if 0" in t else t
for s in [l.strip() for l in open(os.path.join(ROOT, "harness", "build", "stubs_cleared.txt")) if l.strip()]:
    a = addr.get(s); name = s[1:].split("@")[0]
    if a is None: continue
    g.FUNC_SIZES[a] = fsize.get(a, 0); have = stack_arg_count(a, fsize.get(a, 0))
    decls = set()
    for p, b in srcs.items():
        for m in re.finditer(r"^[ \t]*extern[^;]*\b" + re.escape(name) + r"\s*\(([^;]*)\)\s*;", b, re.M | re.S):
            ps = [x for x in split_args(m.group(1)) if x.strip() and x.strip() != "void"]
            n = sum(2 if re.search(r"\b(double|int64_t|uint64_t|__int64)\b", x) and "*" not in x else 1 for x in ps if x.strip() != "...")
            decls.add((n, "..." in m.group(1), os.path.basename(p)))
    if have is None or any(d[0] != have and not d[1] for d in decls):
        print(s, have, sorted(decls)[:3])
