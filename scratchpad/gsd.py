"""gsd.py START STOP: Ghidra decompilation (scratchpad/gs_decomp.c) of every function in [START, STOP)."""
import re, sys
s = open(r'C:\Users\Liam-\halo-re\scratchpad\gs_decomp.c', encoding='utf-8', errors='replace').read()
a, b = (int(x, 16) for x in sys.argv[1:3])
for p in re.split(r'(?m)^// ===== ', s):
    m = re.match(r'(\S+) @ ([0-9a-f]+)', p)
    if m and a <= int(m.group(2), 16) < b:
        body = re.sub(r'\n\s*\n', '\n', p)
        print('// ===== ' + body.rstrip())
