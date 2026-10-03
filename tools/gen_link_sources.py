"""Maintenance tool: lists the engine functions that the standalone data tables point at.

The standalone exe links in one pass from committed files only (tools/gen_standalone_link.py, or CMakeLists.txt). The
old generated link sources (code_entries.c, image_bindings.c, the cp_trap_ stubs) are gone: the tables in
standalone/data/*.cpp name the real functions through standalone/data/code_refs.hpp, and an entry whose target is not
part of the game is nullptr. This tool prints the functions code_refs.hpp declares: the worklist of names that
still have only a C-linkage definition (the link is the check that each one resolves).
Usage: python tools/gen_link_sources.py"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SA = os.path.join(ROOT, "standalone")
REFS = os.path.join(SA, "data", "code_refs.hpp")


def declared_functions():
    """function names declared in code_refs.hpp"""
    text = open(REFS, encoding="utf-8").read()
    return sorted(set(re.findall(r"^extern\s+\w+\s+(?:__\w+\s+)?(\w+)\s*\(", text, re.M)))


def main():
    declared = declared_functions()
    print("code_refs.hpp: %d functions the data tables point at (the link resolves them)" % len(declared))
    for n in declared:
        print("  " + n)
    return 0


if __name__ == "__main__":
    sys.exit(main())
