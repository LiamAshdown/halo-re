"""audit_calls.py <dis file>: for every call in a disassembly listing, the setup since the previous call/branch target
and the callee's C definition (signature + blam-cc / register notes from its header)"""
import glob, os, re, sys
R = "C:\\Users\\Liam-\\halo-re\\"

by_addr = {}
for p in glob.glob(R + "src\\*\\*.c"):
    t = open(p, encoding="utf-8", errors="replace").read()
    m = re.search(r"address\s+0x0*([0-9a-f]{6}),\s*size\s+(\d+)", t[:4000], re.I)
    if m:
        by_addr[int(m.group(1), 16)] = (p, t)


def signature(p, t):
    name = os.path.splitext(os.path.basename(p))[0]
    body = re.sub(r"^#if 0.*?^#endif", "", t, flags=re.S | re.M)
    m = re.search(r"^([^\n;#/][^\n;]*?\b%s\s*\([^;{]*?\))\s*\{" % re.escape(name), body, re.S | re.M)
    sig = " ".join(m.group(1).split()) if m else "(no definition)"
    notes = [l.strip() for l in t[:5000].split("\n") if re.search(r"blam-cc|register|EAX|ECX|EDX|ESI|EDI|EBX", l)][:3]
    return name, sig, notes


lines = open(sys.argv[1]).read().split("\n")
setup = []
for line in lines:
    m = re.match(r"\s*([0-9a-f]+):\s+(.*)", line)
    if not m:
        continue
    ins = m.group(2).strip()
    c = re.match(r"call\s+0x([0-9a-f]+)", ins)
    if c:
        a = int(c.group(1), 16)
        if a in by_addr:
            name, sig, notes = signature(*by_addr[a])
        else:
            name, sig, notes = "?", "(no C file)", []
        print("%s call %06x %s" % (m.group(1), a, name))
        print("    setup: %s" % " ; ".join(setup[-4:]))
        print("    C: %s" % sig)
        for n in notes:
            print("    note: %s" % n[:150])
        setup = []
    elif ins.startswith(("ret", "jmp")):
        setup = []
    else:
        setup.append(ins)
