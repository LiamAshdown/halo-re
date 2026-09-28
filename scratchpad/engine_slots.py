"""engine_slots.py -- map missing game engine callbacks to (engine, slot) and show the slot's C field name."""
import os, re, struct
os.chdir(r'C:\Users\Liam-\halo-re')
exec(open('scratchpad/whereptr.py').read().split('for a in sys.argv')[0].replace('import struct, sys', 'import struct'))

def cstr(a):
    out = b''
    while len(out) < 60:
        w = struct.pack('<I', rd(a - a % 4))[a % 4:]
        for ch in w:
            if ch == 0:
                return out.decode('latin1')
            out += bytes([ch])
        a += 4 - a % 4
    return out.decode('latin1')

fields = {}
t = open('types/game.h').read()
body = t[t.index('typedef struct game_engine_definition'):]
body = body[:body.index('} game_engine_definition')]
for m in re.finditer(r'void \*(\w+);\s*// 0x([0-9a-f]+)', body):
    fields[int(m.group(2), 16)] = m.group(1)

engines = [rd(0x688308 + 4 * i) for i in range(8)]
missing = [int(l.split('\t')[0], 16) for l in open('scratchpad/missing_functions.txt') if l.startswith('0x') and l.split('\t')[1].strip() == 'game']
rows = []
for e in engines:
    if not (0x600000 < e < 0x700000):
        continue
    name = cstr(rd(e))
    for off in range(8, 0x100, 4):
        v = rd(e + off)
        if v in missing:
            rows.append((v, name, off, fields.get(off, '?')))
seen = set()
for v, name, off, f in sorted(rows):
    print('%x %-8s +0x%02x %s' % (v, name, off, f))
    seen.add(v)
print('not in an engine table:', ' '.join('%x' % m for m in missing if m not in seen))
