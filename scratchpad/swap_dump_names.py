import os, subprocess
R = "C:\\Users\\Liam-\\halo-re\\"
G = R + "src\\gamespy\\"


def git(*a):
    subprocess.run(["git", "-C", R] + list(a), check=True)


git("mv", "src/gamespy/gt2SetSendDump.c", "src/gamespy/gt2SetReceiveDump_tmp.c")
git("mv", "src/gamespy/gt2SetReceiveDump.c", "src/gamespy/gt2SetSendDump.c")
git("mv", "src/gamespy/gt2SetReceiveDump_tmp.c", "src/gamespy/gt2SetReceiveDump.c")

# 0x614850 (+0x28): the receive dump
p = G + "gt2SetReceiveDump.c"
t = open(p, encoding="utf-8").read()
t = t.replace("// gt2SetSendDump  (GameSpy SDK in halo.exe; no C existed)", "// gt2SetReceiveDump  (GameSpy SDK in halo.exe; no C existed)")
t = t.replace("// WRITTEN 2026-09-28 from objdump 0x614850..0x61485b: stores the dump callback at socket +0x28.",
              "// WRITTEN 2026-09-28 from objdump 0x614850..0x61485b: stores the receive dump callback at socket +0x28\n"
              "// (GTI2Socket.receiveDumpCallback). RENAMED 2026-09-28: this file was named gt2SetSendDump, swapped with 0x61e550.")
t = t.replace("void gt2SetSendDump(void *socket, void *callback)", "void gt2SetReceiveDump(void *socket, void *callback)")
open(p, "w", encoding="utf-8", newline="\n").write(t)

# 0x61e550 (+0x24): the send dump, folded (ICF) with the query engine's public-ip setter
p = G + "gt2SetSendDump.c"
t = open(p, encoding="utf-8").read()
t = t.replace("// gt2SetReceiveDump  (GameSpy SDK in halo.exe; no C existed)", "// gt2SetSendDump  (GameSpy SDK in halo.exe; no C existed)")
t = t.replace("// WRITTEN 2026-09-28 from objdump 0x61e550..0x61e55b: stores the receive dump callback at socket +0x24.",
              "// WRITTEN 2026-09-28 from objdump 0x61e550..0x61e55b: stores the send dump callback at socket +0x24\n"
              "// (GTI2Socket.sendDumpCallback). The linker folded the identical SBQueryEngineSetPublicIP (engine +0x24) into it,\n"
              "// so ServerBrowserNew calls this too. RENAMED 2026-09-28: this file was named gt2SetReceiveDump, swapped with\n"
              "// 0x614850.")
t = t.replace("void gt2SetReceiveDump(void *socket, void *callback)", "void gt2SetSendDump(void *socket, void *callback)")
open(p, "w", encoding="utf-8", newline="\n").write(t)

for name in ("ServerBrowserNew.c", "sb.h"):
    p = G + name
    t = open(p, encoding="utf-8").read()
    t = t.replace("gt2SetReceiveDump", "gt2SetSendDump")
    open(p, "w", encoding="utf-8", newline="\n").write(t)
# stale objects of the old names would otherwise be linked under the swapped names until rebuilt
for n in ("gt2SetSendDump.obj", "gt2SetReceiveDump.obj"):
    o = R + "build\\obj\\gamespy\\" + n
    if os.path.exists(o):
        os.remove(o)
print("ok")
