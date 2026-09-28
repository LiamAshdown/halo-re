"""declare every Windows API the C calls as __stdcall (as the DLLs define them), so the standalone link binds each call
to the exe's own import (__imp__Name@N) instead of a generated cdecl->stdcall adapter through a retail import slot"""
import glob, json, re
R = "C:\\Users\\Liam-\\halo-re\\"
apis = json.load(open(R + "scratchpad\\winapi_adapters.json"))
names = "|".join(sorted(apis, key=len, reverse=True))
EXT = re.compile(r"(^[ \t]*extern\s+)([^;(]*?)\b(%s)(\s*\([^;]*?\)\s*;)" % names, re.M | re.S)
changed_files, changed_decls = 0, 0
for p in glob.glob(R + "src\\*\\*.c"):
    s = open(p, encoding="utf-8", errors="replace").read()
    cut = s.find("\n#if 0")
    head, tail = (s, "") if cut < 0 else (s[:cut], s[cut:])
    n = 0

    def fix(m):
        global n
        prefix = m.group(2)
        if re.search(r"__stdcall|WINAPI|APIENTRY|CALLBACK", prefix):
            return m.group(0)
        sep = "" if prefix.endswith((" ", "*", "\t")) or not prefix else " "
        return m.group(1) + prefix + sep + "__stdcall " + m.group(3) + m.group(4)

    new_head, count = EXT.subn(fix, head)
    if new_head != head:
        changed_files += 1
        changed_decls += sum(1 for a, b in zip(EXT.findall(head), EXT.findall(new_head)) if a != b)
        open(p, "w", encoding="utf-8", newline="\n").write(new_head + tail)
print(changed_decls, "declarations made __stdcall in", changed_files, "files")
