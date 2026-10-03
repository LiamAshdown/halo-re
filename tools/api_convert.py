"""Step B mechaniser: turn a module's extern "C" shims into halo::<module> API functions and rewrite the callers.

  python tools/api_convert.py <module> [--dry]
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
mod = sys.argv[1]
dry = "--dry" in sys.argv
os.chdir(ROOT)

shim_files = sorted(glob.glob(f"src/{mod}/*_c_api.cpp"))
assert len(shim_files) == 1, shim_files
shim_path = shim_files[0]
text = open(shim_path, encoding="utf-8", errors="replace").read()

FUNC = re.compile(r'^(?:extern "C"\s+)?((?:[A-Za-z_][\w:]*[ \t\*]+)+?[\*]*)(\w+)\(([^()]*(?:\([^()]*\)[^()]*)*)\)\s*\n?\{', re.M)
funcs = []
pos = 0
for m in FUNC.finditer(text):
    if m.start() < pos:
        continue
    ret = m.group(1).strip()
    if ret in ("return", "else", "if", "while", "for", "switch"):
        continue
    depth = 0
    i = m.end() - 1
    while True:
        c = text[i]
        if c == "{": depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0: break
        i += 1
    funcs.append(dict(name=m.group(2), ret=ret, params=" ".join(m.group(3).split()), start=m.start(), end=i + 1))
    pos = i + 1
names = [f["name"] for f in funcs]
print(f"{mod}: {len(funcs)} shim functions in {shim_path}")

# which files reference them
def src_files():
    for pat in ("src/**/*.cpp", "src/**/*.hpp", "src/**/*.h", "include/**/*.hpp", "include/**/*.h", "harness/*.cpp", "harness/*.hpp", "standalone/*.cpp", "standalone/*.hpp"):
        for f in glob.glob(pat, recursive=True):
            yield f.replace("\\", "/")

name_re = re.compile(r"\b(" + "|".join(map(re.escape, names)) + r")\b")
used = set()
module_dir = f"src/{mod}/"
api_inc = f'#include "halo/{mod}/api.hpp"'

TOK = re.compile(r'"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'|//[^\n]*|/\*.*?\*/', re.S)


def code_sub(text, fn):
    out = []
    last = 0
    for m in TOK.finditer(text):
        out.append(fn(text[last:m.start()]))
        out.append(m.group(0))
        last = m.end()
    out.append(fn(text[last:]))
    return "".join(out)


_types_text = None


def types_index():
    global _types_text
    if _types_text is None:
        _types_text = {}
        for f in glob.glob("types/*.h"):
            _types_text[f] = open(f, encoding="utf-8", errors="replace").read()
    return _types_text


KEYWORDS = set("void char short int long float double unsigned signed const struct union enum return uint8_t uint16_t uint32_t uint64_t int8_t int16_t int32_t int64_t size_t bool".split())


def type_decl(name):
    """Returns (kind, text, deps_text) for a type name used by the API, searched in types/*.h: ('fwd', 'struct X;', '') for tagged
    structs, ('typedef', text, text) for plain/function-pointer typedefs, ('enum', header, '') for enums, None when unknown."""
    for f, t in types_index().items():
        m = re.search(r"typedef[^;{}]*?\(\s*\*\s*" + re.escape(name) + r"\s*\)[^;{}]*;", t)
        if m:
            return ("typedef", " ".join(m.group(0).split()), m.group(0))
        m = re.search(r"^typedef\s+(?!struct|union|enum)([^;{}()]+?)\s+" + re.escape(name) + r"\s*(\[[^\]]*\])?;", t, re.M)
        if m:
            return ("typedef", " ".join(m.group(0).split()), m.group(0))
        m = re.search(r"^typedef\s+(struct|union)\s+(\w+)\s+" + re.escape(name) + r"\s*;", t, re.M)
        if m:
            return ("fwd", f"{m.group(1)} {m.group(2)};\ntypedef {m.group(1)} {m.group(2)} {name};", "")
        m = re.search(r"^typedef\s+(struct|union)\s*(\w*)\s*\{[^{}\n]*\}\s*" + re.escape(name) + r"\s*;", t, re.M)
        if m:
            tag = m.group(2)
            if not tag:
                return ("anon", os.path.basename(f), "")
            txt = f"{m.group(1)} {tag};"
            if tag != name:
                txt += f"\ntypedef {m.group(1)} {tag} {name};"
            return ("fwd", txt, "")
        m = re.search(r"^\}\s*" + re.escape(name) + r"\s*;", t, re.M)
        if m:
            head = t[:m.start()]
            k = max(head.rfind("\ntypedef struct"), head.rfind("\ntypedef union"), head.rfind("\ntypedef enum"))
            if k < 0:
                continue
            h = re.match(r"\ntypedef\s+(struct|union|enum)\s*(\w*)", head[k:])
            kind, tag = h.group(1), h.group(2)
            if kind == "enum":
                return ("enum", os.path.basename(f), "")
            if not tag:
                return ("anon", os.path.basename(f), "")
            txt = f"{kind} {tag};"
            if tag != name:
                txt += f"\ntypedef {kind} {tag} {name};"
            return ("fwd", txt, "")
    return None


def forward_block(decl_text):
    need = []
    seen = set()
    todo = [w for w in re.findall(r"[A-Za-z_]\w*", decl_text) if w not in KEYWORDS]
    out_typedefs, out_fwd, includes = [], [], []
    while todo:
        w = todo.pop()
        if w in seen:
            continue
        seen.add(w)
        d = type_decl(w)
        if d is None:
            continue
        kind, text, deps = d
        if kind == "fwd":
            out_fwd.append(text)
        elif kind == "typedef":
            out_typedefs.append((w, text))
            todo += [x for x in re.findall(r"[A-Za-z_]\w*", deps) if x not in KEYWORDS]
        else:
            includes.append(f'#include "{text}"')
    # order typedefs so that dependencies come first: simple repeat-until-stable placement
    ordered = []
    names = {n for n, _ in out_typedefs}
    pending = list(out_typedefs)
    placed = set()
    guard = 0
    while pending and guard < 1000:
        guard += 1
        for item in list(pending):
            n, t = item
            deps = {x for x in re.findall(r"[A-Za-z_]\w*", t) if x in names and x != n}
            if deps <= placed:
                ordered.append(t)
                placed.add(n)
                pending.remove(item)
        else:
            if pending and not any(True for _ in []):
                pass
    ordered += [t for _, t in pending]
    return sorted(set(includes)), sorted(set(out_fwd)), ordered


DECL = lambda n: re.compile(r'^[ \t]*extern\s+(?:"C"\s+)?[^;{}()]*?\b' + re.escape(n) + r'\s*\((?:[^;{}()]|\([^()]*\))*\)\s*;[ \t]*\r?\n', re.M)
STR = re.compile(r'("(?:[^"\\\n]|\\.)*"|' + r"'(?:[^'\\\n]|\\.)*')")
BARE = re.compile(r"(?<![\w:.>])(" + "|".join(map(re.escape, names)) + r")\b(?!\s*\[)")
CALL = lambda n: re.compile(r'(?<![\w:.>"\'&])' + re.escape(n) + r'(?=\s*\()')

changed = {}
for f in src_files():
    if f.startswith(module_dir) or f.startswith(f"include/halo/{mod}/"):
        continue
    s = open(f, encoding="utf-8", errors="replace").read()
    if not name_re.search(s):
        continue
    orig = s
    hit = set(name_re.findall(s))
    for n in hit:
        s = DECL(n).sub("", s)
    s = code_sub(s, lambda t: BARE.sub(lambda m: f"halo::{mod}::{m.group(1)}", t))
    still = set(name_re.findall(re.sub(r'"(?:[^"\\\n]|\\.)*"', '""', re.sub(r"//[^\n]*|/\*.*?\*/", "", s, flags=re.S)))) - set()
    # qualified uses count as used
    qualified = set(re.findall(r"halo::" + mod + r"::(\w+)", s)) & set(names)
    if qualified and api_inc not in s:
        inc = list(re.finditer(r'^#include[^\n]*\n', s, re.M))
        if inc:
            p = inc[-1].end()
            s = s[:p] + api_inc + "\n" + s[p:]
        else:
            s = api_inc + "\n" + s
    used |= qualified
    leftovers = still - qualified
    if s != orig:
        changed[f] = s
    if leftovers:
        print(f"  note: {f} still mentions {sorted(leftovers)}")

# table references
tab_changed = {}
tab_used = set()
for f in glob.glob("standalone/data/*.cpp"):
    s = open(f, encoding="utf-8", errors="replace").read()
    if not name_re.search(s):
        continue
    def sub(m):
        tab_used.add(m.group(2))
        return m.group(1) + "&halo::" + mod + "::" + m.group(2)
    TB = re.compile(r"(?<![\w:.>&])(" + "|".join(map(re.escape, names)) + r")\b(?!\s*[\[(])")
    def tsub(m):
        tab_used.add(m.group(1))
        return "&halo::" + mod + "::" + m.group(1)
    s2 = "\n".join(l if l.lstrip().startswith("extern") else code_sub(l, lambda t: TB.sub(tsub, t)) for l in s.split("\n"))
    if s2 != s:
        tab_changed[f] = s2
    s2s = re.sub(r'"(?:[^"\\\n]|\\.)*"', '""', s2)
    rest = [m for m in name_re.finditer(s2s) if s2s[max(0, m.start() - 2):m.start()] != "::"]
    if rest:
        print(f"  table {f}: unhandled {sorted(set(m.group(1) for m in rest))}")
used |= tab_used
print(f"externally used: {len(used)} of {len(names)}; unused: {sorted(set(names) - used)}")

# api.hpp + api.cpp
keep = [f for f in funcs if f["name"] in used]
enum_names = set()
for f in keep:
    for w in re.findall(r"[A-Za-z_]\w*", f["ret"] + " " + f["params"]):
        d = type_decl(w) if w not in KEYWORDS else None
        if d and d[0] == "enum":
            enum_names.add(w)
for f in keep:
    f["sig_old"] = (f["ret"], f["params"])
    for e in enum_names:
        f["ret"] = re.sub(r"\b" + e + r"\b", "int32_t", f["ret"])
        f["params"] = re.sub(r"\b" + e + r"\b", "int32_t", f["params"])
decls = "\n".join(f"{f['ret']} {f['name']}({f['params']});" for f in keep)
decl_text = "\n".join(f"{f['ret']} {f['name']}({f['params']});" for f in keep)
incs, fwds, tds = forward_block(decl_text)
fb = "\n".join(incs + fwds + tds)
hpp = f'''/**
 * @file include/halo/{mod}/api.hpp
 * Functions of the {mod} module that other modules and the data tables call (namespace halo::{mod}). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>

{fb}

namespace halo::{mod} {{

{decls}

}}
'''
new_body = text
for f in reversed(funcs):
    if f["name"] not in used:
        new_body = new_body[:f["start"]] + new_body[f["end"]:]
    elif enum_names:
        bs = new_body.index("{", f["start"])
        head = new_body[f["start"]:bs]
        for e in enum_names:
            head = re.sub(r"\b" + e + r"\b", "int32_t", head)
        new_body = new_body[:f["start"]] + head + new_body[bs:]
if re.search(r'^extern "C" \{', new_body, re.M):
    new_body = re.sub(r'^extern "C" \{', f"namespace halo::{mod} {{", new_body, count=1, flags=re.M)
else:
    new_body = re.sub(r'^extern "C" ', "", new_body, flags=re.M)
    inc = list(re.finditer(r'^#include[^\n]*\n', new_body, re.M))
    p_ = inc[-1].end()
    new_body = new_body[:p_] + "\n" + f"namespace halo::{mod} {{\n" + new_body[p_:].rstrip() + "\n\n}\n"
inc = list(re.finditer(r'^#include[^\n]*\n', new_body, re.M))
new_body = new_body[:inc[-1].end()] + api_inc + "\n" + new_body[inc[-1].end():]
if dry:
    print("dry run, nothing written"); sys.exit(0)

for f, s in changed.items():
    open(f, "w", encoding="utf-8", newline="").write(s)
for f, s in tab_changed.items():
    open(f, "w", encoding="utf-8", newline="").write(s)
os.makedirs(f"include/halo/{mod}", exist_ok=True)
open(f"include/halo/{mod}/api.hpp", "w", encoding="utf-8", newline="\n").write(hpp)
open(shim_path, "w", encoding="utf-8", newline="").write(new_body)
print("wrote api.hpp, edited", len(changed), "files,", len(tab_changed), "tables")
