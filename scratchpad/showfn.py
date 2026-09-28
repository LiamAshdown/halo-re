"""showfn.py FILE ADDR... -- print the '=== addr' blocks of a dump_group.py output file."""
import sys
path, want = sys.argv[1], {a.lower().lstrip('0x') for a in sys.argv[2:]}
blocks, cur = {}, None
for line in open(path, encoding='utf-8', errors='replace'):
    if line.startswith('=== '):
        cur = line.split()[1]
        blocks[cur] = [line]
    elif cur:
        blocks[cur].append(line)
for a in sys.argv[2:]:
    k = a.lower().replace('0x', '')
    sys.stdout.write(''.join(l[:95].rstrip() + '\n' for l in blocks.get(k, ['=== %s not found\n' % k])))
