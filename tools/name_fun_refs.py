"""Replace raw Ghidra names FUN_00xxxxxx in live code (above the final #if 0, outside // comments) with the name
of the rewrite that already exists for that address (src/*/<name>.c with "address 0x..."), so C-to-C calls reach
the rewrite instead of a push/ret stub into original code (which makes the caller unhookable). Comments are left
as they are. A caller whose declaration then disagrees with the definition is flagged by
tools/check_prototypes.py and stays unhookable; files that no longer compile should be reverted (--revert-broken
after a build).  Usage: python tools/name_fun_refs.py [--apply]"""
import os, re, sys, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def code_comment_split(line):
    """(code, comment) of one line, ignoring // inside string or char literals"""
    i, q = 0, None
    while i < len(line):
        c = line[i]
        if q:
            if c == "\\": i += 2; continue
            if c == q: q = None
        elif c in "\"'": q = c
        elif line.startswith("//", i): return line[:i], line[i:]
        i += 1
    return line, ""

def main():
    apply = "--apply" in sys.argv
    addr2name = {}
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        h = re.search(r"address 0x0*([0-9a-f]{6}),\s*size", open(p, encoding="utf-8", errors="replace").read(3000))
        if h: addr2name[int(h.group(1), 16)] = os.path.basename(p)[:-2]
    changed = []
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        own = os.path.basename(p)[:-2]
        t = open(p, encoding="utf-8", errors="replace").read()
        cut = t.rfind("#if 0"); b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        out, n = [], 0
        in_block = False
        for line in b.split("\n"):
            if in_block or line.lstrip().startswith("/*"):
                in_block = "*/" not in line
                out.append(line); continue
            code, comment = code_comment_split(line)
            def rep(m):
                nonlocal n
                name = addr2name.get(int(m.group(1), 16))
                if not name or name == own: return m.group(0)
                n += 1; return name
            code = re.sub(r"\bFUN_00([0-9a-f]{6})\b", rep, code)
            out.append(code + comment)
        if n:
            changed.append(p)
            if apply: open(p, "w", encoding="utf-8", newline="").write("\n".join(out) + tail)
    print(("renamed in" if apply else "would rename in"), len(changed), "files")
    if apply: open(os.path.join(ROOT, "build", "name_fun_changed.txt"), "w").write("\n".join(changed))

if __name__ == "__main__": main()
