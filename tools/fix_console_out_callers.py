"""chimera__console_out (0x496b50) takes the colour in EAX (NULL = the default colour) before the format and its
arguments on the stack; most callers were written as console_out(format, ...). For each caller whose EVERY call site
in the binary sets EAX the same way -- xor eax,eax (NULL) or mov eax,[global] (a colour pointer held in a global) --
replace the declaration with the definition's and prepend that colour argument to each call.
Usage: python tools/fix_console_out_callers.py [--apply]"""
import os, re, sys, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); import gen_hooks as g
from fix_register_arg_callers import split_args

def eax_source(lines, i):
    for x in reversed(lines[max(0, i - 12):i]):
        ins = x.split("\t")[-1].strip()
        if re.match(r"xor\s+eax,eax$", ins): return "null"
        m = re.match(r"mov\s+eax,ds:0x([0-9a-f]+)$", ins)
        if m: return int(m.group(1), 16)
        if re.match(r"(mov|lea|or|pop|call|xor|movzx|movsx|add|sub|and)\s+eax\b", ins) or ins.startswith("call"): return None
    return None

def main():
    apply = "--apply" in sys.argv
    lines = open(os.path.join(ROOT, "build", "halo_text.dis")).read().splitlines()
    sites = []
    for i, l in enumerate(lines):
        if re.search(r"\tcall\s+0x496b50$", l): sites.append((int(l.split(":")[0].strip(), 16), eax_source(lines, i)))
    n = 0; changed = []
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(p, encoding="utf-8", errors="replace").read(); cut = t.rfind("#if 0")
        b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        m = re.search(r"^[ \t]*extern[^;]*\bchimera__console_out\s*\(([^;]*)\)\s*;[^\n]*\n", b, re.M | re.S)
        if not m or "ColorARGB" in m.group(1) or "..." not in m.group(1): continue
        h = g.HDR.search(t[:4000])
        if not h: continue
        a0, sz = int(h.group(1), 16), int(h.group(2))
        mine = {s for a, s in sites if a0 <= a < a0 + sz}
        if len(mine) != 1 or None in mine: continue
        src = mine.pop()
        if src == "null":
            prefix, extra = "(ColorARGB *)0, ", ""
        else:
            prefix = f"(ColorARGB *)console_color_{src:08x}, "
            extra = f"extern void *console_color_{src:08x}; // 0x{src:08x}, a ColorARGB * the original loads into EAX\n"
        body = b[m.end():]
        cs = list(re.finditer(r"\bchimera__console_out\s*\(", body))
        if not cs: continue
        new, pos = "", 0
        for c in cs:
            i = c.end(); depth = 1; j = i
            while depth:
                if body[j] == "(": depth += 1
                elif body[j] == ")": depth -= 1
                j += 1
            new += body[pos:i] + prefix + body[i:j - 1] + ")"; pos = j
        new += body[pos:]
        decl = extra + "extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)\n"
        nb = b[:m.start()] + decl + new
        n += 1; changed.append(os.path.relpath(p, ROOT)); print(os.path.relpath(p, ROOT), len(cs), "call(s)", src if src == "null" else hex(src))
        if apply: open(p, "w", encoding="utf-8", newline="").write(nb + tail)
    print(("fixed" if apply else "would fix"), n)
    if apply: open(os.path.join(ROOT, "build", "register_arg_callers_changed.txt"), "w").write("\n".join(changed))

if __name__ == "__main__": main()
