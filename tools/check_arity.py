"""Lists extern function declarations in src/ whose parameter count differs from the function's definition
(src/<module>/<name>.c). A C call made through such a declaration passes its arguments in the wrong slots: the
common cause is a declaration copied from Ghidra's view of the stack arguments, missing the register argument(s)
(EAX/ECX/...) that the rewritten definition takes as ordinary parameters (see its "blam-cc:" comment).
Read-only.  python tools/check_arity.py [--calls]   (--calls: only declarations the file actually calls)"""
import collections, glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import decl_param_names as dp


def count(params):
    ps = dp.split_params(params)
    if not ps or ps[0].strip() in ("void", ""):
        return 0
    return len(ps) + (100 if any("..." in p for p in ps) else 0)


def main():
    defs = {}
    for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        name = os.path.splitext(os.path.basename(f))[0]
        t = open(f, encoding="utf-8", errors="replace").read().split("\n#if 0")[0]
        m = re.search(r"^[^\n;{}#/]*?\b%s\s*\(([^)]*(?:\([^)]*\)[^)]*)*)\)\s*(?://[^\n]*)?\n?\s*(?://[^\n]*\n\s*)*\{"
                      % re.escape(name), t, re.M)
        if m:
            defs[name] = count(m.group(1))
    decl = re.compile(r"^extern\s+[^;{]*?\b(\w+)\s*\(([^;{]*?)\)\s*;", re.M | re.S)
    rows = collections.defaultdict(list)
    for f in sorted(glob.glob(os.path.join(ROOT, "src", "*", "*.c"))):
        t = open(f, encoding="utf-8", errors="replace").read()
        i = t.find("\n#if 0")
        live = t if i < 0 else t[:i]
        for m in decl.finditer(live):
            fn, params = m.group(1), m.group(2)
            if fn not in defs or "(*" in params:
                continue
            n = count(params)
            if n != defs[fn] and n < 100 and defs[fn] < 100:
                called = re.search(r"(?<![\w*])%s\s*\(" % re.escape(fn), live[m.end():]) is not None
                if "--calls" in sys.argv and not called:
                    continue
                rows[fn].append((os.path.relpath(f, ROOT), n, defs[fn], called))
    total = sum(len(v) for v in rows.values())
    for fn in sorted(rows, key=lambda k: -len(rows[k])):
        for f, n, d, called in rows[fn]:
            print("%-48s declared %d, defined %d%s  %s" % (fn, n, d, "" if called else " (not called)", f))
    print("%d declarations of %d functions differ from the definition's parameter count" % (total, len(rows)))


if __name__ == "__main__":
    main()
