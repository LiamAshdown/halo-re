"""The data image (standalone/data/*.cpp) for the browser build, run by cmake/web.cmake at configure time.

    python tools/web_data.py <output folder> <data source>...

Writes each data file to the output folder without MSVC's section placement (__declspec(allocate(...)) and
#pragma section, so the globals keep definition order), and replaces the /alternatename linker aliases, which wasm-ld
does not have, with aliases defined next to their targets (alias attribute, the symbol name given by an asm label).
Files are only rewritten when their text changes.
"""
import os
import re
import sys

ALLOCATE = re.compile(r'__declspec\(allocate\("[^"]*"\)\) *')
SECTION = re.compile(r'#pragma section\([^)]*\)')
ALIAS = re.compile(r'#pragma comment\(linker, "/alternatename:_(\w+)=_(\w+)"\)')


def defines(text, name):
    """Whether text defines the global name at file scope (not a declaration or a mention)."""
    pattern = re.compile(r'^(?!\s*(extern|/|\*|#)).*[\s*&]%s\s*(\[[^\]]*\])*\s*(=|;)' % re.escape(name), re.M)
    return pattern.search(text) is not None


def main():
    out_dir, sources = sys.argv[1], sys.argv[2:]
    texts = {source: open(source, encoding='utf-8').read() for source in sources}
    aliases = [(m.group(1), m.group(2)) for text in texts.values() for m in ALIAS.finditer(text)]
    os.makedirs(out_dir, exist_ok=True)
    placed = set()
    for source, text in texts.items():
        text = ALLOCATE.sub('', text)
        text = SECTION.sub('', text)
        text = ALIAS.sub('', text)
        extra = []
        for name, target in aliases:
            if name not in placed and defines(text, target):
                # a private C++ name, the alias's symbol name through the asm label: the name may also be a type
                extra.append('extern "C" decltype(%s) halo_alias_%s __asm__("%s") __attribute__((alias("%s")));' % (target, name, name, target))
                placed.add(name)
        if extra:
            text += '\n/* the /alternatename aliases of these globals (tools/web_data.py) */\n' + '\n'.join(extra) + '\n'
        path = os.path.join(out_dir, os.path.basename(source))
        old = open(path, encoding='utf-8').read() if os.path.exists(path) else None
        if old != text:
            open(path, 'w', encoding='utf-8', newline='\n').write(text)
    missing = sorted({name for name, _ in aliases} - placed)
    if missing:
        sys.exit('web_data.py: no definition found for the targets of ' + ', '.join(missing))


if __name__ == '__main__':
    main()
