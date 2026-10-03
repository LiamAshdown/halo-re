"""Former build step of CMakeLists.txt (HALO_REGENERATE): there are no generated link sources any more.

standalone/generated/code_entries.c and image_bindings.c were removed together with the cp_trap_ stubs and the
halo_code_<address> aliases; the data tables in standalone/data/*.cpp name the real functions. Engine globals are
extern "C" definitions in standalone/data/*.cpp, added by hand (tools/update_globals.py lists what a link left
unresolved). This script remains so older command lines keep working: it only touches the stamp file.
  python tools/cmake_regen.py <file listing the objects, one per line> <stamp file>"""
import sys


def main():
    stamp = sys.argv[2] if len(sys.argv) > 2 else None
    if stamp:
        with open(stamp, "w") as f:
            f.write("nothing to regenerate\n")


if __name__ == "__main__":
    main()
