"""Prints the offset of every field of a struct in types/, as the compiler lays it out (x86 cl, the build's
include path), so a raw offset in the C can be matched to a field without adding up sizes by hand.
Usage: python tools/struct_offsets.py <struct> [header, default tags.h] [more headers to include first]
  e.g. python tools/struct_offsets.py ScenarioStructureBSP
       python tools/struct_offsets.py device_data devices.h objects.h"""
import os, re, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness"))
import gen_link as gl

PRELUDE = ["crt.h", "win32.h", "tags.h", "memory.h", "math.h"]


def fields(header, name):
    t = open(os.path.join(ROOT, "types", header), encoding="utf-8", errors="replace").read()
    m = re.search(r"typedef struct %s \{(.*?)\n\} %s;" % (re.escape(name), re.escape(name)), t, re.S)
    if not m:
        raise SystemExit("no typedef struct %s { ... } %s; in types/%s" % (name, name, header))
    body = re.sub(r"//[^\n]*|/\*.*?\*/", "", m.group(1), flags=re.S)
    out = []
    for decl in body.split(";"):
        mm = re.search(r"(\w+)\s*(?:\[[^\]]*\])*\s*$", decl.strip())
        if mm and len(decl.split()) >= 2:
            out.append(mm.group(1))
    return out


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    name = sys.argv[1]
    header = sys.argv[2] if len(sys.argv) > 2 else "tags.h"
    extra = sys.argv[3:]
    names = fields(header, name)
    lines = ["#include <stdio.h>", "#include <stddef.h>", "#include <stdint.h>"]
    headers = []
    for h in PRELUDE + extra + [header]:
        if h not in headers:
            headers.append(h)
    lines += ['#include "%s"' % h for h in headers]
    lines += ["int main(void) {"]
    lines += ['    printf("0x%%03x  %s\\n", (unsigned)offsetof(%s, %s));' % (f, name, f) for f in names]
    lines += ['    printf("size 0x%%x\\n", (unsigned)sizeof(%s));' % name, "    return 0;", "}"]
    d = tempfile.mkdtemp()
    c, exe = os.path.join(d, "offsets.c"), os.path.join(d, "offsets.exe")
    open(c, "w").write("\n".join(lines) + "\n")
    r = subprocess.run([gl.tool("cl"), "/nologo", "/I" + os.path.join(ROOT, "types"),
                        "/I" + r"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include",
                        "/Fo" + d + "\\", "/Fe" + exe, c], capture_output=True, text=True, env=gl.env, errors="replace")
    if r.returncode:
        print(r.stdout[-2000:])
        raise SystemExit("compile failed")
    print(subprocess.run([exe], capture_output=True, text=True).stdout, end="")


if __name__ == "__main__":
    main()
