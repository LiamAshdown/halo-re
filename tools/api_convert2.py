"""Step B mechaniser for modules whose C entry points are `extern "C" { ... }` definition blocks inside the module's own files.

  python tools/api_convert2.py <module>
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
mod = sys.argv[1]
os.chdir(ROOT)
src_text = open("tools/api_convert.py").read()
exec(src_text[src_text.index("TOK = re.compile"):src_text.index("DECL = lambda")])

FUNC = re.compile(r'^((?:[A-Za-z_][\w:]*[ \t\*]+)+?[\*]*)(\w+)\(([^()]*(?:\([^()]*\)[^()]*)*)\)\s*\n?\{', re.M)
OPENER = re.compile(r'^extern "C" \{[ \t]*\r?\n', re.M)


def match_brace(text, i):
    depth = 0
    while True:
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1


units = {}
all_funcs = {}
for path in sorted(glob.glob(f"src/{mod}/*.cpp")):
    text = open(path, encoding="utf-8", errors="replace", newline="").read()
    defs = []
    openers = []
    for om in OPENER.finditer(text):
        ob = om.end() - 1
        while text[ob] != "{":
            ob -= 1
        cb = match_brace(text, ob)
        inner = text[ob + 1:cb]
        pos = 0
        found = []
        for m in FUNC.finditer(inner):
            if m.start() < pos:
                continue
            ret = m.group(1).strip()
            if ret in ("return", "else", "if", "while", "for", "switch", "static", "typedef"):
                continue
            bo = ob + 1 + m.end() - 1
            end = match_brace(text, bo) + 1
            found.append(dict(name=m.group(2), ret=ret, params=" ".join(m.group(3).split()), start=ob + 1 + m.start(), body=bo, end=end, path=path))
            pos = end - (ob + 1)
        if found:
            openers.append(om.start())
            defs += found
    if defs:
        units[path] = dict(text=text, defs=defs, openers=openers)
        for d in defs:
            all_funcs[d["name"]] = d
names = sorted(all_funcs)
print(f"{mod}: {len(names)} extern C definitions in {len(units)} files")

name_re = re.compile(r"\b(" + "|".join(map(re.escape, names)) + r")\b")
BARE = re.compile(r"(?<![\w:.>])(" + "|".join(map(re.escape, names)) + r")\b(?!\s*\[)")
DECL1 = lambda n: re.compile(r'^[ \t]*extern\s+(?:"C"\s+)?[^;{}()]*?\b' + re.escape(n) + r'\s*\((?:[^;{}()]|\([^()]*\))*\)\s*;[ \t]*\r?\n', re.M)
DECL2 = lambda n: re.compile(r'^[ \t]*extern\s+"C"\s*\{\s*extern\s+[^;{}()]*?\b' + re.escape(n) + r'\s*\((?:[^;{}()]|\([^()]*\))*\)\s*;\s*\}[ \t]*\r?\n', re.M)
api_inc = f'#include "halo/{mod}/api.hpp"'


def src_files():
    for pat in ("src/**/*.cpp", "src/**/*.hpp", "src/**/*.h", "include/**/*.hpp", "include/**/*.h", "harness/*.cpp", "harness/*.hpp", "standalone/*.cpp", "standalone/*.hpp"):
        for f in glob.glob(pat, recursive=True):
            yield f.replace("\\", "/")


used = set()
results = {}
for f in src_files():
    if f.startswith(f"include/halo/{mod}/"):
        continue
    key = next((u for u in units if u.replace("\\", "/") == f), None)
    s = open(f, encoding="utf-8", errors="replace", newline="").read()
    if key is None and not name_re.search(s):
        continue
    tokens = {}
    if key is not None:
        u = units[key]
        edits = [(d["start"], d["end"], ("D", d)) for d in u["defs"]]
        edits += [(o, o + len('extern "C" {') + (2 if s[o:].startswith('extern "C" {\r\n') else 1), ("O", None)) for o in u["openers"]]
        edits.sort(reverse=True)
        for i, (a, b, payload) in enumerate(edits):
            tok = f"\x01T{i}\x01"
            tokens[tok] = (s[a:b], payload)
            s = s[:a] + tok + s[b:]
    orig_marker = s
    for n in set(name_re.findall(s)):
        s = DECL1(n).sub("", s)
        s = DECL2(n).sub("", s)
    s = code_sub(s, lambda t: BARE.sub(lambda m: f"halo::{mod}::{m.group(1)}", t))
    for q in re.findall(r"halo::" + mod + r"::(\w+)", s):
        if q in all_funcs:
            used.add(q)
    results[f] = (s, tokens, key)

# tables
tab_changed = {}
tab_used = set()
TB = re.compile(r"(?<![\w:.>&])(" + "|".join(map(re.escape, names)) + r")\b(?!\s*[\[(])")
def tsub(m):
    tab_used.add(m.group(1))
    return "&halo::" + mod + "::" + m.group(1)
for f in glob.glob("standalone/data/*.cpp"):
    s = open(f, encoding="utf-8", errors="replace", newline="").read()
    if not name_re.search(s):
        continue
    s2 = "\n".join(l if l.lstrip().startswith("extern") else code_sub(l, lambda t: TB.sub(tsub, t)) for l in s.split("\n"))
    if s2 != s:
        tab_changed[f] = s2
used |= tab_used
print(f"used: {len(used)} of {len(names)}; unused: {sorted(set(names) - used)}")

keep = [all_funcs[n] for n in names if n in used]
enum_names = set()
for f in keep:
    for w in re.findall(r"[A-Za-z_]\w*", f["ret"] + " " + f["params"]):
        d = type_decl(w) if w not in KEYWORDS else None
        if d and d[0] == "enum" and re.search(r"\b" + w + r"\b(?=\s+\w|\s*\*)", f["ret"] + " " + f["params"] + " x"):
            enum_names.add(w)


def fix_enums(t):
    for e in enum_names:
        t = re.sub(r"\b" + e + r"\b(?=\s+\w|\s*\*)", "int32_t", t)
    return t


decls = "\n".join(f"{fix_enums(f['ret'])} {f['name']}({fix_enums(f['params'])});" for f in keep)
incs, fwds, tds = forward_block(decls)
if incs:
    print("  note: enum-typed names kept out of api.hpp:", incs)
incs = []
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

for f, (s, tokens, key) in results.items():
    for tok, (orig, payload) in tokens.items():
        kind, d = payload
        if kind == "O":
            rep = f"namespace halo::{mod} {{\n"
        elif d["name"] in used:
            body_off = d["body"] - d["start"]
            rep = fix_enums(orig[:body_off]) + orig[body_off:] if enum_names else orig
        else:
            rep = ""
        s = s.replace(tok, rep)
    if tokens and api_inc not in s:
        incs_ = list(re.finditer(r'^#include[^\n]*\n', s, re.M))
        p = incs_[-1].end() if incs_ else 0
        s = s[:p] + api_inc + "\n" + s[p:]
    elif not tokens and api_inc not in s and re.search(r"halo::" + mod + r"::\w+\(", s):
        incs_ = list(re.finditer(r'^#include[^\n]*\n', s, re.M))
        p = incs_[-1].end() if incs_ else 0
        s = s[:p] + api_inc + "\n" + s[p:]
    # collapse blank lines left by removed definitions
    s = re.sub(r"\n{4,}", "\n\n\n", s)
    open(f, "w", encoding="utf-8", newline="").write(s)
for f, s in tab_changed.items():
    open(f, "w", encoding="utf-8", newline="").write(s)
os.makedirs(f"include/halo/{mod}", exist_ok=True)
open(f"include/halo/{mod}/api.hpp", "w", encoding="utf-8", newline="\n").write(hpp)
print("wrote api.hpp; edited", len(results), "files,", len(tab_changed), "tables")
