"""Rename struct field accesses in src/**/*.c by the clang AST, not by text: only member accesses that resolve to
STRUCT's field OLD change (build/field_uses.json from tools/field_uses.py scan gives their exact line and column), so
a same-named field of another struct, a local, a comment or an #if 0 block is never touched.

Usage: python tools/rename_field.py STRUCT OLD NEW [--only FILE ...] [--dry]
       python tools/rename_field.py --map MAPFILE [--dry]
NEW may be a member path (engine.race.race_type) when OLD moves into a nested struct or union.
MAPFILE lines: STRUCT OLD NEW [FILE ...] [:: TEXT]   (FILE restricts the rename to those files; # starts a comment;
               TEXT replaces the declaration's comment after its offset).
The declaration in types/*.h is renamed too (plain NEW only, not member paths), unless --no-header.
All renames of one run are applied from one scan (right to left per line), so run field_uses.py scan first and again
after. A use whose source text at the recorded column is not OLD (a macro expansion) is reported and left alone."""
import glob, json, os, re, sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE = os.path.join(ROOT, "build", "field_uses.json")


def main():
    argv = sys.argv[1:]
    dry = "--dry" in argv
    header = "--no-header" not in argv
    argv = [a for a in argv if a not in ("--dry", "--no-header")]
    notes = {}
    rules = []
    if argv and argv[0] == "--map":
        for line in open(argv[1], encoding="utf-8"):
            line, _, note = line.split("#", 1)[0].partition("::")
            line = line.split()
            if line:
                rules.append((line[0], line[1], line[2], set(f.replace("\\", "/") for f in line[3:])))
                if note.strip():
                    notes[(line[0], line[1])] = note.strip()
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
    if header:
        rename_declarations([(s_, o, n) for s_, o, n, _ in rules if "." not in n], notes, dry)


def wrap(line, width=118):
    """Wrap a declaration's // comment at width, continuation lines aligned under the comment text."""
    code, sep, com = line.partition("//")
    if len(line) <= width:
        return line
    words, out, cur = com.split(), [], ""
    indent = " " * len(code) + "//" + "   "
    first = code + "//"
    for w in words:
        head = first if not out else indent
        if cur and len(head) + 1 + len(cur) + 1 + len(w) > width:
            out.append((first if not out else indent) + " " + cur)
            cur = w
        else:
            cur = (cur + " " + w) if cur else w
    if cur:
        out.append((first if not out else indent) + " " + cur)
    return "\n".join(out)


def rename_declarations(rules, notes, dry):
    """Rename OLD's declarator inside STRUCT's body in types/*.h (and replace its comment when a note is given)."""
    done = set()
    for path in sorted(glob.glob(os.path.join(ROOT, "types", "*.h"))):
        text = open(path, encoding="utf-8", newline="").read()
        changed = False
        for struct, old, new in rules:
            if (struct, old) in done:
                continue
            m = re.search(r"typedef\s+(?:struct|union)\s+%s\s*\{" % re.escape(struct), text) or \
                re.search(r"^(?:struct|union)\s+%s\s*\{" % re.escape(struct), text, re.M)
            if not m:
                continue
            end = re.compile(r"^\}\s*%s\s*;" % re.escape(struct), re.M).search(text, m.end()) or \
                re.compile(r"^\};", re.M).search(text, m.end())
            body = text[m.end():end.start()]
            lines = body.split("\n")
            for i, l in enumerate(lines):
                code, sep, com = l.partition("//")
                if re.search(r"\b%s\b\s*(\[[^\]]*\])*\s*;" % re.escape(old), code):
                    code2 = re.sub(r"\b%s\b" % re.escape(old), new, code, count=1)
                    # keep the comment column: absorb the length change in the padding before //
                    if sep:
                        pad = len(code) - len(code.rstrip())
                        code2 = code2.rstrip() + " " * max(1, pad - (len(new) - len(old)))
                    note = notes.get((struct, old)) if sep else None
                    if sep:
                        if note:
                            off = re.match(r"\s*(0x[0-9a-fA-F]+)", com)
                            com = " " + (off.group(1) + " " if off else "") + note
                    lines[i] = wrap(code2 + sep + com) if note else code2 + sep + com
                    done.add((struct, old))
                    changed = True
                    break
            text = text[:m.end()] + "\n".join(lines) + text[end.start():]
        if changed and not dry:
            open(path, "w", encoding="utf-8", newline="").write(text)
    for struct, old, new in rules:
        if (struct, old) not in done:
            print("  header: no declaration of %s.%s found" % (struct, old))


if __name__ == "__main__":
    main()
