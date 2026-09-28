"""Proves a readability edit (types, struct fields, names) left the compiled code unchanged: compares the disassembly
(dumpbin /disasm, with relocation targets) of a module's objects before and after.
  python tools/objdiff.py snapshot <module>   copy build/obj/<module>/*.obj to build/objsnap/<module>/
  (edit src/<module>, then python tools/msvc_build.py <module>)
  python tools/objdiff.py compare <module> [old=new ...] [--index-folding]
                                              list every object whose code differs from the snapshot (after the
                                              symbol renames old=new); exit 1 if any
--index-folding also accepts objects whose only difference is /Od's constant-index computation (p[3] on a byte
pointer rewritten as p->field: the same access), listed as FOLDED. A difference is not automatically wrong (a corrected signedness can change a compare), but each one must be checked
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
    # compiler-numbered labels ($SG string literals, $LN local labels) shift when a header grows: not a code change
    return re.sub(r"\$(SG|LN)\d+", r"$\1", "\n".join(lines))


def renamed(text, renames):
    for a, b in renames:
        text = re.sub(r"\b_%s\b" % re.escape(a), "_" + b, text)
    return text


def index_folded(text):
    """the code with /Od's constant-index computations folded away: p[3] on a byte pointer compiles to
    mov r,1 / imul r2,r,3 (or shl) / [base+r2], p->field to [base+3]. Registers are renamed and jump targets
    dropped (both shift when the index computation goes), displacements of [a+b] and [a+N] forms are erased,
    so what is left compared is the instruction sequence, operand widths and every memory access's base"""
    ins = []
    for line in text.splitlines():
        m = re.match(r"\s*[0-9A-F]{8}: (?:[0-9A-F]{2} )+\s*(\S+)\s*(.*)", line)
        if m:
            ins.append((m.group(1), m.group(2)))
    out, skip = [], set()
    for i in range(len(ins) - 1):
        # mov r,1 followed by imul r2,r,N or shl r,N: the constant index, gone once it is a field
        m = re.fullmatch(r"(e[a-d]x|e[sd]i),1", ins[i][1]) if ins[i][0] == "mov" else None
        if m and ins[i + 1][0] in ("imul", "shl") and m.group(1) in ins[i + 1][1]:
            skip.update((i, i + 1))
    for i, (op, args) in enumerate(ins):
        if i in skip:
            continue
        args = re.sub(r"\be?[a-d][xlh]\b|\be[sd]i\b", "R", args)
        args = re.sub(r"\[R\+R\]|\[R\+[0-9A-F]+h?\]|\[R\]", "[R+X]", args)
        if op.startswith("j") or op == "call":
            args = re.sub(r"^[0-9A-F]{8}$", "T", args)
        out.append(op + " " + args)
    return out


def main():
    if len(sys.argv) < 3 or sys.argv[1] not in ("snapshot", "compare"):
        raise SystemExit(__doc__)
    mod = sys.argv[2]
    # symbol renames made by the edit (old=new): applied to the snapshot's disassembly before comparing
    renames = [a.split("=", 1) for a in sys.argv[3:] if "=" in a]
    src, snap = os.path.join(ROOT, "build", "obj", mod), os.path.join(SNAP, mod)
    if sys.argv[1] == "snapshot":
        shutil.rmtree(snap, ignore_errors=True)
        shutil.copytree(src, snap)
        print("snapshot: %d objects" % len(glob.glob(os.path.join(snap, "*.obj"))))
        return
    diff, same, folded = [], 0, []
    for old in sorted(glob.glob(os.path.join(snap, "*.obj"))):
        new = os.path.join(src, os.path.basename(old))
        if not os.path.exists(new):
            diff.append(os.path.basename(old) + " (object gone)")
        elif open(old, "rb").read() == open(new, "rb").read() or renamed(disasm(old), renames) == disasm(new):
            same += 1
        elif "--index-folding" in sys.argv and index_folded(renamed(disasm(old), renames)) == index_folded(disasm(new)):
            folded.append(os.path.basename(old))
        else:
            diff.append(os.path.basename(old))
    print("%d identical, %d differ%s" % (same, len(diff), ", %d equal after constant-index folding" % len(folded)
                                         if folded else ""))
    for d in folded:
        print("   FOLDED " + d)
    for d in diff:
        print("   DIFF " + d)
    sys.exit(1 if diff else 0)


if __name__ == "__main__":
    main()
