import re
R = "C:\\Users\\Liam-\\halo-re\\"


def edit(rel, pairs):
    p = R + rel
    t = open(p, encoding="utf-8").read()
    for old, new in pairs:
        if isinstance(old, re.Pattern):
            t, n = old.subn(new, t)
            assert n >= 1, (rel, old.pattern[:60])
        else:
            assert t.count(old) >= 1, (rel, old[:70])
            t = t.replace(old, new)
    open(p, "w", encoding="utf-8", newline="\n").write(t)
    print("edited", rel)


# ReadFileEx itself (windows.h), not the retail import slot that held it
for rel in ("src\\cache\\cache_file_slot_read_header.c", "src\\cache\\cache_io_thread_proc_async.c"):
    edit(rel, [
        (re.compile(r"^extern void \*ReadFileEx_exref;[^\n]*\n", re.M), ""),
        (re.compile(r"(?<![\w])ReadFileEx_exref(?![\w])(?=[^\n]*;)"), "(void *)ReadFileEx"),
    ])
edit("standalone\\globals.asm", [("PUBLIC _ReadFileEx_exref\n_ReadFileEx_exref EQU 063A27Ch\n", "")])

# the loader no longer fills the retail image's import slots: the C calls everything through this exe's own imports
p = R + "standalone\\loader.c"
t = open(p, encoding="utf-8").read()
i = t.index("static void unresolved_import_trap(void)")
j = t.index("}\n", i) + 2
t = t[:i] + t[j:].lstrip("\n")
i = t.index("static void fill_imports(void)")
j = t.index("/* The C calls Windows through this exe's own import table")
t = t[:i] + t[j:]
t = t.replace('''/* The C calls Windows through this exe's own import table (each API is declared __stdcall and links against the SDK
   import libraries), so the CreateFileA data override must also sit in this module's import address table, not only
   in the retail slot fill_imports patches. Only relative paths with a copy under override\\\\ are redirected, so the
   loader's and the CRT's own file opens are unaffected. */''', '''/* The C calls Windows through this exe's own import table (the SDK headers and import libraries; nothing uses the
   retail image's import slots any more), so the CreateFileA data override sits in this module's import address table.
   Only relative paths with a copy under override\\\\ are redirected, so the loader's and the CRT's own file opens are
   unaffected. */''')
assert "    fill_imports();\n" in t
t = t.replace("    fill_imports();\n", "")
t = t.replace("    if (!g_create_file_a) g_create_file_a = (create_file_a_fn)real;\n", "    g_create_file_a = (create_file_a_fn)real;\n")
t = t.replace('''     2. The child commits the range, copies the data image linked into this exe (standalone/image/*.asm: .rdata,
        initialised .data, .tls, .rsrc and the few .text ranges read as data; every code pointer in it is already
        our C function's address, relocated by the linker) to the original addresses, and fills both import tables
        (normal and delay-load) with GetProcAddress.''', '''     2. The child commits the range and copies the data image linked into this exe (standalone/image/*.asm:
        .rdata, initialised .data, .tls and .rsrc; every code pointer in it is already our C function's address,
        relocated by the linker) to the original addresses. The retail import slots in it stay unfilled: the C calls
        Windows and the third-party DLLs through this exe's own (delay-)imports.''')
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("edited loader.c")

edit("standalone\\standalone_tables.h", [
    (re.compile(r"typedef struct standalone_import \{.*?\} standalone_import;\n\n?", re.S), ""),
    ("extern const standalone_import standalone_imports[];\nextern const int standalone_import_count;\n", ""),
])
p = R + "tools\\gen_standalone_link.py"
t = open(p, encoding="utf-8").read()
i = t.index('    lines += ["};", "const int standalone_piece_count = %d;" % len(pieces), "",')
j = t.index('    open(os.path.join(OUT, "standalone_tables.c"), "w")', i)
t = t[:i] + '    lines += ["};", "const int standalone_piece_count = %d;" % len(pieces), ""]\n' + t[j:]
t = t.replace('    imports = json.load(open(os.path.join(OUT, "imports.json")))\n', "", 1)
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("edited gen_standalone_link.py")
