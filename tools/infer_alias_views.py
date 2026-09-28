"""A byte-pointer alias of a typed pointer takes that pointer's struct: for
    uint8_t *x = (uint8_t *)y;        (or (const uint8_t *)y)
where the same file declares y as T *y (a parameter or local) and T is a struct in types/, x's raw-offset accesses
in that file become T's fields (tools/type_access.py; x keeps its type).
  python tools/infer_alias_views.py [--dry]"""
import collections, glob, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import type_access as ta

ALIAS = re.compile(r"\buint8_t \*(\w+) = \((?:const )?uint8_t \*\)(\w+);")


def main():
    dry = "--dry" in sys.argv
    groups = collections.defaultdict(set)
    for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        live = open(f, encoding="utf-8", errors="replace").read().split("\n#if 0")[0]
        for x, y in ALIAS.findall(live):
            m = re.search(r"(?:^|[(,;{]\s*)(?:const\s+)?(?:struct\s+)?(\w+)\s*\*\s*(?:const\s+)?%s\s*[,);=]" % re.escape(y),
                          live, re.M)
            if not m:
                continue
            t = m.group(1)
            if t in ("uint8_t", "void", "char", "int8_t") or not ta.header_of(t):
                continue
            groups[(t, x)].add(os.path.relpath(f, ROOT))
    for (t, x), files in sorted(groups.items()):
        cmd = [sys.executable, os.path.join(ROOT, "tools", "type_access.py"), t, ta.header_of(t), x] + sorted(files) + \
              ["--pre", "memory.h,objects.h,units.h"] + (["--dry"] if dry else [])
        out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT).stdout.strip().splitlines()
        print("%-28s %-14s %s" % (t, x, out[-1] if out else "?"))


if __name__ == "__main__":
    main()
