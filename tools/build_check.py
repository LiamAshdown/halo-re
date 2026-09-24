"""Phase 4 compile gate: syntax/type-check every src/**/*.c against types/ with gcc -fsyntax-only.
Usage: python tools/build_check.py [module ...]   (default: all modules under src/)
Exit code 1 if any file fails. Prints one line per failing file plus the first error lines.
Note: this is a 64-bit host gcc doing a syntax/type pass; it catches undeclared identifiers, wrong
field names, bad prototypes and type mismatches, but not 32-bit layout assumptions."""
import glob, os, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GCC = r"C:\msys64\ucrt64\bin\gcc.exe"
mods = sys.argv[1:] or sorted(d for d in os.listdir(os.path.join(ROOT, "src")) if os.path.isdir(os.path.join(ROOT, "src", d)))
fail = ok = 0
for m in mods:
    for c in sorted(glob.glob(os.path.join(ROOT, "src", m, "*.c"))):
        r = subprocess.run([GCC, "-fsyntax-only", "-std=gnu99", "-Wall", "-Wno-unused-variable", "-Wno-unused-but-set-variable",
                            "-Werror=implicit-function-declaration", "-Werror=implicit-int", "-I", os.path.join(ROOT, "types"), c],
                           capture_output=True, text=True)
        if r.returncode == 0: ok += 1
        else:
            fail += 1
            errs = [l for l in r.stderr.splitlines() if "error" in l][:4]
            print("FAIL %s\n    %s" % (os.path.relpath(c, ROOT), "\n    ".join(errs)))
print("build_check: %d ok, %d failed" % (ok, fail))
sys.exit(1 if fail else 0)
