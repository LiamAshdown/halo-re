"""console_print_error_va (0x4c67c0) takes clear_first in AL before the format and its arguments on the stack; many
callers were written as console_print_error_va(format, ...). For each caller whose EVERY binary call site zeroes AL
(xor al,al / xor eax,eax as the last write to EAX before the call), replace the declaration with the definition's and
prepend 0 to each call.
Usage: python tools/fix_print_error_callers.py [--apply]"""
import os, re, sys, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); import gen_hooks as g
DECL = "extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, AL clear_first\n"

def al_source(lines, i):
    for x in reversed(lines[max(0, i - 15):i]):
        ins = x.split("\t")[-1].strip()
        if re.match(r"xor\s+(al,al|eax,eax)$", ins): return 0
        if ins.startswith("call"): return None
        if re.match(r"(mov|lea|or|pop|xor|movzx|movsx|add|sub|and|inc|dec|set\w+)\s+(eax|ax|al)\b", ins): return None
    return None

def main():
    apply = "--apply" in sys.argv
    lines = open(os.path.join(ROOT, "build", "halo_text.dis")).read().splitlines()
    sites = [(int(l.split(":")[0].strip(), 16), al_source(lines, i)) for i, l in enumerate(lines)
             if re.search(r"\tcall\s+0x4c67c0$", l)]
    n = 0; changed = []
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(p, encoding="utf-8", errors="replace").read(); cut = t.rfind("#if 0")
        b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        m = re.search(r"^[ \t]*extern[^;]*\bconsole_print_error_va\s*\(\s*const char \*format,\s*\.\.\.\s*\)\s*;[^\n]*\n", b, re.M)
        if not m: continue
        h = g.HDR.search(t[:4000])
        if not h: continue
        a0, sz = int(h.group(1), 16), int(h.group(2))
        mine = [s for a, s in sites if a0 <= a < a0 + sz]
        if not mine or any(s != 0 for s in mine): continue
        body = b[m.end():]
        cs = list(re.finditer(r"\bconsole_print_error_va\s*\(", body))
        if len(cs) != len(mine): continue
        new, pos = "", 0
        for c in cs:
            new += body[pos:c.end()] + "0, "; pos = c.end()
        new += body[pos:]
        nb = b[:m.start()] + DECL + new
        n += 1; changed.append(os.path.relpath(p, ROOT)); print(os.path.relpath(p, ROOT), len(cs), "call(s)")
        if apply: open(p, "w", encoding="utf-8", newline="").write(nb + tail)
    print(("fixed" if apply else "would fix"), n)
    if apply: open(os.path.join(ROOT, "build", "register_arg_callers_changed.txt"), "w").write("\n".join(changed))

if __name__ == "__main__": main()
