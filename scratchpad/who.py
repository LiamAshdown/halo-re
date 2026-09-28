"""who.py ADDR... -- for each address: the C file defining it (name, signature, blam-cc) or the most common extern
declaration binding it (functions or globals)."""
import glob, os, re, sys, collections
os.chdir(r'C:\Users\Liam-\halo-re')
defs, externs = {}, collections.defaultdict(collections.Counter)
for f in glob.glob('src/*/*.c'):
    t = open(f, encoding='utf-8', errors='replace').read()
    m = re.search(r'address 0x([0-9a-f]+)', t[:1500])
    name = os.path.basename(f)[:-2]
    if m:
        sig = re.search(r'^[A-Za-z_][\w \*]*\b' + re.escape(name) + r'\([^)]*\)\s*$', t, re.M)
        cc = re.findall(r'blam-cc[^\n]*', t[:3000])
        defs[int(m.group(1), 16)] = (f, sig.group(0) if sig else '?', cc[:1])
    for em in re.finditer(r'^extern ([^;]+);[ \t]*//[ \t]*(?:[\w ]*?)0x([0-9a-f]{6,8})\b([^\n]*)', t, re.M):
        externs[int(em.group(2), 16)][em.group(1).strip() + '  //' + em.group(3)[:60]] += 1
for a in sys.argv[1:]:
    a = int(a, 16)
    if a in defs:
        f, sig, cc = defs[a]
        print('%x DEF %s | %s %s' % (a, f, sig, cc[0] if cc else ''))
    elif externs.get(a):
        print('%x EXT %s' % (a, externs[a].most_common(1)[0][0]))
    else:
        print('%x ???' % a)
