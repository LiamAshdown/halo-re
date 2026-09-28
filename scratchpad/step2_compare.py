import json, os, sys, filecmp
R = r"C:\Users\Liam-\halo-re"
ref = os.path.join(R, "scratchpad", sys.argv[1] if len(sys.argv) > 1 else "step2_ref")
bad = 0
for n in ("layout.json", "imports.json", "code_pointers.json", "code_entries.json"):
    a = json.load(open(os.path.join(ref, n)))
    b = json.load(open(os.path.join(R, "build", "standalone", n)))
    same = a == b
    bad += not same
    print(n, "same" if same else "DIFFERENT", len(a) if isinstance(a, list) else "")
    if not same and isinstance(a, list):
        sa = {json.dumps(x, sort_keys=True) for x in a}
        sb = {json.dumps(x, sort_keys=True) for x in b}
        print("  only ref:", list(sa - sb)[:5])
        print("  only new:", list(sb - sa)[:5])
for n in ("report.txt",):
    s = open(os.path.join(ref, n)).read() == open(os.path.join(R, "build", "standalone", n)).read()
    print(n, "same" if s else "DIFFERENT")
    bad += not s
sys.exit(1 if bad else 0)
