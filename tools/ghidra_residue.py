"""Counts the Ghidra leftovers in live C (outside #if 0 blocks) per module: raw offset accesses through casts
(*(int *)(p + 0x14)), Ghidra variable names (param_1, iVar1, local_10, uVar2, pcVar3 ...) and Ghidra integer type
names (uint, ushort, uchar, byte, dword, ...).
Usage: python tools/ghidra_residue.py [module]        per-module totals, or per-file totals for one module"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OFFSET = re.compile(r"\*\s*\(\s*[\w ]+\*+\s*\)\s*\(?\s*\(?[\w\->.\[\]]+\)?\s*\+\s*(?:0x[0-9a-fA-F]+|\d+)\s*\)")
NAMES = re.compile(r"\b(?:param_\d+|local_[0-9a-f]+|[a-z]{1,3}Var\d+|in_\w+|extraout_\w+|unaff_\w+)\b")
TYPES = re.compile(r"\b(?:uint|ushort|uchar|byte|dword|word|ulonglong|longlong|undefined\d?)\b")


def live(text):
    out, depth = [], 0
    for line in text.splitlines():
        s = line.strip()
        if depth:
            if s.startswith("#if"):
                depth += 1
            elif s.startswith("#endif"):
                depth -= 1
            continue
        if re.match(r"#\s*if\s+0\b", s):
            depth = 1
            continue
        out.append(line)
    return "\n".join(out)


def counts(path):
    t = live(re.sub(r"/\*.*?\*/", "", open(path, encoding="utf-8", errors="replace").read(), flags=re.S))
    t = re.sub(r"//[^\n]*", "", t)
    return len(OFFSET.findall(t)), len(NAMES.findall(t)), len(TYPES.findall(t))


def main():
    mods = sorted(os.listdir(os.path.join(ROOT, "src")))
    if len(sys.argv) > 1:
        rows = [(os.path.basename(p),) + counts(p) for p in sorted(glob.glob(os.path.join(ROOT, "src", sys.argv[1], "*.c")))]
        rows = [r for r in rows if sum(r[1:])]
    else:
        rows = []
        for m in mods:
            c = [counts(p) for p in glob.glob(os.path.join(ROOT, "src", m, "*.c"))]
            rows.append((m,) + tuple(sum(x[i] for x in c) for i in range(3)))
    rows.sort(key=lambda r: sum(r[1:]))
    print("%-40s %8s %8s %8s" % ("", "offsets", "names", "types"))
    for r in rows:
        print("%-40s %8d %8d %8d" % r)
    print("%-40s %8d %8d %8d" % (("total",) + tuple(sum(r[i] for r in rows) for i in (1, 2, 3))))


if __name__ == "__main__":
    main()
