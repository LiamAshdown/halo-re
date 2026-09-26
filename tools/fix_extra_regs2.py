"""Looser companion of tools/fix_extra_regs.py, meant to be CHECKED BY harness/difftest before anything is kept.
For register-skip functions whose notes name a register the original never reads (and miss none it does read):
drop those register mappings and make the parameters stack parameters, placing the moved ones FIRST or LAST among
the stack parameters (--position first|last), when the original's own stack-argument count equals the result.
Files changed are listed in build/extra_regs2_changed.txt; the original text of each is kept in build/extra_regs2_backup/.
Usage: python tools/fix_extra_regs2.py --position first|last [--apply]"""
import os, re, sys, json, glob, shutil
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); sys.path.insert(0, os.path.join(ROOT, "tools"))
import gen_hooks as g
from fix_extra_regs import stack_arg_count
R = {"eax": "EAX", "ecx": "ECX", "edx": "EDX", "ebx": "EBX", "esi": "ESI", "edi": "EDI"}

def main():
    apply = "--apply" in sys.argv
    pos = sys.argv[sys.argv.index("--position") + 1]
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        h = g.HDR.search(open(p, encoding="utf-8", errors="replace").read(4000))
        if h: g.FUNC_SIZES[int(h.group(1), 16)] = int(h.group(2))
    rep = json.load(open(os.path.join(ROOT, "harness", "build", "hooks_report.json")))
    changed = []; bk = os.path.join(ROOT, "build", "extra_regs2_backup"); os.makedirs(bk, exist_ok=True)
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
        if len(d) != 1 or not h: continue
        addr, size = int(h.group(1), 16), int(h.group(2))
        params = [x for x in (g.param_info(q) for q in g.split_params(d[0].group(3))) if x]
        names = [x["name"] for x in params]
        hdr_end = re.search(r'^(#include|extern)', t, re.M); own = t[:hdr_end.start()] if hdr_end else t[:3000]
        above = b[:d[0].start()].rstrip("\n").split("\n"); k = len(above)
        while k > 0 and above[k - 1].lstrip().startswith("//"): k -= 1
        own += "\n" + "\n".join(above[k:]) + "\n" + d[0].group(0)
        regs, _, _, order = g.parse_cc(own, names)
        if not regs: continue
        keep = {n: r for n, r in regs.items() if r in bb}
        moved = [n for n in names if n in regs and n not in keep]
        if sorted(set(keep.values())) != sorted(bb) or not moved: continue
        rest = [n for n in (order if order and sorted(order) == sorted(n for n in names if n not in regs) else names) if n not in regs]
        stack = moved + rest if pos == "first" else rest + moved
        want = sum(2 if x["size"] == 8 else 1 for x in params if x["name"] in stack)
        have = stack_arg_count(addr, size)
        if have is None or have != want: continue
        line = "// blam-cc: " + ", ".join([f"{R[keep[n]]} -> {n}" for n in names if n in keep] + ["stack -> " + ", ".join(stack)])
        changed.append(p); print(f"{name}: moved {moved} to the stack ({pos}); {line[3:]}")
        if not apply: continue
        shutil.copy(p, os.path.join(bk, os.path.basename(p)))
        lines = b.split("\n"); outl = []; skip = False
        for l in lines:
            if (re.search(r"blam-cc:", l) or re.search(r"register convention:", l)) and l.lstrip().startswith("//"): skip = True; continue
            if skip and re.match(r"\s*//\s{2,}", l): continue
            skip = False; outl.append(l)
        nb = "\n".join(outl)
        d2 = [m for m in g.DEF.finditer(nb) if m.group(2) == name][0]
        note = (f"// FIXED (register inputs, objdump + difftest): the original never reads "
                f"{', '.join(R[regs[n]] for n in moved)}; {', '.join(moved)} arrive(s) on the stack ({have} stack argument(s)).\n")
        nb = nb[:d2.start()] + note + line + "\n" + nb[d2.start():]
        open(p, "w", encoding="utf-8", newline="").write(nb + tail)
    print(("changed" if apply else "would change"), len(changed))
    if apply: open(os.path.join(ROOT, "build", "extra_regs2_changed.txt"), "w").write("\n".join(changed))

if __name__ == "__main__": main()
