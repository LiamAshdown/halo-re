"""closure.py [ADDR ...]: the functions reachable by direct call/jmp from the given addresses (default: every
hex address in build/standalone/traps.txt) that have no C in src/ yet, with sizes. Function extents come from
the Ghidra entry list plus every call target."""
import os, re, sys, bisect
os.chdir(r'C:\Users\Liam-\halo-re')
lines = open('scratchpad/_all.txt', encoding='utf-8', errors='replace').read().splitlines()
addr_re = re.compile(r'^([0-9a-f]+):\s*(.*)$')
ins = []
for l in lines:
    m = addr_re.match(l)
    if m:
        ins.append((int(m.group(1), 16), m.group(2)))
addrs = [a for a, _ in ins]
starts = set()
for l in open('symbols/ghidra_entries_fresh.txt'):
    l = l.strip()
    if l.startswith('0x'):
        starts.add(int(l, 16))
for a, t in ins:
    m = re.match(r'call\s+0x([0-9a-f]+)', t)
    if m:
        starts.add(int(m.group(1), 16))
have = set()
for d in os.listdir('src'):
    if os.path.isdir('src/' + d):
        for c in os.listdir('src/' + d):
            if c.endswith('.c'):
                h = open('src/%s/%s' % (d, c), encoding='utf-8', errors='replace').read(600)
                m = re.search(r'// address 0x([0-9a-f]+)', h)
                if m:
                    have.add(int(m.group(1), 16))
sorted_starts = sorted(starts)

def body(a):
    k = bisect.bisect_right(sorted_starts, a)
    end = sorted_starts[k] if k < len(sorted_starts) else a + 0x1000
    i = bisect.bisect_left(addrs, a)
    out = []
    while i < len(ins) and ins[i][0] < end:
        out.append(ins[i])
        i += 1
    return end, out

roots = [int(x, 16) for x in sys.argv[1:]]
if not roots:
    for l in open('build/standalone/traps.txt'):
        p = l.split('\t')
        if len(p) > 1 and p[1].startswith('0x'):
            roots.append(int(p[1], 16))
seen = {}
todo = list(roots)
while todo:
    a = todo.pop()
    if a in seen or a in have:
        continue
    end, b = body(a)
    callees = set()
    for x, t in b:
        m = re.match(r'(call|jmp)\s+0x([0-9a-f]+)', t)
        if m:
            c = int(m.group(2), 16)
            if not (a <= c < end) and c in starts and c < 0x623142:
                callees.add(c)
    seen[a] = (end - a, sorted(callees))
    todo.extend(callees)
tot = 0
for a in sorted(seen):
    size, cs = seen[a]
    tot += size
    print('%x %5d %s %s' % (a, size, 'ROOT' if a in roots else '    ', ' '.join('%x' % c for c in cs if c not in have)))
print('functions', len(seen), 'bytes', tot)
