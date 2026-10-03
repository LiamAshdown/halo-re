"""One-off converter: the data image pieces standalone/image/<label>.asm -> standalone/image/halo_image_<label>.c.

Each piece is a flat array of dwords: plain numbers, runs (`dd N dup (V)`) and code pointers (`dd halo_code_<address>`, bound
to the C function at that address by standalone/generated/image_bindings.c). The C file has the same bytes: numbers as
designated initialisers (zero runs are left implicit), code pointers as `(unsigned int)halo_code_<address>`, which the compiler
turns into a relocation against `_halo_code_<address>`.
Usage: python tools/gen_image_c.py            (reads the .asm files, writes the .c files)"""
import json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMG = os.path.join(ROOT, "standalone", "image")


def num(tok):
    tok = tok.strip()
    if re.fullmatch(r"[0-9][0-9A-Fa-f]*[hH]", tok):
        return int(tok[:-1], 16) & 0xffffffff
    if re.fullmatch(r"-?[0-9]+", tok):
        return int(tok) & 0xffffffff
    raise ValueError(tok)


def parse(label):
    items = []   # ("w", value) | ("c", address) | ("z", count, value)
    started = False
    for raw in open(os.path.join(IMG, label + ".asm"), encoding="utf-8"):
        line = raw.split(";")[0].strip()
        if not started:
            started = "LABEL BYTE" in line
            continue
        if not line.startswith("dd "):
            continue
        body = line[3:].strip()
        m = re.fullmatch(r"(\d+) dup \((\S+)\)", body)
        if m:
            items.append(("z", int(m.group(1)), num(m.group(2))))
            continue
        for tok in body.split(","):
            tok = tok.strip()
            m = re.fullmatch(r"halo_code_([0-9a-f]+)", tok)
            items.append(("c", m.group(1)) if m else ("w", num(tok)))
    return items


def emit(label, size):
    items = parse(label)
    total = sum(1 if it[0] != "z" else it[1] for it in items)
    assert total * 4 == size, (label, total * 4, size)
    codes = sorted({it[1] for it in items if it[0] == "c"})
    out = ["/* standalone/image/halo_image_%s.c -- the data image piece `%s`, generated once by tools/gen_image_c.py from the former"
           % (label, label),
           "   MASM source. %d dwords. Code pointers are relocations against halo_code_<address>, bound to the C functions by" % total,
           "   standalone/generated/image_bindings.c; the loader copies the piece to its original address. */", ""]
    out += ["extern void halo_code_%s(void);" % c for c in codes]
    out += ["", "const unsigned int halo_image_%s[%d] = {" % (label, total)]
    idx = 0
    line = []

    def flush():
        if line:
            out.append("    " + ", ".join(line) + ",")
            line.clear()
    last_explicit = -1
    for it in items:
        if it[0] == "z":
            if it[2] == 0:
                idx += it[1]
                continue
            for _ in range(it[1]):
                line.append(("[%d] = 0x%x" % (idx, it[2])) if idx != last_explicit + 1 else "0x%x" % it[2])
                last_explicit = idx
                idx += 1
                if len(line) >= 8:
                    flush()
            continue
        val = "0x%x" % it[1] if it[0] == "w" else "(unsigned int)halo_code_%s" % it[1]
        if it[0] == "w" and it[1] == 0:
            idx += 1
            continue
        line.append(("[%d] = %s" % (idx, val)) if idx != last_explicit + 1 else val)
        last_explicit = idx
        idx += 1
        if len(line) >= 6:
            flush()
    flush()
    if out[-1].endswith("= {"):
        out.append("    0")
    out.append("};")
    path = os.path.join(IMG, "halo_image_%s.c" % label)
    open(path, "w", newline="\n").write("\n".join(out) + "\n")
    print("%s: %d dwords, %d code pointers -> %s" % (label, total, len(codes), os.path.relpath(path, ROOT)))


def main():
    pieces = json.load(open(os.path.join(IMG, "pieces.json")))
    for p in pieces:
        emit(p["label"], p["size"])


if __name__ == "__main__":
    main()
