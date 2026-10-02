"""C++ conversion, phase 1: give every src/*/*.c (except src/gamespy) C linkage when compiled as C++.
Inserts `#ifdef __cplusplus / extern "C" { / #endif` before the file's first line of real code (after the leading
comments and #include/#define/#pragma/conditional lines; a later #include stays inside the block) and the closing brace
at the end of the file, so every function, global and extern declaration keeps its C symbol name (the generated link
tables, the code-pointer slots and the data definitions in standalone/data/*.c all refer to C names).
Re-running moves an existing wrapper to the right place, so it is idempotent. The guards keep the files valid C.
Usage: python tools/cxx_wrap_linkage.py [--check]"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TAG = "HALO_CXX_LINKAGE"
BEGIN = '#ifdef __cplusplus\nextern "C" { /* ' + TAG + ' */\n#endif'
END = '#ifdef __cplusplus\n} /* ' + TAG + ' */\n#endif\n'
OLD_BEGIN = re.compile(r'#ifdef __cplusplus\r?\nextern "C" \{ /\* ' + TAG + r' \*/\r?\n#endif\r?\n')
OLD_END = re.compile(r'#ifdef __cplusplus\r?\n\} /\* ' + TAG + r' \*/\r?\n#endif\r?\n?')
PRE = re.compile(r"\s*(#\s*(include|define|undef|pragma|if|ifdef|ifndef|else|elif|endif|error)\b.*|//.*|)$")


def first_code_line(lines):
    """index of the first line of real code at preprocessor depth 0: not blank, not a comment, not a preprocessor line,
    and not inside an #if block (an all-dead `#if 0` file gets its wrapper at the end, outside the block)"""
    in_comment = False
    cont = False
    depth = 0
    for i, line in enumerate(lines):
        s = line.strip()
        if in_comment:
            if "*/" in s:
                in_comment = False
            continue
        if cont:                      # continuation of a multi-line #define
            cont = s.endswith("\\")
            continue
        if s.startswith("/*"):
            if "*/" not in s:
                in_comment = True
            continue
        if s.startswith("#"):
            word = re.match(r"#\s*(\w+)", s)
            word = word.group(1) if word else ""
            if word in ("if", "ifdef", "ifndef"):
                depth += 1
            elif word == "endif":
                depth -= 1
            cont = s.endswith("\\")
            continue
        if depth > 0 or s == "" or s.startswith("//"):
            continue
        return i
    return len(lines)


def wrap(path):
    raw = open(path, "rb").read()
    text = raw.decode("utf-8", errors="surrogateescape")
    nl = "\r\n" if "\r\n" in text else "\n"
    text = OLD_BEGIN.sub("", text)
    text = OLD_END.sub("", text)
    lines = text.split(nl)
    ins = first_code_line(lines)
    out = nl.join(lines[:ins] + BEGIN.split("\n") + lines[ins:])
    if not out.endswith(nl):
        out += nl
    out += END.replace("\n", nl)
    open(path, "wb").write(out.encode("utf-8", errors="surrogateescape"))


def sources():
    files = sorted(glob.glob(os.path.join(ROOT, "src", "*", "*.c")))
    return [f for f in files if os.sep + "gamespy" + os.sep not in f]


def main():
    files = sources()
    if "--check" in sys.argv:
        bad = [f for f in files if TAG not in open(f, encoding="utf-8", errors="replace").read()]
        print("%d files without C linkage wrapper" % len(bad))
        sys.exit(1 if bad else 0)
    for f in files:
        wrap(f)
    print("wrapped %d files" % len(files))


if __name__ == "__main__":
    main()
