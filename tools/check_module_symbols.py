"""C++ conversion gate: a converted module must define exactly the same external symbols as before.

Every other module, the generated link tables (standalone/generated/*), the code-pointer slots and standalone/data/*.c
refer to the engine's C symbol names, so a module may be restructured freely (classes, namespaces, merged files) as long
as the set of external symbols its objects define does not change (C linkage shims keep the names).

  python tools/check_module_symbols.py --baseline-from build/cxx [module ...]
        writes symbols/exports/<module>.txt from the linked objects of an existing CMake build dir (build/cxx/halo_game.dir)
  python tools/check_module_symbols.py check <module> [module ...]
        compiles src/<module>/*.c *.cpp with the project's flags into a scratch dir and compares the defined external
        symbols with symbols/exports/<module>.txt; prints missing and extra symbols; exit code 1 on any difference
Symbols are the decorated names dumpbin prints (_name, _name@8, ?mangled@@...). Symbols the converted code adds in namespace halo:: (mangled `?...@halo@@...`) are allowed; any other added symbol is an error.
Duplicated definitions across files are
not detected here (the link does that)."""
import glob, os, re, subprocess, sys, tempfile
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness"))
import gen_link as gl

EXPORTS = os.path.join(ROOT, "symbols", "exports")
DXSDK = r"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include"
SYM = re.compile(r"^\s*[0-9A-F]+ [0-9A-F]+ (SECT[0-9A-F]+|ABS)\s+\S+\s+(\(\)\s+)?External\s+\|\s+(\S+)", re.M)


def module_files(module):
    out = []
    for ext in ("*.c", "*.cpp"):
        out += glob.glob(os.path.join(ROOT, "src", module, ext))
    return sorted(out)


def modules():
    return sorted(d for d in os.listdir(os.path.join(ROOT, "src")) if os.path.isdir(os.path.join(ROOT, "src", d)))


def defined_symbols(obj):
    r = subprocess.run([gl.tool("dumpbin"), "/nologo", "/symbols", obj], capture_output=True, text=True, env=gl.env,
                       errors="replace")
    return {m.group(3) for m in SYM.finditer(r.stdout)}


def compile_flags(src):
    flags = ["/nologo", "/c", "/W3", "/Od", "/GS-", "/Oy-", "/Gy", "/wd4996", "/I", os.path.join(ROOT, "include"),
             "/I", os.path.join(ROOT, "types"), "/I", DXSDK, "/FI" + os.path.join(ROOT, "harness", "msvc_compat.h")]
    if os.sep + "gamespy" + os.sep in src:
        return flags + ["/TC"]
    return flags + ["/std:c++20", "/permissive-", "/GR-"] + (["/TP"] if src.endswith(".c") else [])


def compile_symbols(src, outdir):
    obj = os.path.join(outdir, os.path.basename(os.path.dirname(src)) + "_" + os.path.basename(src) + ".obj")
    r = subprocess.run([gl.tool("cl")] + compile_flags(src) + ["/Fo" + obj, src], capture_output=True, text=True,
                       env=gl.env, errors="replace", cwd=ROOT)
    if r.returncode or not os.path.exists(obj):
        return src, None, [l for l in (r.stdout + r.stderr).splitlines() if "error" in l][:3]
    return src, defined_symbols(obj), []


def baseline_from(builddir, mods):
    objdir = os.path.join(builddir, "halo_game.dir", "Release")
    os.makedirs(EXPORTS, exist_ok=True)
    for m in mods:
        files = module_files(m)
        syms = set()
        objs = [os.path.join(objdir, os.path.splitext(os.path.basename(f))[0] + ".obj") for f in files]
        missing = [o for o in objs if not os.path.exists(o)]
        if missing:
            print("%s: %d objects not found in %s (e.g. %s)" % (m, len(missing), objdir, os.path.basename(missing[0])))
        with ThreadPoolExecutor(12) as ex:
            for s in ex.map(defined_symbols, [o for o in objs if os.path.exists(o)]):
                syms |= s
        with open(os.path.join(EXPORTS, m + ".txt"), "w", newline="\n") as f:
            f.write("\n".join(sorted(syms)) + "\n")
        print("%s: %d symbols from %d files" % (m, len(syms), len(files)))


def check(mods):
    bad = 0
    for m in mods:
        base_path = os.path.join(EXPORTS, m + ".txt")
        if not os.path.exists(base_path):
            print("%s: no baseline %s" % (m, base_path))
            bad += 1
            continue
        base = {l.strip() for l in open(base_path) if l.strip()}
        syms, errors = set(), []
        with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(12) as ex:
            for src, s, err in ex.map(lambda f: compile_symbols(f, tmp), module_files(m)):
                if s is None:
                    errors.append((src, err))
                else:
                    syms |= s
        for src, err in errors:
            print("%s: does not compile: %s" % (os.path.relpath(src, ROOT), "; ".join(err)[:200]))
        missing = sorted(base - syms)
        new = syms - base
        mangled = sorted(s for s in new if s.startswith("?") and "@halo@@" in s)   # new C++ API in namespace halo::
        extra = sorted(new - set(mangled))
        print("%s: %d baseline symbols, %d now, %d missing, %d extra, %d new halo:: C++ symbols (allowed), %d files failing" %
              (m, len(base), len(syms), len(missing), len(extra), len(mangled), len(errors)))
        for s in missing[:40]:
            print("   missing", s)
        for s in extra[:40]:
            print("   extra  ", s)
        bad += bool(missing or extra or errors)
    return bad


def main():
    a = sys.argv[1:]
    if a and a[0] == "--baseline-from":
        baseline_from(a[1], a[2:] or [m for m in modules() if m != "gamespy"])
    elif a and a[0] == "check":
        sys.exit(1 if check(a[1:] or [m for m in modules() if m != "gamespy"]) else 0)
    else:
        print(__doc__)
        sys.exit(2)


if __name__ == "__main__":
    main()
