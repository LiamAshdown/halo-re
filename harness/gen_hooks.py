"""Generate the inbound adapters that let original halo.exe code call the rewritten C functions.
For every src/<module>/<name>.c with an address header and a definition:
  * parameters from the C definition; which of them arrive in registers from the "// blam-cc:" line
    (e.g. "EAX -> actor_index, stack -> zone, EDX -> in_out_value"); every other parameter is a stack argument,
    in declaration order
  * the original's stack cleanup (ret / ret N) from objdump of its body
  * adapter: save the registers an LTCG caller may expect preserved, push the C arguments (registers and the
    caller's stack slots), call the C function, restore, 'ret N' like the original. A per-function call counter.
Hook safety: a function is 'hookable' only if every function it can reach (through the DLL's own C-to-C calls) is
either rewritten here or a standard-convention import / CRT / x87 shim, i.e. never a 'push addr / ret' stub into
original code whose register convention the C caller cannot supply.
Writes harness/gen/adapters.asm, harness/gen/hook_table.c, harness/build/hooks_report.json.
Usage: python harness/gen_hooks.py   (after harness/gen_link.py, which writes resolve.asm and the objects)"""
import os, re, glob, json, struct, subprocess, collections

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
H = os.path.join(ROOT, "harness"); GEN = os.path.join(H, "gen"); OUT = os.path.join(H, "build")
OBJDUMP = r"C:\msys64\ucrt64\bin\objdump.exe"
REG32 = {"EAX": "eax", "ECX": "ecx", "EDX": "edx", "EBX": "ebx", "ESI": "esi", "EDI": "edi",
         "AX": "eax", "AL": "eax", "AH": None, "CX": "ecx", "CL": "ecx", "DX": "edx", "DL": "edx",
         "BX": "ebx", "BL": "ebx", "SI": "esi", "DI": "edi"}
DEF = re.compile(r"^(?!extern|static|typedef|#|//|\s)([A-Za-z_][\w \t\*]*?)\b([A-Za-z_]\w*)\s*\(([^;{]*?)\)\s*(?://[^\n]*)?\s*\{", re.M)
HDR = re.compile(r"address\s+0x0*([0-9a-f]{6}),\s*size\s+(\d+)", re.I)

def split_params(s):
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch == "(": depth += 1
        elif ch == ")": depth -= 1
        if ch == "," and depth == 0: out.append(cur); cur = ""
        else: cur += ch
    if cur.strip(): out.append(cur)
    return [re.sub(r"/\*.*?\*/|//[^\n]*", "", p).strip() for p in out]

def param_info(p):
    if p in ("void", ""): return None
    fp = re.search(r"\(\s*\*\s*(\w+)\s*\)", p)
    name = fp.group(1) if fp else re.findall(r"[A-Za-z_]\w*", re.sub(r"\[.*?\]", "", p))[-1]
    t = p[:p.rfind(name)] if not fp else "fp"
    ptr = "*" in t or fp or "[" in p
    size = 8 if (not ptr and re.search(r"\b(double|int64_t|uint64_t|__int64|long\s+long)\b", t)) else 4
    byval_struct = (not ptr and re.search(r"\b(real_\w+|\w+_data|\w+_t\b(?<!int8_t)(?<!int16_t)(?<!int32_t))", t)
                    and not re.search(r"\b(u?int(8|16|32)_t|float|int|char|short|long|unsigned|signed|bool|BOOL|datum_index|tag|string_id|HWND|HANDLE|DWORD|size_t|uintptr_t|real|angle|fixed_t)\b", t))
    return {"name": name, "size": size, "byval": bool(byval_struct), "type": t.strip()}

REGWORD = r"(EAX|ECX|EDX|EBX|ESI|EDI|AX|AL|AH|CX|CL|DX|DL|BX|BL|SI|DI)"

def parse_cc(text, param_names, type_words=None):
    """register -> parameter from the "// blam-cc:" line, in any of the phrasings the rewrites use
    ("EAX -> x", "EAX (low 16 bits) -> x", "x in EAX", "EAX = x", "x=EDI", "EAX x"); a parameter may also be
    written with spaces for underscores ("element size in EBX")."""
    blocks = [re.sub(r"\n//\s*", " ", m.group(1)) for m in
              re.finditer(r"(?:blam-cc|register convention):\s*([^\n]*(?:\n//\s{2,}[^\n]*)*)", text[:6000])]
    if not blocks: return {}, False, {}, []
    s = " ; ".join(blocks)
    s = re.sub(r"\b(?:in|unaff|extraout)_(E?[A-D]X|E?[SD]I|[A-D][LH])\b", r"\1", s)   # Ghidra's in_EAX / unaff_EBX
    alias = {}
    for n in param_names: alias[n] = n; alias[n.replace("_", " ")] = n
    types = collections.Counter(tw for tw in (type_words or {}).values())
    for n, tw in (type_words or {}).items():                 # "heap* in ESI" -> the one parameter of type heap *
        if tw and types[tw] == 1 and tw not in alias: alias[tw + "*"] = n; alias[tw] = n
    names = "|".join(sorted((re.escape(a) for a in alias), key=len, reverse=True)) or "(?!)"
    regs, widths = {}, {}
    def width_of(r, note):
        # "EDX(DL)", "EAX (low 16 bits)", "AX", "CL": the width actually passed
        sub = re.search(r"\b([ABCD]L)\b|low (?:8 bits|byte)", note or "")
        if r in ("AL", "BL", "CL", "DL") or sub: return 8
        if len(r) == 2 or re.search(r"\b([ABCD]X|SI|DI)\b|low 16|low word", note or ""): return 16
        return 32
    for pat in (r"\b" + REGWORD + r"(\s*\([^)]*\))?\s*(?:->|=|:)?\s*(" + names + r")\b",
                r"\b(" + names + r")(?:\s+[a-z]+){0,2}\s*(\([^)]*\)\s*)?(?:in(?:\s+the)?|=|:)\s*" + REGWORD + r"\b"):
        for a, note, b in re.findall(pat, s):
            r, n = (a, b) if re.fullmatch(REGWORD, a) else (b, a)
            regs.setdefault(alias[n], REG32.get(r))
            widths.setdefault(alias[n], width_of(r, note))
    so = re.search(r"stack\s*->\s*\(?([A-Za-z_][\w ,/]*)\)?", s)
    stack_order = [alias[w] for w in re.split(r"[,/]\s*", so.group(1).strip()) if w.strip() in alias] if so else []
    return regs, True, widths, stack_order

FUNC_SIZES = {}          # entry address -> size, from the src headers (filled in main)
import threading
_LIVE_MEMO, _TLS = {}, threading.local()   # memo shared (complete results only); in-progress set per thread

def live_in_of(addr, depth=0):
    """memoised live_in for a call target; a recursive cycle contributes nothing"""
    if addr in _LIVE_MEMO: return _LIVE_MEMO[addr]
    busy = _TLS.__dict__.setdefault("busy", set())
    if addr in busy or not (0x401000 <= addr < 0x632000): return []
    busy.add(addr)
    r = live_in(addr, FUNC_SIZES.get(addr, 256), depth)
    busy.discard(addr); _LIVE_MEMO[addr] = r
    return r

def live_in(addr, size, _depth=0):
    """registers the original reads before writing them along its fall-through path (through conditional branches
    and calls, which clobber eax/ecx/edx, up to the first ret or jmp): its register arguments. push of a register is a save, not a use; xor r,r / sub r,r are writes."""
    r = subprocess.run([OBJDUMP, "-d", "-M", "intel", "--no-show-raw-insn", f"--start-address=0x{addr:x}",
                        f"--stop-address=0x{addr + size:x}", os.path.join(ROOT, "bin", "halo.exe")],
                       capture_output=True, text=True)
    full = {"eax": "eax", "ax": "eax", "al": "eax", "ah": "eax", "ecx": "ecx", "cx": "ecx", "cl": "ecx", "ch": "ecx",
            "edx": "edx", "dx": "edx", "dl": "edx", "dh": "edx", "ebx": "ebx", "bx": "ebx", "bl": "ebx", "bh": "ebx",
            "esi": "esi", "si": "esi", "edi": "edi", "di": "edi"}
    written, live = set(), set()
    popped = set(re.findall(r"\tpop\s+(e?[abcd]x|e?si|e?di)\b", r.stdout))   # push r is a save only if r is popped
    regs_in = lambda s: {full[x] for x in re.findall(r"\b(e?[abcd]x|[abcd][lh]|e?si|e?di)\b", s)}
    insns = []
    for l in r.stdout.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+(\w+)\s*(.*)$", l.rstrip(chr(13)))
        if m: insns.append((int(m.group(1), 16), m.group(2), m.group(3).split("#")[0].split("<")[0].strip()))
    at = {a: k for k, (a, _, _) in enumerate(insns)}
    k, steps = 0, 0
    while k < len(insns) and steps < 400:
        here, op, args = insns[k]; k += 1; steps += 1
        if op == "push" and re.fullmatch(r"e?[abcd]x|e?si|e?di", args) and args in popped: continue   # a save
        if op == "pop": written |= regs_in(args); continue
        parts = [a.strip() for a in re.split(r",(?![^\[]*\])", args)] if args else []
        dst, srcs = (parts[0], parts[1:]) if parts else ("", [])
        if op in ("xor", "sub") and len(parts) == 2 and parts[0] == parts[1] and parts[0] in full:
            written.add(full[parts[0]]); continue
        reads = set()
        for s_ in srcs: reads |= regs_in(s_)
        if "[" in dst: reads |= regs_in(dst)                 # address registers of a memory destination
        elif op == "push": reads |= regs_in(dst)
        elif not (op in ("mov", "movzx", "movsx", "lea", "fnstsw", "fstsw", "lahf", "rdtsc", "cpuid") or op.startswith("set")):
            reads |= regs_in(dst)                            # read-modify-write
        rep = op.startswith("rep"); sop = (args.split() or [""])[0] if rep else op
        if "xmm" not in args:
            sop = re.sub(r"[bwd]$", "", sop) if sop not in ("movs", "stos", "lods", "scas", "cmps") else sop
            reads |= {"movs": {"esi", "edi"}, "cmps": {"esi", "edi"}, "stos": {"edi", "eax"}, "scas": {"edi", "eax"},
                      "lods": {"esi"}}.get(sop, set()) | ({"ecx"} if rep else set())
        if op == "cdq": reads |= {"eax"}
        if op in ("div", "idiv", "mul", "imul") and len(parts) == 1: reads |= {"eax"}
        live |= reads - written
        if "[" not in dst and dst and op != "push": written |= regs_in(dst)
        if op == "cdq": written.add("edx")
        if op == "call":
            # the callee's own register inputs are read here, unless this function wrote them first
            # (LTCG passes values straight through: heap_allocate hands its caller's EAX to heap_allocate_raw)
            c = re.fullmatch(r"0x([0-9a-f]+)", args)
            if c and _depth < 6:
                live |= set(live_in_of(int(c.group(1), 16), _depth + 1)) - written
            written |= {"eax", "ecx", "edx"}
        if op == "ret": break
        if op == "jmp":                                       # follow a forward jump inside the function
            j = re.fullmatch(r"0x([0-9a-f]+)", args)
            if j and int(j.group(1), 16) > here and int(j.group(1), 16) in at: k = at[int(j.group(1), 16)]; continue
            break
    return sorted(live)

def void_functions_with_eax_readers(addrs):
    """entry addresses (of those given) that have at least one call site reading EAX/AX/AL right after the call"""
    dis = os.path.join(ROOT, "build", "halo_text.dis")
    if not os.path.exists(dis): return set()
    lines = open(dis).read().splitlines(); used = set(); rd = re.compile(r"\b(eax|ax|al|ah)\b")
    for i, l in enumerate(lines):
        m = re.search(r"\tcall\s+0x([0-9a-f]+)$", l)
        if not m or int(m.group(1), 16) not in addrs: continue
        for j in range(i + 1, min(i + 6, len(lines))):
            mm = re.match(r"\s*[0-9a-f]+:\s+(\w+)\s*(.*)", lines[j])
            if not mm: break
            op, args = mm.group(1), mm.group(2)
            parts = [x.strip() for x in re.split(r",(?![^\[]*\])", args)] if args else []
            srcs = parts if op in ("push", "test", "cmp") else parts[1:]
            if any(rd.search(s) for s in srcs) or (parts and "[" in parts[0] and rd.search(parts[0])):
                used.add(int(m.group(1), 16)); break
            if parts and re.fullmatch(r"eax|ax|al", parts[0]): break          # overwritten first
            if op in ("call", "ret", "jmp") or op.startswith("j"): break
    return used

def eax_return_register(f):
    """if every ret of the original is preceded by 'mov eax, R' where R is one of its register arguments and R is
    never modified in the body, return R (the original returns that input, e.g. its out pointer); else None"""
    r = subprocess.run([OBJDUMP, "-d", "-M", "intel", "--no-show-raw-insn", f"--start-address=0x{f['addr']:x}",
                        f"--stop-address=0x{f['addr'] + f['size']:x}", os.path.join(ROOT, "bin", "halo.exe")], capture_output=True, text=True)
    ins = [(m.group(1), m.group(2).strip()) for m in (re.match(r"\s*[0-9a-f]+:\s+(\w+)\s*(.*)$", l.rstrip(chr(13))) for l in r.stdout.splitlines()) if m]
    regs = set(f["regs"].values()); chosen = None
    # EAX never modified: the caller reads back its own EAX (often an out pointer it passed in EAX), which the
    # adapter preserves -- exact, nothing to add
    writes_eax = any((re.match(r"(eax|ax|al|ah)\b", a) and o not in ("cmp", "test", "push")) or o in ("call", "fnstsw", "fstsw",
                     "cdq", "cwde", "cbw", "lahf", "rdtsc", "cpuid") or (o in ("mul", "div", "idiv", "imul") and "," not in a)
                     or o.startswith(("rep", "lods", "scas")) for o, a in ins)
    if not writes_eax: return "preserve"
    for i, (op, args) in enumerate(ins):
        if op != "ret": continue
        k = i - 1                                         # back to the last instruction that writes EAX
        while k >= 0 and k > i - 12 and not (re.match(r"(eax|ax|al|ah)\b", ins[k][1]) and ins[k][0] not in ("cmp", "test", "push")) \
                and ins[k][0] not in ("call", "ret", "jmp") and not ins[k][0].startswith("j") and ins[k][0] != "fnstsw":
            k -= 1
        m = re.fullmatch(r"eax,\s*(e[a-d]x|esi|edi)", ins[k][1]) if k >= 0 and ins[k][0] == "mov" else None
        if not m or m.group(1) not in regs or (chosen and chosen != m.group(1)): return None
        chosen = m.group(1)
    if not chosen: return None
    for op, args in ins:                                  # the register must hold the input unchanged throughout
        dst = args.split(",")[0].strip()
        if dst == chosen and op not in ("push", "pop", "cmp", "test"): return None
    return chosen

def ret_cleanup(addr, size):
    r = subprocess.run([OBJDUMP, "-d", "-M", "intel", f"--start-address=0x{addr:x}", f"--stop-address=0x{addr + size:x}",
                        os.path.join(ROOT, "bin", "halo.exe")], capture_output=True, text=True)
    rets = set()
    for l in r.stdout.splitlines():
        m = re.search(r"\tret\s*(0x[0-9a-f]+)?\s*$", l)
        if m: rets.add(int(m.group(1), 16) if m.group(1) else 0)
    return rets

def coff_undefined(obj):
    d = open(obj, "rb").read()
    nsec, _, symptr, nsym = struct.unpack_from("<HHIII", d, 2)[0], 0, *struct.unpack_from("<II", d, 8)
    strtab = symptr + nsym * 18; out = []; i = 0
    while i < nsym:
        e = d[symptr + i * 18: symptr + i * 18 + 18]
        name = e[:8]
        if name[:4] == b"\0\0\0\0":
            o = struct.unpack_from("<I", name, 4)[0]; name = d[strtab + o:].split(b"\0")[0]
        else: name = name.rstrip(b"\0")
        sec = struct.unpack_from("<h", e, 12)[0]; cls = e[16]; naux = e[17]
        if sec == 0 and cls == 2: out.append(name.decode("latin-1"))
        i += 1 + naux
    return out

def main():
    resolve = open(os.path.join(GEN, "resolve.asm")).read()
    stubs = set(re.findall(r"^PUBLIC (\S+)\n\S+:\n    push 0", resolve, re.M))       # calls into original code
    up = os.path.join(OUT, "unresolved.txt")                                          # still unresolved: would call 0
    if os.path.exists(up): stubs |= {l.split("\t")[0] for l in open(up) if l.strip()}
    kb = os.path.join(H, "known_bad.txt")                                             # shown wrong by harness/difftest
    known_bad = {l.split("#")[0].strip() for l in open(kb)} - {""} if os.path.exists(kb) else set()
    ir = os.path.join(H, "incomplete_rewrites.txt")                                    # rewrites that say they are incomplete
    if os.path.exists(ir): known_bad |= {l.split("#")[0].strip() for l in open(ir)} - {""}
    stubs |= {"_" + n for n in known_bad}
    # a caller whose extern for a rewritten function disagrees with the definition passes the wrong arguments C-to-C
    pm = os.path.join(ROOT, "out", "prototype_mismatches.json")
    bad_calls = collections.defaultdict(set)
    if os.path.exists(pm):
        for m_ in json.load(open(pm)): bad_calls[os.path.splitext(os.path.basename(m_["caller"]))[0]].add(m_["callee"])
    funcs, report = {}, collections.Counter(); skipped = collections.defaultdict(list)
    for p in sorted(glob.glob(os.path.join(ROOT, "src", "*", "*.c"))):
        name = os.path.splitext(os.path.basename(p))[0]; mod = os.path.basename(os.path.dirname(p))
        t = open(p, encoding="utf-8", errors="replace").read(); b = t[:t.rfind("#if 0")] if "#if 0" in t else t
        h = HDR.search(t[:4000]); d = [m for m in DEF.finditer(b) if m.group(2) == name]
        if not h or not d: skipped["no address header or definition"].append(name); continue
        prefix, params_s = d[0].group(1), d[0].group(3)
        if re.search(r"__fastcall|__stdcall", prefix): skipped["fastcall/stdcall definition"].append(name); continue
        if "..." in params_s: skipped["variadic"].append(name); continue
        params = [x for x in (param_info(q) for q in split_params(params_s)) if x]
        if any(x["byval"] for x in params): skipped["struct passed by value"].append(name); continue
        # only this function's own notes: the header before the first #include/extern, and the comment lines
        # directly above the definition (a callee's extern carries its own blam-cc, which must not be read)
        hdr_end = re.search(r'^(#include|extern)', t, re.M); own = t[:hdr_end.start()] if hdr_end else t[:3000]
        above = b[:d[0].start()].rstrip(chr(10)).split(chr(10)); k_ = len(above)
        while k_ > 0 and above[k_ - 1].lstrip().startswith('//'): k_ -= 1
        own += chr(10) + chr(10).join(above[k_:])
        own += chr(10) + d[0].group(0)                # the definition line itself (a trailing '// blam-cc:' note)
        regs, has_cc, widths, stack_order = parse_cc(own, [x['name'] for x in params], {x['name']: (re.findall(r'[A-Za-z_]\w*', x['type'].replace('const', '').replace('struct', '')) or [''])[-1] for x in params})
        if len(set(regs.values())) != len(regs): skipped["two parameters mapped to one register"].append(name); continue
        unknown = [n for n in regs if n not in {x["name"] for x in params}]
        if unknown: skipped["blam-cc names a register argument that is not a parameter"].append(f"{name} ({','.join(unknown)})"); continue
        if any(v is None for v in regs.values()): skipped["high-byte register argument"].append(name); continue
        ret = "void" if re.match(r"\s*void\s*$", prefix.replace("static", "")) else \
              ("float" if re.search(r"\b(float|double|real)\s*$", prefix) else
               ("int64" if re.search(r"\b(u?int64_t|__int64)\s*$", prefix) else
                ("byte" if re.search(r"\b(u?int8_t|bool|boolean|char|BOOLEAN|byte)\s*$", prefix) else
                 ("word" if re.search(r"\b(u?int16_t|short|word)\s*$", prefix) else "int"))))
        funcs[name] = dict(module=mod, addr=int(h.group(1), 16), size=int(h.group(2)), params=params, regs=regs, widths=widths, stack_order=stack_order,
                           has_cc=has_cc, ret=ret, obj=os.path.join(ROOT, "build", "obj", mod, name + ".obj"))
    # original stack cleanup, in parallel
    from concurrent.futures import ThreadPoolExecutor
    cache_p = os.path.join(OUT, "ret_cache.json")
    cache = json.load(open(cache_p)) if os.path.exists(cache_p) else {}
    key = lambda f: f"{f['addr']:x}:{f['size']}"
    FUNC_SIZES.update({f["addr"]: f["size"] for f in funcs.values()})
    todo = [f for f in funcs.values() if key(f) not in cache or not isinstance(cache[key(f)], dict) or cache[key(f)].get("v") != 3]
    with ThreadPoolExecutor(16) as ex:
        for f, res in zip(todo, ex.map(lambda f: (sorted(ret_cleanup(f["addr"], f["size"])), live_in(f["addr"], f["size"])), todo)):
            cache[key(f)] = {"rets": res[0], "live_in": res[1], "v": 3}
    json.dump(cache, open(cache_p, "w"))
    for f in funcs.values(): f["rets"] = cache[key(f)]["rets"]; f["live_in"] = cache[key(f)]["live_in"]
    # reachability over the DLL's own C-to-C calls
    defined = {"_" + n for n in funcs}
    refs = {n: set(coff_undefined(f["obj"])) if os.path.exists(f["obj"]) else set() for n, f in funcs.items()}
    bad_direct = {n: sorted(r & stubs) for n, r in refs.items()}
    unsafe = {}
    def reach_bad(n, seen):
        if n in unsafe: return unsafe[n]
        if n in seen: return None
        seen.add(n)
        if n in known_bad: unsafe[n] = "listed in harness/known_bad.txt"; return unsafe[n]
        if bad_direct.get(n): unsafe[n] = f"calls original {bad_direct[n][0]}"; return unsafe[n]
        if bad_calls.get(n): unsafe[n] = f"declares {sorted(bad_calls[n])[0]} differently from its definition"; return unsafe[n]
        for r in refs.get(n, ()):
            if r in defined:
                why = reach_bad(r[1:], seen)
                if why: unsafe[n] = f"via {r[1:]}: {why}" if not why.startswith("via") else f"via {r[1:]}"; return unsafe[n]
        return None
    for n in funcs: reach_bad(n, set())
    # emit
    asm = [".386", ".model flat", "option casemap:none", ".data"]
    asm += [f"PUBLIC _hk_count_{n}\n_hk_count_{n} DD 0" for n in funcs]
    asm += [".code"]; table = []
    # code elsewhere that jumps into an entry's first 5 bytes would land inside the patched jmp
    jump_targets = set()
    dis = os.path.join(ROOT, "build", "halo_text.dis")
    if os.path.exists(dis):
        for l in open(dis):
            m = re.search(r"\t(?:j\w+|call)\s+0x([0-9a-f]+)\s*$", l)
            if m: jump_targets.add(int(m.group(1), 16))
    eax_returns = {}
    used_addrs = void_functions_with_eax_readers({f["addr"] for f in funcs.values() if f["ret"] == "void"})
    void_used = {n for n, f in funcs.items() if f["addr"] in used_addrs}
    image = open(os.path.join(ROOT, "bin", "halo.exe"), "rb").read()
    def looks_like_entry(f):
        # the 5-byte patch must fit inside the function, and the entry must really start one: MSVC aligns functions
        # to 16 bytes or leaves padding (int3 / nop) or a ret before them. 0x569450 "unit_animation_set_state" was the
        # 1-byte ret closing the previous function, and the patch overwrote the jump table after it.
        if f["size"] < 5: return False
        prev = image[f["addr"] - 0x400000 - 1]
        return f["addr"] % 16 == 0 or prev in (0xcc, 0x90, 0xc3) or image[f["addr"] - 0x400000 - 3] == 0xc2
    for n, f in funcs.items():
        if not looks_like_entry(f):
            skipped["not a real function entry (under 5 bytes, or starts mid-code)"].append(n); continue
        if any(f["addr"] + d in jump_targets for d in range(1, 5)):
            skipped["a jump lands inside the 5 patched entry bytes"].append(n); continue
        mapped = sorted(set(f["regs"].values()))
        if mapped != f["live_in"]:
            skipped["register arguments differ from the original's live-in registers"].append(f"{n} (notes {mapped}, binary {f['live_in']})"); continue
        if len(f["rets"]) != 1: skipped[f"original has {len(f['rets'])} distinct ret forms"].append(n); continue
        if f["ret"] == "void" and n in void_used:
            r_ = eax_return_register(f)
            if r_ is None:
                skipped["void in C, but callers use a value the original leaves in EAX"].append(n); continue
            eax_returns[n] = r_
        cleanup = f["rets"][0]
        save = ["ebx", "esi", "edi"] + [r for r in ("ecx", "edx", "eax") if not
                ((r == "eax" and f["ret"] in ("int", "int64", "byte", "word")) or (r == "edx" and f["ret"] == "int64"))]
        body = [f"PUBLIC _hk_{n}", f"EXTERN _{n}:PROC", f"_hk_{n}:", f"    inc dword ptr [_hk_count_{n}]", "    push ebp", "    mov ebp, esp"]
        body += [f"    push {r}" for r in save]
        pushes, stack_off, total = [], 8, 0
        # caller's stack slot of each stack parameter: in the order the notes give ("stack -> (speed, origin, out)"),
        # which came from the disassembly, else in C declaration order
        stack_params = [x for x in f["params"] if x["name"] not in f["regs"]]
        order = f.get("stack_order") or []
        if sorted(order) != sorted(x["name"] for x in stack_params): order = [x["name"] for x in stack_params]
        slot, o_ = {}, 8
        for nm in order:
            slot[nm] = o_; o_ += next(x["size"] for x in stack_params if x["name"] == nm)
        f["stack_order_used"] = order
        for x in f["params"]:
            if x["name"] not in f["regs"]:
                stack_off = slot[x["name"]]
            if x["name"] in f["regs"]:
                # a byte or word register argument: only the low part is defined, so extend it in the pushed copy
                # (bytes zero-extended: flags and small counts; words sign-extended: indexes where -1 means none)
                w = f["widths"].get(x["name"], 32); unsigned = bool(re.search(r"(^|[^A-Za-z])u(int|nsigned)", x["type"]))
                ext = [] if w == 32 else (["    and dword ptr [esp], 0FFh"] if w == 8 and (unsigned or "int8_t" not in x["type"]) else
                       [f"    shl dword ptr [esp], {32 - w}", f"    {'shr' if unsigned else 'sar'} dword ptr [esp], {32 - w}"])
                pushes.append([f"    push {f['regs'][x['name']]}"] + ext)
            else:
                if x["size"] == 8: pushes.append([f"    push dword ptr [ebp+{stack_off + 4}]", f"    push dword ptr [ebp+{stack_off}]"])
                else: pushes.append([f"    push dword ptr [ebp+{stack_off}]"])
            total += x["size"]
        # registers are still the caller's values here (only pushes so far), so they can be pushed in any order
        for pp in reversed(pushes): body += pp
        body += [f"    call _{n}"] + ([f"    add esp, {total}"] if total else [])
        body += [f"    pop {r}" for r in reversed(save)]
        if eax_returns.get(n, "preserve") != "preserve":   # void in C, but the original hands an input register back in EAX
            body += [f"    mov eax, {eax_returns[n]}"]
        body += ["    pop ebp", f"    ret {cleanup}" if cleanup else "    ret"]
        stack_bytes = sum(x["size"] for x in stack_params)
        if cleanup and cleanup != stack_bytes: skipped["ret N disagrees with the stack parameters"].append(f"{n} (ret {cleanup}, params {stack_bytes})"); continue
        asm += body
        table.append((n, f))
    asm += ["END", ""]
    open(os.path.join(GEN, "adapters.asm"), "w").write("\n".join(asm))
    c = ["/* generated by harness/gen_hooks.py */", "#include \"../hook_table.h\""]
    c += [f"extern void hk_{n}(void); extern unsigned long hk_count_{n};" for n, _ in table]
    c += ["const hook_entry hook_table[] = {"]
    c += [f'    {{0x{f["addr"]:08x}u, hk_{n}, &hk_count_{n}, "{n}", "{f["module"]}", {0 if n in unsafe else 1}}},' for n, f in table]
    c += ["};", f"const unsigned hook_count = {len(table)};", ""]
    open(os.path.join(GEN, "hook_table.c"), "w").write("\n".join(c))
    # description of every adapter's calling shape, for harness/difftest (original vs rewrite on the same inputs)
    REGIDX = {"eax": 0, "ecx": 1, "edx": 2, "ebx": 3, "esi": 4, "edi": 5}
    def kind(x):
        t = x["type"]
        if x["size"] == 8: return "d"
        if "*" in t or "[" in t or t == "fp": return "p"
        if re.search(r"\b(float|real|angle)\b", t): return "f"
        return "i"
    dt = ["/* generated by harness/gen_hooks.py */", "#include \"../difftest.h\""]
    dt += [f"extern void hk_{n}(void);" for n, _ in table]
    dt += ["const difftest_entry difftest_table[] = {"]
    for n, f in table:
        by_name = {x["name"]: x for x in f["params"]}
        ordered = [x for x in f["params"] if x["name"] in f["regs"]] + [by_name[nm] for nm in f.get("stack_order_used", [])]
        shape = ",".join(f"{REGIDX[f['regs'][x['name']]] if x['name'] in f['regs'] else -1}{kind(x)}" for x in ordered)
        dt.append(f'    {{0x{f["addr"]:08x}u, hk_{n}, "{n}", "{f["module"]}", "{shape}", \'{f["ret"][0]}\', {0 if n in unsafe else 1}}},')
    dt += ["};", f"const unsigned difftest_count = {len(table)};", ""]
    open(os.path.join(GEN, "difftest_table.c"), "w").write("\n".join(dt))
    rep = {"adapters": len(table), "hookable": sum(1 for n, _ in table if n not in unsafe),
           "unsafe (reach an original register-convention function)": {n: unsafe[n] for n, _ in table if n in unsafe},
           "skipped": {k: v for k, v in skipped.items()}}
    json.dump(rep, open(os.path.join(OUT, "hooks_report.json"), "w"), indent=1)
    print("functions parsed:", len(funcs), "| adapters:", len(table), "| hookable (safe call tree):", rep["hookable"])
    print("skipped:", {k: len(v) for k, v in skipped.items()})
    by_mod = collections.Counter(f["module"] for n, f in table if n not in unsafe)
    tot_mod = collections.Counter(f["module"] for n, f in table)
    print("hookable by module:", {m: f"{by_mod[m]}/{tot_mod[m]}" for m in sorted(tot_mod)})

if __name__ == "__main__": main()
