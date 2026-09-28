import os
os.chdir(r'C:\Users\Liam-\halo-re')
p = 'tools/gen_standalone_link.py'
s = open(p, encoding='utf-8').read()

old = '''def code_pointer_asm():'''
new = '''def fastcall_definitions():
    """function name -> argument bytes, for rewrites defined __fastcall (their symbol is @name@N): C++ methods the CRT
    calls with __thiscall through a stored pointer (this in ECX, callee pops), e.g. std_exception_what (2026-09-28)"""
    out = {}
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        name = os.path.splitext(os.path.basename(p))[0]
        t = open(p, encoding="utf-8", errors="replace").read()
        m = re.search(r"^[^\\n;{]*__fastcall\\s+%s\\s*\\(([^)]*)\\)\\s*\\n?\\{" % re.escape(name), t, re.M)
        if m:
            params = [x for x in m.group(1).split(",") if x.strip() and x.strip() != "void"]
            out[name] = 4 * len(params)
    return out


def c_symbol(n, std, fast):
    """the decorated symbol of a rewrite: cdecl _name, __stdcall _name@N, __fastcall @name@N"""
    if n in std:
        return "_%s@%d" % (n, std[n])
    if n in fast:
        return "@%s@%d" % (n, fast[n])
    return "_" + n


def code_pointer_asm():'''
assert old in s
s = s.replace(old, new, 1)

old2 = '''    std = stdcall_definitions()
    ext, rows, names, traps = [], [], set(), []'''
new2 = '''    std = stdcall_definitions()
    fast = fastcall_definitions()
    ext, rows, names, traps = [], [], set(), []'''
assert old2 in s
s = s.replace(old2, new2, 1)

old3 = '''            n = p["c_symbol"]
            sym = "_%s@%d" % (n, std[n]) if n in std else "_" + n
            names.add(sym)'''
new3 = '''            n = p["c_symbol"]
            sym = c_symbol(n, std, fast)
            names.add(sym)'''
assert old3 in s
s = s.replace(old3, new3, 1)

old4 = '''        n = e["c_symbol"]
        sym = "_%s@%d" % (n, std[n]) if n in std else "_" + n
        names.add(sym)
        entry_rows.append'''
new4 = '''        n = e["c_symbol"]
        sym = c_symbol(n, std, fast)
        names.add(sym)
        entry_rows.append'''
assert old4 in s
s = s.replace(old4, new4, 1)
open(p, 'w', encoding='utf-8').write(s)
print('patched')
