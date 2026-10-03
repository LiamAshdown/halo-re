"""Progress census for the modernisation loop: how much C-style code is left, per module.

Counted per module (src/<module>/*.cpp, include/halo/<module>/*.hpp, comments and strings excluded):
  extern_c      lines with `extern "C"` (linkage blocks and declarations)
  extern_decl   plain `extern <type> name...;` declarations (cross-module function/variable declarations)
  shims         functions in <module>_c_api.cpp / *_c_api.cpp files (the C entry points kept for the link tables)
  hex_literals  hex literals >= 0x100 (magic numbers: offsets, flags, addresses)
  raw_casts     casts to raw byte/word pointers ((uint8_t *), (int32_t *), (void *), (char *)): pointer arithmetic on records
  macros        #define lines
  gotos         goto statements
Usage: python tools/modernization_census.py [--json]      prints a table sorted by the total score (higher = more C left)."""
import glob, json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def strip(text):
    out, i, n = [], 0, len(text)
    while i < n:
        if text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
        elif text[i] in "\"'":
            q = text[i]
            j = i + 1
            while j < n and text[j] != q:
                j += 2 if text[j] == "\\" else 1
            out.append(q + q)
            i = j + 1
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


RAW = re.compile(r"\(\s*(?:const\s+)?(?:u?int(?:8|16|32|64)_t|char|void|unsigned char|unsigned int|float)\s*\*+\s*\)")
HEX = re.compile(r"\b0[xX](?:[0-9a-fA-F]{3,})\b")
EXTERN_DECL = re.compile(r"^\s*extern\s+(?!\"C\")[^\n;{]*;", re.M)


def census():
    rows = {}
    for mdir in sorted(glob.glob(os.path.join(ROOT, "src", "*"))):
        m = os.path.basename(mdir)
        if m == "gamespy":
            continue
        r = dict(extern_c=0, extern_decl=0, shims=0, hex_literals=0, raw_casts=0, macros=0, gotos=0, files=0, lines=0)
        files = glob.glob(os.path.join(mdir, "*.cpp")) + glob.glob(os.path.join(ROOT, "include", "halo", m, "*.hpp"))
        for f in files:
            raw = open(f, encoding="utf-8", errors="replace").read()
            t = strip(raw)
            r["files"] += 1
            r["lines"] += raw.count("\n")
            r["extern_c"] += len(re.findall(r'extern\s+"C"', raw))
            r["extern_decl"] += len(EXTERN_DECL.findall(t))
            r["hex_literals"] += len(HEX.findall(t))
            r["raw_casts"] += len(RAW.findall(t))
            r["macros"] += len(re.findall(r"^\s*#\s*define\b", t, re.M))
            r["gotos"] += len(re.findall(r"\bgoto\b", t))
            if f.endswith("_c_api.cpp") or "_c_api" in os.path.basename(f):
                r["shims"] += len(re.findall(r'^[A-Za-z_][^\n;{}]*\b\w+\s*\([^;{}]*\)\s*\{', t, re.M))
        r["score"] = r["extern_c"] + r["extern_decl"] + r["shims"] + r["hex_literals"] // 4 + r["raw_casts"] // 2 + r["macros"] + r["gotos"]
        rows[m] = r
    return rows


def main():
    rows = census()
    if "--json" in sys.argv:
        print(json.dumps(rows, indent=1))
        return
    cols = ["files", "lines", "extern_c", "extern_decl", "shims", "hex_literals", "raw_casts", "macros", "gotos", "score"]
    print("%-12s" % "module" + "".join("%13s" % c for c in cols))
    tot = dict.fromkeys(cols, 0)
    for m, r in sorted(rows.items(), key=lambda kv: -kv[1]["score"]):
        print("%-12s" % m + "".join("%13d" % r[c] for c in cols))
        for c in cols:
            tot[c] += r[c]
    print("%-12s" % "TOTAL" + "".join("%13d" % tot[c] for c in cols))


if __name__ == "__main__":
    main()
