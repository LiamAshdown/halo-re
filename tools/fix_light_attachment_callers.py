"""object_for_each_light_attachment (0x4f9a20) is (EAX object_index, stack register_in_table, invoke_callback). Many
callers declare (object_index, flag) and pass two arguments. For each caller whose EVERY binary call site pushes two
immediates right before the call, whose C call count matches, and whose existing second argument equals one of the
two pushed values at the same site, switch to the real declaration and pass both values exactly as the binary does.
Usage: python tools/fix_light_attachment_callers.py [--apply]"""
import os, re, sys, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); sys.path.insert(0, os.path.join(ROOT, "tools"))
import gen_hooks as g
from fix_register_arg_callers import split_args
DECL = ("extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, "
        "int32_t invoke_callback); // 0x4f9a20, EAX object, stack (register_in_table, invoke_callback)\n")

def pushes(lines, i):
    """the two immediate pushes feeding the call at line i, as (first_arg, second_arg), or None"""
    vals = []
    for x in reversed(lines[max(0, i - 8):i]):
        ins = x.split("\t")[-1].strip()
        m = re.match(r"push\s+(0x[0-9a-f]+)$", ins)
        if m: vals.append(int(m.group(1), 16));
        elif ins.startswith("push") or ins.startswith("call"): return None
        if len(vals) == 2: return vals[0], vals[1]      # last pushed = first argument
    return None

def main():
    apply = "--apply" in sys.argv
    lines = open(os.path.join(ROOT, "build", "halo_text.dis")).read().splitlines()
    sites = [(int(l.split(":")[0].strip(), 16), pushes(lines, i)) for i, l in enumerate(lines)
             if re.search(r"\tcall\s+0x4f9a20$", l)]
    n = 0; changed = []
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(p, encoding="utf-8", errors="replace").read(); cut = t.rfind("#if 0")
        b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        m = re.search(r"^[ \t]*extern void object_for_each_light_attachment\(\s*uint32_t \w+,\s*uint32_t \w+\s*\)\s*;[^\n]*\n", b, re.M)
        if not m: continue
        h = g.HDR.search(t[:4000])
        if not h: continue
        a0, sz = int(h.group(1), 16), int(h.group(2))
        mine = [pv for a, pv in sites if a0 <= a < a0 + sz]
        if not mine or any(pv is None for pv in mine): continue
        body = b[m.end():]
        cs = list(re.finditer(r"\bobject_for_each_light_attachment\s*\(", body))
        if len(cs) != len(mine): continue
        new, pos, good = "", 0, True
        for c, (first, second) in zip(cs, mine):
            i = c.end(); depth = 1; j = i
            while depth:
                if body[j] == "(": depth += 1
                elif body[j] == ")": depth -= 1
                j += 1
            args = [x.strip() for x in split_args(body[i:j - 1])]
            # the one argument Ghidra kept is either stack slot; the binary's two immediates are the truth
            if len(args) != 2 or not re.fullmatch(r"\d+", args[1]) or int(args[1]) not in (first, second): good = False; break
            new += body[pos:i] + f"{args[0]}, {first}, {second})"; pos = j
        if not good: continue
        new += body[pos:]
        n += 1; changed.append(os.path.relpath(p, ROOT)); print(os.path.relpath(p, ROOT), len(cs), "call(s)", mine)
        if apply: open(p, "w", encoding="utf-8", newline="").write(b[:m.start()] + DECL + new + tail)
    print(("fixed" if apply else "would fix"), n)
    if apply: open(os.path.join(ROOT, "build", "register_arg_callers_changed.txt"), "w").write("\n".join(changed))

if __name__ == "__main__": main()
