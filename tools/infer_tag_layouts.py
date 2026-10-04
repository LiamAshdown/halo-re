"""
Infers the pointer layout of every tag type from the retail .map files, and proves it.

A cache file's tag data block holds absolute pointers that assume it is loaded at 0x40440000 (and structure BSPs at the
address the scenario lists), so the engine keeps map memory at 0x40000000. Loading it anywhere else would mean relocating
those pointers, which needs, for every struct type: its size and where its tag blocks
(count, address, definition), tag data (size, flags, file offset, address, definition) and tag references
(group, name address, name length, tag id) sit. The engine has no tag definitions, so they are inferred:

- tool.exe packs each tag contiguously, root struct first, then the arrays and data its pointers lead to. A struct's
  size is the distance to the first allocation after it (the mode over all instances of the type in all maps).
- a field is a pointer field only if every instance of the type, in every map, fits that field's pattern.
- proof: walking the inferred layouts from every tag must cover the tag data block after the path strings exactly
  once (no gaps, no overlaps) in every map, and every reference name must point into the path strings.

Not used by the build: kept so map memory can be relocated later (for a smaller WebAssembly memory). --out writes the
layouts as C++ tables.

usage: python tools/infer_tag_layouts.py <maps folder | map files> [--out tag_layouts.inc]
"""
import bisect
import collections
import glob
import os
import struct
import sys
from array import array

BASE = 0x40440000
SBSP = 0x73627370


class Region:
    """A block of tag data loaded at a fixed address: the main tag data, or one structure BSP."""

    def __init__(self, name, data, base, alloc_begin, strings, tag_ids, roots):
        self.name, self.data, self.base = name, data, base
        self.n = len(data)
        self.words = array("I", data[:self.n - self.n % 4])
        self.alloc_begin = alloc_begin      # offset where block arrays and data blobs start
        self.strings = strings              # (low, high) addresses of the tag path strings
        self.tag_ids = tag_ids
        self.roots = roots                  # [(offset, group)] root structs
        self.targets = self.scan_targets()
        self.target_set = set(self.targets)

    def scan_targets(self):
        """Every address a block or data pattern points to, plus the roots and the end: candidate allocation starts."""
        w, low, high = self.words, self.base + self.alloc_begin, self.base + self.n
        found = {a for a, g in self.roots}
        found.add(self.n)
        for i in range(self.alloc_begin >> 2, len(w) - 4):
            if w[i + 2] == 0 and 0 < w[i] <= 1000000 and low <= w[i + 1] < high:
                found.add(w[i + 1] - self.base)
            if w[i + 4] == 0 and low <= w[i + 3] < high and 0 < w[i] <= self.n and (w[i + 1] == 0 or w[i + 1] in self.tag_ids):
                found.add(w[i + 3] - self.base)
        return sorted(found)

    def inside(self, address):
        return self.base <= address < self.base + self.n

    def allocation(self, address):
        """Whether address can start a block array or data blob."""
        return self.base + self.alloc_begin <= address < self.base + self.n

    def word(self, offset):
        return self.words[offset >> 2]


def load_map(path):
    """The regions of one cache file: its tag data, then each structure BSP the scenario lists."""
    f = open(path, "rb").read()
    offset, size = struct.unpack_from("<II", f, 0x10)
    data = f[offset:offset + size]
    tags_address, _, _, count = struct.unpack_from("<IIIi", data, 0)
    table = tags_address - BASE
    records = [struct.unpack_from("<IIIIIIII", data, table + i * 32) for i in range(count)]
    roots = sorted((r[5] - BASE, r[0]) for r in records if BASE <= r[5] < BASE + size)
    strings = (BASE + table + count * 32, BASE + roots[0][0])
    tag_ids = {r[3] for r in records}
    name = os.path.basename(path)
    main = Region(name, data, BASE, roots[0][0], strings, tag_ids, roots)
    regions = [main]
    scenario = [r for r in records if r[0] == map_group_code("scnr")][0][5] - BASE
    for o in range(0, 0x5b0 - 8, 4):   # the structure_bsps block: elements of 0x20 bytes ending in an sbsp reference
        bsps, address = main.word(scenario + o), main.word(scenario + o + 4)
        if 0 < bsps < 64 and main.word(scenario + o + 8) == 0 and main.inside(address) and main.word(address - BASE + 0x10) == SBSP:
            for i in range(bsps):
                start, length, bsp_address = struct.unpack_from("<III", data, address - BASE + i * 0x20)
                blob = f[start:start + length]
                root = struct.unpack_from("<I", blob, 0)[0] - bsp_address
                regions.append(Region("%s bsp%d" % (name, i), blob, bsp_address, 0x18, strings, tag_ids, [(root, SBSP)]))
            break
    return regions


def map_group_code(text):
    return struct.unpack(">I", text.encode("latin1"))[0]


def group_name(code):
    return struct.pack(">I", code).decode("latin1")


def is_block(m, o):
    if o + 12 > m.n:
        return None
    count, address, definition = m.word(o), m.word(o + 4), m.word(o + 8)
    if count > 0x7fffffff or count > 1000000 or definition != 0:
        return None
    if count == 0:
        return "empty"
    return "full" if m.allocation(address) and address % 4 == 0 else None


def is_data(m, o):
    if o + 20 > m.n:
        return None
    size, flags, address, definition = m.word(o), m.word(o + 4), m.word(o + 12), m.word(o + 16)
    if size > 0x7fffffff or definition != 0 or (flags != 0 and flags not in m.tag_ids):
        return None
    if address == 0:
        return "empty"
    if not (0 < size <= m.base + m.n - address and m.allocation(address)):
        return None
    # the next allocation starts where the blob ends (padded to 4 bytes)
    end = address - m.base + size
    return "full" if ((end + 3) & ~3) in m.target_set or end == m.n else None


def is_reference(m, o):
    if o + 16 > m.n:
        return None
    group, name, length, tag_id = m.word(o), m.word(o + 4), m.word(o + 8), m.word(o + 12)
    printable = all(32 <= b < 127 for b in struct.pack(">I", group))
    if length != 0 or not (printable or group == 0xffffffff):
        return None
    if tag_id == 0xffffffff and name == 0:
        return "empty"
    return "full" if m.strings[0] <= name < m.strings[1] else None


def element_size(m, a, count):
    """Element size of an array of count elements at a: the first candidate allocation start after a that the array,
    padded to 4 bytes, ends exactly at."""
    i = bisect.bisect_right(m.targets, a)
    while i < len(m.targets):
        gap = m.targets[i] - a
        size = gap // count
        if size > 0 and ((count * size + 3) & ~3) == gap:
            return size
        i += 1
    return None


def mode(values):
    return collections.Counter(values).most_common(1)[0][0] if values else None


class Type:
    def __init__(self, name):
        self.name = name
        self.size = None
        self.fields = []   # (offset, kind, child Type or None)
        self.instances = []  # (map, offset, count) arrays of this type


def solve(types, maps):
    """Breadth-first: size each type from its instances, find its pointer fields, queue the block element types."""
    queue = list(types.values())
    while queue:
        t = queue.pop(0)
        # every array fits in its region; exact estimates above that bound come from coincidental pointer-like values,
        # and an array that is always the last allocation (followed only by padding) gets the bound itself
        bound = min((m.n - a) // count for m, a, count in t.instances)
        sizes = [size for size in (element_size(m, a, count) for m, a, count in t.instances) if size and size <= bound]
        t.size = mode(sizes) or bound
        if not t.size:
            raise SystemExit("%s: no size (%d instances)" % (t.name, len(t.instances)))
        elements = [(m, a + i * t.size) for m, a, count in t.instances for i in range(count)]
        covered = set()
        for o in range(0, t.size, 4):
            if o in covered:
                continue
            for kind, test, width in (("data", is_data, 20), ("block", is_block, 12), ("reference", is_reference, 16)):
                if o + width > t.size:
                    continue
                results = [test(m, a + o) for m, a in elements]
                if all(results) and "full" in results:
                    child = None
                    if kind == "block":
                        child = Type("%s+%x" % (t.name, o))
                        for m, a in elements:
                            if m.word(a + o) > 0:
                                child.instances.append((m, m.word(a + o + 4) - m.base, m.word(a + o)))
                        queue.append(child)
                    t.fields.append((o, kind, child))
                    covered.update(range(o, o + width, 4))
                    break
    return types


def walk(m, t, a, count, spans, names=None):
    spans.append((a, a + count * t.size, "%s[%d]" % (t.name, count)))
    for i in range(count):
        e = a + i * t.size
        for o, kind, child in t.fields:
            if kind == "reference" and names is not None:
                names.add(e + o + 4)
            if kind == "block" and m.word(e + o) > 0:
                walk(m, child, m.word(e + o + 4) - m.base, m.word(e + o), spans, names)
            elif kind == "data" and m.word(e + o + 12) != 0:
                begin = m.word(e + o + 12) - m.base
                spans.append((begin, begin + m.word(e + o), "%s+%x data" % (t.name, o)))


def looks_like_reference_field(m, o, spans, types):
    """Whether the field of the struct containing o holds a reference in most of its instances (a coincidental match in
    a float field holds in only one or two)."""
    k = bisect.bisect_right(spans, (o, 1 << 62, "")) - 1
    while k >= 0 and not (spans[k][0] <= o < spans[k][1] and "[" in spans[k][2]):
        k -= 1
    t = types[spans[k][2].split("[")[0]]
    field = (o - spans[k][0]) % t.size
    results = [is_reference(mm, a + i * t.size + field) for mm, a, count in t.instances for i in range(count)]
    return results.count(None) * 2 < len(results)


def all_types(roots):
    types = {}

    def collect(t):
        if t.name not in types:
            types[t.name] = t
            for o, kind, child in t.fields:
                if child:
                    collect(child)
    for t in roots.values():
        collect(t)
    return types


def prove(maps, roots, verbose=4):
    """Walks every tag; the spans must tile the tag data after the path strings exactly, and every reference-like field
    must be in the layout. Prints the first problems."""
    ok, types = True, all_types(roots)
    for m in maps:
        spans, names = [], set()
        for a, group in m.roots:
            walk(m, roots[group], a, 1, spans, names)
        spans.sort()
        low, high = m.strings
        blobs = sorted((b, e) for b, e, label in spans if label.endswith("data"))
        starts = [b for b, e in blobs]

        def in_blob(o):
            k = bisect.bisect_right(starts, o) - 1
            return k >= 0 and o < blobs[k][1]
        stray = [i * 4 for i in range(m.alloc_begin >> 2, len(m.words) - 2)
                 if low <= m.words[i] < high and i * 4 not in names and is_reference(m, i * 4 - 4) and not in_blob(i * 4)]
        cursor, last, bad, problems = m.alloc_begin, "start", 0, []
        for begin, end, label in spans:
            if cursor < begin and begin == (cursor + 3) & ~3:
                cursor = begin   # every allocation starts on a 4-byte boundary
            if begin != cursor:
                bad += 1
                if len(problems) < verbose:
                    problems.append("%s %#x..%#x [%s], previous [%s] ends %#x" % ("gap before" if begin > cursor else "overlap",
                                                                                begin, end, label, last, cursor))
            if end >= cursor:
                cursor, last = end, label
        # a structure BSP is padded to its file size with leftover bytes after its last allocation; nothing may point there
        tail = range(m.base + cursor, m.base + m.n)
        tail_ok = m.base != BASE and cursor <= m.n and not any(
            (m.words[i] in tail and is_block(m, i * 4 - 4) == "full") or (m.words[i] in tail and is_data(m, i * 4 - 12) == "full")
            for i in range(3, len(m.words)))
        if cursor != m.n and not tail_ok:
            bad += 1
            problems.append("covered up to %#x of %#x" % (cursor, m.n))
        missed = [o for o in stray if looks_like_reference_field(m, o - 4, spans, types)]
        print("%-20s spans %6d mismatches %5d references %5d, missed reference fields %d (coincidences %d)" % (
            m.name, len(spans), bad, len(names), len(missed), len(stray) - len(missed)))
        for o in missed[:3]:
            problems.append("reference-like field at %#x is not in the layout" % (o - 4))
        bad += len(missed)
        for line in problems:
            print("    " + line)
        ok = ok and bad == 0
    return ok


KINDS = {"block": 0, "data": 1, "reference": 2}


def emit(roots, path):
    """Writes the layouts as C++ tables: structurally identical types are merged, leaf types (no pointer fields) are
    not needed by the relocation walk, so a block of them points at type 0xffff."""
    ids, types, fields = {}, [], []

    def signature(t):
        return (t.size, tuple((o, kind, signature(c) if c and c.fields else None) for o, kind, c in t.fields))

    def add(t):
        key = signature(t)
        if key in ids:
            return ids[key]
        children = [(o, KINDS[kind], add(c) if c and c.fields else 0xffff) for o, kind, c in t.fields]
        ids[key] = len(types)
        types.append((t.size, len(fields), len(children), t.name))
        fields.extend(children)
        return ids[key]
    groups = sorted((code, add(t)) for code, t in roots.items())
    out = ["/* Generated by tools/infer_tag_layouts.py from the retail maps; do not edit. */",
           "/* kind 0: tag block (count, address, definition); 1: tag data (size, flags, file offset, address, definition);",
           "   2: tag reference (group, name address, name length, tag id). child 0xffff: elements hold no pointers. */",
           "", "static constexpr TagLayoutField k_tag_layout_fields[] = {"]
    out += ["    {0x%x, %d, 0x%x}," % f for f in fields]
    out += ["};", "", "static constexpr TagLayoutType k_tag_layout_types[] = {"]
    out += ["    {0x%x, %d, %d},  // %s" % t for t in types]
    out += ["};", "", "static constexpr TagLayoutGroup k_tag_layout_groups[] = {"]
    out += ["    {0x%08x, %d},  // %s" % (code, t, group_name(code)) for code, t in groups]
    out += ["};", ""]
    open(path, "w", newline="\n").write("\n".join(out))
    print("wrote %s: %d groups, %d types, %d fields" % (path, len(groups), len(types), len(fields)))


def main():
    paths = [a for a in sys.argv[1:] if a.endswith(".map")] or sorted(glob.glob(os.path.join(sys.argv[1], "*.map")))
    paths = [p for p in paths if p != sys.argv[-1] or "--out" not in sys.argv]
    maps = [r for p in paths if open(p, "rb").read(4) == b"daeh" for r in load_map(p)]   # bitmaps/sounds.map are archives
    roots = {}
    for m in maps:
        for a, group in m.roots:
            roots.setdefault(group, Type(group_name(group))).instances.append((m, a, 1))
    solve(roots, maps)
    count = sum(1 for _ in roots)
    print("%d tag groups" % count)
    if not prove(maps, roots):
        raise SystemExit("layouts do not cover the tag data exactly")
    if "--out" in sys.argv:
        emit(roots, sys.argv[sys.argv.index("--out") + 1])


if __name__ == "__main__":
    main()
