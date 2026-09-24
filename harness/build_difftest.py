"""Build harness/build/difftest.exe from the same objects as halo_rewrite.dll (run harness/gen_hooks.py and
harness/gen_link.py first), then run it: python harness/build_difftest.py [module|function ...] [-n N] [-v]"""
import os, sys, glob, subprocess
sys.argv_saved = sys.argv; sys.argv = [sys.argv[0]]
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_link as g
sys.argv = sys.argv_saved

def main():
    out = g.OUT; objs = []
    for c in ("difftest_main.c", "gen/difftest_table.c"):
        o = os.path.join(out, os.path.basename(c).replace(".c", ".obj")); objs.append(o)
        r = subprocess.run([g.tool("cl"), "/nologo", "/c", "/GS-", "/O2", "/W3", "/wd4996", "/Fo" + o, os.path.join(g.H, c)],
                           capture_output=True, text=True, env=g.env)
        if r.returncode: print(r.stdout[-3000:]); return 1
    o = os.path.join(out, "difftest_call.obj"); objs.append(o)
    r = subprocess.run([g.tool("ml"), "/nologo", "/c", "/coff", "/Fo" + o, os.path.join(g.H, "difftest_call.asm")], capture_output=True, text=True, env=g.env)
    if r.returncode: print(r.stdout[-3000:]); return 1
    objs += [os.path.join(out, n) for n in ("x87_shims.obj", "adapters.obj", "resolve.obj", "hook_table.obj")]
    objs += glob.glob(os.path.join(g.ROOT, "build", "obj", "*", "*.obj"))
    rsp = os.path.join(out, "difftest.rsp"); open(rsp, "w").write("\n".join('"%s"' % x for x in objs))
    exe = os.path.join(out, "difftest.exe")
    r = subprocess.run([g.tool("link"), "/nologo", "/MACHINE:X86", "/FIXED", "/BASE:0x20000000", "/SAFESEH:NO", "/SUBSYSTEM:CONSOLE",
                        "/FORCE:UNRESOLVED", "/OUT:" + exe, "@" + rsp] + g.SYS_LIBS + ["libcmt.lib", "libvcruntime.lib", "libucrt.lib"],
                       capture_output=True, text=True, env=g.env)
    if not os.path.exists(exe): print(r.stdout[-3000:]); return 1
    args = [exe, os.path.join(g.ROOT, "bin", "halo.exe")] + (sys.argv[1:] or ["math"])
    return subprocess.run(args).returncode

if __name__ == "__main__": sys.exit(main())
