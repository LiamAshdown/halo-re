"""every extern declaration of the adapted Windows APIs (scratchpad/winapi_adapters.json) in src/ and headers"""
import collections, glob, json, re
R = "C:\\Users\\Liam-\\halo-re\\"
apis = json.load(open(R + "scratchpad\\winapi_adapters.json"))
EXT = re.compile(r"^[ \t]*extern\s+([^;(]*?)\b(%s)\s*\(([^;]*?)\)\s*;" % "|".join(sorted(apis, key=len, reverse=True)), re.M | re.S)
paths = glob.glob(R + "src\\*\\*.c") + glob.glob(R + "src\\*\\*.h") + glob.glob(R + "types\\*.h") + [R + "harness\\msvc_compat.h"]
found = collections.defaultdict(list)
stdcall_already = 0
for p in paths:
    s = open(p, encoding="utf-8", errors="replace").read()
    cut = s.find("\n#if 0")
    s = s if cut < 0 else s[:cut]
    for m in EXT.finditer(s):
        prefix = m.group(1)
        is_std = bool(re.search(r"__stdcall|WINAPI|APIENTRY|CALLBACK", prefix))
        params = [x for x in re.sub(r"/\*.*?\*/|//[^\n]*", "", m.group(3)).split(",") if x.strip() and x.strip() != "void"]
        found[m.group(2)].append((p[len(R):], is_std, len(params)))
        stdcall_already += is_std
print(sum(len(v) for v in found.values()), "extern declarations,", stdcall_already, "already __stdcall,",
      len({f for v in found.values() for f, *_ in v}), "files")
bad = [(a, f, n) for a, v in found.items() for f, s, n in v if n * 4 != apis[a][1]]
print(len(bad), "declarations whose parameter count disagrees with the DLL's stdcall size:")
for a, f, n in sorted(bad):
    print("   %-28s declares %d, DLL takes %d  %s" % (a, n, apis[a][1] // 4, f))
print("headers:", sorted({f for v in found.values() for f, *_ in v if not f.endswith(".c")}))
missing = [a for a in apis if a not in found]
print("no extern declaration found:", missing)
