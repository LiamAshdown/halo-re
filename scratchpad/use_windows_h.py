"""replace every local declaration of a Windows API function in src/ with #include "win32.h" (the SDK headers)"""
import glob, json, re
R = "C:\\Users\\Liam-\\halo-re\\"
names = set(json.load(open(R + "scratchpad\\winapi_names.json")))
alt = "|".join(sorted(names, key=len, reverse=True))
# a whole extern declaration (possibly spanning lines) plus the rest of its last line
EXT = re.compile(r"^[ \t]*extern\s+[^;(]*?\b(?:%s)\s*\([^;]*?\)\s*;[^\n]*\n" % alt, re.M | re.S)
WIN_TYPES = {"HWND", "HDC", "HKEY", "DWORD", "BYTE", "LPSTR", "LPSECURITY_ATTRIBUTES", "LPDWORD", "LSTATUS", "LPBYTE",
             "HBITMAP", "HANDLE", "HINSTANCE", "HMODULE", "BOOL", "WORD", "UINT", "LONG", "LPVOID", "LPCSTR", "WPARAM",
             "LPARAM", "LRESULT", "HICON", "HCURSOR", "HBRUSH", "HMENU", "HGDIOBJ", "ATOM", "HGLOBAL", "HRSRC", "HFONT"}
TYPEDEF = re.compile(r"^typedef\s+[^;{}]*?\b(%s)\s*;[^\n]*\n" % "|".join(WIN_TYPES), re.M)
files = decls = typedefs = 0
for p in sorted(glob.glob(R + "src\\*\\*.c")):
    s = open(p, encoding="utf-8", errors="replace").read()
    cut = s.find("\n#if 0")
    head, tail = (s, "") if cut < 0 else (s[:cut], s[cut:])
    new, n = EXT.subn("", head)
    if n == 0:
        continue
    new, k = TYPEDEF.subn("", new)
    if '#include "win32.h"' not in new:
        i = new.find("#include")
        if i < 0:
            raise SystemExit("no #include in " + p)
        new = new[:i] + '#include "win32.h"\n' + new[i:]
    open(p, "w", encoding="utf-8", newline="\n").write(new + tail)
    files += 1
    decls += n
    typedefs += k
print(decls, "declarations removed,", typedefs, "local Windows typedefs removed,", files, "files now include win32.h")
