"""Register notes that name a parameter by an abbreviation ("ECX -> object_list_header" for a parameter called
object_list_header_handle) are not matched by harness/gen_hooks.py's note parser, so the register looks missing.
For register-skip functions whose missing register is named in the notes, replace each note name that is not a
parameter but is a unique prefix of one (or that a parameter uniquely prefixes) with the parameter's name, and keep
the change only when tools/regcheck.py then agrees with the binary. Usage: python tools/fix_note_names.py [--apply]"""
import os, re, sys, json, glob, subprocess
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); import gen_hooks as g
REG = r"(?:E?[ABCD]X|[ABCD]L|E?SI|E?DI)"

def main():
    apply = "--apply" in sys.argv
    rep = json.load(open(os.path.join(ROOT, "harness", "build", "hooks_report.json")))
    fixed = 0
    for s in rep["skipped"]["register arguments differ from the original's live-in registers"]:
        name = s.split(" ")[0]
        notes, binary = re.findall(r"notes \[(.*?)\], binary \[(.*?)\]", s)[0]
        nn, bb = set(re.findall(r"\w+", notes)), set(re.findall(r"\w+", binary))
        if not (bb - nn): continue
        p = glob.glob(os.path.join(ROOT, "src", "*", name + ".c"))[0]
        t = open(p, encoding="utf-8").read(); cut = t.rfind("#if 0"); b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        d = [m for m in g.DEF.finditer(b) if m.group(2) == name]
        if len(d) != 1: continue
        params = [x["name"] for x in (g.param_info(q) for q in g.split_params(d[0].group(3))) if x]
        def fix_line(line):
            def rep_(m):
                reg, nm = m.group(1), m.group(2)
                if nm in params: return m.group(0)
                c = [q for q in params if q.startswith(nm) or nm.startswith(q)]
                return m.group(0).replace(nm, c[0]) if len(c) == 1 else m.group(0)
            return re.sub(r"\b(" + REG + r")\s*->\s*(\w+)", rep_, line)
        nb = "\n".join(fix_line(l) if "blam-cc" in l or "register convention" in l else l for l in b.split("\n"))
        if nb == b: continue
        open(p, "w", encoding="utf-8", newline="").write(nb + tail)
        r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "regcheck.py"), name], capture_output=True, text=True).stdout
        ok = " == binary" in r
        if not ok or not apply: open(p, "w", encoding="utf-8", newline="").write(t)
        if ok: fixed += 1; print(name, "->", r.strip().splitlines()[-1][:120])
    print(("fixed" if apply else "would fix"), fixed)

if __name__ == "__main__": main()
