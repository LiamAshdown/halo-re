"""Real 32-bit build of src/ with MSVC (x86 cl from Visual Studio 2022), one object per function file.
Objects go to build/obj/<module>/<name>.obj; errors to build/msvc_errors.txt; summary on stdout.
Usage: python tools/msvc_build.py [module ...]   (default: all modules)   [-j N]"""
import os, sys, subprocess, glob, json, re, shutil
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VCVARS = r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
CACHE = os.path.join(ROOT, "build", "msvc_env.json")
# /TC C, /W3, /Zi-free, /Od keeps a 1:1 shape for debugging in the harness; /GS- /Oy- like a 2004 build; /J not used (char is signed)
DXSDK_INCLUDE = r"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include"
CFLAGS = ["/nologo", "/c", "/TP", "/std:c++20", "/permissive-", "/GR-", "/W3", "/Od", "/GS-", "/Oy-", "/Gy", "/wd4996", "/I", os.path.join(ROOT, "include"), "/I", os.path.join(ROOT, "types"),
          "/I", DXSDK_INCLUDE,
          "/FI" + os.path.join(ROOT, "harness", "msvc_compat.h")]

# GameSpy stays vendored C (docs/CPP_ARCHITECTURE.md): compiled as C, everything else as C++20
CFLAGS_C = [f for f in CFLAGS if f not in ("/std:c++20", "/permissive-", "/GR-")]
CFLAGS_C[CFLAGS_C.index("/TP")] = "/TC"

def msvc_env():
    if os.path.exists(CACHE): return json.load(open(CACHE))
    out = subprocess.run(f'cmd /s /c ""{VCVARS}" x86 >nul && set"', capture_output=True, text=True, shell=True).stdout
    env = dict(l.split("=", 1) for l in out.splitlines() if "=" in l and not l.startswith("="))
    os.makedirs(os.path.dirname(CACHE), exist_ok=True); json.dump(env, open(CACHE, "w"))
    return env

def main():
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import retail_guard; retail_guard.forbid_retail()   # the build never reads the retail binary
    args = sys.argv[1:]; jobs = os.cpu_count() or 4
    if "-j" in args: i = args.index("-j"); jobs = int(args[i + 1]); del args[i:i + 2]
    env = msvc_env()
    cl = shutil.which("cl", path=env.get("PATH") or env.get("Path"))
    mods = args or sorted(d for d in os.listdir(os.path.join(ROOT, "src")) if os.path.isdir(os.path.join(ROOT, "src", d)))
    files = [f for m in mods for ext in ("*.c", "*.cpp") for f in sorted(glob.glob(os.path.join(ROOT, "src", m, ext)))]
    def build(c):
        mod = os.path.basename(os.path.dirname(c)); od = os.path.join(ROOT, "build", "obj", mod); os.makedirs(od, exist_ok=True)
        obj = os.path.join(od, os.path.splitext(os.path.basename(c))[0] + ".obj")
        r = subprocess.run([cl] + (CFLAGS_C if mod == "gamespy" else CFLAGS) + ["/Fo" + obj, c], capture_output=True, text=True, env=env, errors="replace")
        lines = [l for l in r.stdout.splitlines() if re.search(r": (fatal )?error |: warning C4(013|020|029|047|133|024|028|087|113|716)", l)]
        return c, r.returncode, lines
    fails, warn_files, out = 0, 0, []
    with ThreadPoolExecutor(jobs) as ex:
        for c, rc, lines in ex.map(build, files):
            rel = os.path.relpath(c, ROOT)
            if rc != 0: fails += 1; out.append("FAIL " + rel)
            elif lines: warn_files += 1; out.append("WARN " + rel)
            out += ["    " + l.strip() for l in lines[:12]]
    open(os.path.join(ROOT, "build", "msvc_errors.txt"), "w", encoding="utf-8").write("\n".join(out) + "\n")
    print(f"msvc_build: {len(files) - fails} ok, {fails} failed, {warn_files} with significant warnings (build/msvc_errors.txt)")

if __name__ == "__main__": main()
