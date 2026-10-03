"""Behaviour-equivalence test of the math module conversion: original C sources vs converted C++ sources.

  python tests/math/run_difftest.py [--ref REF]       (default REF: c54d5e1f, the last commit with src/math/*.c)

Builds two executables with the halo_game compile flags (tools/check_module_symbols.py compile_flags):
  reference  src/math/*.c, types/ and harness/msvc_compat.h exactly as they are at REF (git archive into a scratch dir)
  converted  src/math/*.cpp of the working tree
both linked with the same driver (tests/math/math_difftest.cpp), stubs (tests/math/math_test_stubs.cpp) and the x87
shims of the real build (harness/x87_shims.c), compiled ONCE and shared. Each writes every output of every exported
function as raw hex; the two files must be byte-for-byte identical. Build output: build/math_difftest/."""
import glob, io, os, shutil, subprocess, sys, tarfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "harness")); sys.path.insert(0, os.path.join(ROOT, "tools"))
import check_module_symbols as cms  # noqa: E402
gl = cms.gl
OUT = os.path.join(ROOT, "build", "math_difftest")
DEFAULT_REF = "c54d5e1f"


def flags(root, src):
    """The project flags of check_module_symbols.compile_flags, with the include directories rooted at `root`."""
    f = cms.compile_flags(src)
    return [x.replace(ROOT, root) if x.startswith(ROOT) else x for x in f]


def compile_all(srcs, objdir, root, extra=()):
    os.makedirs(objdir, exist_ok=True)
    objs = []
    for s in srcs:
        obj = os.path.join(objdir, os.path.basename(s) + ".obj")
        r = subprocess.run([gl.tool("cl")] + flags(root, s) + list(extra) + ["/Fo" + obj, s], capture_output=True,
                           text=True, env=gl.env, errors="replace", cwd=root)
        if r.returncode:
            print(r.stdout[-3000:]); raise SystemExit("compile failed: " + s)
        objs.append(obj)
    return objs


def link(objs, exe):
    r = subprocess.run([gl.tool("link"), "/nologo", "/OUT:" + exe, "/SUBSYSTEM:CONSOLE", "/MACHINE:X86"] + objs +
                       ["kernel32.lib", "legacy_stdio_definitions.lib"], capture_output=True, text=True, env=gl.env,
                       errors="replace")
    if r.returncode:
        print(r.stdout[-4000:]); raise SystemExit("link failed: " + exe)


def main():
    ref = sys.argv[sys.argv.index("--ref") + 1] if "--ref" in sys.argv else DEFAULT_REF
    shutil.rmtree(OUT, ignore_errors=True)
    refroot = os.path.join(OUT, "reference_tree")
    os.makedirs(refroot)
    tar = subprocess.run(["git", "archive", "--format=tar", ref, "src/math", "types", "harness/msvc_compat.h"],
                         cwd=ROOT, capture_output=True, check=True).stdout
    tarfile.open(fileobj=io.BytesIO(tar)).extractall(refroot, filter="data")
    glm = ["/I", os.path.join(ROOT, "third_party", "glm")]
    shared = compile_all([os.path.join(ROOT, "tests", "math", "math_difftest.cpp"),
                          os.path.join(ROOT, "tests", "math", "math_test_stubs.cpp")], os.path.join(OUT, "shared"), ROOT,
                         ["/I", os.path.join(ROOT, "tests", "math")])
    shims = os.path.join(OUT, "shared", "x87_shims.obj")
    r = subprocess.run([gl.tool("cl"), "/nologo", "/c", "/TC", "/O2", "/GS-", "/Fo" + shims,
                        os.path.join(ROOT, "harness", "x87_shims.c")], capture_output=True, text=True, env=gl.env)
    if r.returncode:
        print(r.stdout); raise SystemExit("x87_shims failed")
    shared.append(shims)
    ref_objs = compile_all(sorted(glob.glob(os.path.join(refroot, "src", "math", "*.c"))), os.path.join(OUT, "reference"), refroot)
    new_objs = compile_all(sorted(glob.glob(os.path.join(ROOT, "src", "math", "*.cpp"))), os.path.join(OUT, "converted"), ROOT, glm)
    results = {}
    for name, objs in (("reference", ref_objs), ("converted", new_objs)):
        exe = os.path.join(OUT, "math_difftest_%s.exe" % name)
        link(shared + objs, exe)
        txt = os.path.join(OUT, "%s.txt" % name)
        r = subprocess.run([exe, txt], capture_output=True, text=True, timeout=600)
        if r.returncode:
            raise SystemExit("%s run failed (%d): %s" % (name, r.returncode, r.stdout + r.stderr))
        results[name] = open(txt, "rb").read()
        print("%s: %d objects, %s, %d bytes of output" % (name, len(objs), r.stdout.strip(), len(results[name])))
    api = compile_all([os.path.join(ROOT, "tests", "math", "math_api_test.cpp")], os.path.join(OUT, "api"), ROOT,
                      ["/I", os.path.join(ROOT, "tests", "math")])
    exe = os.path.join(OUT, "math_api_test.exe")
    link(api + shared[1:] + new_objs, exe)
    r = subprocess.run([exe], capture_output=True, text=True, timeout=600)
    print("api test (C++ members/operators/glm/random_stream vs the C functions): %s" % r.stdout.strip().splitlines()[-1])
    if r.returncode:
        print(r.stdout); return 1
    a, b = results["reference"].splitlines(), results["converted"].splitlines()
    if results["reference"] == results["converted"]:
        names = sorted({l.split(b" ", 1)[0].decode() for l in a if not l.startswith(b"calls")})
        print("IDENTICAL: %d lines, %d function labels" % (len(a), len(names)))
        return 0
    diff = [(x, y) for x, y in zip(a, b) if x != y]
    labels = sorted({x.split(b" ", 1)[0].decode() for x, _ in diff})
    print("DIFFERENT: %d of %d lines differ (line count %d vs %d); functions: %s" % (len(diff), len(a), len(a), len(b), ", ".join(labels)))
    for x, y in diff[:6]:
        print("  ref ", x[:300].decode()); print("  new ", y[:300].decode())
    return 1


if __name__ == "__main__":
    sys.exit(main())
