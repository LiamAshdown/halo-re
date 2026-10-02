"""C++ conversion, phase 1: give every src/*/*.c (except src/gamespy) C linkage when compiled as C++.
Inserts `#ifdef __cplusplus / extern "C" { / #endif` after the file's last #include and the closing brace at the end of
the file, so every function and global keeps its C symbol name (the generated link tables, the code-pointer slots and
the data definitions in standalone/data/*.c all refer to C names). Idempotent; the guards keep the files valid C.
Usage: python tools/cxx_wrap_linkage.py [--check]"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BEGIN = '#ifdef __cplusplus\nextern "C" { /* HALO_CXX_LINKAGE */\n#endif\n'
END = '#ifdef __cplusplus\n} /* HALO_CXX_LINKAGE */\n#endif\n'


def wrap(path):
    raw = open(path, "rb").read()
    text = raw.decode("utf-8", errors="surrogateescape")
    if "HALO_CXX_LINKAGE" in text:
        return "skip"
    nl = "\r\n" if "\r\n" in text else "\n"
    lines = text.split(nl)
    last_inc = -1
    for i, l in enumerate(lines):
        if re.match(r"\s*#\s*include\b", l):
            last_inc = i
    # a #include inside the trailing `#if 0` original-decompile block is not real
    first_if0 = next((i for i, l in enumerate(lines) if l.startswith("#if 0")), None)
    if first_if0 is not None and last_inc > first_if0:
        last_inc = max((i for i, l in enumerate(lines[:first_if0]) if re.match(r"\s*#\s*include\b", l)), default=-1)
    ins = last_inc + 1
    begin = BEGIN.replace("\n", nl)
    end = END.replace("\n", nl)
    new = lines[:ins] + [begin.rstrip(nl)] + lines[ins:]
    out = nl.join(new)
    if not out.endswith(nl):
        out += nl
    out += end
    open(path, "wb").write(out.encode("utf-8", errors="surrogateescape"))
    return "wrapped"


def main():
    files = sorted(glob.glob(os.path.join(ROOT, "src", "*", "*.c")))
    files = [f for f in files if os.sep + "gamespy" + os.sep not in f]
    if "--check" in sys.argv:
        bad = [f for f in files if "HALO_CXX_LINKAGE" not in open(f, encoding="utf-8", errors="replace").read()]
        print("%d files without C linkage wrapper" % len(bad))
        sys.exit(1 if bad else 0)
    n = {"wrapped": 0, "skip": 0}
    for f in files:
        n[wrap(f)] += 1
    print(n)


if __name__ == "__main__":
    main()
