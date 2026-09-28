p = r"C:\Users\Liam-\halo-re\tools\gen_standalone.py"
t = open(p, encoding="utf-8").read()
start = t.index("def main():")
end = t.index("    # ---- report")
body = t[start:end]

# the retail part: everything from reading the exe up to the code_pointers.json dump
retail = body[len("def main():\n    os.makedirs(OUT, exist_ok=True)\n"):body.index('    json.dump(pointers, open(os.path.join(OUT, "code_pointers.json")')]
retail = retail.replace('    json.dump(layout, open(os.path.join(OUT, "layout.json"), "w"), indent=1)\n', '')
retail = retail.replace('    json.dump(slots, open(os.path.join(OUT, "imports.json"), "w"), indent=1)\n', '')
retail = retail.replace('    pointers = []\n', '    pointer_slots = []\n')
old_tail = '''            entry = {"slot": va, "target": v, "name": (r or f)["name"], "module": module}
            if r:
                entry["c_symbol"] = r["name"]
            pointers.append(entry)
'''
assert old_tail in retail
retail = retail.replace(old_tail, '''            # the retail name (the rewrite's name only for an entry Ghidra never listed); the C symbol is attached at
            # build time from src/, so renaming or adding C never needs the retail image
            pointer_slots.append({"slot": va, "target": v, "name": (f or r)["name"], "module": module})
''')

new = '''def extract_retail():
    """reads bin/halo.exe and out/functions.json: writes halo_image.bin and refreshes the frozen build inputs"""
''' + retail + '''
    rg.write_frozen("layout.json", layout)
    rg.write_frozen("imports.json", slots)
    rg.write_frozen("code_pointer_slots.json", pointer_slots)
    rg.write_frozen("game_crt.json", {x["name"].lstrip("_"): int(x["addr"], 16) for x in funcs.values()
                                      if x.get("lib") or x.get("fid")})


def main():
    os.makedirs(OUT, exist_ok=True)
    if not rg.NO_RETAIL:
        extract_retail()
    # from here on only committed files: standalone/frozen/ and src/
    layout = rg.read_frozen("layout.json")
    slots = rg.read_frozen("imports.json")
    json.dump(layout, open(os.path.join(OUT, "layout.json"), "w"), indent=1)
    json.dump(slots, open(os.path.join(OUT, "imports.json"), "w"), indent=1)
    rewritten = rewritten_functions()
    pointers = []
    for p in rg.read_frozen("code_pointer_slots.json"):
        entry = dict(p)
        r = rewritten.get(p["target"])
        if r:
            entry["name"] = r["name"]
            entry["c_symbol"] = r["name"]
        pointers.append(entry)
    pointers.sort(key=lambda p: p["slot"])
    json.dump(pointers, open(os.path.join(OUT, "code_pointers.json"), "w"), indent=1)
    # every rewritten function by its original address: the loader redirects a jump into original .text (a constant
    # function address a stable rewrite still passes, e.g. structure_picked_polygon_draw's callbacks) to its C.
    entries = [{"addr": a, "c_symbol": r["name"], "module": r["module"]} for a, r in sorted(rewritten.items())]
    json.dump(entries, open(os.path.join(OUT, "code_entries.json"), "w"), indent=1)
    piece = {p["name"]: p for p in layout["pieces"]}
    rdata = {"rsize": piece[".rdata"]["raw"], "va": piece[".rdata"]["va"]}
    data = {"rsize": piece[".data"]["raw"], "vsize": piece[".data"]["virtual"], "va": piece[".data"]["va"]}

'''
t = t[:start] + new + t[end:]
t = t.replace('import bisect, os, re, sys, json, glob, struct, collections\n',
              'import bisect, os, re, sys, json, glob, struct, collections\n\n'
              'sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))\n'
              'import retail_guard as rg   # HALO_NO_RETAIL=1: build from standalone/frozen/ only\n', 1)
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
