"""Pointers already declared with a struct type but still accessed by byte offset, e.g.
    Scenario *scenario; ... *(int16_t *)((uint8_t *)scenario + 0x1a4)
get the struct's field (tools/type_access.py, per file; the variable keeps its type).
  python tools/infer_typed_vars.py [--dry]"""
import collections, glob, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import type_access as ta

DECL = re.compile(r"(?:^|[(,;{]\s*)(?:const\s+)?(?:struct\s+)?([A-Za-z_]\w*)\s*\*\s*(?:const\s+)?(\w+)\s*(?=[,);=\[])", re.M)
SKIP = {"uint8_t", "int8_t", "void", "char", "uint16_t", "int16_t", "uint32_t", "int32_t", "float", "real", "double",
        "wchar_t", "unsigned", "signed", "int", "short", "long", "const", "return", "datum_index", "FILE", "HANDLE"}


def main():
    dry = "--dry" in sys.argv
    groups = collections.defaultdict(set)
    for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        live = open(f, encoding="utf-8", errors="replace").read().split("\n#if 0")[0]
        live_code = re.sub(r"//[^\n]*|/\*.*?\*/", "", live, flags=re.S)
        for t, v in set(DECL.findall(live_code)):
            if t in SKIP or not ta.header_of(t):
                continue
            if re.search(r"\(\(?(?:\((?:const )?[\w ]+\*\))?%s\)? \+ (?:0x[0-9a-fA-F]+|\d+)\)" % re.escape(v), live_code):
                groups[(t, v)].add(os.path.relpath(f, ROOT))
    total = 0
    for (t, v), files in sorted(groups.items()):
        cmd = [sys.executable, os.path.join(ROOT, "tools", "type_access.py"), t, ta.header_of(t), v] + sorted(files) + \
              ["--pre", "memory.h,objects.h,units.h"] + (["--dry"] if dry else []) + (["--allow-unknown"] if "--allow-unknown" in sys.argv else [])
        out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT).stdout.strip().splitlines()
        last = out[-1] if out else "?"
        n = int(last.split()[0]) if last[:1].isdigit() else 0
        total += n
        if n:
            print("%-30s %-16s %s" % (t, v, last))
    print(total, "accesses")


if __name__ == "__main__":
    main()
