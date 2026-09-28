p = "C:\\Users\\Liam-\\halo-re\\tools\\gen_standalone_link.py"
t = open(p, encoding="utf-8").read()

old = '''def code_pointer_asm():'''
assert old in t
t = t.replace(old, '''def defines_function(e):
    """whether a code entry's object defines its function: fragment files (a range inside another function whose C
    covers it, e.g. object_update_functions_clone_4f93b0) carry an address header but no code, and an entry for one
    would redirect a jump to its own original address forever (2026-09-28)"""
    obj = os.path.join(ROOT, "build", "obj", e["module"], e["c_symbol"] + ".obj")
    if not os.path.exists(obj):
        return False
    data = open(obj, "rb").read()
    name = e["c_symbol"].encode()
    return any(p + name + s in data for p in (b"_", b"@") for s in (b"\\0", b"@"))


def code_pointer_asm():''', 1)

old = '''    for e in json.load(open(os.path.join(OUT, "code_entries.json"))):
        if not os.path.exists(os.path.join(ROOT, "build", "obj", e["module"], e["c_symbol"] + ".obj")):
            continue
        n = e["c_symbol"]'''
assert old in t
t = t.replace(old, '''    for e in json.load(open(os.path.join(OUT, "code_entries.json"))):
        if not defines_function(e):
            continue
        n = e["c_symbol"]''', 1)

old = '''    rewritten_at = {e["addr"]: e["c_symbol"] for e in json.load(open(os.path.join(OUT, "code_entries.json")))
                    if os.path.exists(os.path.join(ROOT, "build", "obj", e["module"], e["c_symbol"] + ".obj"))}'''
assert old in t
t = t.replace(old, '''    rewritten_at = {e["addr"]: e["c_symbol"] for e in json.load(open(os.path.join(OUT, "code_entries.json")))
                    if defines_function(e)}''', 1)
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
