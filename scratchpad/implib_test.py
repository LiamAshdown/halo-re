"""what symbol and import name does lib /def produce for the Bink export names? (experiment)"""
import os, subprocess, sys
sys.path.insert(0, "C:\\Users\\Liam-\\halo-re\\harness")
import gen_link as gl
D = "C:\\Users\\Liam-\\halo-re\\scratchpad\\implib\\"
os.makedirs(D, exist_ok=True)
for variant, body in (("a", "EXPORTS\n    BinkOpen@8\n"), ("b", "EXPORTS\n    _BinkOpen@8\n"),
                      ("c", "EXPORTS\n    BinkOpen@8 == _BinkOpen@8\n")):
    open(D + variant + ".def", "w").write("LIBRARY binkw32.dll\n" + body)
    r = subprocess.run([gl.tool("lib"), "/nologo", "/machine:x86", "/def:" + D + variant + ".def", "/out:" + D + variant + ".lib"],
                       capture_output=True, text=True, env=gl.env)
    h = subprocess.run([gl.tool("dumpbin"), "/nologo", "/headers", D + variant + ".lib"], capture_output=True, text=True, env=gl.env)
    lines = [l.strip() for l in h.stdout.splitlines() if l.strip().startswith(("Symbol name", "Name type", "Name   ", "Export name", "Name  "))]
    print(variant, r.stdout.strip()[:200], lines[:6])
