"""Host-gcc syntax gate with a baseline diff, for renaming/refactor work on machines without MSVC.

  python tools/host_gate.py [module ...]              print every failing file (default: all modules)
  python tools/host_gate.py --baseline REF [module ...]
                                                      compare against the failures at git REF (default HEAD when the
                                                      flag is given without a value is not supported; pass a ref) and
                                                      print only NEW failures; exit 1 if there are any

Uses `gcc -m32 -fsyntax-only` with __stdcall/__cdecl/__fastcall defined away, so many files still fail on a bare
Linux box (winsock2.h, libc headers): that is why the comparison is against a baseline, not against zero. It checks
names/types/fields, not 32-bit layout or the compiled code -- run tools/objdiff.py / the MSVC build for that."""
import concurrent.futures, glob, os, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GCC = os.environ.get("GCC", "gcc")
FLAGS = ["-m32", "-D__stdcall=", "-D__cdecl=", "-D__fastcall=", "-fsyntax-only", "-std=gnu99", "-Wall",
         "-Wno-unused-variable", "-Wno-unused-but-set-variable", "-Werror=implicit-function-declaration",
         "-Werror=implicit-int"]


def compiles(args):
    root, c = args
    r = subprocess.run([GCC] + FLAGS + ["-I", os.path.join(root, "types"), c], capture_output=True)
    return None if r.returncode == 0 else os.path.relpath(c, root).replace(os.sep, "/")


def failures(root, mods):
    """Failing files under src/<mods>, compiled in parallel (one gcc per core; the whole tree takes ~35 s on 8 cores)."""
    jobs = [(root, c) for m in mods for c in sorted(glob.glob(os.path.join(root, "src", m, "*.c")))]
    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as ex:
        return {f for f in ex.map(compiles, jobs) if f}


def baseline_failures(ref, mods):
    """Failures at git REF, cached in build/host_gate_cache/<sha>-<mods>.txt so repeat runs skip the baseline."""
    sha = subprocess.run(["git", "-C", ROOT, "rev-parse", ref], capture_output=True, text=True, check=True).stdout.strip()
    key = "%s-%s.txt" % (sha[:12], "all" if len(mods) > 8 else "_".join(mods))
    cache = os.path.join(ROOT, "build", "host_gate_cache", key)
    if os.path.exists(cache):
        return set(open(cache).read().split())
    with tempfile.TemporaryDirectory() as tmp:
        subprocess.run(["git", "-C", ROOT, "worktree", "add", "--detach", tmp, sha], check=True, capture_output=True)
        try:
            base = failures(tmp, mods)
        finally:
            subprocess.run(["git", "-C", ROOT, "worktree", "remove", "--force", tmp], capture_output=True)
    os.makedirs(os.path.dirname(cache), exist_ok=True)
    open(cache, "w").write("\n".join(sorted(base)) + "\n")
    return base


def main():
    args = sys.argv[1:]
    ref = None
    if "--baseline" in args:
        i = args.index("--baseline")
        ref = args[i + 1]
        del args[i:i + 2]
    src = os.path.join(ROOT, "src")
    mods = args or sorted(d for d in os.listdir(src) if os.path.isdir(os.path.join(src, d)))
    now = failures(ROOT, mods)
    if ref is None:
        for f in sorted(now):
            print("FAIL", f)
        print("host_gate: %d failing" % len(now))
        return 0
    base = baseline_failures(ref, mods)
    new = sorted(now - base)
    for f in new:
        print("NEW FAIL", f)
    print("host_gate: %d failing now, %d at %s, %d new" % (len(now), len(base), ref, len(new)))
    return 1 if new else 0


if __name__ == "__main__":
    sys.exit(main())
