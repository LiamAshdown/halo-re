"""Unify the extern names of engine globals across src/ (reconciliation follow-up: R06, R79 and friends).
For each address in CANONICAL, every src file whose code (above the last '#if 0') declares an extern for that address
under another name gets that name renamed, declaration and uses, to the canonical one. Only the name changes; the
declared type is left as is, since retyping changes pointer arithmetic and is done by hand. A file is skipped (and
listed) when the canonical name already appears in it, so a local or a second declaration is never captured.
Usage: python tools/propagate_globals.py [--apply]   (default is a dry run)"""
import re, glob, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APPLY = "--apply" in sys.argv
CANONICAL = {  # address -> name; owners: scenario.h (bsp globals), game.h (game_time), saved_games.h (local_player_count)
    "746f9c": "global_structure_bsp",
    "746f98": "global_structure_collision_bsp",
    "69e8d8": "global_structure_bsp_index",
    "6f1d6c": "game_time",
    "6894b8": "local_player_count",
}
KEYWORDS = {"void", "int", "char", "short", "long", "float", "double", "unsigned", "signed", "struct", "const", "extern"}
renamed, skipped, files_changed = [], [], 0
for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
    text = open(p, encoding="utf-8", errors="replace").read()
    cut = text.rfind("#if 0")
    body, tail = (text, "") if cut < 0 else (text[:cut], text[cut:])
    new_body = body
    for addr, want in CANONICAL.items():
        decl = re.compile(r"^\s*extern\s+[^;(]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*(\[[^\]]*\])?\s*;[^\n]*?0x0{0,2}" + addr + r"\b", re.M | re.I)
        for m in decl.finditer(new_body):
            old = m.group(1)
            if old == want or old in KEYWORDS: continue
            rel = os.path.relpath(p, ROOT)
            if re.search(r"\b" + re.escape(want) + r"\b", new_body):
                skipped.append((rel, addr, old, want)); continue
            new_body = re.sub(r"(?<!\.)(?<!->)\b" + re.escape(old) + r"\b", want, new_body)  # never a field access
            renamed.append((rel, addr, old, want))
            break  # one extern per address per file
    if new_body != body:
        files_changed += 1
        if APPLY: open(p, "w", encoding="utf-8", newline="").write(new_body + tail)
print(("APPLIED" if APPLY else "DRY RUN"), "renames:", len(renamed), "files:", files_changed, "skipped:", len(skipped))
for s in skipped: print("  skipped", *s)
