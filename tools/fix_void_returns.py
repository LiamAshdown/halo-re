"""Give a return value to rewrites declared void whose original leaves a known value in EAX at every ret:
  * game_engine_variant_defaults_*: EAX = the variant_options argument (mov eax,[ebp+8] before the rep movs)
  * a fixed list whose every ret is preceded by mov eax,1
The definition's return type changes and every `return;` / the end of the body returns that value.
Usage: python tools/fix_void_returns.py [--apply]"""
import os, re, sys, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); import gen_hooks as g
CONST1 = {"actor_build_order_guard", "actor_build_order_return_to_anchor", "network_game_session_reset_defaults"}

def patch(p, name, rtype, value, note, apply):
    t = open(p, encoding="utf-8").read(); cut = t.rfind("#if 0"); b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
    d = [m for m in g.DEF.finditer(b) if m.group(2) == name]
    if not d or not re.match(r"\s*void\b", d[0].group(1)): return False
    start = d[0].end() - 1; depth = 0; end = None
    for i in range(start, len(b)):
        if b[i] == "{": depth += 1
        elif b[i] == "}":
            depth -= 1
            if depth == 0: end = i; break
    body = b[start:end]
    body = re.sub(r"\breturn\s*;", f"return {value};", body)
    body = body.rstrip() + f"\n    return {value};\n"
    head = b[:d[0].start()] + f"// FIXED: {note}\n" + re.sub(r"\bvoid\b", rtype, d[0].group(0)[:-1], count=1) + "{"
    nb = head + body[1:] + b[end:]
    if apply: open(p, "w", encoding="utf-8", newline="").write(nb + tail)
    return True

def main():
    apply = "--apply" in sys.argv; n = 0
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        name = os.path.basename(p)[:-2]
        if name.startswith("game_engine_variant_defaults_"):
            t = open(p, encoding="utf-8").read()
            m = re.search(r"\bvoid\s+" + name + r"\s*\(\s*game_variant\s*\*\s*(\w+)\s*\)", t)
            if m and patch(p, name, "game_variant *", m.group(1),
                           "the original returns its argument in EAX (mov eax,[ebp+8] ... rep movs; callers keep it)", apply):
                n += 1; print(name)
        elif name in CONST1:
            if patch(p, name, "int32_t", "1", "every ret of the original is preceded by mov eax,1 and callers test it", apply):
                n += 1; print(name)
    print(("patched" if apply else "would patch"), n)

if __name__ == "__main__": main()
