"""Standalone build generator, stage 2: link every rewritten function into build/standalone/halo_rebuilt.exe.

Run tools/gen_standalone.py (stage 1) and tools/msvc_build.py first. This stage:
  - writes build/standalone/standalone_tables.c: the image pieces, the import slots the loader fills, the Halo folder
  - writes build/standalone/resolve.asm, resolving what the objects reference but do not define:
      engine globals    -> absolute EQU symbols (the loader maps halo.exe's data back at those addresses)
      Windows/DirectX   -> halo.exe's import slots (filled by the loader), with gen_link's cdecl->stdcall adapters
      C runtime         -> libcmt (never the game's copy: none of the original code is present)
      D3DX              -> d3dx9.lib from the DirectX SDK
      engine functions with no C rewrite -> TRAP stubs: log the name and exit (build/standalone/traps.txt)
    plus the code-pointer table: every dword in halo.exe's data that held a function address now holds ours
  - compiles standalone/loader.c, the tables and harness/x87_shims.c, links an exe at 0x10000000
  - writes build/standalone/link_report.txt: traps, unresolved names, what each resolution kind covered
Usage: python tools/gen_standalone_link.py [halo folder]"""
import os, re, sys, glob, json, subprocess, collections

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness"))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import retail_guard as rg   # the build reads standalone/frozen/, never the retail binary
import gen_link as gl

OUT = os.path.join(ROOT, "build", "standalone")
SA = os.path.join(ROOT, "standalone")
EXE = os.path.join(OUT, "halo_rebuilt.exe")
BASE = 0x10000000
HALO_FOLDER = sys.argv[1] if len(sys.argv) > 1 else r"C:\Program Files (x86)\Microsoft Games\Halo"
DXSDK_LIB = r"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Lib\x86"
EXTRA_LIBS = ["d3d9.lib", "d3dx9.lib", "legacy_stdio_definitions.lib"]


def c_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_tables():
    pieces = json.load(open(os.path.join(SA, "image", "pieces.json")))
    imports = json.load(open(os.path.join(OUT, "imports.json")))
    lines = ['#include "standalone_tables.h"', "",
             "const char standalone_halo_folder[] = %s;" % c_string(HALO_FOLDER), ""]
    # the data image the loader copies to the original addresses (standalone/image/<label>.asm)
    lines += ["extern const unsigned char halo_image_%s[];" % p["label"] for p in pieces]
    lines += ["", "const standalone_piece standalone_pieces[] = {"]
    for p in pieces:
        lines.append("    { %s, 0x%08xUL, halo_image_%s, 0x%08xUL }," % (c_string(p["label"]), p["va"], p["label"],
                                                                       p["size"]))
    lines += ["};", "const int standalone_piece_count = %d;" % len(pieces), "",
              "const standalone_import standalone_imports[] = {"]
    for s in imports:
        if s["name"].startswith("#"):
            name, ordinal = "0", int(s["name"][1:])
        else:
            name, ordinal = c_string(s["name"]), 0
        lines.append("    { 0x%08xUL, %s, %s, %d, 0x%08xUL }," % (s["slot"], c_string(s["dll"]), name, ordinal,
                                                                   s.get("module_handle_slot", 0) if s["delay"] else 0))
    lines += ["};", "const int standalone_import_count = %d;" % len(imports), ""]
    open(os.path.join(OUT, "standalone_tables.c"), "w").write("\n".join(lines) + "\n")


# Functions the standalone build supplies itself instead of the library the name suggests (standalone/*.c)
STANDALONE_REPLACEMENTS = {
    "D3DXCreateEffect": "_standalone_d3dx_create_effect",   # wraps the June 2010 effect in the 2003 vtable layout
}

# Game library functions whose Ghidra name is not a libcmt symbol, mapped to the libcmt function that does the same
# thing. Each entry was checked against the original bytes (objdump) before it went in.
LIBCMT_ALIASES = {
    "operator_new": "_malloc",   # 0x6277da: push 1; push size; call __nh_malloc -- VC6 new: malloc + new handler, NULL on failure
}

# Winsock 1.1 ordinals, shared by WS2_32.dll and WSOCK32.dll (halo.exe imports several by ordinal)
WINSOCK_ORDINALS = {1: "accept", 2: "bind", 3: "closesocket", 4: "connect", 5: "getpeername", 6: "getsockname",
                    7: "getsockopt", 8: "htonl", 9: "htons", 10: "ioctlsocket", 11: "inet_addr", 12: "inet_ntoa",
                    13: "listen", 14: "ntohl", 15: "ntohs", 16: "recv", 17: "recvfrom", 18: "select", 19: "send",
                    20: "sendto", 21: "setsockopt", 22: "shutdown", 23: "socket", 51: "gethostbyaddr",
                    52: "gethostbyname", 53: "getprotobyname", 54: "getprotobynumber", 55: "getservbyname",
                    56: "getservbyport", 57: "gethostname", 111: "WSAGetLastError", 112: "WSASetLastError",
                    115: "WSAStartup", 116: "WSACleanup", 151: "__WSAFDIsSet"}


def import_map():
    """every import slot the loader fills: base name -> (slot, dll, stdcall argument bytes or None), and slot -> name"""
    by_name, by_slot = {}, {}
    for s in json.load(open(os.path.join(OUT, "imports.json"))):
        nm, dll = s["name"], s["dll"].lower()
        if nm.startswith("#"):
            nm = WINSOCK_ORDINALS.get(int(nm[1:])) if dll in ("ws2_32.dll", "wsock32.dll") else None
            if not nm:
                continue
        m = re.fullmatch(r"_?(\w+?)@(\d+)", nm)
        base, nb = (m.group(1), int(m.group(2))) if m else (nm, None)
        by_name.setdefault(base, (s["slot"], dll, nb))
        by_slot[s["slot"]] = base
    return by_name, by_slot


def d3dx_sizes():
    """stdcall argument bytes of the SDK's D3DX exports (halo.exe linked an older D3DX statically); frozen in
    standalone/frozen/d3dx_sizes.json so the build needs neither the SDK library listing nor a local cache"""
    if os.path.exists(rg.frozen_path("d3dx_sizes.json")):
        return rg.read_frozen("d3dx_sizes.json")
    r = subprocess.run([gl.tool("dumpbin"), "/nologo", "/linkermember:1", os.path.join(DXSDK_LIB, "d3dx9.lib")],
                       capture_output=True, text=True, env=gl.env, errors="replace")
    sizes = {}
    for m in re.finditer(r"(?<![\w@])_(D3DX\w*)@(\d+)", r.stdout):
        sizes.setdefault(m.group(1), int(m.group(2)))
    rg.write_frozen("d3dx_sizes.json", sizes)
    return sizes


def original_ret_bytes(address):
    """bytes a function at an original address pops on return (the immediate of its first ret), None if not known.
    Answers come from the committed standalone/frozen/code_address_ret.json (tools/freeze_retail_inputs.py)."""
    if not os.path.exists(rg.frozen_path("code_address_ret.json")):
        return None
    return rg.read_frozen("code_address_ret.json").get("0x%06x" % address)


def stdcall_definitions():
    """function name -> argument bytes, for rewrites defined __stdcall (their symbol is _name@N)"""
    out = {}
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        name = os.path.splitext(os.path.basename(p))[0]
        t = open(p, encoding="utf-8", errors="replace").read()
        m = re.search(r"^[^\n;{]*__stdcall\s+%s\s*\(([^)]*)\)\s*\n?\{" % re.escape(name), t, re.M)
        if m:
            params = [x for x in m.group(1).split(",") if x.strip() and x.strip() != "void"]
            out[name] = 4 * len(params)
    return out


def fastcall_definitions():
    """function name -> argument bytes, for rewrites defined __fastcall (their symbol is @name@N): C++ methods the CRT
    calls with __thiscall through a stored pointer (this in ECX, callee pops), e.g. std_exception_what (2026-09-28)"""
    out = {}
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        name = os.path.splitext(os.path.basename(p))[0]
        t = open(p, encoding="utf-8", errors="replace").read()
        m = re.search(r"^[^\n;{]*__fastcall\s+%s\s*\(([^)]*)\)\s*\n?\{" % re.escape(name), t, re.M)
        if m:
            params = [x for x in m.group(1).split(",") if x.strip() and x.strip() != "void"]
            out[name] = 4 * len(params)
    return out


def c_symbol(n, std, fast):
    """the decorated symbol of a rewrite: cdecl _name, __stdcall _name@N, __fastcall @name@N"""
    if n in std:
        return "_%s@%d" % (n, std[n])
    if n in fast:
        return "@%s@%d" % (n, fast[n])
    return "_" + n


def defines_function(e):
    """whether a code entry's object defines its function: fragment files (a range inside another function whose C
    covers it, e.g. object_update_functions_clone_4f93b0) carry an address header but no code, and an entry for one
    would redirect a jump to its own original address forever (2026-09-28)"""
    obj = os.path.join(ROOT, "build", "obj", e["module"], e["c_symbol"] + ".obj")
    if not os.path.exists(obj):
        return False
    data = open(obj, "rb").read()
    name = e["c_symbol"].encode()
    return any(p + name + s in data for p in (b"_", b"@") for s in (b"\0", b"@"))


def code_pointer_asm():
    """the named traps for library code pointers without C, and the table of (original address, our function) the
    loader redirects jumps into original .text with, in MASM so decorated names work. (The code pointers themselves
    are relocations in the image source: image_source() binds them.)"""
    ptrs = json.load(open(os.path.join(OUT, "code_pointers.json")))
    std = stdcall_definitions()
    fast = fastcall_definitions()
    ext, names, traps = [], set(), []
    for p in ptrs:
        if "c_symbol" not in p:
            sym = "cp_trap_%06x" % p["target"]
            if sym not in {x[0] for x in traps}:
                traps.append((sym, p["name"]))
    # (original address, our function) for every rewritten function whose object was built, sorted by address
    entry_rows = []
    for e in json.load(open(os.path.join(OUT, "code_entries.json"))):
        if not defines_function(e):
            continue
        n = e["c_symbol"]
        sym = c_symbol(n, std, fast)
        names.add(sym)
        entry_rows.append("    dd 0%Xh, %s" % (e["addr"], sym))
    for sym in sorted(names):
        ext.append("EXTERN %s:PROC" % sym)
    stubs = ["EXTERN _standalone_missing_function:PROC", ".code"]
    strs = [".const"]
    for sym, name in traps:
        stubs += ["PUBLIC %s" % sym, "%s:" % sym, "    push offset %s_name" % sym, "    call _standalone_missing_function"]
        strs.append('%s_name db "%s (stored code pointer)", 0' % (sym, name))
    return ext + stubs + strs, entry_rows


# extra linker options (the image source's /ALTERNATENAME bindings), written into the response file
LINK_OPTIONS = []


def image_source():
    """assembles the data image (standalone/image/*.asm, written by tools/gen_image_source.py) and returns its objects
    plus the /ALTERNATENAME options that bind each halo_code_<address> it stores to the C rewrite of that function"""
    image = os.path.join(SA, "image")
    objs = [assemble(os.path.join(image, p["label"] + ".asm"), os.path.join(OUT, "image_%s.obj" % p["label"]))
            for p in json.load(open(os.path.join(image, "pieces.json")))]
    std = stdcall_definitions()
    fast = fastcall_definitions()
    bound = {}
    for p in json.load(open(os.path.join(OUT, "code_pointers.json"))):
        # a library pointer without C (the retail D3DX / CRT tables) gets the same named trap the table below gives it
        bound[p["target"]] = c_symbol(p["c_symbol"], std, fast) if "c_symbol" in p else "cp_trap_%06x" % p["target"]
    return objs, ["/ALTERNATENAME:halo_code_%06x=%s" % (a, s) for a, s in sorted(bound.items())]


def link(objs, force):
    rsp = os.path.join(OUT, "objs.rsp")
    open(rsp, "w").write("\n".join(['"%s"' % o for o in objs] + LINK_OPTIONS))
    cmd = [gl.tool("link"), "/nologo", "/MACHINE:X86", "/SUBSYSTEM:WINDOWS", "/FIXED", "/BASE:0x%x" % BASE,
           "/SAFESEH:NO", "/OPT:NOREF", "/OPT:NOICF", "/LARGEADDRESSAWARE:NO", "/NODEFAULTLIB:msvcrt.lib",
           "/LIBPATH:" + DXSDK_LIB, "/OUT:" + EXE, "/MAP:" + os.path.join(OUT, "halo_rebuilt.map"), "@" + rsp] + \
          (["/FORCE:UNRESOLVED"] if force else []) + gl.SYS_LIBS + EXTRA_LIBS + ["libcmt.lib", "libvcruntime.lib", "libucrt.lib"]
    r = subprocess.run(cmd, capture_output=True, text=True, env=gl.env, errors="replace")
    open(os.path.join(OUT, "link.log"), "w").write(r.stdout)
    return r.returncode, sorted(set(re.findall(r"unresolved external symbol (\S+)", r.stdout)))


def compile_c(src, obj, includes=()):
    cmd = [gl.tool("cl"), "/nologo", "/c", "/GS-", "/O2", "/Fo" + obj] + ["/I" + i for i in includes] + [src]
    r = subprocess.run(cmd, capture_output=True, text=True, env=gl.env, errors="replace")
    if r.returncode:
        print(r.stdout[-3000:])
        raise SystemExit("compile failed: " + src)
    return obj


def assemble(src, obj):
    r = subprocess.run([gl.tool("ml"), "/nologo", "/c", "/coff", "/Fo" + obj, src], capture_output=True, text=True,
                       env=gl.env, errors="replace")
    if r.returncode:
        print(r.stdout[-3000:])
        raise SystemExit("assemble failed: " + src)
    return obj


def main():
    rg.forbid_retail()
    os.makedirs(OUT, exist_ok=True)
    write_tables()
    extra = [compile_c(os.path.join(SA, "loader.c"), os.path.join(OUT, "loader.obj"), [SA]),
             compile_c(os.path.join(OUT, "standalone_tables.c"), os.path.join(OUT, "standalone_tables.obj"), [SA]),
             compile_c(os.path.join(ROOT, "harness", "x87_shims.c"), os.path.join(OUT, "x87_shims.obj")),
             compile_c(os.path.join(SA, "d3dx_compat.c"), os.path.join(OUT, "d3dx_compat.obj"),
                       [SA, os.path.join(os.path.dirname(DXSDK_LIB), "..", "Include")])]
    ext, entry_rows = code_pointer_asm()
    pointer_asm = [".386", ".model flat", "option casemap:none"] + ext + [
        ".const", "PUBLIC _standalone_code_entries", "PUBLIC _standalone_code_entry_count",
        "_standalone_code_entry_count dd %d" % len(entry_rows), "_standalone_code_entries LABEL DWORD"] + entry_rows + [
        "END", ""]
    open(os.path.join(OUT, "code_pointers.asm"), "w").write("\n".join(pointer_asm))
    extra.append(assemble(os.path.join(OUT, "code_pointers.asm"), os.path.join(OUT, "code_pointers.obj")))
    image_objs, alternates = image_source()
    extra += image_objs
    LINK_OPTIONS[:] = alternates

    # only objects whose source still exists: a renamed or deleted .c leaves its old object behind, which would link
    # stale code and its unbound references (shell_console_window_state_initialize.obj: nine '?' traps) (2026-09-28)
    objs = [o for o in glob.glob(os.path.join(ROOT, "build", "obj", "*", "*.obj"))
            if os.path.exists(os.path.join(ROOT, "src", os.path.basename(os.path.dirname(o)),
                                           os.path.splitext(os.path.basename(o))[0] + ".c"))] + extra
    rc, unres = link(objs, False)

    addr, kind = gl.extern_map()
    nearby = gl.nearby_addresses()
    imps, imp_by_slot = import_map()
    sizes = gl.stdcall_sizes()
    dx_sizes = d3dx_sizes()
    # game CRT functions by name (Ghidra's library matches, frozen by gen_standalone.py), plus two it never matched
    crt = rg.read_frozen("game_crt.json")
    crt.setdefault("fopen", 0x624186)
    crt.setdefault("wcscpy", 0x625bba)
    code = [".386", ".model flat", "option casemap:none", "EXTERN _standalone_missing_function:PROC", ".code"]
    data_eq, strings, report, left, traps = [], [], collections.Counter(), [], []
    externs = set()
    std_defs = stdcall_definitions()
    fast_defs = fastcall_definitions()
    all_defs = {os.path.splitext(os.path.basename(x))[0] for x in glob.glob(os.path.join(ROOT, "src", "*", "*.c"))}
    # rewritten functions by original address (only those whose object was built), for names that reach one by
    # address alone (2026-09-28)
    rewritten_at = {e["addr"]: e["c_symbol"] for e in json.load(open(os.path.join(OUT, "code_entries.json")))
                    if defines_function(e)}
    nearby_used = []
    for s in unres:
        if not gl.asm_name_ok(s):
            left.append((s, "name not expressible in MASM")); continue
        dec = re.fullmatch(r"_(\w+?)(?:@(\d+))?", s)
        if not dec:
            left.append((s, "not a C symbol")); continue
        n, argbytes = dec.group(1), dec.group(2)
        if n in STANDALONE_REPLACEMENTS and argbytes is None:
            externs.add(STANDALONE_REPLACEMENTS[n])
            code += ["PUBLIC %s" % s, "%s:" % s, "    jmp %s" % STANDALONE_REPLACEMENTS[n]]
            report["standalone replacement"] += 1
            continue
        if n.startswith("code_address_") and n[len("code_address_"):] in all_defs:
            # C that stores an original function's address for someone else to call (a window procedure, an APC):
            # the hooked build keeps the original address (an absolute symbol from the declaration's comment); here
            # it becomes a thunk into the C function, converting when the original was __stdcall (ret N) and the C
            # is cdecl
            fn = n[len("code_address_"):]
            a0 = addr[n].most_common(1)[0][0] if n in addr and addr[n] else None
            ret_n = original_ret_bytes(a0) if a0 else None
            target = "_%s@%d" % (fn, std_defs[fn]) if fn in std_defs else "_" + fn
            externs.add(target)
            if ret_n is None:
                left.append((s, "code address: no ret byte count in standalone/frozen/code_address_ret.json "
                                "(tools/freeze_retail_inputs.py)")); continue
            if fn in std_defs and std_defs[fn] != ret_n:
                left.append((s, "code address: C is __stdcall@%d but the original returns %d" % (std_defs[fn], ret_n)))
                continue
            code += ["PUBLIC %s" % s, "%s:" % s]
            if fn in std_defs or ret_n == 0:
                code += ["    jmp %s" % target]
            else:
                code += ["    push ebp", "    mov ebp, esp"]
                code += ["    push dword ptr [ebp+%d]" % (8 + off) for off in range(ret_n - 4, -4, -4)]
                code += ["    call %s" % target, "    add esp, %d" % ret_n, "    pop ebp", "    ret %d" % ret_n]
            report["code address thunk"] += 1
            continue
        if argbytes is None and n in std_defs:
            # our C defines it __stdcall but this caller declared it cdecl: copy the arguments, call, return (the
            # callee pops its own copy; the caller pops the originals)
            nb = std_defs[n]; target = "_%s@%d" % (n, nb); externs.add(target)
            code += ["PUBLIC %s" % s, "%s:" % s, "    push ebp", "    mov ebp, esp"]
            code += ["    push dword ptr [ebp+%d]" % (8 + off) for off in range(nb - 4, -4, -4)]
            code += ["    call %s" % target, "    pop ebp", "    ret"]; report["cdecl reference to a __stdcall rewrite"] += 1
            continue
        m = re.fullmatch(r"(DAT|FUN|PTR_DAT|PTR_FUN|LAB)_([0-9a-fA-F]{8})", n)
        a, k = None, None
        if m:
            a = int(m.group(2), 16); k = "data" if m.group(1).endswith("DAT") else "func"
        elif n in addr and addr[n]:
            a = addr[n].most_common(1)[0][0]; k = kind.get(n, "func")
        elif re.search(r"(?:_|0x)(00[4-8][0-9a-fA-F]{5}|[4-8][0-9a-fA-F]{5})(?:_|$)", n):
            a = int(re.search(r"(?:_|0x)(00[4-8][0-9a-fA-F]{5}|[4-8][0-9a-fA-F]{5})(?:_|$)", n).group(1), 16)
            k = kind.get(n, "func" if re.search(r"(callback|proc|LAB_|FUN_)", n) else "data")
        elif n in nearby:
            a = nearby[n]; k = kind.get(n, "data"); report["address from a nearby comment"] += 1
            nearby_used.append((s, a, k))
        if (a in rewritten_at and k == "func" and argbytes is None and rewritten_at[a] != n
                and rewritten_at[a] not in std_defs and rewritten_at[a] not in fast_defs):
            # a placeholder name (FUN_006147a0) for a function that now has C under its own name: a cdecl call to a
            # cdecl function, so a plain jump
            target = "_" + rewritten_at[a]; externs.add(target)
            code += ["PUBLIC %s" % s, "%s:" % s, "    jmp %s" % target]; report["rewritten function by address"] += 1
            continue
        if n in imps and (imp_by_slot.get(a) == n or (k if a else kind.get(n)) != "data"):
            slot, dll, nb = imps[n]
            nb = nb if nb is not None else sizes.get(n)
            if argbytes is not None or nb is None or dll.startswith("msvcr"):
                code += ["PUBLIC %s" % s, "%s:" % s, "    jmp dword ptr ds:[0%Xh]" % slot]; report["import (IAT jmp)"] += 1
            else:
                code += ["PUBLIC %s" % s, "%s:" % s, "    push ebp", "    mov ebp, esp"]
                code += ["    push dword ptr [ebp+%d]" % (8 + off) for off in range(nb - 4, -4, -4)]
                code += ["    call dword ptr ds:[0%Xh]" % slot, "    pop ebp", "    ret"]
                report["import (cdecl->stdcall adapter)"] += 1
            continue
        if argbytes is None and n in dx_sizes:
            nb = dx_sizes[n]; imp = "__imp__%s@%d" % (n, nb)
            code += ["EXTERN %s:DWORD" % imp, "PUBLIC %s" % s, "%s:" % s, "    push ebp", "    mov ebp, esp"]
            code += ["    push dword ptr [ebp+%d]" % (8 + off) for off in range(nb - 4, -4, -4)]
            code += ["    call dword ptr [%s]" % imp, "    pop ebp", "    ret"]; report["D3DX from the SDK (adapter)"] += 1
            continue
        if a is None and argbytes is None and n in sizes and n not in addr:
            nb = sizes[n]; imp = "__imp__%s@%d" % (n, nb)
            code += ["EXTERN %s:DWORD" % imp, "PUBLIC %s" % s, "%s:" % s, "    push ebp", "    mov ebp, esp"]
            code += ["    push dword ptr [ebp+%d]" % (8 + off) for off in range(nb - 4, -4, -4)]
            code += ["    call dword ptr [%s]" % imp, "    pop ebp", "    ret"]; report["SDK import (cdecl->stdcall adapter)"] += 1
            continue
        if n in LIBCMT_ALIASES:
            externs.add(LIBCMT_ALIASES[n])
            code += ["PUBLIC %s" % s, "%s:" % s, "    jmp %s" % LIBCMT_ALIASES[n]]
            report["C runtime (verified alias)"] += 1
            continue
        if re.fullmatch(r"_*(a?ll(div|mul|rem|shl|shr)|aull(div|rem|shr)|ftol2?(_sse)?|chkstk|alloca_probe\w*)", n):
            left.append((s, "compiler helper: must come from the MSVC runtime")); continue
        # C runtime: libcmt. Ghidra's CRT names already carry the C underscore (__isnan is libcmt's symbol __isnan);
        # a plain name (strlen -> _strlen) was already searched under s itself, so libcmt lacks it: game library code
        crt_name = n.lstrip("_")
        if n.startswith("_") and n != s and (a is None or crt_name in crt) and (a is None or k != "data"):
            externs.add(n)
            code += ["PUBLIC %s" % s, "%s:" % s, "    jmp %s" % n]
            report["C runtime (libcmt)"] += 1
            continue
        if k == "data" and a is not None:
            data_eq.append("PUBLIC %s\n%s EQU 0%Xh" % (s, s, a)); report["global (absolute)"] += 1; continue
        if a is None and kind.get(n) == "data":
            left.append((s, "data declared without an address"))   # never a trap: code would read or write it
            continue
        # an engine function with no C rewrite: trap
        label = "trap_str_%d" % len(strings)
        strings.append('%s db "%s", 0' % (label, n))
        code += ["PUBLIC %s" % s, "%s:" % s, "    push offset %s" % label, "    call _standalone_missing_function"]
        lib = crt_name in crt
        traps.append((n, a, "library" if lib else "game"))
        report["TRAP (game library code)" if lib else "TRAP (no C rewrite)"] += 1

    defined = set(re.findall(r"^PUBLIC (\S+)", "\n".join(code + data_eq), re.M))
    code[4:4] = ["EXTERN %s:PROC" % x for x in sorted(externs - defined)]
    src = "\n".join(code[:3] + data_eq + code[3:] + [".const"] + strings + ["END", ""])
    open(os.path.join(OUT, "resolve.asm"), "w").write(src)
    extra.append(assemble(os.path.join(OUT, "resolve.asm"), os.path.join(OUT, "resolve.obj")))
    rc2, unres2 = link(objs + [os.path.join(OUT, "resolve.obj")], True)

    with open(os.path.join(OUT, "traps.txt"), "w") as f:
        for n, a, what in sorted(traps):
            f.write("%s\t%s\t%s\n" % (n, ("0x%06x" % a) if a else "?", what))
    lines = ["Standalone link report", "", "exe: %s (exit %d, %d unresolved after resolution)" % (EXE, rc2, len(unres2)),
             "first link unresolved: %d" % len(unres), "resolved: %s" % dict(report), "",
             "left unresolved (%d):" % len(left)] + ["   %s\t%s" % x for x in left] + \
            ["", "still unresolved after the second link (%d):" % len(unres2)] + ["   " + x for x in unres2] + \
            ["", "traps (%d): see traps.txt" % len(traps), "",
             "bound from a nearby comment (check each; %d):" % len(nearby_used)] +             ["   %s -> 0x%x (%s)" % x for x in nearby_used]
    open(os.path.join(OUT, "link_report.txt"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines[:6]))
    print("left unresolved:", len(left), "| after second link:", len(unres2), "| traps:", len(traps))


if __name__ == "__main__":
    main()
