import os, re
R = "C:\\Users\\Liam-\\halo-re\\"
os.makedirs(R + "standalone\\libs", exist_ok=True)
open(R + "standalone\\libs\\binkw32.def", "w", newline="\n").write(
    "; binkw32.dll (the Halo folder) -- the Bink functions the game calls, under the DLL's decorated export names.\n"
    "; lib /def makes each import symbol __Name@N and asks the DLL for _Name@N (types/bink.h declares them to match).\n"
    "LIBRARY binkw32.dll\nEXPORTS\n" + "".join("    %s\n" % n for n in (
        "_BinkClose@4", "_BinkCopyToBuffer@28", "_BinkDoFrame@4", "_BinkNextFrame@4", "_BinkOpen@8",
        "_BinkOpenDirectSound@4", "_BinkPause@8", "_BinkSetSoundSystem@8", "_BinkWait@4")))
open(R + "standalone\\libs\\vorbisfile.def", "w", newline="\n").write(
    "; vorbisfile.dll (the Halo folder) -- the Ogg Vorbis functions the game calls (cdecl, plain export names).\n"
    "LIBRARY vorbisfile.dll\nEXPORTS\n    ov_clear\n    ov_crosslap\n    ov_open_callbacks\n    ov_read\n")


def edit(rel, pairs, include=None):
    p = R + rel
    t = open(p, encoding="utf-8").read()
    for old, new in pairs:
        if isinstance(old, re.Pattern):
            t, n = old.subn(new, t)
            assert n >= 1, (rel, old.pattern[:60])
        else:
            assert t.count(old) == 1, (rel, old[:70])
            t = t.replace(old, new)
    if include and include not in t:
        i = t.index("#include")
        t = t[:i] + include + "\n" + t[i:]
    open(p, "w", encoding="utf-8", newline="\n").write(t)
    print("edited", rel)


BINK = re.compile(r"^extern[^;]*\bBink\w*\s*\([^;]*\);[^\n]*\n", re.M | re.S)
OV = re.compile(r"^extern[^;]*\bov_(?:clear|crosslap|open_callbacks|read)\s*\([^;]*\);[^\n]*\n", re.M | re.S)
edit("src\\main\\movie_play_bink.c", [(BINK, "")], include='#include "bink.h"')
edit("src\\sound\\sound_ogg_stream_open.c", [(OV, "")], include='#include "vorbisfile.h"')
edit("src\\sound\\sound_ogg_stream_read.c", [(OV, "")], include='#include "vorbisfile.h"')
edit("src\\sound\\sound_stream_decoder_close_slot.c", [(OV, "")], include='#include "vorbisfile.h"')

# the link: import libraries from the committed .def files, delay-loaded
p = R + "tools\\gen_standalone_link.py"
t = open(p, encoding="utf-8").read()
old = '''def link(objs, force):
    rsp = os.path.join(OUT, "objs.rsp")'''
assert t.count(old) == 1
t = t.replace(old, '''# third-party DLLs in the Halo folder without an import library: made from standalone/libs/*.def and delay-loaded, so
# the DLL is loaded at the first call, from the folder the loader's SetDllDirectory names
THIRD_PARTY_DLLS = ["binkw32", "vorbisfile"]


def third_party_import_libs():
    libs = []
    for name in THIRD_PARTY_DLLS:
        out = os.path.join(OUT, name + ".lib")
        r = subprocess.run([gl.tool("lib"), "/nologo", "/machine:x86", "/def:" + os.path.join(SA, "libs", name + ".def"),
                            "/out:" + out], capture_output=True, text=True, env=gl.env, errors="replace")
        if r.returncode:
            print(r.stdout[-2000:])
            raise SystemExit("import library failed: " + name)
        libs.append(out)
    return libs + ["delayimp.lib"] + ["/DELAYLOAD:%s.dll" % n for n in THIRD_PARTY_DLLS]


def link(objs, force):
    rsp = os.path.join(OUT, "objs.rsp")''')
old = '''          (["/FORCE:UNRESOLVED"] if force else []) + gl.SYS_LIBS + EXTRA_LIBS + ["libcmt.lib", "libvcruntime.lib", "libucrt.lib"]'''
assert t.count(old) == 1
t = t.replace(old, '''          (["/FORCE:UNRESOLVED"] if force else []) + gl.SYS_LIBS + EXTRA_LIBS + third_party_import_libs() + \\
          ["libcmt.lib", "libvcruntime.lib", "libucrt.lib"]''')
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("link edited")
