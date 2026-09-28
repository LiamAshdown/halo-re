"""Gives every fixed-address global one name. standalone/globals.asm maps each C name to its address; an address
with several names means several files called the same variable different things. For each such address the most
used real name (not DAT_/unknown_/external_/g_ placeholders) becomes the name everywhere, by renaming only: each file
keeps its own declared type, so the compiled code is unchanged (tools/objdiff.py compare <module> --renames
build/alias_renames.txt proves it). A file is skipped for an alias when
  - the chosen name already means something else there (used without an extern declaration: a local, a parameter),
  - or it would end up declaring the chosen name twice with different types.
An alias's globals.asm entry is dropped only when no file uses it any more.
  python tools/unify_aliases.py [--dry]      writes build/alias_renames.txt (old=new per line)"""
import collections, glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GLOBALS = os.path.join(ROOT, "standalone", "globals.asm")
PLACEHOLDER = re.compile(r"^(DAT_|PTR_|unknown_|external_|g_[0-9a-f]{6}|global_[0-9a-f]{6,8}$)|_[0-9a-f]{6,8}$")


def live_split(t):
    i = t.find("\n#if 0")
    return (t, "") if i < 0 else (t[:i], t[i:])


def decl_re(name):
    return re.compile(r"^extern\s+[^;(]*?\b%s\s*(?:\[[^\]]*\])?\s*;[^\n]*\n" % re.escape(name), re.M)


def rename_outside_literals(text, old, new):
    """rename the identifier old to new, but never inside a string or character literal (command-line flags such as
    "-nosound" share names with the globals they set) and never as a field (->old, .old)"""
    rx = re.compile(r'"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'|(?<![\w.>])%s\b' % re.escape(old))
    return rx.sub(lambda m: m.group(0) if m.group(0)[0] in "\"'" else new, text)


def main():
    dry = "--dry" in sys.argv
    g = open(GLOBALS, encoding="utf-8").read()
    by_addr = collections.defaultdict(list)
    for m in re.finditer(r"^_(\w+) EQU (0[0-9A-F]+h)$", g, re.M):
        by_addr[m.group(2)].append(m.group(1))
    files = {f: open(f, encoding="utf-8").read() for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c"))}
    uses = collections.Counter()
    for t in files.values():
        live, _ = live_split(t)
        for name in set(re.findall(r"^extern\s+[^;(]*?\b(\w+)\s*(?:\[[^\]]*\])?\s*;", live, re.M)):
            uses[name] += 1
    renames, skipped, dropped = [], [], []
    changed = {}
    for addr, names in sorted(by_addr.items()):
        if len(names) < 2:
            continue
        real = [n for n in names if not PLACEHOLDER.search(n)] or names
        canon = max(real, key=lambda n: (uses[n], -len(n)))
        for alias in names:
            if alias == canon:
                continue
            left = False
            for f in files:
                t = changed.get(f, files[f])
                live, rest = live_split(t)
                if not decl_re(alias).search(live):
                    continue                        # only files where the alias is the extern global
                has_canon_decl = decl_re(canon).search(live) is not None
                code = re.sub(r"//[^\n]*|/\*.*?\*/", "", live, flags=re.S)
                other = [m for m in re.finditer(r"(?<![\w.>])%s\b" % re.escape(canon), code)]
                if other and not has_canon_decl:
                    skipped.append((os.path.relpath(f, ROOT), alias, canon, "name already used locally"))
                    left = True
                    continue
                new = rename_outside_literals(live, alias, canon)
                decls = decl_re(canon).findall(new)
                if len(decls) > 1:
                    norm = {re.sub(r"\s+", " ", re.sub(r"//.*", "", d)).strip() for d in decls}
                    if len(norm) > 1:
                        skipped.append((os.path.relpath(f, ROOT), alias, canon, "two declarations, different types"))
                        left = True
                        continue
                    first = new.index(decls[0]) + len(decls[0])
                    new = new[:first] + new[first:].replace(decls[0], "", len(decls) - 1)
                    for d in decls[1:]:
                        new = new[:first] + new[first:].replace(d, "", 1)
                changed[f] = new + rest
            renames.append((alias, canon))
            if not left:
                dropped.append((alias, addr))
    os.makedirs(os.path.join(ROOT, "build"), exist_ok=True)
    open(os.path.join(ROOT, "build", "alias_renames.txt"), "w").write(
        "".join("%s=%s\n" % r for r in renames))
    print("%d aliases -> canonical names, %d files change, %d aliases fully removed, %d file skips"
          % (len(renames), sum(1 for f in changed if changed[f] != files[f]), len(dropped), len(skipped)))
    for s in skipped[:15]:
        print("  skip %s: %s -> %s (%s)" % s)
    if dry:
        return
    for f, t in changed.items():
        if t != files[f]:
            open(f, "w", encoding="utf-8", newline="\n").write(t)
    for alias, addr in dropped:
        g = g.replace("PUBLIC _%s\n_%s EQU %s\n" % (alias, alias, addr), "")
    open(GLOBALS, "w", encoding="utf-8", newline="\n").write(g)
    mods = sorted({os.path.relpath(f, ROOT).split(os.sep)[1] for f in changed if changed[f] != files[f]})
    print("modules:", " ".join(mods))


if __name__ == "__main__":
    main()
