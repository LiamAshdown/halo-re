"""Turns the parts of the old retail data image that our code actually reaches into readable C, so no image dump is needed.

Roots are the stale numeric pointers in standalone/data/eq_data.c (and a few raw addresses in code). Everything reachable from them is
followed through pointer-looking dwords:
  - a target whose bytes are printable text ending in NUL becomes an inline string literal ("..." / L"...") at every use;
  - any other target belongs to a block: a span of dwords bounded by text, by the start of a named global, by the region end
    and (read-only data only) by long zero runs. A block becomes a named uint32_t array whose rows keep the original offsets
    as comments; pointers inside it are written as addresses of other blocks, string literals, C globals or function names;
  - a target in the zero-filled gap becomes a zero-initialised array.
Writing: standalone/data/tables.c + tables.h, and eq_data.c is rewritten to use them. The raw image is read from git (commit
RAW) because the working copies are deleted afterwards.
Usage: python tools/materialize_image.py [--apply]     (default: report only)"""
import bisect, collections, glob, os, re, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = "293e5c40"           # commit with the raw standalone/image/halo_image_<piece>.c
EQ_COMMIT = "921e2577"     # commit with the raw eq_data.c literals
APPLY = "--apply" in sys.argv

REGIONS = {"rdata": (0x63a000, 0x676000), "data": (0x676000, 0x6a0088), "bss": (0x6a0088, 0x881000),
           "tls": (0x881000, 0x882000), "rsrc": (0x882000, 0x891000)}
LO, HI = 0x63a000, 0x883000


def git(*args):
    return subprocess.run(["git"] + list(args), capture_output=True, text=True, encoding="utf-8", cwd=ROOT).stdout


def load_piece(label):
    lo, hi = REGIONS[label]
    text = git("show", "%s:standalone/image/halo_image_%s.c" % (RAW, label))
    body = text[text.index("= {") + 3: text.rindex("};")]
    words = [("w", 0)] * ((hi - lo) // 4)
    idx = 0
    for item in re.split(r",\s*", body.strip()):
        item = item.strip()
        if not item:
            continue
        m = re.fullmatch(r"\[(\d+)\] = (.*)", item)
        if m:
            idx, item = int(m.group(1)), m.group(2)
        m = re.fullmatch(r"\(unsigned int\)halo_code_([0-9a-f]+)", item)
        words[idx] = ("c", m.group(1)) if m else ("w", int(item, 16))
        idx += 1
    return words


class Image:
    def __init__(self):
        self.words = {k: load_piece(k) for k in ("rdata", "data", "tls", "rsrc")}
        self.raw = {}
        for k, ws in self.words.items():
            b = bytearray()
            for kind, v in ws:
                b += struct.pack("<I", v if kind == "w" else 0xfffffffe)
            self.raw[k] = bytes(b)

    @staticmethod
    def region(a):
        for k, (lo, hi) in REGIONS.items():
            if lo <= a < hi:
                return k
        return None

    def byte(self, a):
        k = self.region(a)
        if k is None:
            return 0
        if k == "bss":
            return 0
        return self.raw[k][a - REGIONS[k][0]]

    def dword(self, a):
        k = self.region(a)
        if k is None or k == "bss":
            return ("w", 0)
        return self.words[k][(a - REGIONS[k][0]) // 4]

    def cstring(self, a, limit=4096):
        k = self.region(a)
        if k is None or k == "bss":
            return None
        raw, base = self.raw[k], REGIONS[k][0]
        i = a - base
        j = i
        while j < len(raw) and raw[j] != 0 and j - i < limit:
            j += 1
        if j >= len(raw) or raw[j] != 0:
            return None
        return raw[i:j]


def printable(b):
    return 0x20 <= b < 0x7f or b in (9, 10, 13)


def as_ascii(img, a):
    s = img.cstring(a)
    if s is None or len(s) < 2 or not all(printable(c) for c in s):
        return None
    return s.decode("latin-1")


def as_wide(img, a):
    k = img.region(a)
    if k is None or k == "bss" or a % 2:
        return None
    raw, base = img.raw[k], REGIONS[k][0]
    i = a - base
    out = []
    while i + 1 < len(raw):
        lo, hi = raw[i], raw[i + 1]
        if lo == 0 and hi == 0:
            break
        if hi != 0 or not printable(lo):
            return None
        out.append(chr(lo))
        i += 2
        if len(out) > 2000:
            return None
    else:
        return None
    return "".join(out) if len(out) >= 2 else None


def esc(s):
    out = []
    for ch in s:
        c = ord(ch)
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        elif ch == "?":
            out.append("\\?")
        elif 0x20 <= c < 0x7f:
            out.append(ch)
        else:
            out.append("\\%03o" % c)
    return "".join(out)


def text_mask(img):
    """per region: bytearray, 1 where a byte belongs to an ASCII (>=6) or UTF-16 (>=5) text run"""
    masks = {}
    for k in ("rdata", "data"):
        raw = img.raw[k]
        n = len(raw)
        mask = bytearray(n)
        i = 0
        while i < n:
            if printable(raw[i]):
                j = i
                while j < n and printable(raw[j]):
                    j += 1
                run = raw[i:j]
                good = sum(1 for c in run if chr(c).isalnum() or chr(c) in " _.:/\\%-,()<>[]'!#&*+=;")
                letters = sum(1 for c in run if chr(c).isalpha())
                if j - i >= 6 and (j >= n or raw[j] == 0) and letters >= 2 and good * 100 >= 85 * len(run):
                    mask[i:j] = b"\x01" * (j - i)
                i = j
            else:
                i += 1
        i = 0
        while i + 1 < n:
            if printable(raw[i]) and raw[i + 1] == 0:
                j = i
                while j + 1 < n and printable(raw[j]) and raw[j + 1] == 0:
                    j += 2
                if (j - i) // 2 >= 5 and (j + 1 >= n or (raw[j] == 0 and raw[j + 1] == 0)):
                    mask[i:j] = b"\x01" * (j - i)
                i = j
            else:
                i += 1
        masks[k] = mask
    return masks


def orig_globals():
    """{original address: C name} of the globals defined in the current build; also all original addresses"""
    old = git("show", "8252d572:standalone/globals.asm")
    defined = set()
    mp = os.path.join(ROOT, "build", "cxx", "Release", "halo_rebuilt.map")
    if os.path.exists(mp):
        for line in open(mp, errors="replace"):
            m = re.match(r"\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+[0-9a-f]{8}\s", line)
            if m:
                defined.add(m.group(1))
    names, every = {}, set()
    for m in re.finditer(r"^(\S+) EQU 0([0-9A-Fa-f]+)h", old, re.M):
        a = int(m.group(2), 16)
        every.add(a)
        if m.group(1) in defined and a not in names:
            names[a] = m.group(1)[1:]
    return names, every


def function_names():
    """halo_code_<address> -> C function name, for cdecl functions with a real name (not stubs, not decorated)"""
    out = {}
    src = open(os.path.join(ROOT, "standalone", "generated", "image_bindings.c"), encoding="utf-8").read()
    for m in re.finditer(r"alternatename:_halo_code_([0-9a-f]+)=(\S+?)\"\)", src):
        target = m.group(2)
        if target.startswith("_") and "@" not in target and not target.startswith("_cp_trap_") and "?" not in target:
            out[m.group(1)] = target[1:]
    return out


class Plan:
    def __init__(self):
        self.img = Image()
        self.mask = text_mask(self.img)
        self.gnames, self.gaddrs = orig_globals()
        self.fnames = function_names()
        self.strings = {}       # addr -> ("a"|"w", text)
        self.blocks = []        # (start, end)
        self.bss = []           # (start, end)
        self.targets = set()
        self.root_symbol = {}   # target -> first root symbol that references it
        self.referrer = {}      # target -> address of the first dword that holds it

    # ---- pointer test (same rules as tools/rebase_image.py)
    def is_pointer(self, v, region_mask_hit=False):
        if region_mask_hit or not (LO <= v < HI) or v == 0:
            return False
        if v in self.gnames:
            return True
        k = Image.region(v)
        if k in ("bss", "tls", "rsrc"):
            return False
        if self.is_text_start(v):
            return True
        return v % 4 == 0 and (v & 0xffff) != 0

    def is_text_start(self, v):
        return as_ascii(self.img, v) is not None or as_wide(self.img, v) is not None

    def masked(self, a):
        k = Image.region(a)
        if k not in self.mask:
            return False
        off = a - REGIONS[k][0]
        return any(self.mask[k][off + i] for i in range(4) if off + i < len(self.mask[k]))

    def masked_first(self, a):
        """True when the dword's first byte is part of a text run (text that starts later in the dword does not count)"""
        k = Image.region(a)
        return k in self.mask and bool(self.mask[k][a - REGIONS[k][0]])

    # ---- block extent
    def block_end(self, start):
        k = Image.region(start)
        lo, hi = REGIONS[k]
        a = start
        zero_run = 0
        while a + 4 <= hi:
            if a != start and (a in self.gaddrs or self.masked_first(a)):
                break
            kind, v = self.img.dword(a)
            zero_run = zero_run + 1 if (kind == "w" and v == 0) else 0
            if k == "rdata" and zero_run >= 32:
                a -= 4 * 31
                break
            a += 4
        return min(a, hi)

    def discover(self, roots):
        queue = collections.deque()
        for v, sym in roots:
            self.targets.add(v)
            self.root_symbol.setdefault(v, sym)
            queue.append(v)
        seen_blocks = []
        while queue:
            t = queue.popleft()
            k = Image.region(t)
            if t in self.gnames:
                continue
            s = as_ascii(self.img, t)
            if s is not None and not self._inside_block(t, seen_blocks):
                self.strings[t] = ("a", s)
                continue
            w = as_wide(self.img, t)
            if w is not None and not self._inside_block(t, seen_blocks):
                self.strings[t] = ("w", w)
                continue
            if k == "bss":
                continue
            if self._inside_block(t, seen_blocks):
                continue
            end = self.block_end(t)
            seen_blocks.append((t, end))
            for a in range(t, end, 4):
                kind, v = self.img.dword(a)
                if kind == "w" and self.is_pointer(v, self.masked(a)) and v not in self.targets:
                    self.targets.add(v)
                    self.referrer[v] = a
                    queue.append(v)
                elif kind == "w" and self.is_pointer(v, self.masked(a)):
                    pass
        self.blocks = self._merge(seen_blocks)
        # unnamed BSS targets
        bss_t = sorted(t for t in self.targets if Image.region(t) == "bss" and t not in self.gnames)
        bounds = sorted(self.gaddrs | set(bss_t))
        for t in bss_t:
            i = bisect.bisect_right(bounds, t)
            end = bounds[i] if i < len(bounds) else REGIONS["bss"][1]
            self.bss.append((t, min(end, t + 0x2000)))

    @staticmethod
    def _inside_block(t, blocks):
        return any(s <= t < e for s, e in blocks)

    @staticmethod
    def _merge(blocks):
        out = []
        for s, e in sorted(blocks):
            if out and s <= out[-1][1]:
                out[-1] = (out[-1][0], max(out[-1][1], e))
            else:
                out.append((s, e))
        return out


def eq_roots():
    text = git("show", "%s:standalone/data/eq_data.c" % EQ_COMMIT)
    return text


def stale_literals(plan, text):
    """[(match, value, enclosing symbol)] for the literals of the eq_data text that are pointers into the image"""
    masked = mask_text(text)
    out = []
    lit = re.compile(r"\b0[xX]([0-9a-fA-F]{6,8})([uU]?[lL]?)\b")
    defs = [(m.start(), m.group(1)) for m in re.finditer(r"\b(?:uint32_t|uint8_t|uint16_t)\s+(\w+)\s*\[", masked)]
    starts = [d[0] for d in defs]
    for m in lit.finditer(masked):
        v = int(m.group(1), 16)
        if not (LO <= v < HI):
            continue
        a = v
        if not plan.is_pointer(v):
            continue
        i = bisect.bisect_right(starts, m.start()) - 1
        sym = defs[i][1] if i >= 0 else "table"
        out.append((m, v, sym))
    return out


def mask_text(text):
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            out[i:j] = " " * (j - i)
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            for k in range(i, min(j + 1, n)):
                if out[k] != "\n":
                    out[k] = " "
            i = j + 1
        else:
            i += 1
    return "".join(out)


SLICE_FIX = {"known_campaign_levels_00692acc": "slice03.c", "default_time_unit_table": "slice03.c",
             "multiplayer_sound_enabled": "slice02.c"}
BYTE_ARRAY = re.compile(r"uint8_t (\w+)\[(\d+)\] = \{([^}]*)\};")


def slice_arrays():
    """{symbol: (file name, file text, match, bytes)} for the byte-array tables of the slice files that hold pointers"""
    out = {}
    for name, fname in SLICE_FIX.items():
        text = open(os.path.join(ROOT, "standalone", "data", fname), encoding="utf-8", newline="").read()
        for m in BYTE_ARRAY.finditer(text):
            if m.group(1) == name:
                nums = [int(x.strip(), 0) for x in m.group(3).split(",") if x.strip()]
                out[name] = (fname, text, m, bytes(nums))
    return out


def identifier(sym, addr):
    sym = re.sub(r"\W", "_", sym or "")
    return ("%s_%08x" % (sym, addr)) if sym and sym != "?" else "table_%08x" % addr


class Emitter:
    def __init__(self, plan):
        self.plan = plan
        self.block_name = {}
        for s, e in plan.blocks:
            roots = sorted(a for a in plan.targets if s <= a < e and a in plan.root_symbol)
            self.block_name[(s, e)] = identifier(plan.root_symbol[roots[0]] if roots else None, s)
        self.bss_name = {(s, e): "bss_%08x" % s for s, e in plan.bss}
        self.ext_globals = set()
        self.ext_funcs = set()

    def block_of(self, v):
        for s, e in self.plan.blocks:
            if s <= v < e:
                return s, e
        return None

    def ref(self, v):
        """C expression (uint32_t) for the pointer value v, or None when v has no home"""
        plan = self.plan
        if v in plan.gnames:
            self.ext_globals.add(plan.gnames[v])
            return "(uint32_t)%s" % plan.gnames[v]
        if v in plan.strings:
            kind, text = plan.strings[v]
            return '(uint32_t)%s"%s"' % ("L" if kind == "w" else "", esc(text))
        b = self.block_of(v)
        if b:
            name, off = self.block_name[b], v - b[0]
            return "(uint32_t)&%s[%d]" % (name, off // 4) if off % 4 == 0 else "((uint32_t)%s + 0x%x)" % (name, off)
        for (s, e), name in self.bss_name.items():
            if s <= v < e:
                return "(uint32_t)&%s[%d]" % (name, v - s) if v != s else "(uint32_t)%s" % name
        return None

    def ptr(self, v):
        r = self.ref(v)
        return "(void *)" + r[len("(uint32_t)"):] if r and r.startswith("(uint32_t)") else r

    def word(self, a):
        kind, v = self.plan.img.dword(a)
        if kind == "c":
            name = self.plan.fnames.get(v)
            if name:
                self.ext_funcs.add(name)
                return "(uint32_t)%s" % name
            self.ext_funcs.add("halo_code_" + v)
            return "(uint32_t)halo_code_%s" % v
        if v == 0:
            return "0"
        if v in self.plan.targets and self.plan.is_pointer(v, self.plan.masked(a)):
            r = self.ref(v)
            if r:
                return r
        return "0x%08xu" % v if v > 0x7fffffff else "0x%x" % v

    def stride(self, s, e):
        offs = sorted(a - s for a in self.plan.root_symbol if s <= a < e and a not in self.plan.strings)
        diffs = collections.Counter(b - a for a, b in zip(offs, offs[1:]) if 8 <= b - a <= 64 and (b - a) % 4 == 0)
        if diffs:
            d, c = diffs.most_common(1)[0]
            if c >= 3:
                return d // 4
        return 4

    def block_text(self, s, e):
        name = self.block_name[(s, e)]
        n = (e - s) // 4
        starts = sorted({0} | {(a - s) // 4 for a in self.plan.root_symbol if s <= a < e and a not in self.plan.strings and (a - s) % 4 == 0})
        starts.append(n)
        lines = ["/* 0x%08x..0x%08x */" % (s, e), "uint32_t %s[%d] = {" % (name, n)]
        for lo, hi in zip(starts, starts[1:]):
            for i in range(lo, hi, 8):
                items = [self.word(s + 4 * k) for k in range(i, min(hi, i + 8))]
                lines.append("    /* +0x%04x */ %s," % (4 * i, ", ".join(items)))
        lines.append("};")
        return chr(10).join(lines), name, n


def apply(plan, text, lits):
    em = Emitter(plan)
    bodies = []
    for s, e in plan.blocks:
        bodies.append(em.block_text(s, e))
    # accessors for the raw addresses the code used
    acc = []
    for v, name, ctype in ((0x687500, "network_index_cache_table", "void *"), (0x6e09e8, "flag_render_device_slot", "void *")):
        r = em.ptr(v)
        acc.append(("%s *const %s = %s;" % ("void", name, r), name))
    decls = ["extern uint32_t %s[%d];" % (n, cnt) for _b, n, cnt in bodies]
    decls += ["extern uint8_t %s[%d];" % (nm, e - s) for (s, e), nm in em.bss_name.items()]
    header = ["/* standalone/data/tables.h -- the tables and zero-filled objects of standalone/data/tables.c (generated by",
              "   tools/materialize_image.py). */", "#ifndef HALO_DATA_TABLES_H", "#define HALO_DATA_TABLES_H", "",
              "#include <stdint.h>", ""] + decls + ["", "extern void *const network_index_cache_table;",
              "extern void *const flag_render_device_slot;", "", "#endif", ""]
    src = ["/* standalone/data/tables.c -- data tables that the engine's own tables point into: records of definitions (script",
           "   functions, enums, message deltas, throw info, default colours, ...), referenced by standalone/data/eq_data.c and",
           "   each other. Strings are inline literals, function pointers are names, pointers between tables are addresses of",
           "   these arrays. Generated once by tools/materialize_image.py from the original data; edit freely. */", "",
           '#include "tables.h"', ""]
    src += ["extern char %s[];" % g for g in sorted(em.ext_globals)]
    src += ["extern void %s(void);" % f for f in sorted(em.ext_funcs)]
    src += ["extern uint8_t %s[];" % nm for nm in em.bss_name.values()] if False else []
    src += [""]
    for body, _n, _c in bodies:
        src += [body, ""]
    for (s, e), nm in em.bss_name.items():
        src += ["/* zero-filled object at 0x%08x (%d bytes) */" % (s, e - s), "uint8_t %s[%d];" % (nm, e - s), ""]
    src += [a for a, _n in acc] + [""]
    # eq_data.c
    pieces, last, skipped = [], 0, []
    for m, v, sym in lits:
        r = em.ref(v)
        if r is None or v in plan.gnames:
            skipped.append((v, sym))
            continue
        pieces += [text[last:m.start()], r]
        last = m.end()
    pieces.append(text[last:])
    new = "".join(pieces)
    k = new.index("#include") if "#include" in new else 0
    new = new[:k] + '#include "tables.h"\n' + new[k:]
    # map_download points into cache_file_current_header_crc32 (a converted global, not part of the image)
    md = "uint32_t map_download[1] = {0x006a8960u};"
    if md in new:
        new = new.replace(md, "uint32_t map_download[1] = {(uint32_t)(cache_file_current_header_crc32 + 0x7a8)};")
        new = new.replace('#include "tables.h"\n', '#include "tables.h"\nextern uint8_t cache_file_current_header_crc32[];\n', 1)
    # byte-array tables of the slice files that hold pointers become dword tables
    slice_out = {}
    for name, (fname, stext, m, data) in slice_arrays().items():
        n = len(data) // 4
        rows = []
        for i in range(0, n, 4):
            items = []
            for j in range(i, min(n, i + 4)):
                v = struct.unpack_from("<I", data, 4 * j)[0]
                r = em.ref(v) if (v in plan.targets and plan.is_pointer(v)) else None
                items.append(r if r else ("0" if v == 0 else "0x%08xu" % v))
            rows.append("    " + ", ".join(items) + ",")
        body = "uint32_t %s[%d] = {\n%s\n};" % (name, n, "\n".join(rows))
        cur = slice_out.get(fname, stext)
        old_def = stext[m.start():m.end()]
        assert old_def in cur, name
        cur = cur.replace(old_def, body, 1)
        cur = re.sub(r"extern uint8_t %s\[\d+\];" % re.escape(name), "extern uint32_t %s[%d];" % (name, n), cur)
        slice_out[fname] = cur
    for fname, cur in list(slice_out.items()):
        if '"tables.h"' not in cur:
            k = cur.index("#include")
            cur = cur[:k] + '#include "tables.h"\n' + cur[k:]
        slice_out[fname] = cur
    return "\n".join(header), "\n".join(src) + "\n", new, em, skipped, acc, slice_out


def main():
    plan = Plan()
    text = eq_roots()
    lits = stale_literals(plan, text)
    roots = [(v, sym) for _m, v, sym in lits]
    # raw addresses used by code
    for v, sym in ((0x671fac, "controls_tag_label"), (0x687500, "network_index_cache"), (0x6e09e8, "flag_render_device")):
        roots.append((v, sym))
    for name, (_f, _t, _m, data) in slice_arrays().items():
        for off in range(0, len(data) - 3, 4):
            v = struct.unpack_from("<I", data, off)[0]
            if plan.is_pointer(v):
                roots.append((v, name))
    plan.discover(roots)
    n_ptr = len(plan.targets)
    print("roots: %d literals, %d distinct targets reached" % (len(lits), n_ptr))
    print("strings: %d (ascii %d, wide %d), %d bytes of text" % (
        len(plan.strings), sum(1 for k, _ in plan.strings.values() if k == "a"),
        sum(1 for k, _ in plan.strings.values() if k == "w"), sum(len(t) for _k, t in plan.strings.values())))
    tot = sum(e - s for s, e in plan.blocks)
    print("blocks: %d, %d bytes (%d dwords)" % (len(plan.blocks), tot, tot // 4))
    print("unnamed bss objects: %d" % len(plan.bss))
    big = sorted(plan.blocks, key=lambda b: b[0] - b[1])[:15]
    for s, e in big:
        print("   block 0x%x..0x%x (%d bytes) first root: %s" % (s, e, e - s, plan.root_symbol.get(s, "?")))
    for v in sorted(plan.targets):
        if Image.region(v) in ("tls", "rsrc") or v >= 0x881000:
            print("   target in tls/rsrc: 0x%x referenced from 0x%x" % (v, plan.referrer.get(v, 0)))
    print("wide string:", [(hex(a), s) for a, (k, s) in plan.strings.items() if k == "w"])
    print("0x671fac:", plan.strings.get(0x671fac), " in block:", any(s <= 0x671fac < e for s, e in plan.blocks))
    code = sum(1 for s, e in plan.blocks for a in range(s, e, 4) if plan.img.dword(a)[0] == "c")
    print("function pointers inside blocks: %d" % code)
    header, src, new_eq, em, skipped, _acc, slice_out = apply(plan, text, lits)
    print("emitted: tables.h %d lines, tables.c %d lines, eq_data %d literals replaced, %d left (global starts / unhomed)" % (
        header.count("\n"), src.count("\n"), len(lits) - len(skipped), len(skipped)))
    print("externs: %d globals, %d functions" % (len(em.ext_globals), len(em.ext_funcs)))
    for v, sym in skipped[:20]:
        print("   left: 0x%x in %s" % (v, sym))
    if APPLY:
        open(os.path.join(ROOT, "standalone", "data", "tables.h"), "w", newline="\n").write(header)
        open(os.path.join(ROOT, "standalone", "data", "tables.c"), "w", newline="\n").write(src)
        open(os.path.join(ROOT, "standalone", "data", "eq_data.c"), "w", newline="\n").write(new_eq)
        for fname, body in slice_out.items():
            open(os.path.join(ROOT, "standalone", "data", fname), "w", newline=chr(10)).write(body)
        print("written (slice files: %s)" % ", ".join(sorted(slice_out)))


if __name__ == "__main__":
    main()
