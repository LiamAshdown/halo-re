"""For each call 0x46be40 in the binary: the enclosing repo function, and the ESI/EDI/pushed values set just before."""
import os, re, bisect
os.chdir(r'C:\Users\Liam-\halo-re')
lines = open('scratchpad/_all.txt', encoding='utf-8', errors='replace').read().splitlines()
addr_re = re.compile(r'^([0-9a-f]+):\s*(.*)$')
starts = {}
for f in os.listdir('src'):
    d = os.path.join('src', f)
    if not os.path.isdir(d):
        continue
    for c in os.listdir(d):
        if c.endswith('.c'):
            try:
                head = open(os.path.join(d, c), encoding='utf-8', errors='replace').read(600)
            except Exception:
                continue
            m = re.search(r'// address 0x([0-9a-f]+), size (\d+)', head)
            if m:
                starts[int(m.group(1), 16)] = (int(m.group(2)), os.path.join(d, c))
keys = sorted(starts)
for i, l in enumerate(lines):
    if 'call   0x46be40' not in l:
        continue
    a = int(l.split(':')[0], 16)
    k = bisect.bisect_right(keys, a) - 1
    owner = '?'
    if k >= 0 and keys[k] + starts[keys[k]][0] > a:
        owner = '%x %s' % (keys[k], starts[keys[k]][1])
    ctx = []
    for j in range(i - 1, max(i - 12, 0), -1):
        t = addr_re.match(lines[j])
        if not t:
            continue
        ins = t.group(2).split(';')[0].strip()
        if re.search(r'\b(esi|edi)\b,', ins) or ins.startswith('push') or ins.startswith('call') or ins.startswith('j'):
            ctx.append(t.group(1) + ' ' + ins)
        if ins.startswith('call') or ins.startswith('ret'):
            break
    print('%x  %s' % (a, owner))
    for c in reversed(ctx):
        print('      ' + c)
