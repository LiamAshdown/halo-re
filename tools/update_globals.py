"""Maintenance tool: lists the engine globals a link left unresolved, with the original address their declaration's
comment gives (`// 0x0087a480`). Not part of the build.

Every engine global is a C definition in standalone/data/*.c (the globals->C slices and eq_*.c); there is no table of
absolute symbols any more, so a new global gets a definition there by hand: zero-initialised when its original address
lies past the initialised .data piece (standalone/image/pieces.json), otherwise with the image's bytes, and inside the
run of its neighbours when the code reaches it as part of a larger object (see standalone/data/eq_bss.c).
  python tools/update_globals.py          list the globals the last link left unresolved (build/standalone/link.log)
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LINK_LOG = os.path.join(ROOT, "build", "standalone", "link.log")


def unresolved_globals():
    """the unresolved data symbols in the last link, at their declarations' address comments"""
    sys.path.insert(0, os.path.join(ROOT, "harness"))
    import gen_link as gl
    addr, kind = gl.extern_map()
    out = {}
    log = open(LINK_LOG, encoding="utf-8", errors="replace").read() if os.path.exists(LINK_LOG) else ""
    for s in sorted(set(re.findall(r"unresolved external symbol (_\w+)", log))):
        n = s[1:]
        m = re.fullmatch(r"(?:PTR_)?DAT_([0-9a-fA-F]{8})", n)
        if m:
            out[s] = int(m.group(1), 16)
        elif n in addr and addr[n] and kind.get(n) == "data":
            if len(addr[n]) > 1:
                print("%s: address comments disagree (%s); fix the C first" % (s, ", ".join("0x%x" % a for a in addr[n])))
                continue
            out[s] = next(iter(addr[n]))
        else:
            print("%s: not a global with an address comment (a missing function or declaration?)" % s)
    return out


def main():
    new = unresolved_globals()
    for k, v in sorted(new.items(), key=lambda t: t[1]):
        print("%s 0x%08x" % (k[1:], v))
    if new:
        print("%d unresolved globals: define each in standalone/data/*.c" % len(new))
    sys.exit(1 if new else 0)


if __name__ == "__main__":
    main()
