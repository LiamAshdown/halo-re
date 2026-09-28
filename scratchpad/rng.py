"""rng.py A-B [A-B ...]: print _all.txt lines whose address lies in each hex range."""
import sys, os
os.chdir(r'C:\Users\Liam-\halo-re')
lines = open('scratchpad/_all.txt', encoding='utf-8', errors='replace').read().splitlines()
for r in sys.argv[1:]:
    a, b = (int(x, 16) for x in r.split('-'))
    print('==', r)
    for l in lines:
        h = l.split(':', 1)[0]
        try:
            v = int(h, 16)
        except ValueError:
            continue
        if a <= v <= b:
            print(l[:110])
