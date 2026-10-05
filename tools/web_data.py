"""The data image (standalone/data/*.cpp) for the browser build, run by cmake/web.cmake at configure time.

    python tools/web_data.py <output folder> <data source>...

Writes each data file to the output folder without MSVC's section placement (__declspec(allocate(...)) and
#pragma section, so the globals keep definition order; every one is kept even when unreferenced, pads included), and replaces the /alternatename linker aliases, which wasm-ld
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


GROUP = re.compile(r'__declspec\(allocate\("(\.\w+\$)([^"]*)"\)\)')


def regroup(texts):
    """MSVC sorts a section group by name across files; definition order does not. A file's own group is the one most of
    its definitions use. A definition placed in a group that is another file's own (eq_bss.cpp's main_globals_data in
    slice08's .g08) moves into that file, before the first of its definitions whose section name sorts after it, with the
    #pragma section line above it. Such definitions are one line each."""
    counts = {}
    for source, text in texts.items():
        for m in GROUP.finditer(text):
            counts.setdefault(source, {}).setdefault(m.group(1), 0)
            counts[source][m.group(1)] += 1
    own = {source: max(groups, key=groups.get) for source, groups in counts.items()}
    homes = {group: source for source, group in sorted(own.items(), key=lambda item: counts[item[0]][item[1]])}
    lines = {source: text.split('\n') for source, text in texts.items()}
    for source in texts:
        kept = []
        for line in lines[source]:
            m = GROUP.search(line)
            owner = homes.get(m.group(1)) if m else None
            if owner is None or m.group(1) == own.get(source):
                kept.append(line)
                continue
            moved = [kept.pop()] if kept and kept[-1].startswith('#pragma section') else []
            target = lines[owner]
            at = next((j for j, other in enumerate(target) if (o := GROUP.search(other)) and o.group(1) == m.group(1)
                       and o.group(2) > m.group(2)), None)
            if at is None:
                sys.exit('web_data.py: no place in %s for %s' % (owner, line))
            if at > 0 and target[at - 1].startswith('#pragma section'):
                at -= 1
            target[at:at] = moved + [line]
        lines[source] = kept
    return {source: '\n'.join(lines[source]) for source in texts}


DEFINITION = re.compile(r'^(?P<decl>(?!extern\b|typedef\b|static\b|struct\b|#)[A-Za-z_][^;/=]*?)\s*(?P<init>=[^;]*)?;'
                        r'(?P<rest>\s*(?://|/\*)\s*(?P<addr>0x[0-9a-fA-F]{8})\b.*)$')
NAME = re.compile(r'(\w+)\s*(\[[^\]]*\]\s*)*$')


def lay_out(text, tag):
    """Places the one-line definitions that carry their original address (`... ; // 0x006b3830`) as the original image
    did: each aligned to what its address implies (at most 16, which may be less than clang's preferred alignment for
    large arrays), with the gaps between them filled (sized from sizeof the previous definition, so a wrong address
    fails to compile), and the run started at its address modulo 16. Code that runs from one global into the next then
    finds what it found in the original. Definitions spanning lines end a run."""
    out = []
    previous = None  # (address, name) of the last definition laid out
    for line in text.split('\n'):
        m = DEFINITION.match(line)
        name = NAME.search(m.group('decl')) if m else None
        if m is None or name is None or 'align' in line:
            if line.strip() and not line.strip().startswith(('//', '/*', '*')):
                previous = None  # anything but a comment or a blank line ends the run
            out.append(line)
            continue
        address = int(m.group('addr'), 16)
        alignment = min(16, address & -address)
        if previous is None or address <= previous[0]:
            out.append('__attribute__((used, retain, aligned(16))) uint8_t web_lead_%s_%08x[%d];' % (tag, address, address % 16))
        else:
            # a declaration larger than the room before the next global overlaps it in the original (a few buffers
            # do), which C++ cannot express: the next global then follows it directly
            out.append('__attribute__((used, retain, aligned(1))) uint8_t web_gap_%s_%08x[sizeof(%s) < 0x%x ? 0x%x - sizeof(%s) : 0];' % (
                tag, address, previous[1], address - previous[0], address - previous[0], previous[1]))
        out.append('__attribute__((used, retain, aligned(%d))) %s' % (alignment, line))
        previous = (address, name.group(1))
    return '\n'.join(out)


def main():
    out_dir, sources = sys.argv[1], sys.argv[2:]
    texts = regroup({source: open(source, encoding='utf-8').read() for source in sources})
    aliases = [(m.group(1), m.group(2)) for text in texts.values() for m in ALIAS.finditer(text)]
    os.makedirs(out_dir, exist_ok=True)
    placed = set()
    for source, text in texts.items():
        # the image keeps its layout: nothing references the pads (and some globals), and wasm-ld drops unreferenced data
        text = ALLOCATE.sub('__attribute__((used, retain)) ', text)
        if os.path.basename(source).startswith('slice'):
            text = lay_out(text, os.path.splitext(os.path.basename(source))[0])
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
