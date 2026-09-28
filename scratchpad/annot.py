"""annot.py DUMP [ADDR...] -- print dump_group blocks with call targets and ds: globals annotated from the repo's C
definitions / extern declarations; for hs evaluators also prints the hs function name and signature."""
import glob, os, re, sys, collections, struct
os.chdir(r'C:\Users\Liam-\halo-re')
exec(open('scratchpad/whereptr.py').read().split('for a in sys.argv')[0].replace('import struct, sys', 'import struct'))

defs, externs = {}, collections.defaultdict(collections.Counter)
for f in glob.glob('src/*/*.c'):
    t = open(f, encoding='utf-8', errors='replace').read()
    name = os.path.basename(f)[:-2]
    m = re.search(r'address 0x([0-9a-f]+)', t[:1500])
    if m:
        sig = re.search(r'^[A-Za-z_][\w \*]*\b' + re.escape(name) + r'\(([^)]*)\)', t, re.M)
        cc = re.search(r'blam-cc:?([^\n]*)', t[:3000])
        defs[int(m.group(1), 16)] = '%s(%s)%s' % (name, (sig.group(1) if sig else '?')[:70], (' {' + cc.group(1).strip()[:40] + '}') if cc else '')
    for em in re.finditer(r'^extern ([^;]+);[ \t]*//[ \t]*(?:[\w ]*?)0x([0-9a-f]{6,8})\b', t, re.M):
        externs[int(em.group(2), 16)][re.sub(r'\s+', ' ', em.group(1))[:90]] += 1

def label(a):
    if a in defs:
        return defs[a]
    if externs.get(a):
        return externs[a].most_common(1)[0][0]
    return None

def cstr(a):
    out = b''
    while len(out) < 100:
        w = struct.pack('<I', rd(a - a % 4))[a % 4:]
        for ch in w:
            if ch == 0:
                return out.decode('latin1')
            out += bytes([ch])
        a += 4 - a % 4
    return out.decode('latin1')

HS_TYPES = {}
for m in re.finditer(r'_hs_type_(\w+)\s*=\s*(\w+)', open('types/hs.h').read()):
    HS_TYPES[int(m.group(2), 0)] = m.group(1)
evals = {}
for i in range(0x20a):
    rec = rd(0x688b58 + 4 * i)
    count = struct.unpack('<h', struct.pack('<I', rd(rec + 0x18))[2:4])[0]
    params = []
    for k in range(count):
        v = rd(rec + 0x1c + 2 * k - (2 * k) % 4)
        params.append(HS_TYPES.get((v >> (16 * ((2 * k) % 4 // 2))) & 0xffff, '?'))
    ret = HS_TYPES.get(rd(rec) & 0xffff, '?')
    evals[rd(rec + 0xc)] = '%d %s(%s) -> %s' % (i, cstr(rd(rec + 4)), ', '.join(params), ret)

path = sys.argv[1]
want = {int(a, 16) for a in sys.argv[2:]}
cur = None
for line in open(path, encoding='utf-8', errors='replace'):
    if line.startswith('=== '):
        cur = int(line.split()[1], 16)
        if want and cur not in want:
            continue
        print(line.rstrip() + '   ' + evals.get(cur, ''))
        continue
    if want and cur not in want:
        continue
    s = line.rstrip()
    notes = []
    for m in re.finditer(r'(?:call|jmp)\s+0x([0-9a-f]+)', s):
        l = label(int(m.group(1), 16))
        if l:
            notes.append(l)
    for m in re.finditer(r'0x([0-9a-f]{6})\b', s):
        a = int(m.group(1), 16)
        if 0x63a000 <= a < 0x881000 and 'call' not in s:
            l = label(a)
            if l:
                notes.append(l)
            elif 0x63a000 <= a < 0x676000:
                t = cstr(a)
                if t and all(32 <= ord(c) < 127 for c in t[:20]) and len(t) > 2:
                    notes.append('"%s"' % t[:60])
    print(s[:90] + (('   ; ' + ' | '.join(notes)) if notes else ''))
