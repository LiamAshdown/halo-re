"""every function the C declares locally that the Windows SDK import libraries export (stdcall_sizes.json from
kernel32 / user32 / gdi32 / advapi32 / ole32 / oleaut32 / winmm / ws2_32 / shell32 / version / shlwapi / comdlg32 /
dsound / dinput8 / dxguid), plus the cdecl ones windows.h declares; DirectX names are listed separately"""
import collections, glob, json, os, re
R = "C:\\Users\\Liam-\\halo-re\\"
sizes = json.load(open(R + "harness\\build\\stdcall_sizes.json"))
CDECL_WIN = {"wsprintfA", "wsprintfW", "wvsprintfA", "wvsprintfW"}
names = set(sizes) | CDECL_WIN
EXT = re.compile(r"^[ \t]*extern\s+([^;(]*?)\b(\w+)\s*\(([^;]*?)\)\s*;", re.M | re.S)
decl = collections.defaultdict(list)
for p in glob.glob(R + "src\\*\\*.c"):
    s = open(p, encoding="utf-8", errors="replace").read()
    cut = s.find("\n#if 0")
    s = s if cut < 0 else s[:cut]
    for m in EXT.finditer(s):
        if m.group(2) in names:
            decl[m.group(2)].append(p[len(R):])
dx = {n for n in decl if re.match(r"(Direct|D3D|XInput)", n)}
win = {n: v for n, v in decl.items() if n not in dx}
json.dump(sorted(win), open(R + "scratchpad\\winapi_names.json", "w"), indent=0)
print(len(win), "Windows API names,", sum(len(v) for v in win.values()), "declarations in",
      len({f for v in win.values() for f in v}), "files")
print("DirectX (left alone):", {n: len(decl[n]) for n in sorted(dx)})
old = set(json.load(open(R + "scratchpad\\winapi_adapters.json")))
print("beyond the 156 former adapters:", sorted(set(win) - old))
