"""Names param_N parameters after what every caller passes: when all calls in src/ to a rewritten function (whose
argument count matches the definition) pass the same named value at position k -- a variable, or the last member
of a field path (a->b.name), ignoring casts and & -- that name becomes param_k's. Literals, expressions and
disagreeing callers prove nothing. Renames through tools/rename_in.py (live code only, never onto a used name).
Names never change the compiled code; follow with tools/decl_param_names.py.
  python tools/backward_param_names.py [--dry]"""
import collections, glob, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import decl_param_names as dp
import forward_param_names as fp

GHIDRA = re.compile(r"^(param_\d+|[a-z]{1,3}Var\d+|local_[0-9a-f]+|in_\w+|unaff_\w+|extraout_\w+|DAT_\w+|unused\w*|i|j|k|n|p|a|b|x|y|z|t|result|value|data|ptr|tmp)$")


def arg_name(a):
    a = a.strip()
    a = re.sub(r"^(\((?:const\s+)?[\w\s]+\**\)\s*)+", "", a).strip()     # casts
    a = a.lstrip("&*").strip()
    m = re.fullmatch(r"(?:\w+(?:->|\.))*(\w+)", a)
    if not m or re.fullmatch(r"0x[0-9a-fA-F]+|\d+|[A-Z_0-9]+", m.group(1)):
        return None
    return m.group(1)


def main():
    dry = "--dry" in sys.argv
    defs = dp.definitions()
    wanted = {fn: [k for k, n in enumerate(ps) if n and re.fullmatch(r"param_\d+", n)]
              for fn, ps in defs.items()}
    wanted = {fn: ks for fn, ks in wanted.items() if ks}
    seen = collections.defaultdict(lambda: collections.defaultdict(list))
    for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        code = open(f, encoding="utf-8", errors="replace").read().split("\n#if 0")[0]
        code = re.sub(r"//[^\n]*|/\*.*?\*/", "", code, flags=re.S)
        code = re.sub(r"^[ \t]*extern\b[^;]*;", "", code, flags=re.M | re.S)
        own = os.path.splitext(os.path.basename(f))[0]
        for m in re.finditer(r"\b(\w+)\s*\(", code):
            fn = m.group(1)
            if fn not in wanted or fn == own:
                continue
            args = fp.call_args(code, m.end() - 1)
            if not args or len(args) != len(defs[fn]):
                continue
            for k in wanted[fn]:
                seen[fn][k].append(arg_name(args[k]))
    total = 0
    for fn, ks in sorted(seen.items()):
        renames = {}
        for k, names in ks.items():
            if names and None not in names and len(set(names)) == 1:
                new = names[0]
                if len(new) >= 4 and not GHIDRA.match(new) and "param" not in new and new not in defs[fn] \
                        and new not in renames.values():
                    renames[defs[fn][k]] = new
        if not renames:
            continue
        path = glob.glob(os.path.join(ROOT, "src", "*", fn + ".c"))[0]
        r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "rename_in.py"), path] +
                           ["%s=%s" % kv for kv in renames.items()], capture_output=True, text=True) if not dry else None
        ok = dry or r.returncode == 0
        if ok:
            total += len(renames)
        print("%-58s %s%s" % (os.path.relpath(path, ROOT), " ".join("%s=%s" % kv for kv in renames.items()),
                              "" if ok else "  (refused: name used)"))
    print(total, "parameters named")


if __name__ == "__main__":
    main()
