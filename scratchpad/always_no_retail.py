import re
R = "C:\\Users\\Liam-\\halo-re\\tools\\"


def edit(name, pairs):
    p = R + name
    t = open(p, encoding="utf-8").read()
    for old, new in pairs:
        assert t.count(old) == 1, (name, old[:70])
        t = t.replace(old, new)
    open(p, "w", encoding="utf-8", newline="\n").write(t)
    print("edited", name)


# ---- link tool: guard always on; ret bytes only from the frozen table
p = R + "gen_standalone_link.py"
t = open(p, encoding="utf-8").read()
i = t.index("def original_ret_bytes(address):")
j = t.index("def stdcall_definitions():")
t = t[:i] + '''def original_ret_bytes(address):
    """bytes a function at an original address pops on return (the immediate of its first ret), None if not known.
    Answers come from the committed standalone/frozen/code_address_ret.json (tools/freeze_retail_inputs.py)."""
    if not os.path.exists(rg.frozen_path("code_address_ret.json")):
        return None
    return rg.read_frozen("code_address_ret.json").get("0x%06x" % address)


''' + t[j:]
t = t.replace("import retail_guard as rg   # HALO_NO_RETAIL=1: build from standalone/frozen/ only",
              "import retail_guard as rg   # the build reads standalone/frozen/, never the retail binary")
old = '''def main():
    os.makedirs(OUT, exist_ok=True)
    write_tables()'''
assert t.count(old) == 1
t = t.replace(old, '''def main():
    rg.forbid_retail()
    os.makedirs(OUT, exist_ok=True)
    write_tables()''')
t = t.replace('left.append((s, "code address: original function end not found")); continue',
              'left.append((s, "code address: no ret byte count in standalone/frozen/code_address_ret.json "\n'
              '                                "(tools/freeze_retail_inputs.py)")); continue')
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("edited gen_standalone_link.py")

# ---- image source generator (maintenance): pe_layout now lives in the freeze tool
edit("gen_image_source.py", [
    ("Usage: python tools/gen_image_source.py   (reads bin/halo.exe; refuses under HALO_NO_RETAIL=1)",
     "Usage: python tools/gen_image_source.py   (a maintenance tool like tools/freeze_retail_inputs.py: reads\n"
     "bin/halo.exe; never part of a build)"),
    ("from gen_standalone import EXE, pe_layout", "from freeze_retail_inputs import EXE, pe_layout"),
    ('''    if rg.NO_RETAIL:
        raise SystemExit("gen_image_source.py reads bin/halo.exe; run it without HALO_NO_RETAIL")
''', ""),
])

# ---- the compile step never reads retail data either
edit("msvc_build.py", [
    ('''def main():
    args = sys.argv[1:]; jobs = os.cpu_count() or 4''', '''def main():
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import retail_guard; retail_guard.forbid_retail()   # the build never reads the retail binary
    args = sys.argv[1:]; jobs = os.cpu_count() or 4'''),
])
