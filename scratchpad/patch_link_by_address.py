"""gen_standalone_link: bind an unresolved name whose address is a rewritten cdecl C function to that function
(callers that still declare the original under a placeholder name such as FUN_006147a0)."""
import os
os.chdir(r'C:\Users\Liam-\halo-re')
p = 'tools/gen_standalone_link.py'
s = open(p, encoding='utf-8').read()

old1 = '''    std_defs = stdcall_definitions()
    all_defs = {os.path.splitext(os.path.basename(x))[0] for x in glob.glob(os.path.join(ROOT, "src", "*", "*.c"))}
'''
new1 = '''    std_defs = stdcall_definitions()
    fast_defs = fastcall_definitions()
    all_defs = {os.path.splitext(os.path.basename(x))[0] for x in glob.glob(os.path.join(ROOT, "src", "*", "*.c"))}
    # rewritten functions by original address (only those whose object was built), for names that reach one by
    # address alone (2026-09-28)
    rewritten_at = {e["addr"]: e["c_symbol"] for e in json.load(open(os.path.join(OUT, "code_entries.json")))
                    if os.path.exists(os.path.join(ROOT, "build", "obj", e["module"], e["c_symbol"] + ".obj"))}
'''
assert s.count(old1) == 1
s = s.replace(old1, new1)

old2 = '''        if n in imps and (imp_by_slot.get(a) == n or (k if a else kind.get(n)) != "data"):'''
new2 = '''        if (a in rewritten_at and k == "func" and argbytes is None and rewritten_at[a] != n
                and rewritten_at[a] not in std_defs and rewritten_at[a] not in fast_defs):
            # a placeholder name (FUN_006147a0) for a function that now has C under its own name: a cdecl call to a
            # cdecl function, so a plain jump
            target = "_" + rewritten_at[a]; externs.add(target)
            code += ["PUBLIC %s" % s, "%s:" % s, "    jmp %s" % target]; report["rewritten function by address"] += 1
            continue
        if n in imps and (imp_by_slot.get(a) == n or (k if a else kind.get(n)) != "data"):'''
assert s.count(old2) == 1
s = s.replace(old2, new2)
open(p, 'w', encoding='utf-8').write(s)
print('ok')
