"""Standalone link: build/standalone/halo_rebuilt.exe from the rewritten functions and committed sources, in one pass.

Run tools/msvc_build.py first (it compiles src/ into build/obj/). This script compiles the standalone
sources and links everything; it generates nothing and reads no retail file:
  standalone/loader.cpp, d3dx_compat.cpp, harness/x87_shims.c   the loader and runtime support
  standalone/data/*.cpp                                      the engine globals and tables as extern "C" definitions
  standalone/bridges.cpp                                     D3DXCreateEffect and the code_address_ thunks
  standalone/libs/*.def                                      import libraries for binkw32 / vorbisfile (delay-loaded)
Anything unresolved fails the link. When functions or globals are added, regenerate the committed sources:
  python tools/update_globals.py        lists the globals the failed link reported (define them in standalone/data)
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
HALO_FOLDER = sys.argv[1] if len(sys.argv) > 1 else None   # default: the one loader.cpp names
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


def compile_cpp(src, obj, includes=(), defines=()):
    run([gl.tool("cl"), "/nologo", "/c", "/std:c++20", "/permissive-", "/GR-", "/GS-", "/O2", "/EHs-c-", "/Fo" + obj] +
        ["/I" + i for i in includes] + ["/D" + d for d in defines] + [src], "compile " + src)
    return obj


def compile_data_cpp(src, obj):
    """a standalone/data/*.cpp file: the game code's headers and flags (tools/msvc_build.py CFLAGS)"""
    run([gl.tool("cl"), "/nologo", "/c", "/std:c++20", "/permissive-", "/GR-", "/W3", "/Od", "/GS-", "/Oy-", "/Gy", "/wd4996",
         "/I" + os.path.join(ROOT, "types"), "/I" + os.path.join(DXSDK, "Include"),
         "/FI" + os.path.join(ROOT, "harness", "msvc_compat.h"), "/Fo" + obj, src], "compile " + src)
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
    extra = [compile_cpp(os.path.join(SA, "loader.cpp"), o("loader.obj"), [SA], folder),
             compile_cpp(os.path.join(SA, "d3dx_compat.cpp"), o("d3dx_compat.obj"), [SA, os.path.join(DXSDK, "Include")]),
             compile_c(os.path.join(ROOT, "harness", "x87_shims.c"), o("x87_shims.obj"))]
    for c in sorted(glob.glob(os.path.join(SA, "data", "*.cpp"))):   # the engine globals and tables, one file per slice
        extra.append(compile_data_cpp(c, o("data_" + os.path.splitext(os.path.basename(c))[0] + ".obj")))
    extra += [compile_cpp(os.path.join(SA, "bridges.cpp"), o("bridges.obj"))]

    # only objects whose source still exists: a renamed or deleted .c leaves its old object behind
    objs = [x for x in glob.glob(os.path.join(ROOT, "build", "obj", "*", "*.obj"))
            if any(os.path.exists(os.path.join(ROOT, "src", os.path.basename(os.path.dirname(x)),
                                               os.path.splitext(os.path.basename(x))[0] + ext)) for ext in (".c", ".cpp"))] + extra
    rsp = o("objs.rsp")
    open(rsp, "w").write("\n".join('"%s"' % x for x in objs))
    cmd = [gl.tool("link"), "/nologo", "/MACHINE:X86", "/SUBSYSTEM:WINDOWS", "/FIXED", "/BASE:0x%x" % BASE,
           "/SAFESEH:NO", "/OPT:NOREF", "/OPT:NOICF", "/LARGEADDRESSAWARE:NO", "/NODEFAULTLIB:msvcrt.lib", "/NODEFAULTLIB:libcpmt.lib",
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
