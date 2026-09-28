"""the exact export names the game uses from the third-party DLLs in the Halo folder (read only)"""
import subprocess, sys
sys.path.insert(0, "C:\\Users\\Liam-\\halo-re\\harness")
import gen_link as gl
HALO = "C:\\Program Files (x86)\\Microsoft Games\\Halo\\"
WANT = ("BinkClose", "BinkCopyToBuffer", "BinkDoFrame", "BinkNextFrame", "BinkOpen", "BinkOpenDirectSound", "BinkPause",
        "BinkSetSoundSystem", "BinkWait", "ov_clear", "ov_crosslap", "ov_open_callbacks", "ov_read")
for d in ("binkw32.dll", "vorbisfile.dll"):
    r = subprocess.run([gl.tool("dumpbin"), "/nologo", "/exports", HALO + d], capture_output=True, text=True, env=gl.env)
    names = [l.split()[-1] for l in r.stdout.splitlines() if len(l.split()) >= 4 and l.split()[0].isdigit()]
    print(d, len(names), "exports;", [n for n in names if n.lstrip("_").split("@")[0] in WANT])
