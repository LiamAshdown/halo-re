"""C++ conversion, phase 1 gate: does each src/*/*.c file compile as C++20? (syntax only, cl /Zs /TP, no output files)
Usage: python tools/cxx_probe.py [files or substring filters ...] [--list FILE] [--summary]
  no argument      every src/*/*.c except src/gamespy (GameSpy stays vendored C)
  paths/filters    only the matching files; --list FILE reads one path per line
Prints the failing files with their first errors; exit code 1 if any file fails.
A fix must keep the file's behaviour identical (casts, real prototypes, renamed C++ keywords); it must not change layout."""
import sys, os, glob, subprocess, re, collections
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness"))
import gen_link as gl
DXSDK = r"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include"


def probe(f):
    cmd = [gl.tool("cl"), "/nologo", "/Zs", "/TP", "/std:c++20", "/W3", "/wd4996", "/permissive-",
           "/I", os.path.join(ROOT, "types"), "/I", DXSDK, "/FI" + os.path.join(ROOT, "harness", "msvc_compat.h"), f]
    r = subprocess.run(cmd, capture_output=True, text=True, env=gl.env, errors="replace", cwd=ROOT)
    errs = [l for l in (r.stdout + r.stderr).splitlines() if re.search(r"error C\d+", l)]
    return f, r.returncode, errs


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    files = sorted(f.replace("\\", "/") for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c")))
    files = [os.path.relpath(f, ROOT).replace("\\", "/") for f in files if "/gamespy/" not in f.replace("\\", "/")]
    if "--list" in sys.argv:
        lst = sys.argv[sys.argv.index("--list") + 1]
        want = {l.strip() for l in open(lst) if l.strip()}
        files = [f for f in files if f in want]
        args = [a for a in args if a != lst]
    elif args:
        files = [f for f in files if any(a.replace("\\", "/") in f for a in args)]
    with ThreadPoolExecutor(12) as ex:
        res = list(ex.map(probe, files))
    bad = [r for r in res if r[1]]
    for f, rc, errs in bad:
        print(f)
        for e in errs[:4]:
            print("   ", e.split(f.split("/")[-1], 1)[-1].strip()[:230] if "--summary" not in sys.argv else e[:100])
    print("%d files, %d fail as C++" % (len(res), len(bad)))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
