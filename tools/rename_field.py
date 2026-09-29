"""Rename struct field accesses in src/**/*.c by the clang AST, not by text: only member accesses that resolve to
STRUCT's field OLD change (build/field_uses.json from tools/field_uses.py scan gives their exact line and column), so
a same-named field of another struct, a local, a comment or an #if 0 block is never touched. The header declaration
is edited by hand.

Usage: python tools/rename_field.py STRUCT OLD NEW [--only FILE ...] [--dry]
       python tools/rename_field.py --map MAPFILE [--dry]
NEW may be a member path (engine.race.race_type) when OLD moves into a nested struct or union.
MAPFILE lines: STRUCT OLD NEW [FILE ...]   (FILE restricts the rename to those files; # starts a comment).
All renames of one run are applied from one scan (right to left per line), so run field_uses.py scan first and again
after. A use whose source text at the recorded column is not OLD (a macro expansion) is reported and left alone."""
import json, os, sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE = os.path.join(ROOT, "build", "field_uses.json")


def main():
    argv = sys.argv[1:]
    dry = "--dry" in argv
    argv = [a for a in argv if a != "--dry"]
    rules = []
    if argv and argv[0] == "--map":
        for line in open(argv[1], encoding="utf-8"):
            line = line.split("#", 1)[0].split()
            if line:
                rules.append((line[0], line[1], line[2], set(f.replace("\\", "/") for f in line[3:])))
    else:
        only = set()
        if "--only" in argv:
            i = argv.index("--only")
            only = set(f.replace("\\", "/") for f in argv[i + 1:])
            argv = argv[:i]
        rules.append((argv[0], argv[1], argv[2], only))

    data = json.load(open(CACHE))
    edits = defaultdict(set)  # file -> {(line, col, old, new)}
    for struct, old, new, only in rules:
        hits = 0
        for rel, uses in data.items():
            if only and rel not in only:
                continue
            for u in uses:
                if len(u) < 6:
                    sys.exit("build/field_uses.json has no columns: rerun tools/field_uses.py scan")
                if u[0] == struct and u[1] == old:
                    edits[rel].add((u[3], u[5], old, new))
                    hits += 1
        print("%-28s %-22s -> %-34s %4d uses" % (struct, old, new, hits))

    bad = 0
    for rel, es in sorted(edits.items()):
        path = os.path.join(ROOT, rel)
        lines = open(path, encoding="utf-8", newline="").read().split("\n")
        for line, col, old, new in sorted(es, key=lambda e: (e[0], -e[1])):
            text = lines[line - 1]
            at = col - 1
            if text[at:at + len(old)] != old or (at + len(old) < len(text) and (text[at + len(old)].isalnum() or text[at + len(old)] == "_")):
                print("  SKIP %s:%d:%d (text there: %r)" % (rel, line, col, text[at:at + len(old) + 4]))
                bad += 1
                continue
            lines[line - 1] = text[:at] + new + text[at + len(old):]
        if not dry:
            open(path, "w", encoding="utf-8", newline="").write("\n".join(lines))
    print("%d files, %d edits%s%s" % (len(edits), sum(len(e) for e in edits.values()) - bad,
                                       ", %d skipped" % bad if bad else "", " (dry run)" if dry else ""))


if __name__ == "__main__":
    main()
