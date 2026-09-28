"""a file whose extern declaration names one global but whose address comment (and the binary) says another address:
use the name that standalone/globals.asm has for that address, throughout the file"""
import glob, re
R = "C:\\Users\\Liam-\\halo-re\\"
# (declared name, address the file really means) -> the global at that address
FIX = {
    ("server_browser_join_target", 0x695420): "DAT_00695420",
    ("server_browser_query_pending", 0x719488): "DAT_00719488",
    ("global_origin3d_pointer", 0x6966f8): "global_zero_vector3d_pointer",
    ("global_zero_point3d_pointer", 0x696714): "global_origin3d_pointer",
    ("global_zero_vector3d_pointer", 0x696714): "global_origin3d_pointer",
}
DECL = re.compile(r"^[ \t]*extern\s+[^;(]*?\b(\w+)\s*(?:\[[^\]]*\])?\s*;[ \t]*//\s*0x0*([0-9a-fA-F]{6,8})\b", re.M)
for p in glob.glob(R + "src\\*\\*.c"):
    t = open(p, encoding="utf-8", errors="replace").read()
    cut = t.find("\n#if 0")
    head, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
    changed = []
    for m in DECL.finditer(head):
        key = (m.group(1), int(m.group(2), 16))
        if key in FIX:
            changed.append((m.group(1), FIX[key]))
    if not changed:
        continue
    for old, new in changed:
        if re.search(r"\b%s\b" % new, head):
            # the file already uses the target name too: only the mis-named declaration and its uses are renamed, and
            # the now duplicate declaration is dropped
            head = re.sub(r"^[ \t]*extern\s+[^;(]*?\b%s\s*;[^\n]*\n" % old, "", head, flags=re.M)
        head = re.sub(r"\b%s\b" % old, new, head)
    note = "".join("// FIXED 2026-09-28: %s here is the global at its address comment, %s (the name belonged to another\n"
                   "// global at a different address, so the link bound it there).\n" % (new, old) for old, new in changed)
    i = head.index("#include")
    head = head[:i] + note + "\n" + head[i:]
    open(p, "w", encoding="utf-8", newline="\n").write(head + tail)
    print(p[len(R):], changed)
