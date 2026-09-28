"""hsprocs.py -- for every hs function record: name, parse proc, evaluate proc; lists the procs that have no C."""
import os, re, glob, struct, collections
os.chdir(r'C:\Users\Liam-\halo-re')
exec(open('scratchpad/whereptr.py').read().split('for a in sys.argv')[0].replace('import struct, sys', 'import struct'))

def cstr(a):
    out = b''
    while True:
        w = struct.pack('<I', rd(a - a % 4))[a % 4:]
        for ch in w:
            if ch == 0:
                return out.decode('latin1')
            out += bytes([ch])
        a += 4 - a % 4

have = set()
for f in glob.glob('src/*/*.c'):
    m = re.search(r'address 0x([0-9a-f]+)', open(f, encoding='utf-8', errors='replace').read(1500))
    if m:
        have.add(int(m.group(1), 16))
parse_users, eval_users = collections.defaultdict(list), collections.defaultdict(list)
for i in range(0x20a):
    rec = rd(0x688b58 + 4 * i)
    name = cstr(rd(rec + 4))
    p, e = rd(rec + 8), rd(rec + 0xc)
    parse_users[p].append(name)
    eval_users[e].append(name)
print('parse procs without C:')
for p, names in sorted(parse_users.items()):
    if p and p not in have:
        print('  %x (%d): %s' % (p, len(names), ' '.join(names[:8])))
print('evaluate procs without C:', sum(1 for e in eval_users if e and e not in have))
for e, names in sorted(eval_users.items()):
    if e and e not in have:
        print('  %x (%d): %s' % (e, len(names), ' '.join(names[:6])))
