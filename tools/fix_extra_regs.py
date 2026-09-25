"""For rewrites whose register notes name a register the original never reads as an input (it overwrites it,
only saves it, or the value really arrives on the stack): drop those mappings so the parameters become stack
parameters, but only when the binary's own stack-argument count confirms it.
Stack-argument count: the highest [esp+N] / [ebp+N] slot above the return address that the original reads,
tracking ESP through push/pop/sub/add/call cleanup along the fall-through path (small functions only), and it
must also agree with ret N when the function pops its own arguments.
Only functions harness/build/hooks_report.json lists under the register-argument skip, and only when the notes
name MORE registers than the binary reads (never fewer).  Usage: python tools/fix_extra_regs.py [--apply]"""
import os, re, sys, json, glob, subprocess
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); sys.path.insert(0, os.path.join(ROOT, "tools"))
import gen_hooks as g
import normalize_blamcc as nb

def stack_arg_count(addr, size):
    """number of 4-byte stack arguments the original reads, or None when it cannot be tracked"""
    ins = g._insns_of(addr, g.pop_limit(addr, size))
    delta, ebp_delta, top = 0, None, 0     # delta = bytes pushed since entry (return address at [esp+delta])
    for a, op, args in ins:
        a_ = args.replace(" ", "")
        for m in re.finditer(r"\[esp\+0x([0-9a-f]+)\]", a_):
            off = int(m.group(1), 16) - delta
            if off >= 4: top = max(top, (off - 4) // 4 + 1)
        if ebp_delta is not None:
            for m in re.finditer(r"\[ebp\+0x([0-9a-f]+)\]", a_):
                off = int(m.group(1), 16) - ebp_delta
                if off >= 4: top = max(top, (off - 4) // 4 + 1)
        if op == "push": delta += 4
        elif op == "pop": delta -= 4
        elif op == "pushf": delta += 4
        elif op == "popf": delta -= 4
        elif op == "sub" and a_.startswith("esp,0x"): delta += int(a_[6:], 16)
        elif op == "add" and a_.startswith("esp,0x"): delta -= int(a_[6:], 16)
        elif op == "mov" and a_ == "ebp,esp": ebp_delta = delta
        elif op in ("ret", "jmp") or op.startswith("j") or op == "call" and not re.fullmatch(r"0x[0-9a-f]+", args):
            if op in ("ret", "jmp"): break
        if op == "call" and a_ == "0x628240": return None          # _chkstk: frame size in EAX, not trackable here
        if delta < 0: return None
    return top

def main():
    apply = "--apply" in sys.argv
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        h = g.HDR.search(open(p, encoding="utf-8", errors="replace").read(4000))
        if h: g.FUNC_SIZES[int(h.group(1), 16)] = int(h.group(2))
    rep = json.load(open(os.path.join(ROOT, "harness", "build", "hooks_report.json")))
    done = 0
    for s in rep["skipped"]["register arguments differ from the original's live-in registers"]:
        name = s.split(" ")[0]
        notes, binary = re.findall(r"notes \[(.*?)\], binary \[(.*?)\]", s)[0]
        nn, bb = set(re.findall(r"\w+", notes)), set(re.findall(r"\w+", binary))
        if not (nn - bb) or (bb - nn): continue
        files = glob.glob(os.path.join(ROOT, "src", "*", name + ".c"))
        if not files: continue
        p = files[0]; t = open(p, encoding="utf-8", errors="replace").read()
        cut = t.rfind("#if 0"); b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        d = [m for m in g.DEF.finditer(b) if m.group(2) == name]; h = g.HDR.search(t[:4000])
        if not d or not h: continue
        addr, size = int(h.group(1), 16), int(h.group(2))
        if size > 400: continue
        params = [x for x in (g.param_info(q) for q in g.split_params(d[0].group(3))) if x]
        names = [x["name"] for x in params]
        hdr_end = re.search(r'^(#include|extern)', t, re.M); own = t[:hdr_end.start()] if hdr_end else t[:3000]
        above = b[:d[0].start()].rstrip("\n").split("\n"); k = len(above)
        while k > 0 and above[k - 1].lstrip().startswith("//"): k -= 1
        own += "\n" + "\n".join(above[k:]) + "\n" + d[0].group(0)
        regs, _, _, order = g.parse_cc(own, names)
        keep = {n: r for n, r in regs.items() if r in bb}
        if sorted(set(keep.values())) != sorted(bb): continue
        stack = [n for n in names if n not in keep]
        if order and sorted(order) == sorted(n for n in names if n not in regs): stack = [n for n in order] + [n for n in stack if n not in order]
        ordered = bool(order) and sorted(order) == sorted(stack)
        if len(stack) > 1 and not ordered: continue       # the slot order would be a guess: leave for a person
        want = sum(2 if x["size"] == 8 else 1 for x in params if x["name"] in stack)
        have = stack_arg_count(addr, size)
        rets = g.ret_cleanup(addr, size)
        if have is None or have != want or (rets and max(rets) not in (0, 4 * want)): continue
        R = {"eax": "EAX", "ecx": "ECX", "edx": "EDX", "ebx": "EBX", "esi": "ESI", "edi": "EDI"}
        # keep the width the notes gave (AL/DX...) by reusing the register token from the note text where possible
        line = "// blam-cc: " + ", ".join(f"{R[keep[n]]} -> {n}" for n in names if n in keep) + \
               ((", " if keep else "") + "stack -> " + ", ".join(stack) if stack else "")
        line = line.replace("blam-cc: , ", "blam-cc: ")
        print(f"{name}: dropped {sorted(nn - bb)}; {line[3:]}  (binary reads {have} stack args)")
        done += 1
        if not apply: continue
        lines = b.split("\n"); outl = []; skip = False
        for l in lines:
            if (re.search(r"blam-cc:", l) or re.search(r"register convention:", l)) and l.lstrip().startswith("//"): skip = True; continue
            if skip and re.match(r"\s*//\s{2,}", l): continue
            skip = False; outl.append(l)
        nb_ = "\n".join(outl)
        d2 = [m for m in g.DEF.finditer(nb_) if m.group(2) == name][0]
        note = (f"// FIXED (register inputs, objdump): the original never reads {', '.join(r.upper() for r in sorted(nn - bb))} as an input "
                f"(it overwrites or only saves it); those parameters arrive on the stack ({have} stack argument(s) read).\n")
        nb_ = nb_[:d2.start()] + note + line + "\n" + nb_[d2.start():]
        open(p, "w", encoding="utf-8", newline="").write(nb_ + tail)
    print(("fixed" if apply else "would fix"), done)

if __name__ == "__main__": main()
