"""python tools/api_exports.py <module>: drops from symbols/exports/<module>.txt the C symbols the module no longer defines (step B)."""
import os, sys, tempfile
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check_module_symbols as c

m = sys.argv[1]
path = os.path.join(c.EXPORTS, m + ".txt")
base = [l.rstrip("\n") for l in open(path) if l.strip()]
syms = set()
with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(12) as ex:
    for src, s, err in ex.map(lambda f: c.compile_symbols(f, tmp), c.module_files(m)):
        assert s is not None, (src, err)
        syms |= s
keep = [b for b in base if b in syms or b.startswith(("__real@", "__xmm@", "__ymm@", "??_C@_", "??_R", "??_7"))]
open(path, "w", newline="\n").write("\n".join(keep) + "\n")
print(m, "exports:", len(base), "->", len(keep))
