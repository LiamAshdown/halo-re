R = "C:\\Users\\Liam-\\halo-re\\tools\\verify_image_source.py"
t = open(R, encoding="utf-8").read()
old = '''    # what the loader's table patches: slot -> our address
    count = struct.unpack("<I", read(symbols["_standalone_code_pointer_count"], 4))[0]
    table = read(symbols["_standalone_code_pointers"], 8 * count)
    patched = {}
    for i in range(count):
        slot, target = struct.unpack_from("<II", table, 8 * i)
        patched[slot] = target
'''
assert old in t
t = t.replace(old, '''    # what each code-pointer slot must hold: the address of the C rewrite of its target, found in the map by name
    # (cdecl _name, __stdcall _name@N, __fastcall @name@N), or the named trap of a library pointer without C
    by_name = {}
    for s, a in symbols.items():
        m = re.fullmatch(r"[_@]?([A-Za-z_]\\w*?)(?:@\\d+)?", s)
        if m:
            by_name.setdefault(m.group(1), set()).add(a)
    patched = {}
    for p in json.load(open(os.path.join(OUT, "code_pointers.json"))):
        if "c_symbol" in p:
            found = by_name.get(p["c_symbol"], set())
            if len(found) != 1:
                raise SystemExit("C symbol %s for slot 0x%x: %d map entries" % (p["c_symbol"], p["slot"], len(found)))
            patched[p["slot"]] = next(iter(found))
        else:
            patched[p["slot"]] = symbols["cp_trap_%06x" % p["target"]]
''', 1)
t = t.replace('''the retail dword, except code-pointer slots, which must equal what the loader's code-pointer table
(_standalone_code_pointers, same exe) would have patched into that slot.''', '''the retail dword, except code-pointer slots (build/standalone/code_pointers.json), which must hold the address the map
gives the C rewrite of the slot's target (or its named library trap).''')
t = t.replace('print("0x%06x: image holds 0x%08x, the code-pointer table patches 0x%08x"', 'print("0x%06x: image holds 0x%08x, its C function is at 0x%08x"')
open(R, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
