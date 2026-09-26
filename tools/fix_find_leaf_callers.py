"""bsp3d_node_find_leaf (0x5013a0) is (node_index EAX, bsp ECX, point EDX). Many callers declare it as
(void *globals, real_point3d *point, int32_t index) and call it (global_collision_bsp, point, 0). For each such caller
whose every binary call site zeroes EAX and loads ECX from 0x746f90 (global_collision_bsp), switch to the definition's
declaration and reorder each call to (index, (ModelCollisionGeometryBSP *)globals, point).
Usage: python tools/fix_find_leaf_callers.py [--apply]"""
import os, re, sys, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); sys.path.insert(0, os.path.join(ROOT, "tools"))
import gen_hooks as g
from fix_register_arg_callers import split_args
DECL = ("extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);"
        " // 0x5013a0, EAX node, ECX bsp, EDX point\n")

def main():
    apply = "--apply" in sys.argv
    lines = open(os.path.join(ROOT, "build", "halo_text.dis")).read().splitlines()
    sites = []
    for i, l in enumerate(lines):
        if re.search(r"\tcall\s+0x5013a0$", l):
            win = " ".join(x.split("\t")[-1] for x in lines[max(0, i - 10):i])
            sites.append((int(l.split(":")[0].strip(), 16),
                          bool(re.search(r"xor\s+eax,eax", win) and re.search(r"mov\s+ecx,DWORD PTR ds:0x746f90", win))))
    n = 0; changed = []
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(p, encoding="utf-8", errors="replace").read(); cut = t.rfind("#if 0")
        b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        m = re.search(r"^[ \t]*extern[^;]*\bbsp3d_node_find_leaf\s*\(\s*void \*globals, real_point3d \*point, int32_t index\s*\)\s*;[^\n]*\n", b, re.M)
        if not m: continue
        h = g.HDR.search(t[:4000])
        if not h: continue
        a0, sz = int(h.group(1), 16), int(h.group(2))
        mine = [ok for a, ok in sites if a0 <= a < a0 + sz]
        if not mine or not all(mine): continue
        body = b[m.end():]
        cs = list(re.finditer(r"\bbsp3d_node_find_leaf\s*\(", body))
        new, pos, good = "", 0, True
        for c in cs:
            i = c.end(); depth = 1; j = i
            while depth:
                if body[j] == "(": depth += 1
                elif body[j] == ")": depth -= 1
                j += 1
            args = [x.strip() for x in split_args(body[i:j - 1])]
            if len(args) != 3 or args[2] != "0": good = False; break
            new += body[pos:i] + f"0, (ModelCollisionGeometryBSP *){args[0]}, {args[1]})"; pos = j
        if not good or not cs: continue
        new += body[pos:]
        nb = b[:m.start()] + DECL + new
        n += 1; changed.append(os.path.relpath(p, ROOT)); print(os.path.relpath(p, ROOT), len(cs), "call(s)")
        if apply: open(p, "w", encoding="utf-8", newline="").write(nb + tail)
    print(("fixed" if apply else "would fix"), n)
    if apply: open(os.path.join(ROOT, "build", "register_arg_callers_changed.txt"), "w").write("\n".join(changed))

if __name__ == "__main__": main()
