"""message_delta_encode_message (0x4ec940) takes the output buffer in EAX and its size in EDX (network_message_scratch,
0x7ff8 at almost every call site) plus seven stack arguments. Callers written from Ghidra declare only the seven
stack arguments. For each caller whose EVERY call site in the binary loads EAX = 0x871de0 and EDX = 0x7ff8 in the
preceding instructions, and whose C calls all pass seven arguments, replace the declaration with the definition's
and prepend the two register arguments to each call. Usage: python tools/fix_encode_message_callers.py [--apply]"""
import os, re, sys, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); import gen_hooks as g
DECL = ("extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0\n"
        "extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,\n"
        "    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size")

def split_args(s):
    out, depth, cur = [], 0, ""
    for c in s:
        if c in "([": depth += 1
        elif c in ")]": depth -= 1
        if c == "," and depth == 0: out.append(cur); cur = ""
        else: cur += c
    out.append(cur); return out

def main():
    apply = "--apply" in sys.argv
    lines = open(os.path.join(ROOT, "build", "halo_text.dis")).read().splitlines()
    calls = []
    for i, l in enumerate(lines):
        if re.search(r"\tcall\s+0x4ec940$", l):
            a = int(l.split(":")[0].strip(), 16); win = " ".join(lines[max(0, i - 14):i])
            calls.append((a, bool(re.search(r"mov\s+eax,0x871de0", win) and re.search(r"mov\s+edx,0x7ff8", win))))
    n = 0
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(p, encoding="utf-8", errors="replace").read(); cut = t.rfind("#if 0")
        b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        m = re.search(r"^[ \t]*extern[^;]*\bmessage_delta_encode_message\s*\(([^;]*)\)\s*;[^\n]*\n", b, re.M | re.S)
        if not m or "extra_eax" in m.group(1): continue
        if len(split_args(m.group(1))) != 7: continue
        h = g.HDR.search(t[:4000])
        if not h: continue
        a0, sz = int(h.group(1), 16), int(h.group(2))
        mine = [ok for a, ok in calls if a0 <= a < a0 + sz]
        if not mine or not all(mine): continue
        body = b[m.end():]
        cs = list(re.finditer(r"\bmessage_delta_encode_message\s*\(", body))
        new, pos, good = "", 0, True
        for c in cs:
            i = c.end(); depth = 1; j = i
            while depth:
                if body[j] == "(": depth += 1
                elif body[j] == ")": depth -= 1
                j += 1
            args = body[i:j - 1]
            if len(split_args(args)) != 7: good = False; break
            new += body[pos:i] + "(int32_t)network_message_scratch, 0x7ff8, " + args + ")"; pos = j
        if not good or not cs: continue
        new += body[pos:]
        nb = b[:m.start()] + DECL + "\n" + new
        n += 1; print(os.path.relpath(p, ROOT), len(cs), "call(s)")
        if apply: open(p, "w", encoding="utf-8", newline="").write(nb + tail)
    print(("fixed" if apply else "would fix"), n)

if __name__ == "__main__": main()
