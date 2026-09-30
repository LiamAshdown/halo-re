"""Disassemble a range of bin/halo.exe with capstone (Linux, no Ghidra/objdump needed).
Usage: python3 tools/disasm.py ADDR [SIZE|end=ADDR] [--names]   e.g. tools/disasm.py 0x4105c0 237
--names annotates call/jmp targets with symbols/functions.txt names."""
import sys, os, re, pefile, capstone
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def load():
    pe = pefile.PE(os.path.join(ROOT, "bin", "halo.exe"))
    return pe, pe.OPTIONAL_HEADER.ImageBase
def read(pe, base, va, n):
    return pe.get_data(va - base, n)
def names():
    d = {}
    p = os.path.join(ROOT, "symbols", "functions.txt")
    if os.path.exists(p):
        for l in open(p):
            m = re.match(r"(0x[0-9a-fA-F]+)\s+(\S+)", l)
            if m: d[int(m.group(1), 16)] = m.group(2)
    return d
def main():
    a = [x for x in sys.argv[1:] if not x.startswith("--")]
    addr = int(a[0], 16)
    size = 256
    if len(a) > 1:
        size = (int(a[1][4:], 16) - addr) if a[1].startswith("end=") else int(a[1], 0)
    pe, base = load()
    nm = names() if "--names" in sys.argv else {}
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for i in md.disasm(read(pe, base, addr, size), addr):
        note = ""
        m = re.match(r"(call|jmp)\s+0x([0-9a-f]+)$", f"{i.mnemonic} {i.op_str}")
        if m and int(m.group(2), 16) in nm: note = "  ; " + nm[int(m.group(2), 16)]
        print(f"{i.address:08x}  {i.mnemonic:6} {i.op_str}{note}")
if __name__ == "__main__": main()
