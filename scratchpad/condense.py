"""condense.py ANNOT_FILE -- drop the shared hs evaluator prologue (up to the hs_evaluate_typed_arguments call and its
null test) and the trailing ret blocks, keeping headers and bodies."""
import sys, re
blocks, cur = [], None
for line in open(sys.argv[1], encoding='utf-8', errors='replace'):
    if line.startswith('=== '):
        cur = [line.rstrip()]
        blocks.append(cur)
    elif cur is not None:
        cur.append(line.rstrip())
for b in blocks:
    head, body = b[0], b[1:]
    idx = next((i for i, l in enumerate(body) if 'call   0x48a850' in l), None)
    if idx is not None:
        body = body[idx + 1:]
        body = [l for l in body[:3] if not re.search(r'add +esp,0x10|test +eax,eax|je +0x', l)] + body[3:]
        head += '   [args]'
    out = []
    for l in body:
        l = re.sub(r'^\s*[0-9a-f]+:\s*', '', l)
        l = re.sub(r'hs_thread_return\(int32_t value, uint32_t thread_index\) \{EAX -> value, ECX -> thread_index\}', 'RETURN', l)
        out.append(l)
    print(head)
    print('   ' + ' / '.join(x for x in out if x and not x.startswith('int16_t *expected_)')))
