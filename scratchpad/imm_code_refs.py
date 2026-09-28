"""functions referenced only as immediates in retail code (push/mov imm of a .text address) that have no C:
the missing-function scan only looked at data tables. Reports target, who references it and whether the referencing
function has C."""
import bisect, json, re
R = "C:\\Users\\Liam-\\halo-re\\"
funcs = {int(f["addr"], 16): f for f in json.load(open(R + "out\\functions.json"))}
starts = sorted(funcs)
entries = {e["addr"]: e["c_symbol"] for e in json.load(open(R + "build\\standalone\\code_entries.json"))}
entry_starts = sorted(entries)
T0, T1 = 0x401000, 0x639596
exe = open(R + "bin\\halo.exe", "rb").read()


def container(a):
    i = bisect.bisect_right(entry_starts, a) - 1
    return entries[entry_starts[i]] if i >= 0 else "?"


def looks_like_entry(v):
    if v in funcs or v in entries:
        return True
    o = v - 0x400000
    return v % 16 == 0 and exe[o - 1] in (0xcc, 0x90, 0xc3)


hits = {}
for line in open(R + "scratchpad\\text_all.dis", errors="replace"):
    m = re.match(r"\s*([0-9a-f]+):\s+(push|mov)\s+(.*)", line)
    if not m:
        continue
    for h in re.findall(r"0x([0-9a-f]{6})\b", m.group(3)):
        v = int(h, 16)
        if not (T0 <= v < T1) or "[" in m.group(3).split(",")[-1] and m.group(2) == "mov" and "PTR" in m.group(3).split(",")[-1]:
            continue
        if m.group(2) == "mov" and "ds:" in m.group(3):
            continue
        if v in entries or not looks_like_entry(v):
            continue
        a = int(m.group(1), 16)
        hits.setdefault(v, []).append((a, container(a)))
for v in sorted(hits):
    f = funcs.get(v, {})
    lib = "lib" if f.get("lib") or f.get("fid") else ""
    print("%06x %-28s %-4s refs: %s" % (v, f.get("name", "?")[:28], lib,
                                        ", ".join("%06x(%s)" % r for r in hits[v][:3])))
print(len(hits), "targets")
