"""Context pack for one or more functions: everything an agent needs to name/type/rewrite it.
Usage: python tools/pack.py 0x4xxxxx [0x4yyyyy ...] [--out dir]
Reads out/functions.json (ExportMeta), out/halo_decompiled.c (ExportDecompiledC), symbols/candidates_*.json, modules.json."""
import json, sys, os, re, bisect

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def P(*a): return os.path.join(ROOT, *a)

_meta = None; _dec = None; _cea = None; _xbox = None; _mods = None
def meta():
    global _meta
    if _meta is None:
        _meta = {int(f["addr"], 16): f for f in json.load(open(P("out", "functions.json")))}
    return _meta
def decomp():
    """addr -> decompiled C, built once from the single export file."""
    global _dec
    if _dec is None:
        _dec = {}
        cur = None; buf = []
        for line in open(P("out", "halo_decompiled.c"), encoding="utf-8", errors="replace"):
            m = re.match(r"// ===== (\S+) @ ([0-9a-f]{8}) =====", line)
            if m:
                if cur is not None: _dec[cur] = "".join(buf)
                cur = int(m.group(2), 16); buf = []
            else:
                buf.append(line)
        if cur is not None: _dec[cur] = "".join(buf)
    return _dec
def cea():
    global _cea
    if _cea is None: _cea = json.load(open(P("symbols", "candidates_cea.json")))
    return _cea
def xbox():
    global _xbox
    if _xbox is None: _xbox = json.load(open(P("symbols", "candidates_xbox.json")))
    return _xbox
def modules():
    global _mods
    if _mods is None:
        _mods = json.load(open(P("modules.json"))) if os.path.exists(P("modules.json")) else {}
    return _mods

def candidates(f, limit=8):
    """CEA / Xbox functions sharing string literals or name tokens with this function."""
    out = []
    strs = set(s.strip() for s in f.get("strings", []))
    if strs:
        for name, v in cea().items():
            hit = strs & set(v.get("strings", []))
            if hit: out.append(("cea-pdb", name, v["module"], sorted(hit)[:3]))
    # callee-name overlap: if callees carry engine names, CEA functions with the same prefix are likely neighbors
    prefixes = set()
    for c in f.get("callees", []):
        nm = c.rsplit(":", 1)[-1]
        if c.startswith("EXTERNAL") or nm.startswith(("FUN_", "thunk_", "sig__", "os__", "_")): continue
        p = nm.split("_")[0]
        if p: prefixes.add(p)
    out.sort(key=lambda x: -len(x[3]))
    return out[:limit], sorted(prefixes)

def pack(addr):
    f = meta().get(addr)
    if f is None: return "no function at 0x%06x\n" % addr
    m = modules().get("%06x" % addr) or modules().get(f["addr"])
    if isinstance(m, dict): m = "%s (%.1f)" % (m.get("module"), m.get("confidence", 0))
    lines = []
    lines.append("# %s @ 0x%06x" % (f["name"], addr))
    lines.append("size=%d bytes  callers=%d  cc=%s  lib=%s  module=%s" % (f["size"], f["callers"], f["cc"], f["lib"], m))
    lines.append("signature: " + f["signature"])
    if f["strings"]:
        lines.append("\n## strings referenced"); lines += ["- " + json.dumps(s) for s in f["strings"]]
    if f["globals"]:
        lines.append("\n## globals referenced"); lines.append(", ".join(f["globals"]))
    if f["callees"]:
        lines.append("\n## callees"); lines.append(", ".join(f["callees"]))
    cands, prefixes = candidates(f)
    if cands or prefixes:
        lines.append("\n## naming hints (hints only, verify against the code)")
        for src, name, mod, hit in cands: lines.append("- %s: %s (%s) via strings %s" % (src, name, mod, hit))
        if prefixes: lines.append("- callee name prefixes: " + ", ".join(prefixes))
    lines.append("\n## decompiled C (Ghidra)\n```c")
    lines.append(decomp().get(addr, "// (not exported)").rstrip())
    lines.append("```")
    return "\n".join(lines) + "\n"

if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    outdir = None
    if "--out" in sys.argv: outdir = sys.argv[sys.argv.index("--out") + 1]; args = [a for a in args if a != outdir]
    for a in args:
        addr = int(a, 16)
        text = pack(addr)
        if outdir:
            os.makedirs(outdir, exist_ok=True)
            open(os.path.join(outdir, "%06x.md" % addr), "w", encoding="utf-8").write(text)
        else:
            sys.stdout.write(text)
