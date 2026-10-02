"""Standalone link: build/standalone/halo_rebuilt.exe from the rewritten functions and committed sources, in one pass.

Run tools/msvc_build.py first (it compiles src/ into build/obj/). This script compiles and assembles the standalone
sources and links everything; it generates nothing and reads no retail file:
  standalone/loader.c, d3dx_compat.c, harness/x87_shims.c   the loader and runtime support
  standalone/image/*.asm, image/pieces.c                     the data image the loader copies to 0x63a000..
  standalone/generated/image_bindings.c, code_entries.asm    code pointers in the image -> C functions, and the
                                                             original address -> C function table
  standalone/globals.asm                                     the engine globals at their fixed original addresses
  standalone/bridges.asm                                     D3DXCreateEffect and the code_address_ thunks
  standalone/libs/*.def                                      import libraries for binkw32 / vorbisfile (delay-loaded)
Anything unresolved fails the link. When functions or globals are added, regenerate the committed sources:
  python tools/gen_link_sources.py      image_bindings.c, code_entries.asm, pieces.c
  python tools/update_globals.py        globals the failed link reported, from their address comments
CMakeLists.txt builds the same exe without Python.
Usage: python tools/gen_standalone_link.py [halo folder]"""
import os, re, sys, glob, json, subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness"))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import retail_guard as rg
import gen_link as gl

OUT = os.path.join(ROOT, "build", "standalone")
SA = os.path.join(ROOT, "standalone")
EXE = os.path.join(OUT, "halo_rebuilt.exe")
BASE = 0x10000000
HALO_FOLDER = sys.argv[1] if len(sys.argv) > 1 else None   # default: the one loader.c names
DXSDK = r"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)"
EXTRA_LIBS = ["d3d9.lib", "d3dx9.lib", "legacy_stdio_definitions.lib", "wininet.lib"]

# third-party DLLs in the Halo folder without an import library: made from standalone/libs/*.def and delay-loaded, so
# the DLL is loaded at the first call, from the folder the loader's SetDllDirectory names
THIRD_PARTY_DLLS = ["binkw32", "vorbisfile"]


def run(cmd, what):
    r = subprocess.run(cmd, capture_output=True, text=True, env=gl.env, errors="replace")
    if r.returncode:
        print(r.stdout[-3000:])
        raise SystemExit("%s failed" % what)
    return r


def compile_c(src, obj, includes=(), defines=()):
    run([gl.tool("cl"), "/nologo", "/c", "/GS-", "/O2", "/Fo" + obj] + ["/I" + i for i in includes] +
        ["/D" + d for d in defines] + [src], "compile " + src)
    return obj


def assemble(src, obj):
    run([gl.tool("ml"), "/nologo", "/c", "/coff", "/Fo" + obj, src], "assemble " + src)
    return obj


def third_party_import_libs():
    libs = []
    for name in THIRD_PARTY_DLLS:
        out = os.path.join(OUT, name + ".lib")
        run([gl.tool("lib"), "/nologo", "/machine:x86", "/def:" + os.path.join(SA, "libs", name + ".def"), "/out:" + out],
            "import library " + name)
        libs.append(out)
    return libs + ["delayimp.lib"] + ["/DELAYLOAD:%s.dll" % n for n in THIRD_PARTY_DLLS]


def main():
    rg.forbid_retail()
    os.makedirs(OUT, exist_ok=True)
    o = lambda name: os.path.join(OUT, name)
    folder = ['HALO_FOLDER="\\"%s\\""' % HALO_FOLDER.replace("\\", "\\\\")] if HALO_FOLDER else []
    extra = [compile_c(os.path.join(SA, "loader.c"), o("loader.obj"), [SA], folder),
             compile_c(os.path.join(SA, "d3dx_compat.c"), o("d3dx_compat.obj"), [SA, os.path.join(DXSDK, "Include")]),
             compile_c(os.path.join(ROOT, "harness", "x87_shims.c"), o("x87_shims.obj")),
             compile_c(os.path.join(SA, "image", "pieces.c"), o("pieces.obj"), [SA]),
             compile_c(os.path.join(SA, "generated", "image_bindings.c"), o("image_bindings.obj"))]
    for src in sorted(glob.glob(os.path.join(SA, "data", "*.c"))):   # the engine globals as C definitions
        extra.append(compile_c(src, o("data_%s.obj" % os.path.splitext(os.path.basename(src))[0])))
    for p in json.load(open(os.path.join(SA, "image", "pieces.json"))):
        extra.append(assemble(os.path.join(SA, "image", p["label"] + ".asm"), o("image_%s.obj" % p["label"])))
    extra += [assemble(os.path.join(SA, "globals.asm"), o("globals.obj")),
              assemble(os.path.join(SA, "generated", "code_entries.asm"), o("code_entries.obj")),
              assemble(os.path.join(SA, "bridges.asm"), o("bridges.obj"))]

    # only objects whose source still exists: a renamed or deleted .c leaves its old object behind
    objs = [x for x in glob.glob(os.path.join(ROOT, "build", "obj", "*", "*.obj"))
            if os.path.exists(os.path.join(ROOT, "src", os.path.basename(os.path.dirname(x)),
                                           os.path.splitext(os.path.basename(x))[0] + ".c"))] + extra
    rsp = o("objs.rsp")
    open(rsp, "w").write("\n".join('"%s"' % x for x in objs))
    cmd = [gl.tool("link"), "/nologo", "/MACHINE:X86", "/SUBSYSTEM:WINDOWS", "/FIXED", "/BASE:0x%x" % BASE,
           "/SAFESEH:NO", "/OPT:NOREF", "/OPT:NOICF", "/LARGEADDRESSAWARE:NO", "/NODEFAULTLIB:msvcrt.lib",
           "/LIBPATH:" + os.path.join(DXSDK, "Lib", "x86"), "/OUT:" + EXE, "/MAP:" + o("halo_rebuilt.map"), "@" + rsp] + \
          gl.SYS_LIBS + EXTRA_LIBS + third_party_import_libs() + ["libcmt.lib", "libvcruntime.lib", "libucrt.lib"]
    r = subprocess.run(cmd, capture_output=True, text=True, env=gl.env, errors="replace")
    open(o("link.log"), "w").write(r.stdout)
    unres = sorted(set(re.findall(r"unresolved external symbol (\S+)", r.stdout)))
    if r.returncode or unres:
        print(r.stdout[-4000:])
        print("%d unresolved. New globals: python tools/update_globals.py; new or renamed functions: "
              "python tools/gen_link_sources.py" % len(unres))
        raise SystemExit(1)
    print("linked %s (%d objects, 0 unresolved)" % (os.path.relpath(EXE, ROOT), len(objs)))


if __name__ == "__main__":
    main()
