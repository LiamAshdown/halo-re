"""Merge Phase 2 agent outputs (out/phase2/results/*.json) into
  symbols/agent_phase2.txt   0xADDR name func conf source        (names, applied by merge_symbols -> ApplySymbols)
  symbols/prototypes.txt     0xADDR <C prototype>;               (applied by ApplySymbols)
  symbols/review_queue.txt   low-confidence proposals for the Opus hard-queue pass
Result file shape: {"results":[{"addr","name","confidence","evidence","prototype","summary"}]}
Rules: name accepted into agent_phase2.txt only when confidence >= 0.7 and name is a valid C identifier not
already used for a different address; everything below goes to the review queue."""
import json, glob, os, re
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
P = lambda *a: os.path.join(ROOT, *a)
IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
THRESH = 0.5

funcs = {f["addr"][-6:]: f for f in json.load(open(P("out", "functions.json")))}
accepted, queue, protos = {}, [], {}
used = Counter()
stats = Counter()
for f in sorted(glob.glob(P("out", "phase2", "results", "*.json"))):
    try: d = json.load(open(f, encoding="utf-8"))
    except Exception as e: print("bad json", f, e); continue
    for r in (d if isinstance(d, list) else d.get("results", [])):
        a = r["addr"].lower().replace("0x", "")[-6:].rjust(6, "0")
        if a not in funcs: stats["bad-addr"] += 1; continue
        name = (r.get("name") or "").strip(); conf = float(r.get("confidence", 0))
        if not IDENT.match(name) or name.startswith(("FUN_", "sig__", "os__")): stats["bad-name"] += 1; continue
        if conf >= THRESH:
            if a in accepted and accepted[a][1] >= conf: continue
            accepted[a] = (name, conf, r.get("evidence", "")[:160].replace("\n", " "), os.path.basename(f))
            if r.get("prototype"): protos[a] = r["prototype"].strip().rstrip(";")
        else:
            queue.append((a, name, conf, r.get("evidence", "")[:160].replace("\n", " ")))
# de-duplicate names: two addresses claiming the same name get suffixed by address
byname = Counter(v[0] for v in accepted.values())
for a, (name, conf, ev, src) in list(accepted.items()):
    if byname[name] > 1: accepted[a] = (name + "_" + a, conf, ev + " [dup-name]", src); stats["dup-name"] += 1
with open(P("symbols", "agent_phase2.txt"), "w", encoding="ascii", errors="replace") as fh:
    fh.write("# Phase 2 agent names (conf >= %.1f). addr name kind confidence source\n" % THRESH)
    for a, (name, conf, ev, src) in sorted(accepted.items()):
        fh.write("0x%s %s func %.2f agent-phase2:%s %s\n" % (a, name, conf, src, ev.replace(" ", "_")))
with open(P("symbols", "prototypes.txt"), "w", encoding="ascii", errors="replace") as fh:
    fh.write("# addr prototype (applied after names)\n")
    for a, p in sorted(protos.items()):
        fh.write("0x%s %s;\n" % (a, p))
with open(P("symbols", "review_queue.txt"), "w", encoding="ascii", errors="replace") as fh:
    fh.write("# below-threshold proposals for the hard-queue pass: addr name confidence evidence\n")
    for a, name, conf, ev in sorted(queue):
        fh.write("0x%s %s %.2f %s\n" % (a, name, conf, ev))
print("accepted", len(accepted), "prototypes", len(protos), "review-queue", len(queue), dict(stats))
