"""Seed modules.json from out/functions.json:
  - lib: FID-identified library code, thunks, externals, plus D3DX/CRT string heuristics
  - module seeds from OpenSauce names (prefix -> module) and CEA candidate matches
modules.json: { "addr6hex": {"module": str, "confidence": float, "evidence": str} }
Later phases overwrite entries with higher confidence only."""
import json, os, re, sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
funcs = json.load(open(os.path.join(ROOT, "out", "functions.json")))
path = os.path.join(ROOT, "modules.json")
mods = json.load(open(path)) if os.path.exists(path) else {}

# OpenSauce/CEA name prefix -> module. CEA's own directory names are the canonical module list.
PREFIX = {
    "actor": "ai", "ai": "ai", "prop": "ai", "swarm": "ai", "encounter": "ai", "squad": "ai",
    "object": "objects", "objects": "objects", "damage": "objects", "attachment": "objects",
    "unit": "units", "biped": "units", "vehicle": "units", "player": "game", "players": "game",
    "weapon": "items", "item": "items", "equipment": "items", "projectile": "objects", "device": "devices",
    "effect": "effects", "effects": "effects", "particle": "effects", "contrail": "effects", "decal": "effects", "light": "effects",
    "physics": "physics", "collision": "physics", "havok": "physics",
    "game": "game", "scenario": "scenario", "hs": "hs", "script": "hs", "hud": "interface", "ui": "interface", "widget": "interface", "menu": "interface",
    "rasterizer": "rasterizer", "shader": "rasterizer", "render": "render", "camera": "camera", "cinematic": "cutscene",
    "sound": "sound", "input": "input", "network": "networking", "net": "networking", "server": "networking", "client": "networking",
    "cache": "cache", "tag": "cache", "tags": "cache", "saved": "saved_games", "main": "main", "shell": "shell",
    "memory": "memory", "data": "memory", "math": "math", "matrix": "math", "vector": "math", "bitmap": "bitmaps", "model": "models",
    "structure": "structures", "bsp": "structures", "text": "text", "font": "text", "console": "interface", "cseries": "cseries",
    "d3d": "lib", "crt": "lib",
}
def set_mod(addr, module, conf, ev):
    cur = mods.get(addr)
    if cur is None or cur["confidence"] < conf:
        mods[addr] = {"module": module, "confidence": conf, "evidence": ev}

c = Counter()
for f in funcs:
    a = f["addr"][-6:]
    if f["lib"]:
        set_mod(a, "lib", 1.0, "fid" if f["fid"] else ("thunk" if f["thunk"] else "external")); c["lib"] += 1; continue
    name = f["name"]
    if not name.startswith(("FUN_", "thunk_")):
        p = name.split("_")[0].lower()
        if p in PREFIX: set_mod(a, PREFIX[p], 0.8, "name-prefix:" + name); c["name"] += 1; continue
    # D3DX / CRT statically linked code: recognizable strings
    if any(s.startswith(("ID3DXEffect", "D3DX", "ID3DX")) for s in f["strings"]):
        set_mod(a, "lib", 0.9, "d3dx-string"); c["d3dx"] += 1; continue
json.dump(dict(sorted(mods.items())), open(path, "w"), indent=0)
print("modules.json:", len(mods), "entries;", dict(c), "; unassigned:", len(funcs) - len(mods))
print(Counter(v["module"] for v in mods.values()).most_common())
