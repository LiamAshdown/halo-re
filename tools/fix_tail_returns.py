"""Rewrites declared void whose callers read EAX, where the original simply hands back its last callee's result:
every ret is reached from a final `call X` with no EAX/AX/AL write in between (only pops, stack adjustment and
stores of other registers), the C body's last statement is a call to X's rewrite, and the C has no early
`return;`. The definition becomes `<X's return type> name(...)` and the last statement `return X(...);`.
Only functions harness/build/hooks_report.json lists under "void in C, but callers use a value the original
leaves in EAX".  Usage: python tools/fix_tail_returns.py [--apply]"""
import os, re, sys, json, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); import gen_hooks as g

EAX_W = re.compile(r"^(eax|ax|al|ah)\b")

def tail_callee(addr, size):
    """the single callee whose result reaches every ret unchanged, or None"""
    ins = g._insns_of(addr, g.pop_limit(addr, size))
    found = set()
    for k, (a, op, args) in enumerate(ins):
        if op != "ret": continue
        j = k - 1
        while j >= 0:
            aj, opj, argsj = ins[j]
            if opj == "call":
                c = re.fullmatch(r"0x([0-9a-f]+)", argsj)
                if not c: return None
                found.add(int(c.group(1), 16)); break
            if opj in ("pop", "push") and not EAX_W.match(argsj): j -= 1; continue
            if opj in ("add", "sub", "mov", "lea", "fstp", "fst") and not EAX_W.match(argsj) and \
               not (opj == "mov" and argsj.startswith("esp")): j -= 1; continue
            return None
        if j < 0: return None
    return found.pop() if len(found) == 1 else None

def main():
    apply = "--apply" in sys.argv
    by_addr, rettype = {}, {}
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        name = os.path.splitext(os.path.basename(p))[0]
        t = open(p, encoding="utf-8", errors="replace").read(); b = t[:t.rfind("#if 0")] if "#if 0" in t else t
        h = g.HDR.search(t[:4000])
        if h: g.FUNC_SIZES[int(h.group(1), 16)] = int(h.group(2)); by_addr[int(h.group(1), 16)] = name
        for m in g.DEF.finditer(b):
            if m.group(2) == name: rettype[name] = re.sub(r"\b(static|inline)\b", "", m.group(1)).strip()
    rep = json.load(open(os.path.join(ROOT, "harness", "build", "hooks_report.json")))
    done = 0
    for s in rep["skipped"]["void in C, but callers use a value the original leaves in EAX"]:
        name = s.split(" ")[0]
        files = glob.glob(os.path.join(ROOT, "src", "*", name + ".c"))
        if not files: continue
        p = files[0]; t = open(p, encoding="utf-8", errors="replace").read()
        cut = t.rfind("#if 0"); b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        h = g.HDR.search(t[:4000]); d = [m for m in g.DEF.finditer(b) if m.group(2) == name]
        if not h or not d: continue
        x = tail_callee(int(h.group(1), 16), int(h.group(2)))
        callee = by_addr.get(x) if x else None
        if not callee or rettype.get(callee, "void") in ("void", ""): continue
        # the definition's body: from its opening brace to the matching close
        start = d[0].end() - 1; depth = 0; end = None
        for i in range(start, len(b)):
            if b[i] == "{": depth += 1
            elif b[i] == "}":
                depth -= 1
                if depth == 0: end = i; break
        if end is None: continue
        body = b[start + 1:end]
        if re.search(r"\breturn\s*;", body): continue
        stmts = [l for l in body.rstrip().split("\n") if l.strip() and not l.strip().startswith("//")]
        if not stmts: continue
        last = stmts[-1]
        m = re.match(r"^(\s*)" + re.escape(callee) + r"\s*\((.*)\);\s*(//.*)?$", last)
        if not m: continue
        newlast = f"{m.group(1)}return {callee}({m.group(2)});" + (f" {m.group(3)}" if m.group(3) else "") + \
                  "  // the original returns this call's result (EAX) unchanged"
        k = body.rfind(last)
        nbody = body[:k] + newlast + body[k + len(last):]
        head = b[:d[0].start()] + re.sub(r"^void\b", rettype[callee], d[0].group(0), count=1)
        if head == b[:d[0].start()] + d[0].group(0): continue
        nb = head + nbody + b[end:]
        print(f"{name}: returns {callee}()  ({rettype[callee]})")
        done += 1
        if apply: open(p, "w", encoding="utf-8", newline="").write(nb + tail)
    print(("fixed" if apply else "would fix"), done)

if __name__ == "__main__": main()
