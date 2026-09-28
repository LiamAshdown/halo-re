"""src/gamespy/gamespy_api.h: the prototype of every GameSpy function the game calls, copied from its definition, so
callers include one header instead of declaring GameSpy themselves"""
import glob, json, os, re
R = "C:\\Users\\Liam-\\halo-re\\"
names = sorted(set(json.load(open(R + "scratchpad\\placeholder_map.json")).values()))
out, missing = [], []
for n in names:
    p = R + "src\\gamespy\\%s.c" % n
    if not os.path.exists(p):
        continue
    s = open(p, encoding="utf-8").read().split("\n#if 0")[0]
    body = re.sub(r"//[^\n]*", "", s)
    m = re.search(r"^([A-Za-z_][\w \t\*]*?\b%s\s*\([^;{]*?\))\s*\{" % re.escape(n), body, re.M | re.S)
    if not m:
        missing.append(n)
        continue
    proto = " ".join(m.group(1).split())
    a = re.search(r"address 0x0*([0-9a-f]+)", s)
    out.append("%s; // 0x%s" % (proto, a.group(1) if a else "?"))
hdr = """/* gamespy_api.h -- the GameSpy SDK functions the game calls (GT2, the server browser, query / reporting 2, CD key,
   NAT negotiation, HTTP, patching), with the prototypes of their definitions in src/gamespy/. Callers include this
   instead of declaring GameSpy functions themselves. Generated once from the definitions by
   scratchpad/gen_gamespy_api.py; keep it in step with them. */
#ifndef HALO_GAMESPY_API_H
#define HALO_GAMESPY_API_H

#include "gamespy.h"
#include "gt2.h"
#include "sb.h"
#include "ghttp.h"

""" + "\n".join(out) + "\n\n#endif\n"
open(R + "src\\gamespy\\gamespy_api.h", "w", encoding="utf-8", newline="\n").write(hdr)
print(len(out), "prototypes; no definition parsed for:", missing)
