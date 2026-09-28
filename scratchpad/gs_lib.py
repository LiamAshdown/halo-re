"""Generator for src/gamespy/*.c: one function per file with the repo header."""
import os, textwrap
os.chdir(r'C:\Users\Liam-\halo-re')


def emit(addr, size, name, note, code, confidence='0.85', name_confidence='0.8', cc='cdecl', extra=''):
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    src = ('// %s  (GameSpy SDK in halo.exe; no C existed)\n// address 0x%x, size %d bytes\n'
           '// name confidence: %s   rewrite confidence: %s\n%s// blam-cc: %s\n\n#include "gamespy.h"\n%s\n%s'
           % (name, addr, size, name_confidence, confidence, wr, cc, extra, code.lstrip('\n')))
    p = 'src/gamespy/%s.c' % name
    open(p, 'w', encoding='utf-8').write(src)
