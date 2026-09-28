"""Retail-independence step 3: the loader's data image as committed MASM source (standalone/image/).

The standalone exe keeps the engine's globals at their original addresses: before any game code runs, the loader copies
this image into the reserved range 0x63a000.. (see standalone/loader.c). This tool writes that image once from
bin/halo.exe as assembler source, so neither halo.exe nor a halo_image.bin is needed afterwards:
  standalone/image/<piece>.asm   one file per piece; every dword the code-pointer scan found (standalone/frozen/
                                 code_pointer_slots.json) is `dd halo_code_<address>`, which the link binds to the C
                                 rewrite of that function (/ALTERNATENAME in tools/gen_standalone_link.py), so the
                                 linker relocates it and the loader patches nothing
  standalone/image/pieces.json   name, original address, size and label of every piece, for the loader's table
Pieces: all of .rdata, the initialised part of .data (the zero tail is the reserve's own zero fill), .tls and .rsrc.
Nothing of .text: it is original code, and the ranges the C once read as data are gone (see TEXT_DATA). It stays
reserved, zero and not executable in the standalone.
Import slots keep their retail contents (hint/name RVAs); the loader overwrites them.
Usage: python tools/gen_image_source.py   (reads bin/halo.exe; refuses under HALO_NO_RETAIL=1)"""
import json, os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import retail_guard as rg
from gen_standalone import EXE, pe_layout

ROOT = rg.ROOT
IMAGE = os.path.join(ROOT, "standalone", "image")

# (label, first address, end address) of the .text ranges the standalone reads as data
# (none: the DIOBJECTDATAFORMAT[256] at 0x613400 behind the retail c_dfDIKeyboard 0x64dfdc was the last one, and the C
# takes c_dfDIKeyboard / c_dfDIMouse2 from dinput8.lib; the switch table at 0x4a0268 became a C switch)
TEXT_DATA = []


def hexnum(v):
    s = "%Xh" % v
    return "0" + s if s[0] in "ABCDEF" else s


def emit_piece(label, va, data, pointers, names):
    """MASM for one piece: dwords, zero runs as dup, code pointers as symbols"""
    assert va % 4 == 0 and len(data) % 4 == 0
    lines = [".386", ".model flat", "option casemap:none", ""]
    targets = sorted({pointers[a] for a in pointers if va <= a < va + len(data)})
    lines += ["EXTERN halo_code_%06x:PROC" % t for t in targets]
    lines += ["", ".const", "ALIGN 4", "PUBLIC _halo_image_%s" % label, "_halo_image_%s LABEL BYTE" % label]
    row = []
    zeros = 0

    def flush():
        nonlocal row
        if row:
            lines.append("    dd " + ", ".join(row))
            row = []

    def flush_zeros():
        nonlocal zeros
        if zeros:
            flush()
            lines.append("    dd %d dup (0)" % zeros)
            zeros = 0

    for o in range(0, len(data), 4):
        a = va + o
        if o % 0x100 == 0:
            flush_zeros()
            flush()
            lines.append("; 0x%06x" % a)
        if a in pointers:
            flush_zeros()
            flush()
            lines.append("    dd halo_code_%06x ; %s" % (pointers[a], names[a]))
            continue
        v = struct.unpack_from("<I", data, o)[0]
        if v == 0:
            zeros += 1
            continue
        flush_zeros()
        row.append(hexnum(v))
        if len(row) == 8:
            flush()
    flush_zeros()
    flush()
    lines += ["", "END", ""]
    return "\n".join(lines)


def main():
    if rg.NO_RETAIL:
        raise SystemExit("gen_image_source.py reads bin/halo.exe; run it without HALO_NO_RETAIL")
    exe = open(EXE, "rb").read()
    base, dirs, sections = pe_layout(exe)
    sec = {s["name"]: s for s in sections}
    slots = rg.read_frozen("code_pointer_slots.json")
    pointers = {p["slot"]: p["target"] for p in slots}
    names = {p["slot"]: p["name"] for p in slots}

    def raw(va, size):
        for s in sections:
            if s["va"] <= va and va + size <= s["va"] + s["rsize"]:
                return exe[s["raw"] + va - s["va"]:s["raw"] + va - s["va"] + size]
        raise ValueError("0x%x+0x%x is not in the file" % (va, size))

    pieces = [(label, lo, hi - lo) for label, lo, hi in TEXT_DATA]
    for name in (".rdata", ".data", ".tls", ".rsrc"):
        s = sec[name]
        size = s["rsize"]
        if name == ".data":
            # only the initialised part: the zero tail of the raw data joins .bss
            data = exe[s["raw"]:s["raw"] + size]
            size = len(data.rstrip(b"\0") + b"\0\0\0") // 4 * 4
        pieces.append((name[1:], s["va"], size))
    os.makedirs(IMAGE, exist_ok=True)
    table = []
    covered = set()
    for label, va, size in pieces:
        data = raw(va, size)
        text = emit_piece(label, va, data, pointers, names)
        open(os.path.join(IMAGE, label + ".asm"), "w", newline="\n").write(text)
        table.append({"label": label, "va": va, "size": size})
        covered |= {a for a in pointers if va <= a < va + size}
        print("%-14s 0x%06x %7d bytes  %6d lines" % (label, va, size, text.count("\n")))
    lost = sorted(set(pointers) - covered)
    if lost:
        raise SystemExit("code pointer slots outside the image: %s" % ", ".join("0x%x" % a for a in lost[:10]))
    rg_json = json.dumps(table, indent=1) + "\n"
    open(os.path.join(IMAGE, "pieces.json"), "w", newline="\n").write(rg_json)


if __name__ == "__main__":
    main()
