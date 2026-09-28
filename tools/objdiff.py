"""Proves a readability edit (types, struct fields, names) left the compiled code unchanged: compares the disassembly
(dumpbin /disasm, with relocation targets) of a module's objects before and after.
  python tools/objdiff.py snapshot <module>   copy build/obj/<module>/*.obj to build/objsnap/<module>/
  (edit src/<module>, then python tools/msvc_build.py <module>)
  python tools/objdiff.py compare <module>    list every object whose code differs from the snapshot; exit 1 if any
A difference is not automatically wrong (a corrected signedness can change a compare), but each one must be checked
against the original function before it is kept."""
import glob, os, re, shutil, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness"))
import gen_link as gl

SNAP = os.path.join(ROOT, "build", "objsnap")


def disasm(obj):
    r = subprocess.run([gl.tool("dumpbin"), "/nologo", "/disasm", "/relocations", obj], capture_output=True, text=True,
                       env=gl.env, errors="replace")
    lines = [l for l in r.stdout.splitlines() if l.strip() and not l.startswith("Dump of file")
             and not re.match(r"\s*File Type:", l)]
    return "\n".join(lines)


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("snapshot", "compare"):
        raise SystemExit(__doc__)
    mod = sys.argv[2]
    src, snap = os.path.join(ROOT, "build", "obj", mod), os.path.join(SNAP, mod)
    if sys.argv[1] == "snapshot":
        shutil.rmtree(snap, ignore_errors=True)
        shutil.copytree(src, snap)
        print("snapshot: %d objects" % len(glob.glob(os.path.join(snap, "*.obj"))))
        return
    diff, same = [], 0
    for old in sorted(glob.glob(os.path.join(snap, "*.obj"))):
        new = os.path.join(src, os.path.basename(old))
        if not os.path.exists(new):
            diff.append(os.path.basename(old) + " (object gone)")
        elif open(old, "rb").read() == open(new, "rb").read() or disasm(old) == disasm(new):
            same += 1
        else:
            diff.append(os.path.basename(old))
    print("%d identical, %d differ" % (same, len(diff)))
    for d in diff:
        print("   DIFF " + d)
    sys.exit(1 if diff else 0)


if __name__ == "__main__":
    main()
