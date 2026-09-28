"""Gives param_N names in extern function declarations the definition's parameter names. A declaration's
parameter names do not affect the compiled code; the definition (src/<module>/<function>.c) is the authority.
Only a param_N whose definition parameter at the same position has a real name (not param_N) is renamed, and only
when the declaration and definition have the same number of parameters.
  python tools/decl_param_names.py [--dry]"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GHIDRA = re.compile(r"^(param_\d+|in_\w+|unaff_\w+|extraout_\w+)$")


def split_params(s):
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch == "," and depth == 0:
            out.append(cur)
            cur = ""
            continue
        depth += ch == "("
        depth -= ch == ")"
        cur += ch
    if cur.strip():
        out.append(cur)
    return out


def param_name(p):
    p = re.sub(r"\[[^\]]*\]", "", p).strip()
    m = re.search(r"\(\s*\*\s*(\w+)\s*\)\s*\(", p)          # function pointer parameter
    if m:
        return m.group(1)
    m = re.search(r"(\w+)\s*$", p)
    return m.group(1) if m and len(p.split()) > 1 or (m and "*" in p) else None


def definitions():
    out = {}
    for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        name = os.path.splitext(os.path.basename(f))[0]
        t = open(f, encoding="utf-8", errors="replace").read().split("\n#if 0")[0]
        m = re.search(r"^[^\n;{}#/]*?\b%s\s*\(([^)]*(?:\([^)]*\)[^)]*)*)\)\s*(?://[^\n]*)?\n?\{" % re.escape(name), t, re.M)
        if m:
            ps = split_params(m.group(1))
            if ps and ps[0].strip() != "void":
                out[name] = [param_name(p) for p in ps]
    return out


def main():
    dry = "--dry" in sys.argv
    defs = definitions()
    decl = re.compile(r"(^extern\s+[^;{]*?\b(\w+)\s*\()([^;{]*?)(\)\s*;)", re.M | re.S)
    renamed, files = 0, 0
    for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(f, encoding="utf-8").read()
        i = t.find("\n#if 0")
        live, rest = (t, "") if i < 0 else (t[:i], t[i:])

        def rep(m):
            nonlocal renamed
            fn, params = m.group(2), m.group(3)
            if fn not in defs or "param_" not in params:
                return m.group(0)
            ps = split_params(params)
            dn = defs[fn]
            if len(ps) != len(dn):
                return m.group(0)
            used = {param_name(p) for p in ps}
            new = []
            for p, d in zip(ps, dn):
                n = param_name(p)
                if n and GHIDRA.match(n) and d and not GHIDRA.match(d) and d not in used:
                    p = re.sub(r"\b%s\b" % re.escape(n), d, p)
                    used.add(d)
                    renamed += 1
                new.append(p)
            return m.group(1) + ",".join(new) + m.group(4)
        new_live = decl.sub(rep, live)
        if new_live != live:
            files += 1
            if not dry:
                open(f, "w", encoding="utf-8", newline="\n").write(new_live + rest)
    print("%d parameter names in %d files (%d definitions indexed)" % (renamed, files, len(defs)))


if __name__ == "__main__":
    main()
