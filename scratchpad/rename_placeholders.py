"""call functions by their C names: every FUN_xxxxxxxx / thunk_FUN_xxxxxxxx / LAB_xxxxxxxx the link maps to a
rewritten function by address (resolve.asm 'jmp' aliases) is renamed to that function in src/ (outside #if 0)"""
import glob, json, re
R = "C:\\Users\\Liam-\\halo-re\\"
t = open(R + "build\\standalone\\resolve.asm").read()
pairs = re.findall(r"PUBLIC (\S+)\n\1:\n    jmp (_\w+)\n", t)
MAP = {a[1:]: b[1:] for a, b in pairs if re.match(r"_(thunk_)?(FUN|LAB)_[0-9a-f]{8}$", a)}
json.dump(MAP, open(R + "scratchpad\\placeholder_map.json", "w"), indent=1, sort_keys=True)
pat = re.compile(r"\b(%s)\b" % "|".join(sorted(MAP, key=len, reverse=True)))
files = uses = 0
for p in glob.glob(R + "src\\*\\*.c"):
    s = open(p, encoding="utf-8", errors="replace").read()
    cut = s.find("\n#if 0")
    head, tail = (s, "") if cut < 0 else (s[:cut], s[cut:])
    new, n = pat.subn(lambda m: MAP[m.group(1)], head)
    if n:
        open(p, "w", encoding="utf-8", newline="\n").write(new + tail)
        files += 1
        uses += n
print(len(MAP), "placeholder names;", uses, "uses renamed in", files, "files")
