"""Write C for the unlisted data-pool callbacks: functions Ghidra never created (reachable only through callback
tables in halo.exe's data, see tools/gen_standalone.py's "unlisted" traps) whose whole body is one of three
compiler idioms. A function is written only when its bytes match an idiom exactly, instruction for instruction:

  initialize   push ebx; push MAX; push NAME; mov ebx,SIZE; call game_state_new 0x5380d0; add esp,8;
               mov ds:GLOBAL,eax; pop ebx; ret
                   -> GLOBAL = game_state_new(NAME, MAX, SIZE);
  dispose      push esi; mov esi,ds:GLOBAL; mov byte [esi+0x24],1; call data_delete_all 0x4d0580; pop esi;
               ret  |  jmp TARGET (a tail call, emitted only when TARGET has C)
                   -> GLOBAL->valid = 1; data_delete_all(GLOBAL); [TARGET();]
  clear flag   mov eax,ds:GLOBAL; mov byte [eax+0x24],0; ret
                   -> GLOBAL->valid = 0;
Global names come from the extern declarations already in src (the annotated disassembly's names); a global with
no known name is skipped. Output: src/<module>/<name>.c, named <global>_initialize / _dispose / _clear_disposing_flag
after the pool, module taken from the file that declares the global.
Usage: python tools/gen_pool_callbacks.py [--write]"""
import os, re, sys, glob, json, struct, subprocess, collections

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
from fx_state_tables import Image


def disassemble(a, n=0x60):
    out = subprocess.run(["objdump", "-d", "-M", "intel", "--start-address=0x%x" % a, "--stop-address=0x%x" % (a + n),
                          os.path.join(ROOT, "bin", "halo.exe")], capture_output=True, text=True).stdout
    ins = []
    for line in out.split("\n")[7:]:
        parts = line.split("\t")
        if len(parts) >= 3:
            ins.append((int(parts[0].strip().rstrip(":"), 16), " ".join(parts[2].split())))
    return ins


def known_globals():
    names, modules = {}, {}
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(p, encoding="utf-8", errors="replace").read()
        live = t[:t.rfind("#if 0")] if "#if 0" in t else t
        for m in re.finditer(r"^[ \t]*extern\s+data_array\s*\*\s*(\w+)\s*;\s*//\s*0x0*([0-9a-f]{6})\b", live, re.M):
            a = int(m.group(2), 16)
            names.setdefault(a, m.group(1))
            modules.setdefault(a, os.path.basename(os.path.dirname(p)))
    return names, modules


def rewritten():
    out = {}
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        m = re.search(r"address\s+0x0*([0-9a-f]{6}),", open(p, encoding="utf-8", errors="replace").read(3000))
        if m:
            out[int(m.group(1), 16)] = os.path.splitext(os.path.basename(p))[0]
    return out


def match(a, img):
    ins = disassemble(a)
    text = [i for _, i in ins]
    if len(text) >= 9 and text[0] == "push ebx" and re.fullmatch(r"push 0x[0-9a-f]+", text[1]) and \
            re.fullmatch(r"push 0x[0-9a-f]+", text[2]) and re.fullmatch(r"mov ebx,0x[0-9a-f]+", text[3]) and \
            text[4] == "call 0x5380d0" and text[5] == "add esp,0x8" and \
            re.fullmatch(r"mov ds:0x[0-9a-f]+,eax", text[6]) and text[7] == "pop ebx" and text[8] == "ret":
        return ("initialize", {"max": int(text[1].split()[1], 16), "name": img.cstr(int(text[2].split()[1], 16)),
                               "size": int(text[3].split(",")[1], 16), "global": int(text[6][7:].split(",")[0], 16),
                               "end": ins[8][0] + 1})
    if len(text) >= 6 and text[0] == "push esi" and re.fullmatch(r"mov esi,DWORD PTR ds:0x[0-9a-f]+", text[1]) and \
            text[2] == "mov BYTE PTR [esi+0x24],0x1" and text[3] == "call 0x4d0580" and text[4] == "pop esi":
        g = int(text[1].split("ds:")[1], 16)
        if text[5] == "ret":
            return ("dispose", {"global": g, "tail": None, "end": ins[5][0] + 1})
        m = re.fullmatch(r"jmp 0x([0-9a-f]+)", text[5])
        if m:
            return ("dispose", {"global": g, "tail": int(m.group(1), 16), "end": ins[5][0] + 5})
    if len(text) >= 3 and re.fullmatch(r"mov eax,ds:0x[0-9a-f]+", text[0]) and \
            text[1] == "mov BYTE PTR [eax+0x24],0x0" and text[2] == "ret":
        return ("clear", {"global": int(text[0].split("ds:")[1], 16), "end": ins[2][0] + 1})
    return None


def main():
    img = Image(os.path.join(ROOT, "bin", "halo.exe"))
    pointers = json.load(open(os.path.join(ROOT, "build", "standalone", "code_pointers.json")))
    unlisted = sorted({p["target"] for p in pointers if p["name"].startswith("unlisted_")})
    names, modules = known_globals()
    have = rewritten()
    counts = collections.Counter()
    written = []
    for a in unlisted:
        m = match(a, img)
        if not m:
            counts["no idiom"] += 1
            continue
        kind, f = m
        g = names.get(f["global"])
        if not g:
            counts[kind + ": pool global has no name in src"] += 1
            continue
        stem = g[:-5] if g.endswith("_data") else g
        fname = {"initialize": stem + "_initialize", "dispose": stem + "_dispose",
                 "clear": stem + "_clear_disposing_flag"}[kind]
        if kind == "dispose" and f["tail"] is not None and f["tail"] not in have:
            counts["dispose: tail call without C"] += 1
            continue
        if glob.glob(os.path.join(ROOT, "src", "*", fname + ".c")) and kind == "initialize":
            fname = stem + "_allocate"      # <pool>_initialize is taken (usually the per-map setup)
        if glob.glob(os.path.join(ROOT, "src", "*", fname + ".c")):
            counts[kind + ": name already used"] += 1
            continue
        counts[kind] += 1
        written.append((a, kind, f, g, fname))
    print(len(unlisted), "unlisted entries:", dict(counts))
    if "--write" not in sys.argv:
        for a, kind, f, g, fname in written[:10]:
            print("  0x%x %-10s %s" % (a, kind, fname))
        return
    for a, kind, f, g, fname in written:
        module = modules[f["global"]]
        size = f["end"] - a
        head = ["// %s  (not a Ghidra function; data-pool callback)" % fname,
                "// address 0x%x, size %d bytes" % (a, size),
                "// name confidence: 0.7  rewrite confidence: 0.95",
                "// evidence: stored in a callback table in halo.exe's data and never made a Ghidra function; written by",
                "//   tools/gen_pool_callbacks.py because its bytes (objdump 0x%x..0x%x) match the %s idiom exactly." %
                (a, f["end"], {"initialize": "pool initialize", "dispose": "pool dispose",
                               "clear": "clear-disposing-flag"}[kind]),
                "// blam-cc: none", ""]
        inc = ['#include "tags.h"', '#include "memory.h"', '#include "math.h"', ""]
        ext = ["extern data_array *%s; // 0x%08x" % (g, f["global"])]
        if kind == "initialize":
            ext.append("extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); "
                       "// 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count")
            body = ["void %s(void)" % fname, "{",
                    '    %s = game_state_new("%s", 0x%x, 0x%x);' % (g, f["name"], f["max"], f["size"]), "}"]
        elif kind == "dispose":
            ext.append("extern void data_delete_all(data_array *array); // 0x4d0580, blam-cc: ESI -> array")
            body = ["void %s(void)" % fname, "{", "    %s->valid = 1;" % g, "    data_delete_all(%s);" % g]
            if f["tail"] is not None:
                ext.append("extern void %s(void); // 0x%x (tail call)" % (have[f["tail"]], f["tail"]))
                body.append("    %s();" % have[f["tail"]])
            body.append("}")
        else:
            body = ["void %s(void)" % fname, "{", "    %s->valid = 0;" % g, "}"]
        path = os.path.join(ROOT, "src", module, fname + ".c")
        open(path, "w", encoding="utf-8", newline="\n").write("\n".join(head + inc + ext + [""] + body) + "\n")
        print("wrote", os.path.relpath(path, ROOT))


if __name__ == "__main__":
    main()
