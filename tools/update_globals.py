"""Maintenance tool: keeps standalone/globals.asm, the committed list of the engine globals the C uses at their fixed
original addresses, up to date. Not part of the build.

The standalone exe keeps the game's data at its original addresses (the loader copies the data image there), so a
global such as `extern data_array *player_data;` is the absolute symbol `_player_data EQU 087A480h`. Those symbols are
committed source in standalone/globals.asm, which tools/gen_standalone_link.py assembles and links like any object.

When new C references a global that is not in the file yet, the link still resolves it from the declaration's address
comment (`// 0x0087a480`) and prints how many it had to; this tool then adds them:
  python tools/update_globals.py          merge the globals the last link resolved (build/standalone/resolve.asm)
  python tools/update_globals.py --check  compare every address comment on a data declaration in src/ with the file
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GLOBALS = os.path.join(ROOT, "standalone", "globals.asm")
RESOLVE = os.path.join(ROOT, "build", "standalone", "resolve.asm")
EQU = re.compile(r"^(\S+) EQU 0*([0-9A-Fa-f]+)h\s*$", re.M)

HEADER = """; standalone/globals.asm -- the engine globals the C uses at their fixed original addresses.
;
; The standalone exe keeps the game's data where the original executable had it: at start-up its loader copies the
; data image (standalone/image/*.asm) back to 0x63a000.., so every global the C declares, e.g.
;     extern data_array *player_data; // 0x0087a480
; is the absolute symbol below. Standard C cannot give a variable a fixed address, so they live here; the link
; (tools/gen_standalone_link.py) assembles this file like any other source. New globals are added by
; tools/update_globals.py (it merges what the link had to resolve from address comments); --check compares the
; address comments in src/ with this file. Sorted by symbol.

.386
.model flat
option casemap:none
"""


def read_equ(path):
    if not os.path.exists(path):
        return {}
    return {m.group(1): int(m.group(2), 16) for m in EQU.finditer(open(path, encoding="utf-8").read())}


def write(globals_):
    lines = [HEADER]
    for name in sorted(globals_, key=str.lower):
        lines.append("PUBLIC %s\n%s EQU 0%Xh" % (name, name, globals_[name]))
    lines.append("\nEND\n")
    with open(GLOBALS, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


def check():
    sys.path.insert(0, os.path.join(ROOT, "harness"))
    import gen_link as gl
    addr, kind = gl.extern_map()
    have = read_equ(GLOBALS)
    bad = 0
    for sym, a in sorted(have.items()):
        n = sym[1:]
        if n in addr and addr[n]:
            seen = sorted(addr[n])
            if a not in seen:
                bad += 1
                print("%s: globals.asm 0x%x, address comments %s" % (sym, a, ", ".join("0x%x" % x for x in seen)))
    print("%d globals, %d disagree with an address comment in src/" % (len(have), bad))
    return bad


def main():
    if "--check" in sys.argv:
        sys.exit(1 if check() else 0)
    have = read_equ(GLOBALS)
    new = read_equ(RESOLVE)
    added = {k: v for k, v in new.items() if k not in have}
    changed = {k: (have[k], v) for k, v in new.items() if k in have and have[k] != v}
    for k, (old, a) in sorted(changed.items()):
        print("CONFLICT %s: globals.asm 0x%x, this link 0x%x (left as is; fix the C or the file)" % (k, old, a))
    have.update(added)
    write(have)
    print("%d globals in %s (%d added)" % (len(have), os.path.relpath(GLOBALS, ROOT), len(added)))


if __name__ == "__main__":
    main()
