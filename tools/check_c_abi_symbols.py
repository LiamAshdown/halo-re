"""C ABI gate for a converted module: the C-linkage symbols must be exactly the baseline's; new C++ symbols are
allowed only inside namespace halo (or as header-library template instances such as glm).

tools/check_module_symbols.py compares EVERY external symbol, so a converted module always shows "extra" symbols: the
mangled halo::<module>:: functions behind the C shims, and the COMDAT literal pools (__real@..., __xmm@...) whose set
changes whenever the arithmetic moves between files. This check reads the same baseline and the same objects (it reuses
that tool's compile step) and classifies the difference:

  C linkage   names without '?' that are not compiler literal pools: must match the baseline exactly (this is what the
              link tables, code-pointer slots, standalone/data and the unconverted modules use)
  allowed     '?...@halo@@...' (namespace halo), '?...@glm@@...' and '?...@std@@...' template instances,
              literal pools (__real@, __xmm@, __mask@, ??_C@ strings), __Avx2WmemEnabledWeakValue (CRT header)
  foreign     any other mangled name: a function that lost its extern "C" or a global C++ helper; an error

  python tools/check_c_abi_symbols.py <module> [module ...]      exit code 1 on any C-linkage difference or foreign name
"""
import os, re, sys, tempfile
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import check_module_symbols as cms  # noqa: E402

LITERAL = re.compile(r"^(__real@|__xmm@|__mask@|__ymm@|\?\?_C@|__Avx2WmemEnabledWeakValue$|__fltused$)")
ALLOWED_CXX = re.compile(r"@(halo|glm|std)@@")


def classify(syms):
    c, cxx, foreign = set(), set(), set()
    for s in syms:
        if LITERAL.match(s):
            continue
        if s.startswith("?"):
            (cxx if ALLOWED_CXX.search(s) else foreign).add(s)
        else:
            c.add(s)
    return c, cxx, foreign


def check(module):
    base_path = os.path.join(cms.EXPORTS, module + ".txt")
    base = {l.strip() for l in open(base_path) if l.strip()}
    base_c, _, base_foreign = classify(base)
    syms, errors = set(), []
    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(12) as ex:
        for src, s, err in ex.map(lambda f: cms.compile_symbols(f, tmp), cms.module_files(module)):
            if s is None:
                errors.append((src, err))
            else:
                syms |= s
    now_c, now_cxx, now_foreign = classify(syms)
    missing, extra = sorted(base_c - now_c), sorted(now_c - base_c)
    foreign = sorted(now_foreign - base_foreign)
    for src, err in errors:
        print("%s: does not compile: %s" % (os.path.relpath(src, ROOT), "; ".join(err)[:200]))
    print("%s: C linkage %d baseline, %d now: %d missing, %d extra; C++ in namespace halo/glm/std: %d; "
          "foreign C++: %d; files failing: %d" % (module, len(base_c), len(now_c), len(missing), len(extra),
                                                   len(now_cxx), len(foreign), len(errors)))
    for s in missing: print("   missing", s)
    for s in extra: print("   extra  ", s)
    for s in foreign: print("   foreign", s)
    ok = not (missing or extra or foreign or errors)
    print("C ABI IDENTICAL" if ok else "C ABI DIFFERENT")
    return ok


def main():
    mods = sys.argv[1:]
    if not mods:
        print(__doc__); sys.exit(2)
    sys.exit(0 if all([check(m) for m in mods]) else 1)


if __name__ == "__main__":
    main()
