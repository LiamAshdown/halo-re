"""python tools/api_castfix.py <build-log-file>: for each C2664 'cannot convert argument N from X to T' error, wraps argument N in a C cast (T)(arg)."""
import os, re, sys
from collections import defaultdict

os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
log = open(sys.argv[1], errors="replace").read()
errs = defaultdict(set)
for m in re.finditer(r"^(?:\s*)([A-Za-z]:[^()\n]*?)\((\d+),(\d+)\): error C2664: [^\n]*?cannot convert argument (\d+) from '([^']*)' to '([^']*)'", log, re.M):
    path, line, col, argn, frm, to = m.groups()
    errs[path].add((int(line), int(col), int(argn), to))


def split_args(text, open_idx):
    depth = 0
    i = open_idx
    args = []
    start = open_idx + 1
    in_str = None
    while i < len(text):
        c = text[i]
        if in_str:
            if c == "\\":
                i += 1
            elif c == in_str:
                in_str = None
        elif c in "\"'":
            in_str = c
        elif c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
            if depth == 0:
                args.append((start, i))
                return args
        elif c == "," and depth == 1:
            args.append((start, i))
            start = i + 1
        i += 1
    return None


fixed = 0
for path, items in errs.items():
    s = open(path, encoding="utf-8", errors="replace", newline="").read()
    line_starts = [0]
    for m in re.finditer(r"\n", s):
        line_starts.append(m.end())
    for line, col, argn, to in sorted(items, reverse=True):
        off = line_starts[line - 1] + col - 1
        paren = s.find("(", off)
        # the callee may be preceded by '&' or contain a template; the first '(' after the callee name is the call
        args = split_args(s, paren)
        if not args or argn > len(args):
            print("skip", path, line, col, argn)
            continue
        a, b = args[argn - 1]
        arg = s[a:b]
        lead = len(arg) - len(arg.lstrip())
        trail = len(arg) - len(arg.rstrip())
        core = arg.strip()
        if re.match(r"^\(\s*" + re.escape(to) + r"\s*\)", core):
            continue
        if re.fullmatch(r"[\w.>\-\[\]]+|&?\w+", core):
            new = f"({to}){core}"
        else:
            new = f"({to})({core})"
        s = s[:a] + arg[:lead] + new + (arg[len(arg) - trail:] if trail else "") + s[b:]
        fixed += 1
    open(path, "w", encoding="utf-8", newline="").write(s)
print("casts added:", fixed)
