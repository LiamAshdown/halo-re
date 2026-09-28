"""Lists globals that src/ declares with a struct-pointer type in some files and as a raw pointer (uint8_t * /
void * / int32_t *...) in others, with how many raw-offset accesses the raw declarations carry: the files that can
simply take the typed declaration (tools/ghidra_residue.py counts what is left).
Usage: python tools/global_type_split.py"""
import collections, glob, os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DECL = re.compile(r"^extern\s+([A-Za-z_][\w ]*?)\s*(\*+)\s*(\w+)\s*;", re.M)
RAW = {"uint8_t", "void", "int32_t", "uint32_t", "char", "int16_t", "uint16_t", "int8_t", "const uint8_t", "const void"}


def live(t):
    i = t.find("\n#if 0")
    return t if i < 0 else t[:i]


def main():
    typed = collections.defaultdict(collections.Counter)
    raw = collections.defaultdict(list)
    for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = live(open(f, encoding="utf-8", errors="replace").read())
        for m in DECL.finditer(t):
            base, stars, name = m.group(1).strip(), m.group(2), m.group(3)
            if len(stars) != 1:
                continue
            if base in RAW:
                uses = len(re.findall(r"\(\(?(?:\([a-z0-9_ ]+\*\))?%s\)? \+ (?:0x[0-9a-fA-F]+|\d+)\)" % re.escape(name), t))
                raw[name].append((os.path.relpath(f, ROOT), base, uses))
            else:
                typed[name][base] += 1
    rows = []
    for name, files in raw.items():
        if name in typed:
            rows.append((sum(u for _, _, u in files), name, typed[name].most_common(1)[0], files))
    rows.sort(reverse=True)
    for uses, name, (tname, tcount), files in rows:
        print("%-40s typed %s * in %d files; raw in %d files, %d offset uses" % (name, tname, tcount, len(files), uses))


if __name__ == "__main__":
    main()
