"""Link the MSVC objects of src/ (tools/msvc_build.py) into harness/build/halo_rewrite.dll, a DLL meant to live inside
the running retail halo.exe. Everything the rewritten code references but does not define is resolved to the real game:
  engine globals       -> absolute symbols at their original address (from the address comment on each extern, or the
                          address in a DAT_xxxxxxxx name); the DLL is /FIXED so absolute data references need no fixups
  engine functions     -> 'push addr / ret' stubs into the original code (no register is touched, so register-passed
                          arguments survive); only for functions with no rewritten definition
  Windows / DirectX /  -> halo.exe's own import address table slots, so the DLL calls exactly what the game loaded.
  Bink imports            A name declared without __stdcall gets an adapter that re-pushes its arguments and calls the
                          stdcall import (argument size from the SDK import libraries)
  C runtime            -> the game's statically linked CRT copy when Ghidra's function ID named it (keeps one heap),
                          else libcmt
Iterates: link, read the unresolved list, generate harness/gen/resolve.asm, link again.
Usage: python harness/gen_link.py      Writes harness/build/link.log and harness/build/unresolved.txt"""
import os, re, glob, json, shutil, struct, subprocess, collections

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
H = os.path.join(ROOT, "harness"); GEN = os.path.join(H, "gen"); OUT = os.path.join(H, "build")
os.makedirs(GEN, exist_ok=True); os.makedirs(OUT, exist_ok=True)
env = json.load(open(os.path.join(ROOT, "build", "msvc_env.json"))); PATH = env["PATH"]
tool = lambda n: shutil.which(n, path=PATH)
DLL_BASE = 0x30000000
SYS_LIBS = ["kernel32.lib", "user32.lib", "gdi32.lib", "advapi32.lib", "ole32.lib", "oleaut32.lib", "winmm.lib",
            "ws2_32.lib", "shell32.lib", "version.lib", "shlwapi.lib", "comdlg32.lib", "dsound.lib", "dinput8.lib", "dxguid.lib"]

# ---- what the sources say about each external name
def extern_map():
    addr = collections.defaultdict(collections.Counter); kind = {}
    ext = re.compile(r"^[ \t]*extern\s[^;{}]*?\b([A-Za-z_]\w*)\s*(\(|(?:\[[^\]]*\]\s*)+;|;)", re.M)
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(p, encoding="utf-8", errors="replace").read()
        b = t[:t.rfind("#if 0")] if "#if 0" in t else t
        # function-pointer variables: "extern void (*name)(args); // 0x00696664" -- data holding a code address.
        # The general pattern below would read these as a function called "void"
        # (arrays of them too: "extern void (*table[2])(args); // 0x0065743c")
        for m in re.finditer(r"^[ \t]*extern\s[^;{}(]*\(\s*(?:__\w+\s+)?\*\s*([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*\)\s*\(", b, re.M):
            end = b.find(";", m.start()); eol = b.find("\n", end)
            a = re.search(r"0x0{0,2}([4-9a-f][0-9a-f]{5})\b", b[end:eol if eol > 0 else len(b)], re.I)
            kind[m.group(1)] = "data"
            if a: addr[m.group(1)][int(a.group(1), 16)] += 1
        for m in ext.finditer(b):
            end = b.find(";", m.start()); eol = b.find("\n", end)
            tail = b[end:eol if eol > 0 else len(b)]
            a = re.search(r"0x0{0,2}([4-9a-f][0-9a-f]{5})\b", tail, re.I)
            name = m.group(1); kind.setdefault(name, "func" if m.group(2) == "(" else "data")
            if a: addr[name][int(a.group(1), 16)] += 1
    return addr, kind

# ---- names declared without an address on the extern line: take an address written on the same comment line as the
#      name anywhere in the code part of src (e.g. "// 0x0065f2a0 actor_avoidance_circle" or "name @0x...")
def nearby_addresses():
    cnt = collections.defaultdict(collections.Counter)
    pat = re.compile(r"\b([A-Za-z_]\w{3,})\b[^\n]{0,40}?(?:@|\b)0x0{0,2}([4-8][0-9a-f]{5})\b|0x0{0,2}([4-8][0-9a-f]{5})\b[^\n]{0,12}?\b([A-Za-z_]\w{3,})\b", re.I)
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")) + glob.glob(os.path.join(ROOT, "types", "*.h")):
        t = open(p, encoding="utf-8", errors="replace").read()
        b = t[:t.rfind("#if 0")] if (p.endswith(".c") and "#if 0" in t) else t
        for m in pat.finditer(b):
            name, a = (m.group(1), m.group(2)) if m.group(1) else (m.group(4), m.group(3))
            cnt[name][int(a, 16)] += 1
    return {n: c.most_common(1)[0][0] for n, c in cnt.items()}

# ---- halo.exe imports: name -> IAT slot VA
def halo_imports():
    d = open(os.path.join(ROOT, "bin", "halo.exe"), "rb").read()
    pe = struct.unpack_from("<I", d, 0x3c)[0]; nsec = struct.unpack_from("<H", d, pe + 6)[0]
    opt = pe + 24; base = struct.unpack_from("<I", d, opt + 28)[0]
    imp_rva = struct.unpack_from("<I", d, opt + 104)[0]
    secs = [struct.unpack_from("<8sIIII", d, opt + struct.unpack_from("<H", d, pe + 20)[0] + i * 40) for i in range(nsec)]
    def off(rva):
        for _, vs, va, rs, ro in secs:
            if va <= rva < va + max(vs, rs): return rva - va + ro
    res = {}; i = off(imp_rva)
    while True:
        oft, _, _, name_rva, ft = struct.unpack_from("<IIIII", d, i)
        if not name_rva: break
        dll = d[off(name_rva):].split(b"\0")[0].decode().lower()
        j = off(oft or ft); slot = ft
        while True:
            v = struct.unpack_from("<I", d, j)[0]
            if not v: break
            if not v & 0x80000000:
                res[d[off(v) + 2:].split(b"\0")[0].decode()] = (base + slot, dll)
            j += 4; slot += 4
        i += 20
    return res

# ---- stdcall argument sizes from the SDK import libraries: base name -> bytes
def stdcall_sizes():
    cache = os.path.join(OUT, "stdcall_sizes.json")
    if os.path.exists(cache): return json.load(open(cache))
    sizes = {}
    for lib in SYS_LIBS + ["d3d8.lib", "d3d9.lib"]:
        r = subprocess.run([tool("dumpbin"), "/nologo", "/linkermember:1", lib], capture_output=True, text=True, env=env, errors="replace")
        for m in re.finditer(r"\b_([A-Za-z]\w*)@(\d+)\b", r.stdout): sizes.setdefault(m.group(1), int(m.group(2)))
    json.dump(sizes, open(cache, "w")); return sizes

def game_crt():
    fj = os.path.join(ROOT, "out", "functions.json")
    if not os.path.exists(fj): return {}
    crt = {x["name"].lstrip("_"): int(x["addr"], 16) for x in json.load(open(fj)) if x.get("lib") or x.get("fid")}
    crt.setdefault("fopen", 0x624186)
    crt.setdefault("wcscpy", 0x625bba)  # not FID-matched: copies words through the NUL, returns dest   # not FID-matched: push 0x40 (_SH_DENYNO); call __fsopen(path, mode, shflag)
    return crt

FORCE = "--force" in __import__("sys").argv   # link even with unresolved names (they point at 0 until fixed)

def link(extra_objs, force=False):
    objs = glob.glob(os.path.join(ROOT, "build", "obj", "*", "*.obj")) + extra_objs
    rsp = os.path.join(OUT, "objs.rsp"); open(rsp, "w").write("\n".join('"%s"' % o for o in objs))
    cmd = [tool("link"), "/nologo", "/DLL", "/MACHINE:X86", "/FIXED", "/BASE:0x%x" % DLL_BASE, "/SAFESEH:NO", "/OPT:NOREF", "/OPT:NOICF",
           "/NODEFAULTLIB:msvcrt.lib", "/OUT:" + os.path.join(OUT, "halo_rewrite.dll"), "/MAP:" + os.path.join(OUT, "halo_rewrite.map"),
           "@" + rsp] + (["/FORCE:UNRESOLVED"] if force else []) + SYS_LIBS + ["libcmt.lib", "libvcruntime.lib", "libucrt.lib"]
    r = subprocess.run(cmd, capture_output=True, text=True, env=env, errors="replace")
    open(os.path.join(OUT, "link.log"), "w").write(r.stdout)
    return r.returncode, sorted(set(re.findall(r"unresolved external symbol (\S+)", r.stdout)))

def asm_name_ok(s): return re.fullmatch(r"[A-Za-z_$?@][\w$?@]*", s) is not None

def main():
    addr, kind = extern_map(); nearby = nearby_addresses(); imps = halo_imports(); sizes = stdcall_sizes(); crt = game_crt()
    extra = []
    for c in ("harness_entry.c", "x87_shims.c"):
        o = os.path.join(OUT, c.replace(".c", ".obj")); extra.append(o)
        subprocess.run([tool("cl"), "/nologo", "/c", "/GS-", "/O2", "/Fo" + o, os.path.join(H, c)], env=env, check=True, capture_output=True)
    for c in ("gen/hook_table.c",):
        if os.path.exists(os.path.join(H, c)):
            o = os.path.join(OUT, "hook_table.obj"); extra.append(o)
            subprocess.run([tool("cl"), "/nologo", "/c", "/GS-", "/O2", "/Fo" + o, os.path.join(H, c)], env=env, check=True, capture_output=True)
    if os.path.exists(os.path.join(GEN, "adapters.asm")):
        o = os.path.join(OUT, "adapters.obj")
        r = subprocess.run([tool("ml"), "/nologo", "/c", "/coff", "/Fo" + o, os.path.join(GEN, "adapters.asm")], capture_output=True, text=True, env=env)
        if r.returncode: print(r.stdout[-2000:]); return
        extra.append(o)
    rc, unres = link(extra)
    if "LNK1104" in open(os.path.join(OUT, "link.log")).read():
        print("link failed: halo_rewrite.dll is locked (is Halo running?). Close the game and run again."); return
    asm = [".386", ".model flat", "option casemap:none", ".code"]; data_eq = []; report = collections.Counter(); left = []; guessed = []
    for s in unres:
        if not asm_name_ok(s): left.append((s, "name not expressible in MASM")); continue
        dec = re.fullmatch(r"_(\w+?)(?:@(\d+))?", s)
        if not dec: left.append((s, "not a C symbol")); continue
        n, argbytes = dec.group(1), dec.group(2)
        m = re.fullmatch(r"(DAT|FUN|PTR_DAT|PTR_FUN|LAB)_([0-9a-fA-F]{8})", n)
        a = None
        if m: a = int(m.group(2), 16); k = "data" if m.group(1).endswith("DAT") else "func"
        elif n in addr and addr[n]: a = addr[n].most_common(1)[0][0]; k = kind.get(n, "func")
        elif re.search(r"(?:_|0x)(00[4-8][0-9a-fA-F]{5}|[4-8][0-9a-fA-F]{5})(?:_|$)", n):     # address embedded in the name
            a = int(re.search(r"(?:_|0x)(00[4-8][0-9a-fA-F]{5}|[4-8][0-9a-fA-F]{5})(?:_|$)", n).group(1), 16)
            k = kind.get(n, "func" if re.search(r"(callback|proc|LAB_|FUN_)", n) else "data")
        elif n in nearby: a = nearby[n]; k = kind.get(n, "data"); report["address from a nearby comment"] += 1; guessed.append((s, a))
        if n in imps and (k if a else kind.get(n)) != "data":       # an import, by name: call through the game's IAT slot
            slot, dll = imps[n]
            if argbytes is not None or n not in sizes or dll.startswith("msvcr"):
                asm += [f"PUBLIC {s}", f"{s}:", f"    jmp dword ptr ds:[0{slot:X}h]"]; report["import (IAT jmp)"] += 1
            else:                                                    # declared cdecl, really stdcall: re-push and call
                nb = sizes[n]; asm += [f"PUBLIC {s}", f"{s}:", "    push ebp", "    mov ebp, esp"]
                asm += [f"    push dword ptr [ebp+{8 + off}]" for off in range(nb - 4, -4, -4)]
                asm += [f"    call dword ptr ds:[0{slot:X}h]", "    pop ebp", "    ret"]; report["import (cdecl->stdcall adapter)"] += 1
            continue
        if a is None and argbytes is None and n in sizes and n not in addr:
            nb = sizes[n]; imp = f"__imp__{n}@{nb}"
            asm += [f"EXTERN {imp}:DWORD", f"PUBLIC {s}", f"{s}:", "    push ebp", "    mov ebp, esp"]
            asm += [f"    push dword ptr [ebp+{8 + off}]" for off in range(nb - 4, -4, -4)]
            asm += [f"    call dword ptr [{imp}]", "    pop ebp", "    ret"]; report["SDK import (cdecl->stdcall adapter)"] += 1; continue
        # compiler helpers (64-bit arithmetic, float->int, stack probe) have register/FPU conventions and the game's
        # FID labels for them are unreliable (___alldiv was labelled __allmul): always take them from the MSVC runtime
        if re.fullmatch(r"_*(a?ll(div|mul|rem|shl|shr)|aull(div|rem|shr)|ftol2?(_sse)?|chkstk|alloca_probe\w*)", n):
            left.append((s, "compiler helper: must come from the MSVC runtime")); continue
        if a is None and n in crt: a = crt[n]; k = "func"; report["game CRT"] += 1
        if a is None and n.startswith("_") and n.lstrip("_") in crt:   # Ghidra's CRT names carry the C underscore
            a = crt[n.lstrip("_")]; k = "func"; report["game CRT"] += 1
        if a is None and n.startswith("_") and n != n.lstrip("_"):      # else the CRT from libcmt under its C name
            t = "_" + n.lstrip("_"); asm += [f"EXTERN {t}:PROC", f"PUBLIC {s}", f"{s}:", f"    jmp {t}"]
            report["libcmt (renamed)"] += 1; continue
        if a is None: left.append((s, "no address")); continue
        if k == "data": data_eq.append(f"PUBLIC {s}\n{s} EQU 0{a:X}h"); report["global (absolute)"] += 1
        else: asm += [f"PUBLIC {s}", f"{s}:", f"    push 0{a:X}h", "    ret"]; report["function (stub into original)"] += 1
    src = "\n".join(asm[:3] + data_eq + asm[3:] + ["END", ""])
    open(os.path.join(GEN, "resolve.asm"), "w").write(src)
    r = subprocess.run([tool("ml"), "/nologo", "/c", "/coff", "/Fo" + os.path.join(OUT, "resolve.obj"), os.path.join(GEN, "resolve.asm")],
                       capture_output=True, text=True, env=env, errors="replace")
    if r.returncode: print(r.stdout[-3000:]); return
    rc, unres2 = link(extra + [os.path.join(OUT, "resolve.obj")], FORCE)
    open(os.path.join(OUT, "guessed_addresses.txt"), "w").write("".join(f"{s}\t0x{a:08x}\n" for s, a in guessed))
    open(os.path.join(OUT, "unresolved.txt"), "w").write("\n".join(f"{s}\t{why}" for s, why in left) + "\n")
    print("first link unresolved:", len(unres), "| resolved:", dict(report), "| left:", len(left))
    print("second link: exit", rc, "unresolved", len(unres2), "->", "harness/build/link.log")

if __name__ == "__main__": main()
