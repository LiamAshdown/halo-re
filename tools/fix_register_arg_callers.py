"""Callers written from Ghidra often declare a callee with only its STACK arguments, dropping the register
arguments the original loads right before every call (e.g. a buffer in EAX, the server globals in ECX). For a
configured callee: for each caller whose EVERY call site in the binary loads the configured registers in the
preceding instructions, and whose C calls all pass exactly the stack arguments, replace the declaration with the
definition's and prepend the register arguments to each call.
Usage: python tools/fix_register_arg_callers.py <encode|broadcast> [--apply]"""
import os, re, sys, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); import gen_hooks as g

CFG = {
    "encode": dict(
        addr=0x4ec940, name="message_delta_encode_message",
        regs=[r"mov\s+eax,0x871de0", r"mov\s+edx,0x7ff8"], nstack=7,
        prefix="(int32_t)network_message_scratch, 0x7ff8, ", marker="extra_eax",
        decl="extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0\n"
             "extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,\n"
             "    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size"),
    "broadcast": dict(
        addr=0x4e1a80, name="network_session_broadcast_to_flagged",
        regs=[r"mov\s+ecx,DWORD PTR ds:0x71c2d4"], nstack=6,
        prefix="network_server_pointer, ", marker="void *server",
        decl="extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)\n"
             "extern char network_session_broadcast_to_flagged(void *server, int32_t param_1, void *data,\n"
             "    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, ECX server"),
}

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
    cfg = CFG[sys.argv[1]]
    lines = open(os.path.join(ROOT, "build", "halo_text.dis")).read().splitlines()
    calls = []
    for i, l in enumerate(lines):
        if re.search(r"\tcall\s+0x%x$" % cfg["addr"], l):
            a = int(l.split(":")[0].strip(), 16); win = " ".join(lines[max(0, i - 14):i])
            calls.append((a, all(re.search(x, win) for x in cfg["regs"])))
    n = 0; changed = []
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(p, encoding="utf-8", errors="replace").read(); cut = t.rfind("#if 0")
        b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        m = re.search(r"^[ \t]*extern[^;]*\b" + cfg["name"] + r"\s*\(([^;]*)\)\s*;[^\n]*\n", b, re.M | re.S)
        if not m or cfg["marker"] in m.group(1): continue
        if len(split_args(m.group(1))) != cfg["nstack"]: continue
        h = g.HDR.search(t[:4000])
        if not h: continue
        a0, sz = int(h.group(1), 16), int(h.group(2))
        mine = [ok for a, ok in calls if a0 <= a < a0 + sz]
        if not mine or not all(mine): continue
        body = b[m.end():]
        cs = list(re.finditer(r"\b" + cfg["name"] + r"\s*\(", body))
        new, pos, good = "", 0, True
        for c in cs:
            i = c.end(); depth = 1; j = i
            while depth:
                if body[j] == "(": depth += 1
                elif body[j] == ")": depth -= 1
                j += 1
            args = body[i:j - 1]
            if len(split_args(args)) != cfg["nstack"]: good = False; break
            new += body[pos:i] + cfg["prefix"] + args + ")"; pos = j
        if not good or not cs: continue
        new += body[pos:]
        nb = b[:m.start()] + cfg["decl"] + "\n" + new
        n += 1; changed.append(os.path.relpath(p, ROOT)); print(os.path.relpath(p, ROOT), len(cs), "call(s)")
        if apply: open(p, "w", encoding="utf-8", newline="").write(nb + tail)
    print(("fixed" if apply else "would fix"), n)
    if apply: open(os.path.join(ROOT, "build", "register_arg_callers_changed.txt"), "w").write("\n".join(changed))

if __name__ == "__main__": main()
