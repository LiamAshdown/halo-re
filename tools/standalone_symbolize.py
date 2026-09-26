"""Annotate the addresses in a standalone halo_standalone.log with function names.
Addresses inside our exe are looked up in build/standalone/halo_rebuilt.map; addresses in the original range
(0x400000..0x63a000, int3-filled at run time) are looked up in out/functions.json.
Usage: python tools/standalone_symbolize.py [log path]"""
import os, re, sys, json, bisect

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "build", "standalone")


def load_map():
    syms = []
    for line in open(os.path.join(OUT, "halo_rebuilt.map"), errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s", line)
        if m:
            syms.append((int(m.group(2), 16), m.group(1)))
    syms.sort()
    return syms


def load_original():
    fs = sorted((int(f["addr"], 16), f["name"]) for f in json.load(open(os.path.join(ROOT, "out", "functions.json"))))
    return fs


def lookup(table, a):
    i = bisect.bisect_right([x[0] for x in table], a) - 1
    return "%s+0x%x" % (table[i][1], a - table[i][0]) if i >= 0 else "?"


def main():
    log = sys.argv[1] if len(sys.argv) > 1 else os.path.join(OUT, "halo_standalone.log")
    ours, orig = load_map(), load_original()
    lo, hi = ours[0][0] if ours else 0, 0x20000000
    for line in open(log, errors="replace"):
        line = line.rstrip("\n")
        notes = []
        for h in re.findall(r"\b[0-9a-f]{8}\b", line):
            a = int(h, 16)
            if 0x10000000 <= a < hi and ours:
                notes.append("%s=%s" % (h, lookup(ours, a)))
            elif 0x401000 <= a < 0x63a000:
                notes.append("%s=%s (original)" % (h, lookup(orig, a)))
        print(line)
        for n in notes:
            print("      " + n)


if __name__ == "__main__":
    main()
