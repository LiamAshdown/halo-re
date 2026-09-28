"""ptr_sites.py: for every address in scratchpad/missing_functions.txt, the .data/.rdata slots holding it."""
import struct, re
d = open(r'C:/Program Files (x86)/Microsoft Games/Halo/halo.exe', 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
n = struct.unpack_from('<H', d, pe + 6)[0]
so = pe + 24 + struct.unpack_from('<H', d, pe + 20)[0]
secs = [struct.unpack_from('<8sIIII', d, so + 40 * i) for i in range(n)]
targets = {}
for line in open(r'C:\Users\Liam-\halo-re\scratchpad\missing_functions.txt'):
    m = re.match(r'0x([0-9a-f]+)\s+(\S+)', line)
    if m:
        targets[int(m.group(1), 16)] = m.group(2)
sites = {t: [] for t in targets}
for name, vs, va, rs, ra in secs:
    if name.startswith(b'.text'):
        continue
    for off in range(0, min(rs, vs) - 3, 4):
        v = struct.unpack_from('<I', d, ra + off)[0]
        if v in sites:
            sites[v].append(0x400000 + va + off)
for t in sorted(sites):
    print('%x %-10s %s' % (t, targets[t], ' '.join('%x' % s for s in sites[t])))
