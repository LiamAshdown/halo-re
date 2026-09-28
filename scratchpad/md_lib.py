"""md_lib.py: emit() for src/networking message-delta codec files (one function per file)."""
import os, textwrap
os.chdir(r'C:\Users\Liam-\halo-re')
SIG = '(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)'


def emit(addr, size, name, note, code, cc='cdecl', extra_includes='', module='networking',
         header='#include "message_delta_codec.h"\n'):
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    src = ('// %s  (reached only through a .data code pointer; no C existed)\n// address 0x%x, size %d bytes\n'
           '// name confidence: 0.6   rewrite confidence: 0.85\n%s// blam-cc: %s\n\n%s%s\n%s'
           % (name, addr, size, wr, cc, header, extra_includes, code.lstrip('\n')))
    open('src/%s/%s.c' % (module, name), 'w', encoding='utf-8').write(src)
