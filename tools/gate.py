"""Single entry point for the repo's checks. Exit 1 if any hard check fails.
Hard checks (must pass):   check32 (32-bit type/layout pass over every src/*/*.c), coverage_audit
Advisory (reported only):  check_arity, check_prototypes
check32 needs clang and mingw-w64 headers (apt install clang mingw-w64-i686-dev).
Usage: python tools/gate.py [--quick]      (--quick skips check32)"""
import os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
quick = "--quick" in sys.argv
CHECKS = [  # (name, argv, hard)
    ("check32", ["bash", "tools/check32.sh"], True),
    ("coverage_audit", [sys.executable, "tools/coverage_audit.py"], True),
    ("check_arity", [sys.executable, "tools/check_arity.py", "--calls"], False),
    ("check_prototypes", [sys.executable, "tools/check_prototypes.py"], False),
]
failed = []
for name, argv, hard in CHECKS:
    if quick and name == "check32":
        continue
    print("== %s ==" % name, flush=True)
    r = subprocess.run(argv, cwd=ROOT)
    if r.returncode != 0:
        print("-> %s FAILED (exit %d)%s" % (name, r.returncode, "" if hard else " [advisory]"))
        if hard:
            failed.append(name)
print("\ngate: %s" % ("FAIL: " + ", ".join(failed) if failed else "ok"))
sys.exit(1 if failed else 0)
