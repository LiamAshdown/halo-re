"""livein.py addr [addr...]: registers a function reads before writing, along its straight-line entry (heuristic:
follows until the first call / jcc / jmp / ret; push reg at entry counts as a save, not a read)"""
import re, subprocess, sys

REGS = {"eax": "eax", "ax": "eax", "al": "eax", "ah": "eax", "ecx": "ecx", "cx": "ecx", "cl": "ecx", "ch": "ecx",
        "edx": "edx", "dx": "edx", "dl": "edx", "dh": "edx", "ebx": "ebx", "bx": "ebx", "bl": "ebx", "bh": "ebx",
        "esi": "esi", "si": "esi", "edi": "edi", "di": "edi", "ebp": "ebp"}


def regs_in(s):
    return {REGS[r] for r in re.findall(r"\b(e?[abcd]x|[abcd][lh]|e?si|e?di|ebp)\b", s) if r in REGS}


for a in sys.argv[1:]:
    a = int(a, 16)
    out = subprocess.run(["objdump", "-d", "-M", "intel", "--no-show-raw-insn", "--start-address=0x%x" % a,
                          "--stop-address=0x%x" % (a + 0x200), "C:\\Users\\Liam-\\halo-re\\bin\\halo.exe"],
                         capture_output=True, text=True).stdout
    written, read = set(), []
    prologue = True
    stop = ""
    for line in out.split("\n"):
        m = re.match(r"\s*([0-9a-f]+):\s+(\S+)\s*(.*)", line)
        if not m:
            continue
        op, args = m.group(2), m.group(3).split("#")[0].strip()
        if op == "push" and prologue and args in REGS:
            continue
        prologue = prologue and op in ("push", "sub", "mov") and "esp" in args or (prologue and op == "push")
        if op.startswith(("call", "j", "ret")):
            stop = "%s %s %s" % (m.group(1), op, args)
            break
        parts = [p.strip() for p in re.split(r",(?![^\[]*\])", args)] if args else []
        if op in ("mov", "movzx", "movsx", "lea") and parts:
            dst, srcs = parts[0], parts[1:]
            for s in srcs + ([dst] if "[" in dst else []):
                for r in regs_in(s) - written:
                    read.append((r, m.group(1)))
            if "[" not in dst:
                written |= regs_in(dst)
        elif op in ("xor", "sub") and len(parts) == 2 and parts[0] == parts[1]:
            written |= regs_in(parts[0])
        else:
            for s in parts:
                for r in regs_in(s) - written:
                    read.append((r, m.group(1)))
            if op in ("pop",) and parts:
                written |= regs_in(parts[0])
            elif parts and "[" not in parts[0] and op not in ("cmp", "test", "push"):
                written |= regs_in(parts[0])
    seen = []
    for r, at in read:
        if r not in [x for x, _ in seen] and r not in ("ebp",):
            seen.append((r, at))
    print("%06x live-in: %s   (until %s)" % (a, ", ".join("%s@%s" % s for s in seen) or "-", stop))
