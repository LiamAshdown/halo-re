"""network_session_broadcast_to_flagged (0x4e1a80) takes the message length in bits in EAX -- at almost every call site
the return value of the message_delta_encode_message call just before it. For each caller whose EVERY binary call
site has EAX straight from a preceding call to 0x4ec940 (no other call or EAX write in between), pass the encoder's
result: `x = encode(...); ... broadcast(server, ...)` becomes broadcast(x, server, ...), and a bare
`encode(...);` statement immediately followed by the broadcast becomes broadcast(encode(...), server, ...).
Usage: python tools/fix_broadcast_bits.py [--apply]"""
import os, re, sys, glob
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "harness")); sys.path.insert(0, os.path.join(ROOT, "tools"))
import gen_hooks as g
from fix_register_arg_callers import split_args
DECL = ("extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)\n"
        "extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t param_1, void *data,\n"
        "    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, EAX bits, ECX server\n")

def call_span(s, i):
    depth = 1; j = i
    while depth:
        if s[j] == "(": depth += 1
        elif s[j] == ")": depth -= 1
        j += 1
    return j

def main():
    apply = "--apply" in sys.argv
    lines = open(os.path.join(ROOT, "build", "halo_text.dis")).read().splitlines()
    sites = []
    for i, l in enumerate(lines):
        if not re.search(r"\tcall\s+0x4e1a80$", l): continue
        ok = False
        for x in reversed(lines[max(0, i - 30):i]):
            ins = x.split("\t")[-1].strip()
            if ins.startswith("call"): ok = "0x4ec940" in ins; break
            if re.match(r"(mov|lea|xor|or|movsx|movzx|pop|add|sub|and|inc|dec)\s+(eax|ax|al)\b", ins): break
        sites.append((int(l.split(":")[0].strip(), 16), ok))
    n = 0; changed = []
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(p, encoding="utf-8", errors="replace").read(); cut = t.rfind("#if 0")
        b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        m = re.search(r"^[ \t]*extern[^;]*\bnetwork_session_broadcast_to_flagged\s*\(\s*void \*server[^;]*;[^\n]*\n", b, re.M | re.S)
        if not m: continue
        h = g.HDR.search(t[:4000])
        if not h: continue
        a0, sz = int(h.group(1), 16), int(h.group(2))
        mine = [ok for a, ok in sites if a0 <= a < a0 + sz]
        if not mine or not all(mine): continue
        body = b[m.end():]
        calls = list(re.finditer(r"\bnetwork_session_broadcast_to_flagged\s*\(", body))
        if len(calls) != len(mine): continue
        good = True; edits = []
        for c in calls:
            before = body[:c.start()]
            e = list(re.finditer(r"\bmessage_delta_encode_message\s*\(", before))
            if not e: good = False; break
            e = e[-1]; ej = call_span(body, e.end())
            between = body[ej:c.start()]
            assign = re.search(r"(\w+)\s*=\s*$", body[:e.start()])
            if assign and not re.search(r"\b" + assign.group(1) + r"\s*=[^=]", between) and not re.search(r"\bmessage_delta_encode_message\b|\bnetwork_session_broadcast_to_flagged\b", between):
                edits.append(("prefix", c.end(), assign.group(1) + ", "))
            elif re.fullmatch(r"\s*;\s*", between) and re.search(r"(^|[;{}]\s*)$", body[:e.start()]):
                edits.append(("nest", e.start(), ej, c.start(), c.end()))
            else:
                good = False; break
        if not good: continue
        for ed in sorted(edits, key=lambda x: -x[1]):
            if ed[0] == "prefix":
                body = body[:ed[1]] + ed[2] + body[ed[1]:]
            else:
                _, es, ej, cs, ce = ed
                enc = body[es:ej]
                body = body[:es] + body[cs:ce] + enc + ", " + body[ce:]
        nb = b[:m.start()] + DECL + body
        n += 1; changed.append(os.path.relpath(p, ROOT)); print(os.path.relpath(p, ROOT), len(calls), [x[0] for x in edits])
        if apply: open(p, "w", encoding="utf-8", newline="").write(nb + tail)
    print(("fixed" if apply else "would fix"), n)
    if apply: open(os.path.join(ROOT, "build", "register_arg_callers_changed.txt"), "w").write("\n".join(changed))

if __name__ == "__main__": main()
