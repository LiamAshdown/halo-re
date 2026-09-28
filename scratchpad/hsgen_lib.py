"""Shared generator for hs evaluate handlers."""
import os, re, struct, textwrap
os.chdir(r'C:\Users\Liam-\halo-re')
exec(open('scratchpad/whereptr.py').read().split('for a in sys.argv')[0].replace('import struct, sys', 'import struct'))

def _cstr(a):
    out = b''
    while len(out) < 100:
        w = struct.pack('<I', rd(a - a % 4))[a % 4:]
        for ch in w:
            if ch == 0:
                return out.decode('latin1')
            out += bytes([ch])
        a += 4 - a % 4
    return out.decode('latin1')

HS_TYPES = {int(m.group(2), 0): m.group(1) for m in re.finditer(r'_hs_type_(\w+)\s*=\s*(\w+)', open('types/hs.h').read())}

def record_info(evaluate):
    for i in range(0x20a):
        rec = rd(0x688b58 + 4 * i)
        if rd(rec + 0xc) == evaluate:
            return i, rec
    return None, None

def signature(rec):
    count = struct.unpack('<h', struct.pack('<I', rd(rec + 0x18))[2:4])[0]
    params = []
    for k in range(count):
        v = rd(rec + 0x1c + 2 * k - (2 * k) % 4)
        params.append(HS_TYPES.get((v >> (16 * (((2 * k) % 4) // 2))) & 0xffff, '?'))
    return _cstr(rd(rec + 4)), '%s -> %s' % (', '.join(params), HS_TYPES.get(rd(rec) & 0xffff, '?'))

COMMON_EXT = ('extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58\n'
              'extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,\n'
              '    int16_t *expected_types, char first); // 0x48a850\n')
RETURN_EXT = 'extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread\n'

def indent(text, n):
    return ''.join((' ' * n + l if l.strip() else l) + '\n' for l in text.rstrip('\n').split('\n'))

def emit(addr, size, note, externs, body, args=True, name=None, record=None):
    """body: statements (4-space indented at function level), ending in the hs_thread_return."""
    if record is None:
        index, rec = record_info(addr)
    else:
        index, rec = None, record
    hs_name, sig = signature(rec)
    fname = 'hs_evaluate_' + (name or hs_name)
    where = ('hs function %d "%s" (%s)' % (index, hs_name, sig)) if index is not None else \
            ('the orphan hs function record 0x%x "%s" (%s), not in hs_function_definitions' % (rec, hs_name, sig))
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    hdr = ('// %s  (not a Ghidra function; the evaluate handler of %s)\n// address 0x%x, size %d bytes\n'
           '// name confidence: 0.9   rewrite confidence: 0.85\n'
           '// evidence: the function record\'s evaluate slot (+0x0c); only reachable through it, so no C meant\n'
           '//   unlisted_%x trapped.\n%s// blam-cc: stack -> function_index, thread_index, first (cdecl)\n'
           % (fname, where, addr, size, addr, wr))
    inc = '#include "tags.h"\n#include "memory.h"\n#include "math.h"\n#include "hs.h"\n'
    code = body + externs
    if 'object_header' in code or 'object *' in code:
        inc += '#include "objects.h"\n'
    if 'FILE' in code or 'fopen' in code:
        inc += '#include <stdio.h>\n'
    ext = (COMMON_EXT if args else '') + RETURN_EXT + externs
    sigline = 'void %s(int16_t function_index, uint32_t thread_index, char first)' % fname
    if args:
        fn = ('%s\n{\n    hs_function_definition *definition = hs_function_definitions[function_index];\n'
              '    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,\n'
              '        (int16_t *)definition->parameters, first);\n\n    if (arguments != 0) {\n%s    }\n}\n'
              % (sigline, indent(body, 8)))
    else:
        fn = '%s\n{\n%s}\n' % (sigline, indent(body, 4))
    p = 'src/hs/%s.c' % fname
    if os.path.exists(p):
        p = 'src/hs/%s_%x.c' % (fname, addr)
        fn = fn.replace(fname + '(', '%s_%x(' % (fname, addr), 1)
        hdr = hdr.replace('// ' + fname + '  ', '// %s_%x  ' % (fname, addr), 1)
    assert not os.path.exists(p), p
    open(p, 'w', encoding='utf-8').write(hdr + '\n' + inc + '\n' + ext + '\n' + fn)
    return p

# frequently used declarations
X = {
    'object_data': 'extern data_array *object_data; // 0x008603b0\n',
    'encounter_data': 'extern data_array *encounter_data; // 0x008802c8\n',
    'ai_global_data': 'extern uint8_t *ai_global_data; // 0x00880354 (ai_globals *; +0 enabled, +1 encounters live)\n',
    'console_out': 'extern void chimera__console_out(void *color, char *format, ...); // 0x496b50, blam-cc: EAX color\n',
    'settings': 'extern uint8_t player_control_settings_cache[]; // 0x00710328, player_control_settings, stride 0x85c\n',
    'look_rates': 'extern uint8_t player_control_look_rates_0070facc[]; // 0x0070facc, UNSURE: indexed by slot * 0x85c, one record before player_control_settings_cache\n',
}
UNIT_FLAGS = ('uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[arguments[0] & 0xffff].data;\n')
