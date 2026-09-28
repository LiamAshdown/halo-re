"""Names param_N parameters of rewritten functions after the parameter they are forwarded to: when a definition's
param_N is passed unchanged (or through a cast) as argument k of a call to another rewritten function, and every
such call agrees on that function's real parameter name, param_N takes that name. Renames only in live code (not
#if 0 blocks, extern declarations, comments or literals) and never onto a name already used in the file.
Names never change the compiled code. Follow with tools/decl_param_names.py for the declarations.
  python tools/forward_param_names.py [--dry]"""
import glob, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import decl_param_names as dp

GHIDRA = re.compile(r"^(param_\d+|in_\w+|unaff_\w+|extraout_\w+|unused\w*)$")


def call_args(text, start):
    """arguments of the call whose '(' is at text[start]"""
    depth, cur, out, i = 0, "", [], start
    while i < len(text):
        ch = text[i]
        if ch == "(":
            depth += 1
            if depth == 1:
                i += 1
                continue
        elif ch == ")":
            depth -= 1
            if depth == 0:
                out.append(cur)
                return out
        elif ch == "," and depth == 1:
            out.append(cur)
            cur = ""
            i += 1
            continue
        cur += ch
        i += 1
    return None


def main():
    dry = "--dry" in sys.argv
    defs = dp.definitions()
    total = 0
    for f in sorted(glob.glob(os.path.join(ROOT, "src", "*", "*.c"))):
        name = os.path.splitext(os.path.basename(f))[0]
        if name not in defs or not any(n and re.fullmatch(r"param_\d+", n) for n in defs[name]):
            continue
        t = open(f, encoding="utf-8").read()
        live = t.split("\n#if 0")[0]
        code = re.sub(r"//[^\n]*|/\*.*?\*/", "", live, flags=re.S)
        code = re.sub(r"^[ \t]*extern\b[^;]*;", "", code, flags=re.M | re.S)
        renames = {}
        for p in [n for n in defs[name] if n and re.fullmatch(r"param_\d+", n)]:
            names = set()
            for m in re.finditer(r"\b(\w+)\s*\(", code):
                callee = m.group(1)
                if callee not in defs or callee == name:
                    continue
                args = call_args(code, m.end() - 1)
                if not args or len(args) != len(defs[callee]):
                    continue            # a call whose shape differs from the definition (check_arity) proves nothing
                for k, a in enumerate(args):
                    a = re.sub(r"^\s*(\([\w\s\*]+\)\s*)*", "", a).strip()
                    if a == p and k < len(defs[callee]):
                        cn = defs[callee][k]
                        names.add(cn)
            real = {n for n in names if n and not GHIDRA.match(n)}
            if len(real) == 1 and len(names) == len(real) and len(next(iter(real))) >= 3:
                new = real.pop()
                if not re.search(r"\b%s\b" % re.escape(new), code) and new not in renames.values():
                    renames[p] = new
        if renames:
            total += len(renames)
            print("%-60s %s" % (os.path.relpath(f, ROOT), " ".join("%s=%s" % kv for kv in renames.items())))
            if not dry:
                subprocess.run([sys.executable, os.path.join(ROOT, "tools", "rename_in.py"), f] +
                               ["%s=%s" % kv for kv in renames.items()], check=True, capture_output=True)
    print(total, "parameters named")


if __name__ == "__main__":
    main()
