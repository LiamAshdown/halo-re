"""python tools/api_undef.py <log>: adds an #undef of the Win32 `interface` macro after the last #include of every file with error C3083 on 'interface'."""
import re, sys
log = open(sys.argv[1], errors="replace").read()
files = set(m.group(1) for m in re.finditer(r"^\s*([A-Za-z]:[^()\n]*?)\(\d+,\d+\): error C3083: 'interface'", log, re.M))
for f in files:
    s = open(f, encoding="utf-8", errors="replace", newline="").read()
    if "#undef interface" in s:
        continue
    incs = list(re.finditer(r'^#include[^\n]*\n', s, re.M))
    p = incs[-1].end()
    s = s[:p] + "\n#ifdef interface\n#undef interface\n#endif\n" + s[p:]
    open(f, "w", encoding="utf-8", newline="").write(s)
    print("undef", f)
