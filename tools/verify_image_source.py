"""Checks the linked image source (standalone/image/*.asm) against the retail data image, statically.

Reads build/standalone/halo_rebuilt.exe as a file (it is never run) and build/standalone/halo_image.bin (written by a
retail run of tools/gen_standalone.py). For every piece in standalone/image/pieces.json each dword in the exe must equal
the retail dword, except code-pointer slots, which must equal what the loader's code-pointer table
(_standalone_code_pointers, same exe) would have patched into that slot. Everything of the retail image the pieces leave
out must be zero, apart from .text outside the listed data ranges (original code, deliberately not carried).
Usage: python tools/verify_image_source.py"""
import json, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "build", "standalone")


def main():
    exe = open(os.path.join(OUT, "halo_rebuilt.exe"), "rb").read()
    pe = struct.unpack_from("<I", exe, 0x3c)[0]
    nsec = struct.unpack_from("<H", exe, pe + 6)[0]
    optsize = struct.unpack_from("<H", exe, pe + 20)[0]
    base = struct.unpack_from("<I", exe, pe + 24 + 28)[0]
    sections = []
    for i in range(nsec):
        o = pe + 24 + optsize + 40 * i
        vsize, va, rsize, raw = struct.unpack_from("<IIII", exe, o + 8)
        sections.append((base + va, vsize, raw, rsize))

    def read(va, n):
        for sva, vsize, raw, rsize in sections:
            if sva <= va and va + n <= sva + vsize:
                d = va - sva
                chunk = exe[raw + d:raw + min(d + n, rsize)] if d < rsize else b""
                return chunk + b"\0" * (n - len(chunk))
        raise ValueError("0x%x not in the exe" % va)

    symbols = {}
    for line in open(os.path.join(OUT, "halo_rebuilt.map"), errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s", line)
        if m:
            symbols[m.group(1)] = int(m.group(2), 16)

    # what the loader's table patches: slot -> our address
    count = struct.unpack("<I", read(symbols["_standalone_code_pointer_count"], 4))[0]
    table = read(symbols["_standalone_code_pointers"], 8 * count)
    patched = {}
    for i in range(count):
        slot, target = struct.unpack_from("<II", table, 8 * i)
        patched[slot] = target

    layout = json.load(open(os.path.join(OUT, "layout.json")))
    blob = open(os.path.join(OUT, "halo_image.bin"), "rb").read()
    retail = {}                                            # piece name -> (va, bytes)
    for p in layout["pieces"]:
        retail[p["name"]] = (p["va"], blob[p["blob_offset"]:p["blob_offset"] + p["raw"]])

    def retail_bytes(va, n):
        for name, (pva, data) in retail.items():
            if pva <= va and va + n <= pva + len(data):
                return data[va - pva:va - pva + n]
        raise ValueError("0x%x not in halo_image.bin" % va)

    pieces = json.load(open(os.path.join(ROOT, "standalone", "image", "pieces.json")))
    bad = 0
    covered_slots = set()
    carried = []
    for p in pieces:
        va, size = p["va"], p["size"]
        ours = read(symbols["_halo_image_" + p["label"]], size)
        theirs = retail_bytes(va, size)
        relocated = 0
        for o in range(0, size, 4):
            a = va + o
            mine = struct.unpack_from("<I", ours, o)[0]
            if a in patched:
                covered_slots.add(a)
                relocated += 1
                if mine != patched[a]:
                    bad += 1
                    if bad < 20:
                        print("0x%06x: image holds 0x%08x, the code-pointer table patches 0x%08x" % (a, mine, patched[a]))
            elif ours[o:o + 4] != theirs[o:o + 4]:
                bad += 1
                if bad < 20:
                    print("0x%06x: image 0x%08x, retail 0x%08x" % (a, mine, struct.unpack_from("<I", theirs, o)[0]))
        carried.append((va, va + size))
        print("%-14s 0x%06x %7d bytes: %d relocated code pointers" % (p["label"], va, size, relocated))
    missing = sorted(set(patched) - covered_slots)
    if missing:
        bad += len(missing)
        print("code-pointer slots outside the image:", ", ".join("0x%x" % a for a in missing[:10]))
    # everything not carried must be zero (except original code in .text)
    dropped_nonzero = 0
    text_va = retail[".text"][0]
    text_end = text_va + len(retail[".text"][1])
    for name, (pva, data) in retail.items():
        if name == ".text":
            continue
        for o in range(0, len(data), 4):
            a = pva + o
            if not any(lo <= a < hi for lo, hi in carried) and data[o:o + 4] != b"\0\0\0\0":
                dropped_nonzero += 1
    if dropped_nonzero:
        bad += dropped_nonzero
        print("non-zero retail dwords the image leaves out:", dropped_nonzero)
    text_kept = sum(min(hi, text_end) - max(lo, text_va) for lo, hi in carried if lo < text_end and hi > text_va)
    print(".text carried as data: %d of %d bytes (the rest is original code)" % (text_kept, text_end - text_va))
    print("RESULT:", "identical apart from relocated code pointers" if not bad else "%d differences" % bad)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
