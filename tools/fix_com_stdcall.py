"""COM methods (Direct3D 9, D3DX effects, DirectInput 8, DirectSound 8) are __stdcall: the callee pops its
arguments. A rewrite that calls one through a plain function-pointer type makes the caller pop them a second time,
so the stack pointer drifts on every call (rasterizer_render_target_dispose crashed inside d3d9.dll this way).
This adds __stdcall to every function-pointer type that is called through an object's vtable, above the final #if 0:
  * typedef RET (*name)(...)  where name is used to cast a vtable entry  ->  typedef RET (__stdcall *name)(...)
  * inline casts ((RET (*)(...))<vtable expression>)(...)                ->  ((RET (__stdcall *)(...))...)(...)
A vtable expression reads through the object's first word: vtable[...], (*(void ***)x)[...], **(...), *(... *)(*x + N).
Blam's own callback tables are plain C (cdecl) arrays in .data and are not touched.
Usage: python tools/fix_com_stdcall.py [--apply]"""
import re, glob, os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APPLY = "--apply" in sys.argv
VT = r"(?:vtable|vtbl|lpVtbl|\*\s*\(\s*void\s*\*\*\*\s*\)|\*\s*\(\s*\w+\s*\*\*\s*\)\s*\(?\s*\*|\(\s*\*\s*\(\s*(?:uint32_t|int32_t|void)\s*\*\*?\s*\)\s*\w+\s*\)\s*\[|\(\s*\*\s*\w+\s*\)\s*\[\s*\d+\s*\]|\*\s*\w+\s*\+\s*0x[0-9a-f]+)"
stats = {"files": 0, "typedefs": 0, "casts": 0}; review = []
for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
    t = open(p, encoding="utf-8", errors="replace").read()
    cut = t.rfind("#if 0"); b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
    nb = b
    # typedef'd function-pointer types used on vtable entries
    for m in re.finditer(r"typedef\s+([\w\s\*]+?)\(\s*\*\s*(\w+)\s*\)\s*\(", b):
        name = m.group(2)
        uses = re.findall(r"\(\s*\(?\s*" + re.escape(name) + r"\s*\)\s*([^;]{0,120})", b)
        uses += re.findall(r"\b" + re.escape(name) + r"\s+\w+\s*=\s*\(\s*" + re.escape(name) + r"\s*\)\s*([^;]{0,120})", b)
        com_name = re.match(r"(d3dx?|dinput|dsound|direct)[a-z0-9]*_", name, re.I) is not None and bool(uses)
        if not com_name and not any(re.search(VT, u) for u in uses):
            if uses and re.search(r"(d3d|dinput|dsound|direct|iunknown|release)", name, re.I): review.append(f"{os.path.relpath(p, ROOT)}: {name}")
            continue
        if True:
            nb = re.sub(r"typedef(\s+[\w\s\*]+?)\(\s*\*\s*" + re.escape(name) + r"\s*\)", r"typedef\1(__stdcall *" + name + ")", nb, count=1)
            stats["typedefs"] += 1
    # inline casts applied to a vtable read
    def fix_cast(m):
        if re.search(VT, m.group(3)): stats["casts"] += 1; return m.group(1) + "(__stdcall *)" + m.group(2) + m.group(3)
        return m.group(0)
    nb = re.sub(r"(\(\s*\(\s*[\w\s\*]+?)\(\s*\*\s*\)(\s*\([^()]*(?:\([^()]*\)[^()]*)*\)\s*\)\s*)([^;]{0,120})", fix_cast, nb)
    nb = nb.replace("(__stdcall *)", "(__stdcall *)").replace("__stdcall __stdcall", "__stdcall")
    if nb != b:
        stats["files"] += 1
        if APPLY: open(p, "w", encoding="utf-8", newline="").write(nb + tail)
print(("APPLIED" if APPLY else "DRY RUN"), stats)
print("name looks like COM but no vtable use found (left alone):", len(review)); print(chr(10).join(review[:25]))
