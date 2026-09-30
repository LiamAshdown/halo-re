#!/usr/bin/env python3
"""emu_difftest.py - Linux/Unicorn prototype of harness/difftest: run ONE original function from bin/halo.exe and
the clang-compiled C rewrite (src/**/NAME.c) on the same random inputs and diff return value, pointer-arg buffers and
halo.exe's writable sections.

  python3 tools/emu_difftest.py matrix3x3_multiply matrix3x3_transpose -n 300
  python3 tools/emu_difftest.py matrix3x3_multiply --alias out=a          # in-place variant (pointer aliasing)
  python3 tools/emu_difftest.py float_compare_ascending --mutate 'return 1=>return 2'   # prove a bug is caught
  options: -n N  --seed S  --alias P=Q  --src FILE  --mutate 'OLD=>NEW'  --dep FILE.c  --fpcw 0x27f  --exact
           --int-range LO,HI  --icount N  -v      (needs: pip install pefile unicorn; apt: clang lld)

How it works
 * Both sides run in their own Unicorn x86-32 instance with halo.exe's PE image mapped at 0x400000 (headers, sections,
   .data/BSS zero-filled, IAT slots pointed at fake API thunks).  The rewrite is compiled with
   `clang -target i686-w64-windows-gnu -O2 -mno-sse` (x87 codegen like MSVC 7.1), linked by lld-link at 0x10000000 with the
   function as PE entry point and mapped in the second instance.  The C side is plain cdecl.
 * The original's calling convention comes from the `// blam-cc: EAX -> x, ECX (low 16) -> y, stack -> z` header line in the
   source file plus the C prototype: listed params go in that register, all other params are pushed cdecl-style in
   declaration order (right to left).  Pointer params get identical random-float buffers (1 KB + 256 B guard each side) at
   identical addresses; float params in [-4,4]; ints in --int-range.  --alias out=a makes two pointer params share one buffer.
 * Compared: EAX (int return) or ST0 (float/double return, via fstp tword), every buffer incl. guards, all of halo.exe's
   writable sections after the call (globals).  Floats compare with 1e-4 relative tolerance like the Windows tool (--exact: bitwise).
 * Any unmapped access, call to a Win32 import without a stub, or jump to an unresolved symbol stops the run with a message
   naming it (never silently continues).  A sample where BOTH sides fault at the same kind of access is counted "both fault".
"""
import os, re, sys, glob, struct, random, subprocess, tempfile, argparse
import pefile
from unicorn import *
from unicorn.x86_const import *

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HALO = os.path.join(ROOT, "bin", "halo.exe")
BASE = 0x400000
REWRITE_BASE = 0x10000000
STACK_TOP = 0x7fe00000; STACK_SIZE = 0x100000
RET_STUB = 0x7ff00000           # fstp tword [FP_OUT] ; hlt   (return address of every call)
FP_OUT = 0x7ff00100
THUNKS = 0x7e000000            # fake API entry points, 16 bytes apart
BUF_BASE = 0x30000000; BUF_STRIDE = 0x4000; BUF_BYTES = 1024; GUARD = 256
REGS = {"EAX": UC_X86_REG_EAX, "ECX": UC_X86_REG_ECX, "EDX": UC_X86_REG_EDX, "EBX": UC_X86_REG_EBX,
        "ESI": UC_X86_REG_ESI, "EDI": UC_X86_REG_EDI, "EBP": UC_X86_REG_EBP}
POISON = 0xDEADBEEF

class EmuFault(Exception): pass

# ---- tiny libc the compiled rewrite may call (clang lowers struct copies to memcpy) --------------------------------
SHIM_C = r"""
typedef unsigned int size_t;
void *memcpy(void *d, const void *s, size_t n){ void *r=d; __asm__ volatile("rep movsb":"+D"(d),"+S"(s),"+c"(n)::"memory"); return r; }
void *memmove(void *d, const void *s, size_t n){ unsigned char *a=d; const unsigned char *b=s;
  if(a<b){ while(n--) *a++=*b++; } else { a+=n; b+=n; while(n--) *--a=*--b; } return d; }
void *memset(void *d, int c, size_t n){ void *r=d; __asm__ volatile("rep stosb":"+D"(d),"+c"(n):"a"(c):"memory"); return r; }
/* single x87 instructions, like harness/x87_shims.c (same unit as the original code, so bit-exact by construction) */
double sqrt(double x){ double r; __asm__("fsqrt":"=t"(r):"0"(x)); return r; }
double fabs(double x){ double r; __asm__("fabs":"=t"(r):"0"(x)); return r; }
float sqrtf(float x){ float r; __asm__("fsqrt":"=t"(r):"0"(x)); return r; }
float fabsf(float x){ float r; __asm__("fabs":"=t"(r):"0"(x)); return r; }
double sin(double x){ double r; __asm__("fsin":"=t"(r):"0"(x)); return r; }
double cos(double x){ double r; __asm__("fcos":"=t"(r):"0"(x)); return r; }
double fsin(double x){ return sin(x); }
double fcos(double x){ return cos(x); }
double atan2(double y, double x){ double r; __asm__("fpatan":"=t"(r):"0"(x),"u"(y):"st(1)"); return r; }
double fpatan(double y, double x){ return atan2(y, x); }
"""

# ---- Win32 stubs: name -> (stdcall arg dwords, fn(emu) -> eax).  Deliberately tiny; anything else raises EmuFault. ----
API_STUBS = {
    "GetTickCount": (0, lambda e: 1000),
    "GetLastError": (0, lambda e: 0),
    "SetLastError": (1, lambda e: 0),
    "QueryPerformanceCounter": (1, lambda e: (e.uc.mem_write(e.arg(0), struct.pack("<Q", 123456789)), 1)[1]),
}

class Emu:
    def __init__(self, pe_path, fpcw):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.uc.reg_write(UC_X86_REG_FPCW, fpcw)
        self.fault = None; self.thunks = {}
        self.load_pe(pe_path, BASE, fake_iat=True)
        self.writable = self.writable_ranges
        self.uc.mem_map(STACK_TOP - STACK_SIZE, STACK_SIZE)
        self.uc.mem_map(RET_STUB, 0x1000)
        self.uc.mem_write(RET_STUB, b"\xdb\x3d" + struct.pack("<I", FP_OUT) + b"\xf4")   # fstp tword [FP_OUT]; hlt
        self.uc.mem_write(RET_STUB + 0x10, b"\xf4")                                          # plain hlt
        self.uc.mem_map(THUNKS, 0x10000)
        self.uc.hook_add(UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED | UC_HOOK_MEM_FETCH_UNMAPPED, self.on_unmapped)
        self.uc.hook_add(UC_HOOK_CODE, self.on_thunk, begin=THUNKS, end=THUNKS + 0xffff)

    def load_pe(self, path, want_base, fake_iat=False):
        pe = pefile.PE(path); base = pe.OPTIONAL_HEADER.ImageBase
        if base != want_base: raise SystemExit(f"{path}: image base {base:#x} != {want_base:#x}")
        size = (pe.OPTIONAL_HEADER.SizeOfImage + 0xfff) & ~0xfff
        self.uc.mem_map(base, size)
        self.uc.mem_write(base, pe.header[:pe.OPTIONAL_HEADER.SizeOfHeaders])
        ranges = []
        for s in pe.sections:
            data = s.get_data()[:s.Misc_VirtualSize]
            self.uc.mem_write(base + s.VirtualAddress, data)
            if s.Characteristics & 0x80000000: ranges.append((base + s.VirtualAddress, s.Misc_VirtualSize))
        if fake_iat:
            self.writable_ranges = ranges
            for imp in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []):
                for sym in imp.imports:
                    name = sym.name.decode() if sym.name else f"ord{sym.ordinal}"
                    addr = THUNKS + 16 * len(self.thunks); self.thunks[addr] = f"{imp.dll.decode()}!{name}"
                    self.uc.mem_write(sym.address, struct.pack("<I", addr))
        return pe

    # -- fault reporting ---------------------------------------------------------------------------------------
    def on_unmapped(self, uc, access, addr, size, value, _):
        kind = {UC_MEM_READ_UNMAPPED: "read", UC_MEM_WRITE_UNMAPPED: "write", UC_MEM_FETCH_UNMAPPED: "fetch"}[access]
        self.fault = f"unmapped {kind} of {size} bytes at {addr:#010x} (eip={uc.reg_read(UC_X86_REG_EIP):#010x})"
        return False

    def arg(self, i):   # i-th stdcall stack arg while stopped on a thunk (return address at [esp])
        esp = self.uc.reg_read(UC_X86_REG_ESP); return struct.unpack("<I", self.uc.mem_read(esp + 4 + 4 * i, 4))[0]

    def on_thunk(self, uc, addr, size, _):
        name = self.thunks.get((addr & ~0xf))
        stub = API_STUBS.get(name.split("!")[-1]) if name else None
        if stub is None:
            self.fault = f"call to unstubbed Win32 API {name or hex(addr)} (add it to API_STUBS)"; uc.emu_stop(); return
        nargs, fn = stub
        eax = fn(self) & 0xffffffff; esp = uc.reg_read(UC_X86_REG_ESP)
        ret = struct.unpack("<I", uc.mem_read(esp, 4))[0]
        uc.reg_write(UC_X86_REG_EAX, eax); uc.reg_write(UC_X86_REG_ESP, esp + 4 + 4 * nargs); uc.reg_write(UC_X86_REG_EIP, ret)

    # -- one call ----------------------------------------------------------------------------------------------
    def call(self, entry, regs, stack_args, float_ret, icount):
        uc = self.uc; self.fault = None
        for r in REGS.values(): uc.reg_write(r, POISON)
        for k, v in regs.items(): uc.reg_write(REGS[k], v & 0xffffffff)
        uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
        uc.reg_write(UC_X86_REG_FPCW, self.fpcw); uc.reg_write(UC_X86_REG_FPSW, 0); uc.reg_write(UC_X86_REG_FPTAG, 0xffff)
        sp = STACK_TOP - 0x1000
        blob = b"".join(stack_args)
        sp -= len(blob); sp &= ~3
        uc.mem_write(sp, blob); sp -= 4
        stop = RET_STUB + (0 if float_ret else 0x10)
        uc.mem_write(sp, struct.pack("<I", RET_STUB if float_ret else RET_STUB + 0x10))
        uc.reg_write(UC_X86_REG_ESP, sp)
        halt_at = stop + (7 if float_ret else 0)
        try: uc.emu_start(entry, halt_at, count=icount)
        except UcError as e:
            if not self.fault: self.fault = f"emulator error: {e} (eip={uc.reg_read(UC_X86_REG_EIP):#010x})"
        if self.fault: raise EmuFault(self.fault)
        if uc.reg_read(UC_X86_REG_EIP) != halt_at:
            raise EmuFault(f"instruction limit ({icount}) hit, eip={uc.reg_read(UC_X86_REG_EIP):#010x} (loop?)")
        eax = uc.reg_read(UC_X86_REG_EAX)
        st0 = f80_to_float(bytes(uc.mem_read(FP_OUT, 10))) if float_ret else None
        return eax, st0

def f80_to_float(b):
    m = struct.unpack("<Q", b[:8])[0]; se = struct.unpack("<H", b[8:10])[0]
    sign = -1.0 if se & 0x8000 else 1.0; e = se & 0x7fff
    if e == 0x7fff: return float("nan") if (m << 1) & ((1 << 64) - 1) else sign * float("inf")
    if e == 0 and m == 0: return sign * 0.0
    import math
    try: return sign * math.ldexp(m, e - 16383 - 63)
    except OverflowError: return sign * float("inf")

# ---- source parsing ----------------------------------------------------------------------------------------------
INT_SIZES = {"int8_t": (1, 1), "uint8_t": (1, 0), "char": (1, 1), "bool": (1, 0), "byte": (1, 0), "int16_t": (2, 1), "uint16_t": (2, 0),
             "short": (2, 1), "word": (2, 0), "int32_t": (4, 1), "uint32_t": (4, 0), "int": (4, 1), "long": (4, 1), "dword": (4, 0)}

def parse_type(t):
    t = t.strip()
    if "*" in t: return ("ptr", 4, 0)
    words = [w for w in re.split(r"\s+", t) if w not in ("const", "volatile", "signed", "struct", "enum", "__cdecl", "static", "inline")]
    uns = "unsigned" in words; words = [w for w in words if w != "unsigned"]
    w = words[-1] if words else "int"
    if w == "void": return ("void", 0, 0)
    if w in ("float", "real"): return ("float", 4, 1)
    if w == "double": return ("double", 8, 1)
    if w in INT_SIZES: sz, sg = INT_SIZES[w]; return ("int", sz, 0 if uns else sg)
    return ("int", 4, 0)     # unknown typedef / enum: assume a 32-bit unsigned

def find_source(name):
    hits = glob.glob(os.path.join(ROOT, "src", "*", name + ".c"))
    if not hits: raise SystemExit(f"no src/*/{name}.c")
    return hits[0]

def parse_source(text, name):
    addr = int(re.search(r"^//\s*address\s+(0x[0-9a-fA-F]+)", text, re.M).group(1), 16)
    m = re.search(r"^([^\n;{}#/]*?)\b" + re.escape(name) + r"\s*\(([^)]*)\)[ \t]*(?://[^\n]*)?\s*\{", text, re.M)
    if not m: raise SystemExit(f"cannot find the definition of {name}()")
    ret = parse_type(m.group(1))
    params = []
    plist = m.group(2).strip()
    if plist and plist != "void":
        for p in plist.split(","):
            p = p.strip(); pm = re.match(r"(.*?)(\w+)\s*(\[[^\]]*\])?$", p)
            if not pm: raise SystemExit(f"cannot parse parameter '{p}'")
            ty = "ptr" if pm.group(3) else None
            k = parse_type(pm.group(1) + (" *" if ty else ""))
            params.append((pm.group(2), k))
    regmap = {}
    for line in re.findall(r"//\s*blam-cc:\s*(.*)", text):
        for item in line.split(","):
            im = re.match(r"\s*(EAX|ECX|EDX|EBX|ESI|EDI|EBP|stack)\b[^>]*->\s*(\w+)", item.strip())
            if im and im.group(1) != "stack": regmap[im.group(2)] = im.group(1)
    names = [p[0] for p in params]
    for n, r in regmap.items():
        if n not in names: raise SystemExit(f"blam-cc maps {r} -> {n}, but {n} is not a parameter of {name}()")
    return addr, ret, params, regmap

# ---- build the rewrite -------------------------------------------------------------------------------------------
def build(name, src_text, deps, workdir):
    cflags = ["-target", "i686-w64-windows-gnu", "-O2", "-mno-sse", "-mno-mmx", "-fno-builtin", "-fno-stack-protector",
              "-w", "-I", os.path.join(ROOT, "types"), "-I", os.path.join(ROOT, "src")]
    objs = []
    def cc(src, out):
        r = subprocess.run(["clang", *cflags, "-c", src, "-o", out], capture_output=True, text=True)
        if r.returncode: raise SystemExit("clang failed:\n" + r.stderr[-3000:])
        objs.append(out)
    sp = os.path.join(workdir, name + ".c"); open(sp, "w").write(src_text)
    # keep relative includes of the original directory working
    cc(sp, os.path.join(workdir, name + ".o"))
    shim = os.path.join(workdir, "shim.c"); open(shim, "w").write(SHIM_C); cc(shim, os.path.join(workdir, "shim.o"))
    for i, d in enumerate(deps): cc(d, os.path.join(workdir, f"dep{i}.o"))
    exe = os.path.join(workdir, name + ".exe")
    r = subprocess.run(["lld-link", "/machine:x86", "/subsystem:console", "/entry:" + name, "/nodefaultlib", "/fixed",
                        f"/base:{REWRITE_BASE:#x}", "/force:unresolved", "/opt:noref", "/out:" + exe, *objs],
                       capture_output=True, text=True)
    if not os.path.exists(exe): raise SystemExit("lld-link failed:\n" + r.stdout[-2000:] + r.stderr[-2000:])
    unresolved = sorted(set(re.findall(r"undefined symbol: (\S+)", r.stdout + r.stderr)))
    return exe, unresolved

# ---- comparison --------------------------------------------------------------------------------------------------
def fclose(a, b, exact):
    if a == b: return True
    if exact: return False
    fa, fb = struct.unpack("<ff", struct.pack("<II", a, b))
    if fa != fa and fb != fb: return True
    if fa in (float("inf"), float("-inf")) or fb in (float("inf"), float("-inf")) or fa != fa or fb != fb: return False
    return abs(fa - fb) <= 1e-4 * max(1.0, abs(fa), abs(fb))

def first_diff(a, b, exact):
    n = min(len(a), len(b)) & ~3
    if a[:n] == b[:n]: return None
    wa = struct.unpack(f"<{n // 4}I", a[:n]); wb = struct.unpack(f"<{n // 4}I", b[:n])
    for i, (x, y) in enumerate(zip(wa, wb)):
        if x != y and not fclose(x, y, exact): return i * 4, x, y
    return None

def fmt(w):
    return f"{w:#010x} ({struct.unpack('<f', struct.pack('<I', w))[0]:.6g})"

def norm_int(v, k):
    _, sz, sg = k
    if sz == 4: return v & 0xffffffff
    v &= (1 << (8 * sz)) - 1
    if sg and v >> (8 * sz - 1): v -= 1 << (8 * sz)
    return v & 0xffffffff        # sign-extended into 32 bits (regs and pushed dwords alike)

def run_function(name, args, halo_orig, halo_new):
    src_path = args.src or find_source(name)
    text = open(src_path).read()
    if args.mutate:
        old, new = args.mutate.split("=>", 1)
        if old not in text: raise SystemExit(f"--mutate: '{old}' not found in {src_path}")
        text = text.replace(old, new, 1)
    addr, ret, params, regmap = parse_source(text, name)
    tmp = tempfile.mkdtemp(prefix="emu_")
    exe, unresolved = build(name, text, [os.path.abspath(d) for d in args.dep], tmp)
    orig = Emu(HALO, args.fpcw); orig.fpcw = args.fpcw
    new = Emu(HALO, args.fpcw); new.fpcw = args.fpcw
    pe = new.load_pe(exe, REWRITE_BASE)
    rew_entry = REWRITE_BASE + pe.OPTIONAL_HEADER.AddressOfEntryPoint
    ptrs = [p[0] for p in params if p[1][0] == "ptr"]
    alias = {}
    for a in args.alias:
        x, y = a.split("=");
        for n in (x, y):
            if n not in ptrs: raise SystemExit(f"--alias: {n} is not a pointer parameter of {name} ({ptrs})")
        alias[x] = y
    lo, hi = [int(x, 0) for x in args.int_range.split(",")]
    float_ret = ret[0] in ("float", "double"); int_ret = ret[0] in ("int", "ptr")
    rng = random.Random(args.seed ^ hash(name) & 0xffff)
    for e in (orig, new):
        e.snap = [(a, bytes(e.uc.mem_read(a, n))) for a, n in e.writable]
    print(f"{name} @ {addr:#x}  ret={ret[0]}  params=" + ", ".join(
        f"{n}:{k[0]}" + (f"[{regmap[n]}]" if n in regmap else "[stack]") for n, k in params) + (f"  alias={alias}" if alias else "") +
        (f"\n  note: rewrite has unresolved symbols {unresolved} (a call to one is reported as a fault)" if unresolved else ""))
    stats = dict(ok=0, differ=0, both_fault=0, first=None)
    for s in range(args.n):
        vals = {}; bufs = {}
        for n, k in params:
            if k[0] == "ptr":
                tgt = alias.get(n, n)
                if tgt not in bufs:
                    base = BUF_BASE + BUF_STRIDE * len(bufs) + GUARD
                    words = [struct.pack("<f", (rng.randrange(20001) / 10000.0 - 1.0) * 4.0) for _ in range((BUF_BYTES + 2 * GUARD) // 4)]
                    bufs[tgt] = (base, b"".join(words))
                vals[n] = bufs[tgt][0]
            elif k[0] == "float": vals[n] = struct.unpack("<I", struct.pack("<f", (rng.randrange(20001) / 10000.0 - 1.0) * 4.0))[0]
            elif k[0] == "double": vals[n] = struct.pack("<d", (rng.randrange(20001) / 10000.0 - 1.0) * 4.0)
            elif k[0] == "int": vals[n] = norm_int(rng.randint(lo, hi), k)
            else: raise SystemExit(f"unsupported parameter kind {k}")
        res = []
        for side, (emu, entry) in enumerate(((orig, addr), (new, rew_entry))):
            uc = emu.uc
            for a, b in emu.snap: uc.mem_write(a, b)
            for tgt, (base, data) in bufs.items():
                try: uc.mem_map(base - GUARD & ~0xfff, (BUF_BYTES + 2 * GUARD + 0xfff + GUARD) & ~0xfff)
                except UcError: pass
                uc.mem_write(base - GUARD, data)
            regs = {}; stack = []
            if side == 0:
                for n, k in params:
                    if n in regmap: regs[regmap[n]] = vals[n]
                sp = [(n, k) for n, k in params if n not in regmap]
            else: sp = params
            for n, k in sp: stack.append(vals[n] if isinstance(vals[n], bytes) else struct.pack("<I", vals[n]))
            try: eax, st0 = emu.call(entry, regs, stack, float_ret, args.icount)
            except EmuFault as f: res.append(("fault", str(f))); continue
            mem = {t: bytes(uc.mem_read(b - GUARD, BUF_BYTES + 2 * GUARD)) for t, (b, _) in bufs.items()}
            glob_ = [bytes(uc.mem_read(a, n)) for a, n in emu.writable]
            res.append(("ok", eax, st0, mem, glob_))
        msg = None
        if res[0][0] == "fault" and res[1][0] == "fault": stats["both_fault"] += 1; continue
        if res[0][0] == "fault": msg = f"original faulted, rewrite did not: {res[0][1]}"
        elif res[1][0] == "fault": msg = f"rewrite faulted, original did not: {res[1][1]}"
        else:
            (_, ea, fa, ma, ga), (_, eb, fb, mb, gb) = res
            if int_ret and ea != eb and not (ret[1] < 4 and (ea ^ eb) & ((1 << 8 * ret[1]) - 1) == 0):
                msg = f"return eax {ea:#010x} vs {eb:#010x}"
            elif float_ret and not (fa == fb or (fa != fa and fb != fb) or (not args.exact and abs(fa - fb) <= 1e-4 * max(1.0, abs(fa), abs(fb)))):
                msg = f"return st0 {fa!r} vs {fb!r}"
            else:
                for t in ma:
                    d = first_diff(ma[t], mb[t], args.exact)
                    if d: msg = f"pointer arg '{t}' {'guard' if not (GUARD <= d[0] < GUARD + BUF_BYTES) else '+'+hex(d[0]-GUARD)}" \
                                f"{'' if GUARD <= d[0] < GUARD + BUF_BYTES else '@'+hex(d[0]-GUARD)}: {fmt(d[1])} vs {fmt(d[2])}"; break
                if not msg:
                    for (a, n), x, y in zip(orig.writable, ga, gb):
                        d = first_diff(x, y, args.exact)
                        if d: msg = f"global {a + d[0]:#010x}: {fmt(d[1])} vs {fmt(d[2])}"; break
        if msg:
            stats["differ"] += 1
            if stats["first"] is None:
                stats["first"] = (s, msg, {n: (f"{v:#x}" if not isinstance(v, bytes) else struct.unpack("<d", v)[0]) for n, v in vals.items()})
        else: stats["ok"] += 1
    tested = stats["ok"] + stats["differ"]
    status = "MATCH" if stats["differ"] == 0 and tested >= 20 else ("DIFFER" if stats["differ"] else "UNTESTED")
    print(f"{status} {name}: {stats['differ']}/{tested} differ, {stats['both_fault']} both-fault of {args.n}")
    if stats["first"]: print(f"  first difference (sample {stats['first'][0]}): {stats['first'][1]}\n  inputs: {stats['first'][2]}")
    if status == "UNTESTED": print("  (fewer than 20 usable samples: original needs live game state / faults on random input)")
    return status

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("functions", nargs="+"); ap.add_argument("-n", type=int, default=200); ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--alias", action="append", default=[]); ap.add_argument("--src"); ap.add_argument("--mutate")
    ap.add_argument("--dep", action="append", default=[]); ap.add_argument("--fpcw", type=lambda x: int(x, 0), default=0x27f)
    ap.add_argument("--exact", action="store_true"); ap.add_argument("--int-range", default="0,200")
    ap.add_argument("--icount", type=int, default=2000000); ap.add_argument("-v", action="store_true")
    a = ap.parse_args()
    if not os.path.exists(HALO): raise SystemExit(f"{HALO} missing (gitignored; copy the retail halo.exe there)")
    bad = 0
    for f in a.functions:
        bad += run_function(f, a, None, None) == "DIFFER"
    return 1 if bad else 0

if __name__ == "__main__": sys.exit(main())
