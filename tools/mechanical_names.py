"""Zero-agent naming pass -> symbols/agent_mechanical.txt (applied via merge_symbols) and symbols/review_queue_mech.txt.
Sources:
  1. CEA string literals: an engine function whose referenced strings match the literals of exactly one CEA function.
  2. Chimera signatures resolved at a function entry: sig name minus _sig and build-variant tokens.
Nothing here overrides a name that already came from a higher-precedence source."""
import json, re, os
from collections import defaultdict, Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
P = lambda *a: os.path.join(ROOT, *a)
funcs = json.load(open(P("out", "functions.json")))
mods = json.load(open(P("modules.json")))
cea = json.load(open(P("symbols", "candidates_cea.json")))

def unescape(s):
    try: return bytes(s, "utf-8").decode("unicode_escape", errors="ignore")
    except Exception: return s
def distinctive(s):
    s = s.strip()
    if len(s) < 6: return False
    if re.fullmatch(r"[%\w\s.:,\-\\/]*", s) and len(s) < 8: return False
    if s in {"%s", "%d", "%s\n", "%s: %s", "true", "false", "none", "unknown"}: return False
    return True

# index CEA literal -> [cea function names]
lit_index = defaultdict(set)
for name, v in cea.items():
    for s in v.get("strings", []):
        u = unescape(s).strip()
        if distinctive(u): lit_index[u].add(name)

taken_names = Counter()
accepted, queue = {}, []
stats = Counter()
for f in funcs:
    a = f["addr"][-6:]
    m = mods.get(a, {}).get("module", "")
    if m.startswith("lib") or not f["name"].startswith(("FUN_", "sig__", "thunk_FUN")): continue
    strs = [s.strip() for s in f["strings"] if distinctive(s)]
    if not strs: continue
    votes = Counter(); hits = defaultdict(list)
    for s in strs:
        owners = lit_index.get(s)
        if not owners: continue
        w = 1.0 / len(owners)   # shared strings count less
        for o in owners: votes[o] += w; hits[o].append(s)
    if not votes: stats["no-cea-hit"] += 1; continue
    best, score = votes.most_common(1)[0]
    second = votes.most_common(2)[1][1] if len(votes) > 1 else 0
    uniq = [s for s in hits[best] if len(lit_index[s]) == 1]
    cea_mod = cea[best]["module"]
    ev = "cea-pdb strings %s" % json.dumps(sorted(hits[best])[:2])
    if uniq and (len(uniq) >= 2 or len(uniq[0]) >= 14) and score > second:
        conf = 0.8 if len(uniq) >= 2 else 0.7
        if cea_mod != m and m: conf -= 0.1; ev += " module-mismatch(%s vs %s)" % (cea_mod, m)
    elif uniq and score > second:
        conf = 0.6
    else:
        conf = 0.4
    if conf >= 0.7:
        accepted[a] = (best, conf, ev); taken_names[best] += 1; stats["cea-accepted"] += 1
    else:
        queue.append((a, best, conf, ev)); stats["cea-queued"] += 1

# 2. Chimera entry signatures
VARIANT = re.compile(r"_(retail|demo|custom_edition|custom|full|ce|pc)(?=_|$)")
for line in open(P("symbols", "chimera.txt")):
    if line.startswith("#") or " entry" not in line: continue
    t = line.split(); a = t[0][-6:]
    if a in accepted: continue
    name = VARIANT.sub("", t[1]).removesuffix("_sig")
    if not name or name.endswith(("_call", "_hook", "_jmp")): continue
    f = next((x for x in funcs if x["addr"][-6:] == a), None)
    if f is None or not f["name"].startswith(("FUN_", "sig__")): continue
    accepted[a] = ("chimera__" + name, 0.7, "chimera signature at entry: " + t[1]); stats["chimera-entry"] += 1

# de-dup names: same CEA name claimed by several retail functions -> keep the highest, queue the rest
byname = defaultdict(list)
for a, (n, c, e) in accepted.items(): byname[n].append((c, a))
for n, lst in byname.items():
    if len(lst) > 1:
        lst.sort(reverse=True)
        for c, a in lst[1:]:
            queue.append((a, n, min(c, 0.5), accepted[a][2] + " [dup: also " + lst[0][1] + "]")); del accepted[a]; stats["dup-demoted"] += 1

with open(P("symbols", "agent_mechanical.txt"), "w") as fh:
    fh.write("# mechanical names (tools/mechanical_names.py). addr name kind confidence source evidence\n")
    for a, (n, c, e) in sorted(accepted.items()):
        fh.write("0x%s %s func %.2f mechanical %s\n" % (a, n, c, e.replace(" ", "_")))
with open(P("symbols", "review_queue_mech.txt"), "w") as fh:
    fh.write("# below-threshold mechanical proposals: addr name confidence evidence\n")
    for a, n, c, e in sorted(queue): fh.write("0x%s %s %.2f %s\n" % (a, n, c, e))
print("accepted", len(accepted), "queued", len(queue), dict(stats))
print("by module:", Counter(mods.get(a, {}).get("module") for a in accepted).most_common())
