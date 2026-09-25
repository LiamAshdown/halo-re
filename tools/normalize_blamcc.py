"""Rewrite register-convention notes the hook generator cannot parse into its canonical form, when the prose
already says which register carries which parameter and that agrees EXACTLY with the registers the original
binary reads (harness/gen_hooks.py live_in).
Recognised prose (register names EAX..EDI and their 8/16-bit parts; parameter names must be the definition's):
  "name -> REG", "REG -> name", "name in REG", "name (REG)", "name /*REG*/", "REG = name", "name = REG", "name=REG"
The canonical line written directly above the definition (and replacing every other "blam-cc:" line in the
header) is:  // blam-cc: REG -> name, REG -> name, stack -> a, b
Only functions harness/build/hooks_report.json lists under the register-argument skip are considered.
Usage: python tools/normalize_blamcc.py [--apply]"""
import os, re, sys, json, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); import gen_hooks as g

REG = r"(E[A-D]X|E[SD]I|[A-D]X|[A-D]L|SI|DI)"
FULLR = {"EAX": "eax", "AX": "eax", "AL": "eax", "ECX": "ecx", "CX": "ecx", "CL": "ecx", "EDX": "edx", "DX": "edx", "DL": "edx",
         "EBX": "ebx", "BX": "ebx", "BL": "ebx", "ESI": "esi", "SI": "esi", "EDI": "edi", "DI": "edi"}

def mappings(text, names):
    text = re.sub(r"\b(?:in|unaff|extraout)_(E?[A-D]X|E?[SD]I|[A-D][LH])\b", r"\1", text)
    nm = "|".join(sorted((re.escape(n) for n in names), key=len, reverse=True))
    if not nm: return {}
    pats = [rf"\b({nm})\b\s*(?:->|=|:)\s*{REG}\b", rf"\b{REG}\s*(?:->|=|:)\s*\b({nm})\b", rf"\b({nm})\b\s+in\s+{REG}\b",
            rf"\b({nm})\b\s*\(\s*{REG}\s*\)", rf"\b({nm})\b\s*/\*\s*{REG}\s*\*/", rf"\b({nm})\b\s*\(\s*in\s+{REG}\s*\)"]
    out = {}
    for k, p in enumerate(pats):
        for m in re.finditer(p, text):
            a, b = m.group(1), m.group(2)
            name, reg = (b, a) if k == 1 else (a, b)
            out.setdefault(name, reg.upper())
    return out

def main():
    apply = "--apply" in sys.argv
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        h = g.HDR.search(open(p, encoding="utf-8", errors="replace").read(4000))
        if h: g.FUNC_SIZES[int(h.group(1), 16)] = int(h.group(2))
    rep = json.load(open(os.path.join(ROOT, "harness", "build", "hooks_report.json")))
    skipped = rep["skipped"]["register arguments differ from the original's live-in registers"]
    done = 0
    for s in skipped:
        name = s.split(" ")[0]
        files = glob.glob(os.path.join(ROOT, "src", "*", name + ".c"))
        if not files: continue
        p = files[0]; t = open(p, encoding="utf-8", errors="replace").read()
        cut = t.rfind("#if 0"); b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        d = [m for m in g.DEF.finditer(b) if m.group(2) == name]; h = g.HDR.search(t[:4000])
        if not d or not h: continue
        params = [x for x in (g.param_info(q) for q in g.split_params(d[0].group(3))) if x]
        names = [x["name"] for x in params]
        # all comment text above the definition (header and notes) -- prose only, not the Ghidra block
        prose = "\n".join(l for l in b[:d[0].start()].splitlines() if l.lstrip().startswith("//")) + "\n" + d[0].group(0)
        mp = mappings(prose, names)
        live = g.live_in(int(h.group(1), 16), int(h.group(2)))
        regs = sorted({FULLR[r] for r in mp.values()})
        if not mp or regs != live or len(set(FULLR[r] for r in mp.values())) != len(mp): continue
        stack = [n for n in names if n not in mp]
        # keep a stack order the notes already give ("stack -> (b, a)"), which came from the disassembly
        order = g.parse_cc(prose, names)[3]
        if order and sorted(order) == sorted(stack): stack = list(order)
        line = "// blam-cc: " + ", ".join(f"{mp[n]} -> {n}" for n in names if n in mp) + (", stack -> " + ", ".join(stack) if stack else "")
        print(f"{name}: {line[3:]}")
        if not apply: done += 1; continue
        # drop every existing blam-cc line (and its continuation lines) in the live part, then add the canonical one
        lines = b.split("\n"); outl = []; skip = False
        for l in lines:
            if re.search(r"blam-cc:", l) and l.lstrip().startswith("//"): skip = True; continue
            if skip and re.match(r"\s*//\s{2,}", l) and not l.strip().startswith("// FIXED"): continue
            skip = False; outl.append(l)
        nb = "\n".join(outl)
        d2 = [m for m in g.DEF.finditer(nb) if m.group(2) == name][0]
        nb = nb[:d2.start()] + line + "\n" + nb[d2.start():]
        open(p, "w", encoding="utf-8", newline="").write(nb + tail)
        done += 1
    print(("normalized" if apply else "would normalize"), done)

if __name__ == "__main__": main()
