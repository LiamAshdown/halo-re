import re
R = "C:\\Users\\Liam-\\halo-re\\"


def sub(path, old, new, count=1):
    t = open(R + path, encoding="utf-8").read()
    assert t.count(old) >= 1, (path, old[:60])
    t = t.replace(old, new, count)
    open(R + path, "w", encoding="utf-8", newline="\n").write(t)


# ---- tables header
sub("standalone\\standalone_tables.h", '''typedef struct standalone_piece {
    const char *name;
    unsigned long va;
    unsigned long blob_offset;
    unsigned long raw_size;
} standalone_piece;''', '''typedef struct standalone_piece {
    const char *name;
    unsigned long va;                 /* original address */
    const void *source;               /* the piece in our exe (standalone/image/<name>.asm) */
    unsigned long size;
} standalone_piece;''')
sub("standalone\\standalone_tables.h", '''typedef struct standalone_code_pointer {
    unsigned long slot;               /* dword in .rdata/.data that held an original function address */
    void *target;                     /* our C function */
} standalone_code_pointer;

''', '')
sub("standalone\\standalone_tables.h", '''extern const standalone_code_pointer standalone_code_pointers[];
extern const int standalone_code_pointer_count;
''', '')

# ---- loader
sub("standalone\\loader.c", '''     2. The child commits the range, copies halo_image.bin (the .rdata/.data/.tls/.rsrc raw bytes written by
        tools/gen_standalone.py) to the recorded addresses, fills both import tables (normal and delay-load) with
        GetProcAddress, and writes our C functions' addresses into every code pointer stored in that data.''',
    '''     2. The child commits the range, copies the data image linked into this exe (standalone/image/*.asm: .rdata,
        initialised .data, .tls, .rsrc and the few .text ranges read as data; every code pointer in it is already
        our C function's address, relocated by the linker) to the original addresses, and fills both import tables
        (normal and delay-load) with GetProcAddress.''')
old_map = re.search(r"static int map_image\(void\)\n\{.*?\n\}\n", open(R + "standalone\\loader.c", encoding="utf-8").read(), re.S).group(0)
sub("standalone\\loader.c", old_map, '''static int map_image(void)
{
    DWORD old_protect;
    int i;

    if (!VirtualAlloc((void *)RESERVE_BASE, RESERVE_END - RESERVE_BASE, MEM_COMMIT, PAGE_EXECUTE_READWRITE)) {
        log_line("could not commit 0x%x..0x%x (%lu) -- was the range reserved?", RESERVE_BASE, RESERVE_END,
                 GetLastError());
        return 0;
    }
    for (i = 0; i < standalone_piece_count; i++) {
        const standalone_piece *p = &standalone_pieces[i];
        memcpy((void *)p->va, p->source, p->size);   /* the rest of the committed range stays zero */
    }
    /* nothing in the range is code any more: without execute permission a jump into original code faults at once
       (DEP is on for this exe: /NXCOMPAT) and log_exception names the address */
    VirtualProtect((void *)RESERVE_BASE, RESERVE_END - RESERVE_BASE, PAGE_READWRITE, &old_protect);
    log_line("mapped %d image pieces (range now read/write, no execute)", standalone_piece_count);
    return 1;
}
''')
old_fix = re.search(r"static void fix_code_pointers\(void\)\n\{.*?\n\}\n\n", open(R + "standalone\\loader.c", encoding="utf-8").read(), re.S).group(0)
sub("standalone\\loader.c", old_fix, "")
sub("standalone\\loader.c", "    fix_code_pointers();\n", "")

# ---- table generator
sub("tools\\gen_standalone_link.py", '''    layout = json.load(open(os.path.join(OUT, "layout.json")))
    imports = json.load(open(os.path.join(OUT, "imports.json")))
    lines = ['#include "standalone_tables.h"', "",
             "const char standalone_halo_folder[] = %s;" % c_string(HALO_FOLDER), "",
             "const standalone_piece standalone_pieces[] = {"]
    for p in layout["pieces"]:
        lines.append("    { %s, 0x%08xUL, 0x%08xUL, 0x%08xUL }," % (c_string(p["name"]), p["va"], p["blob_offset"], p["raw"]))
    lines += ["};", "const int standalone_piece_count = %d;" % len(layout["pieces"]), "",''',
    '''    pieces = json.load(open(os.path.join(SA, "image", "pieces.json")))
    imports = json.load(open(os.path.join(OUT, "imports.json")))
    lines = ['#include "standalone_tables.h"', "",
             "const char standalone_halo_folder[] = %s;" % c_string(HALO_FOLDER), ""]
    # the data image the loader copies to the original addresses (standalone/image/<label>.asm)
    lines += ["extern const unsigned char halo_image_%s[];" % p["label"] for p in pieces]
    lines += ["", "const standalone_piece standalone_pieces[] = {"]
    for p in pieces:
        lines.append("    { %s, 0x%08xUL, halo_image_%s, 0x%08xUL }," % (c_string(p["label"]), p["va"], p["label"],
                                                                       p["size"]))
    lines += ["};", "const int standalone_piece_count = %d;" % len(pieces), "",''')

sub("tools\\gen_standalone_link.py", '''def code_pointer_asm():
    """the table of (slot, our function) as data, in MASM so decorated names work"""''',
    '''def code_pointer_asm():
    """the named traps for library code pointers without C, and the table of (original address, our function) the
    loader redirects jumps into original .text with, in MASM so decorated names work. (The code pointers themselves
    are relocations in the image source: image_source() binds them.)"""''')
sub("tools\\gen_standalone_link.py", '''    ext, rows, names, traps = [], [], set(), []
    for p in ptrs:
        if "c_symbol" in p:
            n = p["c_symbol"]
            sym = c_symbol(n, std, fast)
            names.add(sym)
        else:
            sym = "cp_trap_%06x" % p["target"]
            if sym not in {x[0] for x in traps}:
                traps.append((sym, p["name"]))
        rows.append("    dd 0%Xh, %s" % (p["slot"], sym))
''', '''    ext, names, traps = [], set(), []
    for p in ptrs:
        if "c_symbol" not in p:
            sym = "cp_trap_%06x" % p["target"]
            if sym not in {x[0] for x in traps}:
                traps.append((sym, p["name"]))
''')
sub("tools\\gen_standalone_link.py", "    return ext + stubs + strs, rows, entry_rows\n", "    return ext + stubs + strs, entry_rows\n")
sub("tools\\gen_standalone_link.py", '''    ext, rows, entry_rows = code_pointer_asm()
    pointer_asm = [".386", ".model flat", "option casemap:none"] + ext + [
        ".const", "PUBLIC _standalone_code_pointers", "PUBLIC _standalone_code_pointer_count",
        "_standalone_code_pointer_count dd %d" % len(rows), "_standalone_code_pointers LABEL DWORD"] + rows + [
        "PUBLIC _standalone_code_entries", "PUBLIC _standalone_code_entry_count",''',
    '''    ext, entry_rows = code_pointer_asm()
    pointer_asm = [".386", ".model flat", "option casemap:none"] + ext + [
        ".const", "PUBLIC _standalone_code_entries", "PUBLIC _standalone_code_entry_count",''')
print("ok")
