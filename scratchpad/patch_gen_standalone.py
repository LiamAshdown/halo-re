import os
os.chdir(r'C:\Users\Liam-\halo-re')
p = 'tools/gen_standalone.py'
s = open(p, encoding='utf-8').read()

old_ranges = '''        (0x679600, 0x67d000),
    ]'''
new_ranges = '''        (0x679600, 0x67d000),
        # DIDATAFORMAT at 0x0064dfdc (pushed at 0x491930 for SetDataFormat): its rgodf field 0x64dff0 points at the
        # DIOBJECTDATAFORMAT array the linker placed in .text at 0x613400 -- data, which the loader maps with .text
        # (2026-09-28)
        (0x64dff0, 0x64dff4),
    ]'''
assert old_ranges in s
s = s.replace(old_ranges, new_ranges, 1)

old_mod = '''            m = mods.get("%x" % v, {})
            module = m.get("module", "?") if isinstance(m, dict) else m
'''
new_mod = '''            m = mods.get("%x" % v, {})
            module = m.get("module", "?") if isinstance(m, dict) else m
            if module in ("?", "") and not f and not r:
                # an entry Ghidra never split off inherits a library module from the function it follows, e.g.
                # unlisted_5c0ba0 in the D3DX vtable 0x646bc0 right after FUN_005c0b45 (lib:d3dx) (2026-09-28)
                i = bisect.bisect_right(function_starts, v) - 1
                if i >= 0:
                    pm = mods.get("%x" % function_starts[i], {})
                    pmodule = pm.get("module", "?") if isinstance(pm, dict) else pm
                    if isinstance(pmodule, str) and pmodule.startswith("lib"):
                        module = pmodule
'''
assert old_mod in s
s = s.replace(old_mod, new_mod, 1)

old_def = '''    def inside_library_function(v):'''
new_def = '''    function_starts = sorted(funcs)

    def inside_library_function(v):'''
assert old_def in s
s = s.replace(old_def, new_def, 1)
open(p, 'w', encoding='utf-8').write(s)
print('patched')
