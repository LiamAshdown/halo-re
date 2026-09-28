"""proto_by_callee.py [module]: prototype mismatches (out/prototype_mismatches.json) grouped by callee"""
import collections, json, os, sys
R = "C:\\Users\\Liam-\\halo-re\\"
mod = sys.argv[1] if len(sys.argv) > 1 else None
d = json.load(open(R + "out\\prototype_mismatches.json"))
by = collections.defaultdict(list)
for x in d:
    m = os.path.basename(os.path.dirname(x["caller"]))
    if mod and m != mod:
        continue
    by[x["callee"]].append((os.path.splitext(os.path.basename(x["caller"]))[0], x["problems"][0], x["declared"],
                            x["definition"]))
for k, v in sorted(by.items(), key=lambda kv: -len(kv[1])):
    print("%2d %-50s | %-45s | %s" % (len(v), k, v[0][1][:45], ", ".join(a for a, *_ in v[:3])))
print(sum(len(v) for v in by.values()), "mismatches,", len(by), "callees")
