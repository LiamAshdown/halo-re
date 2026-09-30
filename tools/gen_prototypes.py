"""Moves per-file `extern` function declarations into generated prototype headers, so callers `#include` the real
declaration instead of each carrying its own copy.

  python tools/gen_prototypes.py <module> [<module> ...] [--dry]

For each function DEFINED in src/<module>/*.c that is also declared `extern` in other files:
  * if every extern declaration of that name, across all of src/, is identical (whitespace-normalised), it is
    "eligible": its declaration (with the address/register comment of the first occurrence) goes into
    types/fn_<module>.h, every extern statement for it is removed from the .c files (outside #if 0 blocks), and each
    file that used one gets `#include "fn_<module>.h"` (the defining file includes it too, so the compiler checks the
    definition against the prototype);
  * names whose extern declarations disagree with each other, or whose callers' declaration differs from the definition's signature (parameter names ignored), are left alone (they are Ghidra operand-count losses etc. and are the
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


BUILTIN = {"int", "char", "short", "long", "float", "double", "void", "unsigned", "signed", "const", "volatile"}


def sig_key(decl):
    """(return-type, [parameter types]) of a declaration with parameter names removed, for comparing signatures."""
    d = norm(decl)
    m = re.match(r'^(.*?\b\w+)\s*\((.*)\)$', d)
    if not m or '(' in m.group(2):
        return None                      # function-pointer parameters etc.: not comparable
    head, params = m.group(1), m.group(2)
    ret = re.sub(r'\s*\b\w+$', '', head).strip()
    out = []
    for p in [x.strip() for x in params.split(',')] if params.strip() else []:
        toks = re.findall(r'\w+|\*|\[[^\]]*\]', p)
        if len(toks) > 1 and re.fullmatch(r'\w+', toks[-1]) and toks[-1] not in BUILTIN and toks[-2] != 'struct':
            toks = toks[:-1]
        out.append(" ".join(toks))
    if out == ["void"]:
        out = []
    return ret.replace(" *", "*"), out


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


_order_cache = []


def header_rank():
    """Header -> position in a global include order: a topological sort of 'X is included before Y' by majority vote over
    every src/*.c file's #include list (the headers have no include-order guards of their own)."""
    if _order_cache:
        return _order_cache[0]
    before = collections.Counter()
    hs = set()
    for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        incs = [h for h in re.findall(r'^#include\s+"([^"]+\.h)"', open(f, encoding="utf-8", errors="replace").read(), re.M)
                if not h.startswith("fn_")]
        for i, a in enumerate(incs):
            hs.add(a)
            for b in incs[i + 1:]:
                if a != b:
                    before[(a, b)] += 1
    succ = {h: set() for h in hs}
    indeg = {h: 0 for h in hs}
    for (a, b), n in before.items():
        if n > before.get((b, a), 0) and b not in succ[a]:
            succ[a].add(b)
            indeg[b] += 1
    order, ready = [], sorted(h for h in hs if indeg[h] == 0)
    while ready:
        h = ready.pop(0)
        order.append(h)
        for s in sorted(succ[h]):
            indeg[s] -= 1
            if indeg[s] == 0:
                ready.append(s)
        ready.sort()
    for h in sorted(hs):                       # cycles: leave in alphabetical order at the end
        if h not in order:
            order.append(h)
    _order_cache.append({h: i for i, h in enumerate(order)})
    return _order_cache[0]


def sort_headers(headers):
    rank = header_rank()
    base = [h for h in TYPE_HEADERS if h in headers]          # the prelude always comes first, in this order
    rest = sorted((h for h in headers if h not in TYPE_HEADERS), key=lambda h: (rank.get(h, 10 ** 6), h))
    return base + rest


def complete_includes(body, mod, headers):
    """Adds the type headers the prototypes need, found by compiling the header alone with host gcc. A type used inside
    another header (e.g. input.h using ui_input_event from interface.h) also constrains the include order: the
    defining header must come before the header that uses it."""
    flags = ["-m32", "-D__stdcall=", "-D__cdecl=", "-D__fastcall=", "-fsyntax-only", "-std=gnu99", "-I", os.path.join(ROOT, "types")]
    must = collections.defaultdict(set)          # header -> headers that must precede it

    def enforce(hs):
        hs = sort_headers(hs)
        for _ in range(len(hs) * len(hs) + 1):
            moved = False
            for user, deps in must.items():
                for d in sorted(deps):
                    if user in hs and d in hs and hs.index(d) > hs.index(user):
                        hs.remove(d)
                        hs.insert(hs.index(user), d)
                        moved = True
            if not moved:
                break
        return hs

    def render(hs):
        return re.sub(r'((?:#include "[^"]+"\n)+)', "".join('#include "%s"\n' % h for h in hs), body, count=1)

    for _ in range(20):
        with tempfile.TemporaryDirectory() as tmp:
            hp = os.path.join(tmp, "fn_%s.h" % mod)
            open(hp, "w").write(body)
            r = subprocess.run(["gcc"] + flags + ["-x", "c", hp], capture_output=True, text=True,
                               env=dict(os.environ, LC_ALL="C", LANG="C"))
        missing = set()
        new = []
        for m in re.finditer(r"([^\s:]+):\d+:\d+: error: unknown type name '(\w+)'", r.stderr):
            user, tp = os.path.basename(m.group(1)), m.group(2)
            missing.add(tp)
            h = defining_header(tp)
            if h and h != user:
                if user.endswith(".h") and not user.startswith("fn_"):
                    must[user].add(h)
                if h not in headers and h not in new:
                    new.append(h)
        order_changed = False
        if must:
            fixed = enforce(headers + new)
            order_changed = fixed != headers
        else:
            fixed = headers + new
        if not new and not order_changed:
            return body, headers, {tp for tp in missing if not defining_header(tp)}
        headers = fixed
        body = render(headers)
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
    defs, defsig = {}, {}
    for f, t in texts.items():
        for live, chunk in live_parts(t):
            if live:
                for m in re.finditer(r'^([A-Za-z_][^;{}=\n]*?\b(\w+)\s*\(([^;{}]*)\))\s*\n\{', chunk, re.M):
                    defs.setdefault(m.group(2), f)
                    defsig.setdefault(m.group(2), m.group(1))
    for mod in args:
        src = os.path.join(ROOT, "src", mod) + os.sep
        cand = sorted(n for n, f in defs.items() if f.startswith(src) and n in variants)
        names, skipped = [], []
        for n in cand:
            ok = len(variants[n]) == 1
            if ok:
                (d,) = variants[n].keys()
                a, b = sig_key(d), sig_key(defsig[n])
                ok = a is not None and a == b        # callers must agree with the definition too
            (names if ok else skipped).append(n)
        header = "fn_%s.h" % mod
        while True:
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
            # a type that no header defines (a local typedef in some .c file) cannot appear in a shared prototype:
            # those names keep their local externs
            bad = [n for n in names if any(re.search(r'\b%s\b' % re.escape(tp), list(variants[n])[0]) for tp in still_missing)]
            if not bad:
                break
            names = [n for n in names if n not in bad]
            skipped += bad
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
