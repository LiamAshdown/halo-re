"""Check one or more rewrites against the binary's register inputs, and compile each file on its own.
For each function: the registers its "// blam-cc:" notes map to parameters, the registers the original halo.exe
code actually reads before writing (harness/gen_hooks.py live_in), whether they agree, and whether the file
compiles cleanly with the project's MSVC flags (to a scratch object, so concurrent runs do not collide).
Usage: python tools/regcheck.py <function name> [...]"""
import os, re, sys, json, glob, shutil, tempfile, subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); sys.path.insert(0, os.path.join(ROOT, "tools"))
import gen_hooks as g
import msvc_build as mb

def own_notes(t, name, d, params):
    b = t[:t.rfind("#if 0")] if "#if 0" in t else t
    hdr_end = re.search(r'^(#include|extern)', t, re.M); own = t[:hdr_end.start()] if hdr_end else t[:3000]
    above = b[:d.start()].rstrip("\n").split("\n"); k = len(above)
    while k > 0 and above[k - 1].lstrip().startswith("//"): k -= 1
    own += "\n" + "\n".join(above[k:]) + "\n" + d.group(0)
    return g.parse_cc(own, [x["name"] for x in params],
                      {x["name"]: (re.findall(r"[A-Za-z_]\w*", x["type"].replace("const", "").replace("struct", "")) or [""])[-1] for x in params})

def main():
    env = mb.msvc_env(); cl = shutil.which("cl", path=env.get("PATH") or env.get("Path"))
    sizes = {}
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        h = g.HDR.search(open(p, encoding="utf-8", errors="replace").read(4000))
        if h: sizes[int(h.group(1), 16)] = int(h.group(2))
    g.FUNC_SIZES.update(sizes)
    for name in sys.argv[1:]:
        files = glob.glob(os.path.join(ROOT, "src", "*", name + ".c"))
        if not files: print(f"{name}: no src/*/{name}.c"); continue
        p = files[0]; t = open(p, encoding="utf-8", errors="replace").read()
        b = t[:t.rfind("#if 0")] if "#if 0" in t else t
        h = g.HDR.search(t[:4000]); d = [m for m in g.DEF.finditer(b) if m.group(2) == name]
        if not h or not d: print(f"{name}: no address header or no definition"); continue
        params = [x for x in (g.param_info(q) for q in g.split_params(d[0].group(3))) if x]
        regs, has_cc, widths, order = own_notes(t, name, d[0], params)
        addr, size = int(h.group(1), 16), int(h.group(2))
        live = g.live_in(addr, size)
        notes = sorted(set(v for v in regs.values() if v))
        ok = notes == live and len(set(regs.values())) == len(regs)
        with tempfile.TemporaryDirectory() as td:
            r = subprocess.run([cl] + mb.CFLAGS + ["/Fo" + os.path.join(td, "x.obj"), p], capture_output=True, text=True, env=env, errors="replace")
        errs = [l.strip() for l in r.stdout.splitlines() if re.search(r": (fatal )?error |: warning C4(013|020|029|047|133|024|028|087|113|716)", l)]
        print(f"{name}: notes {notes} {'==' if ok else '!='} binary {live}; stack params in order {[x['name'] for x in params if x['name'] not in regs]}; "
              f"compile {'OK' if r.returncode == 0 and not errs else 'FAILED'}")
        for e in errs[:6]: print("    " + e)

if __name__ == "__main__": main()
