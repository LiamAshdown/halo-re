"""Replace one C function definition (and optionally an extern declaration) in a src file, keeping the header comment
and the trailing `#if 0` Ghidra block. Used for step-1 rewrites where the new body comes from the disassembly.
  replace_function.py <src file> <function name> <new definition file> [<extern name> <new extern text file>]
The definition is matched from its signature line (starting at column 0) to the first `}` at column 0 after it.
An extern is matched as one `extern ... name(...);` statement plus the rest of its line."""
import re, sys


def replace(path, name, new_def, extern_name=None, new_extern=None, confidence=None):
    t = open(path, encoding="utf-8").read()
    cut = t.rfind("#if 0")
    body, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
    m = re.search(r"^[A-Za-z_][^\n;{]*\b%s\s*\([^;{]*\)\s*(//[^\n]*)?\n?\{" % re.escape(name), body, re.M)
    assert m, "definition of %s not found in %s" % (name, path)
    end = body.index("\n}", m.end()) + 2
    # drop the comment block that sits directly above the old definition (blam-cc / description lines)
    start = m.start()
    lines = body[:start].split("\n")
    k = len(lines) - 1
    while k > 0 and lines[k - 1].startswith("//"):
        k -= 1
    start = len("\n".join(lines[:k])) + (1 if k else 0)
    body = body[:start] + new_def.rstrip() + "\n" + body[end:]
    if extern_name:
        e = re.search(r"^[ \t]*extern\s[^;]*\b%s\s*\([^;]*\)\s*;[^\n]*\n" % re.escape(extern_name), body, re.M)
        assert e, "extern %s not found in %s" % (extern_name, path)
        body = body[:e.start()] + new_extern.rstrip() + "\n" + body[e.end():]
    if confidence:
        body = re.sub(r"rewrite confidence:? ?[0-9.]+", "rewrite confidence: " + confidence, body, count=1)
    open(path, "w", encoding="utf-8", newline="").write(body + tail)


if __name__ == "__main__":
    a = sys.argv[1:]
    replace(a[0], a[1], open(a[2], encoding="utf-8").read(),
            a[3] if len(a) > 3 else None, open(a[4], encoding="utf-8").read() if len(a) > 4 else None)
