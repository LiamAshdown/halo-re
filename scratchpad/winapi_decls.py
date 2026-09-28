"""the Windows API functions the standalone link reaches through cdecl->stdcall adapters (resolve.asm), with their
stdcall argument bytes, and every C declaration of them"""
import glob, json, os, re
R = "C:\\Users\\Liam-\\halo-re\\"
t = open(R + "build\\standalone\\resolve.asm").read()
imps = {s["slot"]: (s["dll"], s["name"]) for s in json.load(open(R + "standalone\\frozen\\imports.json"))}
sizes = json.load(open(R + "build\\standalone\\stdcall_sizes.json")) if os.path.exists(R + "build\\standalone\\stdcall_sizes.json") else {}
apis = {}
for b in re.split(r"\nPUBLIC ", t):
    m = re.search(r"call dword ptr ds:\[0([0-9A-F]+)h\]", b)
    if not m:
        continue
    name = b.split("\n", 1)[0].strip().lstrip("_")
    pushes = len(re.findall(r"push dword ptr \[ebp\+", b))
    apis[name] = (imps.get(int(m.group(1), 16), ("?", "?"))[0], pushes * 4)
json.dump(apis, open(R + "scratchpad\\winapi_adapters.json", "w"), indent=1, sort_keys=True)
decls = {}
for p in glob.glob(R + "src\\*\\*.c"):
    s = open(p, encoding="utf-8", errors="replace").read()
    cut = s.find("\n#if 0")
    s = s if cut < 0 else s[:cut]
    for m in re.finditer(r"^[ \t]*(extern\s+)?([^;(\n]*?)\b(\w+)\s*\(([^;{]*?)\)\s*;", s, re.M):
        if m.group(3) in apis:
            decls.setdefault(m.group(3), []).append((p[len(R):], " ".join(m.group(0).split())))
n_files = len({f for v in decls.values() for f, _ in v})
print(len(apis), "adapted APIs;", sum(len(v) for v in decls.values()), "declarations in", n_files, "files")
for a in sorted(apis):
    ds = decls.get(a, [])
    kinds = sorted({d for _, d in ds})
    print("%-32s %-12s %3d bytes  %d decl  %s" % (a, apis[a][0], apis[a][1], len(ds), kinds[0][:110] if kinds else "NONE"))
