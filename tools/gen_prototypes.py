"""Moves per-file `extern` function declarations into generated prototype headers, so callers `#include` the real
declaration instead of each carrying its own copy.

  python tools/gen_prototypes.py <module> [<module> ...] [--dry]

For each function DEFINED in src/<module>/*.c that is also declared `extern` in other files:
  * if every extern declaration of that name, across all of src/, is identical (whitespace-normalised), it is
    "eligible": its declaration (with the address/register comment of the first occurrence) goes into
    types/fn_<module>.h, every extern statement for it is removed from the .c files (outside #if 0 blocks), and each
    file that used one gets `#include "fn_<module>.h"` (the defining file includes it too, so the compiler checks the
    definition against the prototype);
  * names whose extern declarations disagree are left alone (they are Ghidra operand-count losses etc. and are the
    interesting ones to fix by hand).
The generated header is self-contained: it includes the type headers its prototypes need (found by name).
Check with `python tools/host_gate.py --baseline HEAD` (new host-gcc failures = something the header broke)."""
import collections, glob, os, re, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DECL = re.compile(r'^extern\b([^;{}=]*?\([^;{}]*?\))\s*;([ \t]*//[^\n]*)?', re.M | re.S)
IF0 = re.compile(r'(\n#if 0\b.*?\n#endif[^\n]*)', re.S)
TYPE_HEADERS = ["tags.h", "memory.h", "math.h"]


def fn_name(decl):
    m = re.match(r'.*?(\w+)\s*\(', decl, re.S)
    if not m or re.match(r'.*\(\s*\*', decl.split(')')[0] + ')'):
        return None
    return m.group(1)


def norm(decl):
    return re.sub(r'\s+', ' ', decl).strip()


def live_parts(text):
    """(is_live, chunk) pieces; chunks inside #if 0 .. #endif are not live."""
    parts = IF0.split(text)
    return [(i % 2 == 0, p) for i, p in enumerate(parts)]


def defining_header(name):
    """The types/*.h header that defines `name` as a typedef/struct/enum tag, or None."""
    pat = re.compile(r'\b(?:\}\s*%s\s*;|typedef\s+[^;{}]*\b%s\s*;|struct\s+%s\s*\{|enum\s+%s\s*\{)' % ((re.escape(name),) * 4))
    for h in sorted(glob.glob(os.path.join(ROOT, "types", "*.h"))):
        if pat.search(open(h, encoding="utf-8", errors="replace").read()):
            return os.path.basename(h)
    return None


def complete_includes(body, mod, headers):
    """Adds the type headers the prototypes need, found by compiling the header alone with host gcc."""
    flags = ["-m32", "-D__stdcall=", "-D__cdecl=", "-D__fastcall=", "-fsyntax-only", "-std=gnu99", "-I", os.path.join(ROOT, "types")]
    for _ in range(10):
        with tempfile.TemporaryDirectory() as tmp:
            hp = os.path.join(tmp, "fn_%s.h" % mod)
            open(hp, "w").write(body)
            r = subprocess.run(["gcc"] + flags + ["-x", "c", hp], capture_output=True, text=True)
        missing = set(re.findall(r"unknown type name '(\w+)'", r.stderr))
        new = []
        for n in sorted(missing):
            h = defining_header(n)
            if h and h not in headers and h not in new:
                new.append(h)
        if not new:
            return body, headers, missing
        headers = headers + new
        body = re.sub(r'((?:#include "[^"]+"\n)+)', "".join('#include "%s"\n' % h for h in headers), body, count=1)
    return body, headers, missing


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    dry = "--dry" in sys.argv
    if not args:
        raise SystemExit(__doc__)
    files = sorted(glob.glob(os.path.join(ROOT, "src", "*", "*.c")))
    texts = {f: open(f, encoding="utf-8", errors="replace").read() for f in files}
    variants = collections.defaultdict(lambda: collections.defaultdict(list))
    first_comment = {}
    for f, t in texts.items():
        for live, chunk in live_parts(t):
            if not live:
                continue
            for m in DECL.finditer(chunk):
                n = fn_name(m.group(1))
                if n:
                    variants[n][norm(m.group(1))].append(f)
                    first_comment.setdefault((n, norm(m.group(1))), (m.group(0), (m.group(2) or "").strip()))
    defs = {}
    for f, t in texts.items():
        for live, chunk in live_parts(t):
            if live:
                for m in re.finditer(r'^[A-Za-z_][^;{}=\n]*?\b(\w+)\s*\(([^;{}]*)\)\s*\n\{', chunk, re.M):
                    defs.setdefault(m.group(1), f)
    for mod in args:
        src = os.path.join(ROOT, "src", mod) + os.sep
        names = sorted(n for n, f in defs.items() if f.startswith(src) and n in variants and len(variants[n]) == 1)
        skipped = sorted(n for n, f in defs.items() if f.startswith(src) and n in variants and len(variants[n]) > 1)
        header = "fn_%s.h" % mod
        lines = []
        for n in names:
            (d,) = variants[n].keys()
            _, cm = first_comment[(n, d)]
            lines.append("extern %s;%s" % (d, ("  " + cm) if cm else ""))
        body = ("// Generated by tools/gen_prototypes.py from the extern declarations that used to be copied into every\n"
                "// caller: prototypes of the functions defined in src/%s/. Do not edit by hand; fix a signature in the\n"
                "// definition and regenerate (names whose callers disagreed are NOT here).\n"
                "#ifndef FN_%s_H\n#define FN_%s_H\n\n%s\n\n%s\n\n#endif\n" %
                (mod, mod.upper(), mod.upper(), "\n".join('#include "%s"' % h for h in TYPE_HEADERS), "\n".join(lines)))
        headers = list(TYPE_HEADERS)
        if os.path.exists(os.path.join(ROOT, "types", mod + ".h")) and mod + ".h" not in headers:
            headers.append(mod + ".h")
            body = re.sub(r'((?:#include "[^"]+"\n)+)', "".join('#include "%s"\n' % h for h in headers), body, count=1)
        body, headers, still_missing = complete_includes(body, mod, headers)
        if still_missing:
            print("  warning: types not found in any header (header will not compile alone): %s" % ", ".join(sorted(still_missing)))
        changed = 0
        touched = set()
        for n in names:
            (d,) = variants[n].keys()
            touched |= set(variants[n][d]) | {defs[n]}
        nameset = set(names)
        for f in sorted(touched):
            out = []
            for live, chunk in live_parts(texts[f]):
                if live:
                    chunk = DECL.sub(lambda m: "" if fn_name(m.group(1)) in nameset else m.group(0), chunk)
                out.append(chunk if live else chunk)
            new = "".join(out)
            if ('#include "%s"' % header) not in new:
                incs = list(re.finditer(r'^#include\s+"[^"]+"[^\n]*\n', new, re.M))
                pos = incs[-1].end() if incs else 0
                new = new[:pos] + '#include "%s"\n' % header + new[pos:]
            new = re.sub(r'\n{4,}', '\n\n\n', new)
            if new != texts[f]:
                changed += 1
                texts[f] = new
                if not dry:
                    open(f, "w", encoding="utf-8", newline="").write(new)
        if not dry:
            open(os.path.join(ROOT, "types", header), "w", encoding="utf-8", newline="").write(body)
        print("%s: %d prototypes -> types/%s, %d files edited, %d names skipped (callers disagree): %s" %
              (mod, len(names), header, changed, len(skipped), ", ".join(skipped[:6])))


if __name__ == "__main__":
    main()
