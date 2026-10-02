"""C++ conversion, phases 2+3 (docs/CPP_CONVENTIONS.md): first, MECHANICAL pass that turns a module's one-function-per-file
src/<module>/*.c into grouped C++20 under namespace halo::<module>, plus the extern "C" shim file that keeps every C
symbol. The output is a starting point: it compiles after a short manual review (see the checklist in
docs/CPP_CONVENTIONS.md), and the module's symbol / behaviour gates decide when it is done.

  python tools/cxx_convert_module.py <module> [--from-ref REF] [--dry-run]
      --from-ref REF   read src/<module>/*.c from git (e.g. the commit before the conversion) instead of the tree,
                       so the mechanical pass can be re-run after the .c files are gone

What it does, per src/<module>/<name>.c (one non-static function per file):
  * the `#if 0` original-decompile block goes to docs/original/<module>/<name>.c.txt (verbatim)
  * the header comments (author notes, evidence, UNSURE, blam-cc) stay on top of the function, after a new
    `// original 0x00xxxxxx` line taken from the file's `address 0x...` note
  * local `extern` declarations of functions of the same module are dropped (the namespace function is called);
    every other extern (CRT, x87 shims, other modules, engine globals) is collected, de-duplicated by name, and
    emitted once per output file in an `extern "C"` block
  * file-static helpers move into an anonymous namespace (renamed `<helper>_<function>` when two files of one group
    define different helpers with the same name; identical ones are emitted once)
  * parameters: a `T *p` whose every use in the body is a dereference (`p->x`, `*p`) becomes a reference
    (`const T &p` when the body never writes through it); anything else (indexing, NULL tests, re-seating, pointer
    comparison, passing the pointer on, returning it) stays a pointer. Bodies are rewritten accordingly and calls
    between module functions get their arguments adjusted (`&x` -> `x`, `p` -> `*p`)
  * the function lands in src/<module>/<group>.cpp in namespace halo::<module>, groups in original address order
  * src/<module>/<module>_c_api.cpp gets one `extern "C"` wrapper per function with the ORIGINAL C signature
  * include/halo/<module>/<group>.hpp declares the namespace functions; include/halo/<module>/<module>_c_api.h
    declares the C API (the original prototypes) for C++ callers that want the C symbols
Float expressions are copied character for character; only `->`/`*` on converted parameters change."""
import os, re, sys, glob, collections

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ---- per-module grouping: output file -> function names (anything unlisted goes to DEFAULT_GROUP[module])
GROUPS = {
    "math": {
        "real_vector": """vector2d_normalize vector2d_normalize_with_length vector2d_angle_between vector3d_length
            vector3d_normalize vector3d_normalize_with_length vector3d_cross_product vector3d_cross_product_length
            vector3d_distance vector3d_distance_squared vector3d_magnitude_squared vector3d_major_axis_index
            vector3d_positive_modulo vector3d_project_onto_axis vector3d_project_onto_unit_axis
            vector3d_scalar_triple_product vector3d_build_perpendicular vector3d_angle_between_4cd4f0
            vector3d_angle_between_4cd5e0 vector3d_delta_toward_gravity_biased_clamp_length point3d_add_scaled
            point3d_project_onto_line""",
        "rotation": """vector3d_rotate_about_axis vector3d_rotate_about_axis_perpendicular vector3d_rotate_pair_in_plane
            vector3d_rotate_toward vector3d_rotate_toward_bounded vector3d_rotate_toward_with_acceleration""",
        "matrix": """matrix3x3_from_forward_up matrix3x3_inverse_transform_vector matrix3x3_multiply matrix3x3_transpose
            matrix4x3_extract_forward_up_position matrix4x3_from_axis_angle matrix4x3_from_euler_angles
            matrix4x3_from_forward_up matrix4x3_from_forward_up_position matrix4x3_inverse
            matrix4x3_inverse_transform_normal matrix4x3_inverse_transform_point matrix4x3_inverse_transform_vector
            matrix4x3_multiply matrix4x3_multiply_3dnow matrix4x3_multiply_sse matrix4x3_transform_normal
            matrix4x3_transform_plane matrix4x3_transform_point matrix4x3_transform_vector
            real_matrix4x3_rotation_from_forward real_matrix4x3_rotation_is_orthonormal
            real_matrix4x3_rotation_rebuild_orthonormal euler_angles_to_basis_vectors""",
        "quaternion": """quaternion_from_matrix3x3 quaternion_from_matrix4x3 quaternion_lerp quaternion_multiply
            quaternion_normalize quaternion_rotate_vector quaternion_to_axis_angle matrix4x3_from_quaternion""",
        "geometry": """plane2d_from_points plane3d_from_point_and_normal plane3d_intersect_pair_to_line
            plane3d_intersect_three plane3d_negate point3d_distance_squared_to_segment point3d_within_horizontal_cone
            point3d_within_radius path_find_closest_point_on_segment ray2d_intersect_circle_distance
            ray_intersect_sphere_distance ray_intersects_cylinder ray_intersects_sphere ray_intersects_sphere_test
            segment3d_distance_squared_to_segment segment3d_within_radius_of_segment triangle_point_barycentric_2d
            decal_plane_solve_third_axis vector3d_projection_band_test vector2d_tangent_edge_directions""",
        "polygon": """polygon2d_clip_to_plane polygon2d_clip_to_planes polygon2d_convex_hull_build
            polygon2d_point_inside_margin polygon2d_point_inside_tolerance polygon2d_points_classify
            polygon3d_clip_to_plane""",
        "periodic_functions": """periodic_function_build_noise_table periodic_function_build_table
            periodic_function_build_transition_table periodic_function_evaluate periodic_function_tables_free
            periodic_function_tables_init transition_function_evaluate""",
        "interpolation": """bounded_ramp_profile_build bounded_ramp_profile_evaluate bounded_ramp_profile_synchronize
            cubic_interpolate_divided_difference vector3d_cubic_interpolate vector3d_barycentric_interpolate
            vector3d_lerp real_lerp_clamped real_inverse_lerp_clamped real_seek_toward_clamped
            lerp_find_threshold_byte""",
        "random": """random_int_range random_range_real random_real random_real_range random_real_range_seeded
            random_seed_generate vector3d_randomize_direction""",
        "sphere_mesh": """sphere_mesh_build_face sphere_mesh_generate sphere_mesh_get_edge_point
            sphere_mesh_get_face_point sphere_mesh_interpolate_vertex sphere_point_table_init""",
        "utility": """bit_vector_or uint32_log2_floor float_compare_ascending object_sort_by_flag_then_distance
            color_real_to_argb_pack""",
        "math_initialize": "math_initialize",
    },
}
GROUP_TITLES = {
    "math": {
        "real_vector": "real vectors and points: length, normalize, cross/dot products, projections, angles",
        "rotation": "rotating vectors: axis-angle rotation and the bounded angular servos",
        "matrix": "real_matrix3x3 / real_matrix4x3: build, invert, multiply (scalar, SSE, 3DNow!), transform",
        "quaternion": "real_quaternion: normalize, multiply, lerp, rotate, matrix conversions",
        "geometry": "planes, rays, segments, spheres, cylinders, triangles: intersection and distance queries",
        "polygon": "2D/3D polygons: Sutherland-Hodgman clipping, convex hull, containment",
        "periodic_functions": "periodic (wave) and transition (easing) function tables and evaluators",
        "interpolation": "interpolation, lerps, cubic curves and the bounded acceleration ramp profile",
        "random": "the 32-bit LCG random streams and random directions",
        "sphere_mesh": "the subdivided-octahedron sphere mesh and the 1026-entry direction table",
        "utility": "bit vectors, integer log2, qsort comparators, colour packing",
        "math_initialize": "math_initialize: table set-up and the matrix4x3_multiply CPU dispatch",
    },
}

WRAP_RE = re.compile(r'#ifdef __cplusplus\r?\n(extern "C" \{|\}) /\* (extern "C" )?HALO_CXX_LINKAGE \*/\r?\n#endif\r?\n?')
IF0_RE = re.compile(r"(?ms)^#if 0[^\n]*\n(.*?)^#endif[^\n]*\n?")


def mask(text):
    """Same length as text, with comments and string/char literals replaced by spaces (newlines kept)."""
    out, i, n = list(text), 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i); j = n if j < 0 else j
            for k in range(i, j): out[k] = " "
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2); j = n if j < 0 else j + 2
            for k in range(i, j):
                if text[k] != "\n": out[k] = " "
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            for k in range(i + 1, min(j, n)): out[k] = " "
            i = j + 1
        else:
            i += 1
    return "".join(out)


def split_top(text):
    """Top-level items of a C file: ('comment'|'pp'|'blank'|'item', text). An item ends at a depth-0 `;` or at the
    `}` closing a depth-0 brace; a `// comment` on the same line after it is kept with the item."""
    m = mask(text)
    items, i, n = [], 0, len(text)
    while i < n:
        eol = text.find("\n", i); eol = n if eol < 0 else eol
        line = text[i:eol]
        s = line.strip()
        if not s:
            items.append(("blank", "")); i = eol + 1; continue
        if s.startswith("//"):
            items.append(("comment", line)); i = eol + 1; continue
        if s.startswith("#"):
            items.append(("pp", line)); i = eol + 1; continue
        depth, j = 0, i
        while j < n:
            ch = m[j]
            if ch == "{": depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0 and m[i:j].count("(") and "=" not in m[i:m.find("{", i)]:
                    j += 1; break
            elif ch == ";" and depth == 0:
                j += 1; break
            j += 1
        e = text.find("\n", j); e = n if e < 0 else e
        tail = text[j:e]
        if tail.strip() and not tail.strip().startswith("//"):
            raise SystemExit("unexpected text after item: %r" % tail)
        items.append(("item", text[i:e])); i = e + 1
    return items


def split_params(s):
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "([": depth += 1
        elif ch in ")]": depth -= 1
        if ch == "," and depth == 0:
            out.append(cur); cur = ""
        else:
            cur += ch
    if cur.strip(): out.append(cur)
    return [p.strip() for p in out]


def parse_signature(sig):
    m = re.match(r"(?s)^(.*?)\b(\w+)\s*\((.*)\)\s*$", sig.strip())
    if not m:
        raise SystemExit("cannot parse signature: " + sig)
    ret, name, params = m.group(1).strip(), m.group(2), m.group(3).strip()
    plist = []
    if params and params != "void":
        for p in split_params(" ".join(params.split())):
            pm = re.match(r"^(.*?)(\w+)\s*(\[[^\]]*\])?$", p)
            plist.append({"type": pm.group(1).strip(), "name": pm.group(2), "array": bool(pm.group(3)), "decl": p})
    return ret, name, plist


def is_deref_only(name, body_masked):
    uses = [mm.start() for mm in re.finditer(r"\b%s\b" % re.escape(name), body_masked)]
    if not uses:
        return False, []
    kinds = []
    for u in uses:
        after = body_masked[u + len(name):]
        before = body_masked[:u].rstrip()
        if re.match(r"\s*->", after):
            kinds.append("arrow"); continue
        if before.endswith("*") and not re.match(r"\s*(\+\+|--|\[|\.|\()", after):
            kinds.append("deref"); continue   # unary *: a pointer cannot be the right operand of a multiplication
        return False, []
    return True, list(zip(uses, kinds))


def has_write(name, body_masked):
    if re.search(r"\b%s\b((\s*\.\s*\w+)|(\s*\[[^\]]*\]))*\s*(=(?!=)|[-+*/|&^%%]=|<<=|>>=|\+\+|--)" % re.escape(name), body_masked):
        return True
    if re.search(r"(\+\+|--|(?<![&\w\)\]])&)\s*\b%s\b" % re.escape(name), body_masked):
        return True
    for mm in re.finditer(r"\b%s\b" % re.escape(name), body_masked):   # whole object passed to a call
        before = body_masked[:mm.start()].rstrip()
        after = body_masked[mm.end():].lstrip()
        if before.endswith(("(", ",")) and after[:1] in (")", ","):
            j = len(before) - 1
            if before.endswith(","):
                return True
            k = before[:-1].rstrip()
            if re.search(r"\w$", k) and not re.search(r"\b(return|if|while|switch|sizeof)$", k):
                return True
    return False


class Function:
    pass


def read_source(path, ref):
    if ref is None:
        return open(path, encoding="utf-8").read()
    import subprocess
    rel = os.path.relpath(path, ROOT).replace("\\", "/")
    return subprocess.run(["git", "show", "%s:%s" % (ref, rel)], cwd=ROOT, capture_output=True, check=True).stdout.decode("utf-8")


def parse_file(path, module_names, ref=None):
    raw = read_source(path, ref).replace("\r\n", "\n")
    text = WRAP_RE.sub("", raw)
    originals = IF0_RE.findall(text)
    text = IF0_RE.sub("", text)
    f = Function()
    f.raw = raw
    f.file = os.path.basename(path)
    f.originals = originals
    f.comments, f.externs, f.helpers, f.includes, f.defines, f.trailing = [], [], [], [], [], []
    f.sig = None
    for kind, t in split_top(text):
        if kind == "pp":
            if t.strip().startswith("#include"):
                f.includes.append(t.strip())
            else:
                f.defines.append(t.strip())
        elif kind == "comment":
            (f.comments if f.sig is None else f.trailing).append(t.rstrip())
        elif kind == "blank":
            if f.sig is None and f.comments and f.comments[-1] != "":
                f.comments.append("")
            elif f.sig is not None and f.trailing and f.trailing[-1] != "":
                f.trailing.append("")
        else:
            st = t.strip()
            if st.startswith("extern"):
                decl = st
                mm = re.search(r"\b(\w+)\s*(\(|\[|;|\))", mask(decl).replace("(*", "( "))
                nm = re.search(r"\(\s*\*\s*(\w+)\s*\)", decl)
                ename = nm.group(1) if nm else re.search(r"\b(\w+)\s*(\(|\[|;)", mask(decl)).group(1)
                f.externs.append((ename, decl))
            elif st.startswith("static"):
                hm = re.match(r"(?s)static\s+(.*?)\b(\w+)\s*\(", st)
                f.helpers.append((hm.group(2), st))
            elif re.match(r"(?s)^[^;{]*\([^;{]*\)\s*\{", mask(st)):
                if f.sig is not None:
                    raise SystemExit("%s: two non-static functions" % path)
                brace = mask(st).index("{")
                f.sig = st[:brace].strip()
                f.body = st[brace:]
                f.ret, f.name, f.params = parse_signature(f.sig)
            else:
                raise SystemExit("%s: unhandled top-level item: %s" % (path, st[:80]))
    while f.comments and f.comments[-1] == "": f.comments.pop()
    while f.trailing and f.trailing[-1] == "": f.trailing.pop()
    am = re.search(r"address 0x([0-9a-fA-F]+)", "\n".join(f.comments))
    f.address = int(am.group(1), 16) if am else None
    return f


def convert(funcs, module_names):
    by_name = {f.name: f for f in funcs}
    # NULL literal passed to a module function by another module function: that parameter stays a pointer
    nulled = collections.defaultdict(set)
    for f in funcs:
        mb = mask(f.body)
        for g in funcs:
            for mm in re.finditer(r"(?<![\w.>])%s\s*\(" % g.name, mb):
                args = call_args(mb, mm.end() - 1)
                for k, (a0, a1) in enumerate(args):
                    if re.fullmatch(r"\s*(0|NULL|\(\w[\w\s*]*\)\s*0)\s*", mb[a0:a1]):
                        nulled[g.name].add(k)
    for f in funcs:
        mb = mask(f.body)
        f.ref = {}
        for k, p in enumerate(f.params):
            t = p["type"]
            if p["array"] or not t.endswith("*") or t.endswith("**") or re.match(r"^(const\s+)?(void|char)\b", t):
                continue
            if k in nulled[f.name]:
                continue
            ok, uses = is_deref_only(p["name"], mb)
            if ok:
                f.ref[k] = uses
        # rewrite the body: process occurrences right to left
        edits = []
        for k, uses in f.ref.items():
            nm = f.params[k]["name"]
            for pos, kind in uses:
                if kind == "arrow":
                    a = mb.index("->", pos + len(nm))
                    edits.append((a, a + 2, "."))
                else:
                    star = mb.rindex("*", 0, pos)
                    edits.append((star, pos, ""))
        body = apply_edits(f.body, edits)
        f.body = body
        f.new_params = []
        for k, p in enumerate(f.params):
            if k in f.ref:
                base = p["type"][:-1].strip()
                const = base.startswith("const ")
                if not const and not has_write(p["name"], mask(body)):
                    base = "const " + base
                f.new_params.append("%s &%s" % (base, p["name"]))
            else:
                f.new_params.append(p["decl"])
    # calls between module functions: adjust the argument for every parameter that became a reference
    for f in funcs:
        edits = []
        mb = mask(f.body)
        for g in funcs:
            if not g.ref:
                continue
            for mm in re.finditer(r"(?<![\w.>])%s\s*\(" % g.name, mb):
                for k, (a0, a1) in enumerate(call_args(mb, mm.end() - 1)):
                    if k not in g.ref:
                        continue
                    arg = f.body[a0:a1]
                    lead = len(arg) - len(arg.lstrip())
                    core = arg.strip()
                    am = re.fullmatch(r"&\s*([\w.\[\]>\-]+)", core)
                    if am:
                        new = am.group(1)
                    elif re.fullmatch(r"\w+", core):
                        new = "*" + core
                    else:
                        new = "*(" + core + ")"
                    edits.append((a0 + lead, a0 + lead + len(core), new))
        f.body = apply_edits(f.body, edits)


def call_args(mb, open_paren):
    depth, i, start, out = 0, open_paren, open_paren + 1, []
    while i < len(mb):
        ch = mb[i]
        if ch in "([": depth += 1
        elif ch in ")]":
            depth -= 1
            if depth == 0:
                if mb[start:i].strip(): out.append((start, i))
                return out
        elif ch == "," and depth == 1:
            out.append((start, i)); start = i + 1
        i += 1
    return out


def apply_edits(text, edits):
    for a, b, s in sorted(edits, key=lambda e: -e[0]):
        text = text[:a] + s + text[b:]
    return text


def strip_comments(code):
    """Removes // and /* */ comments; lines that held only a comment disappear, trailing blanks are trimmed."""
    m = mask(code)
    out = []
    for line, ml in zip(code.split("\n"), m.split("\n")):
        if line.strip() and not ml.strip():
            continue
        # mask() blanks comment characters AND string contents; only drop the comment parts
        res, k, n = [], 0, len(line)
        while k < n:
            if line.startswith("//", k) and ml[k] == " ":
                break
            if line.startswith("/*", k) and ml[k] == " ":
                e = line.find("*/", k + 2)
                k = n if e < 0 else e + 2
                continue
            res.append(line[k]); k += 1
        out.append("".join(res).rstrip())
    text = "\n".join(out)
    return re.sub(r"\n{3,}", "\n\n", text)


def load_docblocks(module):
    path = os.path.join(ROOT, "tools", "cxx_work", module + "_docblocks.txt")
    out = {}
    if os.path.exists(path):
        for line in open(path, encoding="utf-8"):
            if "|" in line and not line.startswith("#"):
                n, t = line.split("|", 1); out[n.strip()] = t.strip()
    return out


def wrap_text(text, width=112, prefix=" * "):
    words, lines, cur = text.split(), [], ""
    for w in words:
        if cur and len(cur) + 1 + len(w) > width - len(prefix):
            lines.append(prefix + cur); cur = w
        else:
            cur = (cur + " " + w) if cur else w
    if cur: lines.append(prefix + cur)
    return lines


def docblock(f, docs):
    text = docs.get(f.name) or "TODO: describe."
    bcc = None
    for l in f.comments:
        m = re.search(r"blam-cc:\s*(.*)$", l)
        if m:
            bcc = re.sub(r"\s*/\*\s*(\w+)\s*\*/", r" in \1", m.group(1)).strip(); break
    D = ["/**"] + wrap_text(text)
    if bcc:
        D += [" *"] + wrap_text("Original register convention: %s." % bcc.rstrip("."))
    D += [" * @address %s" % addr(f), " */"]
    return D


def addr(f):
    return "0x%08x" % f.address if f.address is not None else "unknown"


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    module = args[0]
    dry = "--dry-run" in sys.argv
    src_dir = os.path.join(ROOT, "src", module)
    ref = sys.argv[sys.argv.index("--from-ref") + 1] if "--from-ref" in sys.argv else None
    if ref:
        import subprocess
        args = [a for a in args if a != ref]
        ls = subprocess.run(["git", "ls-tree", "--name-only", ref, "src/%s/" % module], cwd=ROOT, capture_output=True,
                            text=True, check=True).stdout.split()
        files = sorted(os.path.join(ROOT, p) for p in ls if p.endswith(".c"))
    else:
        files = sorted(glob.glob(os.path.join(src_dir, "*.c")))
    names = {os.path.splitext(os.path.basename(p))[0] for p in files}
    funcs = [parse_file(p, names, ref) for p in files]
    for f in funcs:
        if f.name + ".c" != f.file:
            raise SystemExit("%s defines %s" % (f.file, f.name))
    convert(funcs, names)
    groups = {n: g for g, s in GROUPS[module].items() for n in s.split()}
    missing = [f.name for f in funcs if f.name not in groups]
    if missing:
        raise SystemExit("no group for: " + " ".join(missing))
    out = collections.defaultdict(list)
    for f in funcs:
        out[groups[f.name]].append(f)
    ns = "halo::%s" % module
    written = []

    def emit(path, text):
        written.append(path)
        if not dry:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            open(path, "w", encoding="utf-8", newline="\n").write(text)

    docs = load_docblocks(module)
    # docs/original/<module>/<file>.txt: the whole original file, author notes, inline notes and `#if 0` decompile
    for f in funcs:
        emit(os.path.join(ROOT, "docs", "original", module, f.file + ".txt"), f.raw)
    for g, fl in out.items():
        fl.sort(key=lambda f: (f.address is None, f.address or 0, f.name))
        includes = []
        for f in fl:
            for inc in f.includes:
                if inc not in includes and inc != '#include "math.h"': includes.append(inc)
        externs = collections.OrderedDict()
        for f in fl:
            for en, decl in f.externs:
                if en in names: continue
                externs.setdefault(en, strip_comments(decl))
        helpers, helper_names = [], {}
        for f in fl:
            for hn, ht in f.helpers:
                ht = strip_comments(ht)
                if hn in helper_names and helper_names[hn] != ht:
                    nn = "%s_%s" % (hn, f.name)
                    ht = re.sub(r"\b%s\b" % hn, nn, ht, count=1)
                    f.body = re.sub(r"\b%s\b" % hn, nn, f.body)
                    helpers.append(ht)
                elif hn not in helper_names:
                    helper_names[hn] = ht; helpers.append(ht)
        L = ["/**", " * @file src/%s/%s.cpp" % (module, g), " * %s." % GROUP_TITLES[module][g].capitalize(),
             " * The original author notes and decompiles are in docs/original/%s/." % module, " */", ""]
        L += ['#include "halo/%s/%s.hpp"' % (module, module), ""]
        L += includes + [""]
        if externs:
            L += ['extern "C" {'] + list(externs.values()) + ["}", ""]
        L += ["namespace %s {" % ns, ""]
        if helpers:
            L += ["namespace {", ""] + [h + "\n" for h in helpers] + ["}  // namespace", ""]
        for f in fl:
            for d in f.defines: L.append(d)
            L.append("%s %s(%s)" % (f.ret, f.name, ", ".join(f.new_params)))
            L.append(strip_comments(f.body).rstrip())
            L.append("")
        L += ["}  // namespace %s" % ns, ""]
        emit(os.path.join(src_dir, g + ".cpp"), "\n".join(L))
        H = ["/**", " * @file include/halo/%s/%s.hpp" % (module, g), " * %s." % GROUP_TITLES[module][g].capitalize(),
             " * The C symbols other modules link against are the wrappers in src/%s/%s_c_api.cpp." % (module, module),
             " */", "#pragma once", "", '#include "halo/%s/%s_types.hpp"' % (module, module), "", "namespace %s {" % ns, ""]
        for f in fl:
            H += docblock(f, docs) + ["%s %s(%s);" % (f.ret, f.name, ", ".join(f.new_params)), ""]
        H += ["}  // namespace %s" % ns, ""]
        emit(os.path.join(ROOT, "include", "halo", module, g + ".hpp"), "\n".join(H))
    # C API: prototypes + wrappers, original signatures, original address order
    allf = sorted(funcs, key=lambda f: (f.address is None, f.address or 0, f.name))
    P = ["/**", " * @file include/halo/%s/%s_c_api.h" % (module, module),
         " * The C ABI of the %s module: every original function with its original C signature and C linkage, the" % module,
         " * symbols the link tables, the code-pointer slots and the unconverted modules use (baseline:",
         " * symbols/exports/%s.txt). Defined in src/%s/%s_c_api.cpp; documented on the %s functions." % (module, module, module, ns),
         " */", "#pragma once", "", '#include "halo/%s/%s_types.hpp"' % (module, module), "", '#ifdef __cplusplus', 'extern "C" {', "#endif", ""]
    W = ["/**", " * @file src/%s/%s_c_api.cpp" % (module, module),
         " * The %s module's C ABI: one extern \"C\" wrapper per original function, same name, signature and calling" % module,
         " * convention, forwarding to the %s implementation (pointers the C++ API takes by reference are" % ns,
         " * dereferenced here). Wrappers do nothing else.", " */", "",
         '#include "halo/%s/%s_c_api.h"' % (module, module), '#include "halo/%s/%s.hpp"' % (module, module), ""]
    for f in allf:
        if f.ret == "": raise SystemExit("no return type: " + f.name)
        P.append("%s %s(%s);" % (f.ret, f.name, ", ".join(p["decl"] for p in f.params) or "void"))
        call = "%s::%s(%s)" % (ns, f.name, ", ".join(("*" if k in f.ref else "") + p["name"] for k, p in enumerate(f.params)))
        W.append('extern "C" %s %s(%s)' % (f.ret, f.name, ", ".join(p["decl"] for p in f.params) or "void"))
        W.append("{")
        W.append("    %s%s;" % ("" if f.ret == "void" else "return ", call))
        W.append("}")
        W.append("")
    P += ["", "#ifdef __cplusplus", "}", "#endif", ""]
    emit(os.path.join(ROOT, "include", "halo", module, module + "_c_api.h"), "\n".join(P))
    emit(os.path.join(src_dir, module + "_c_api.cpp"), "\n".join(W))
    U = ["/**", " * @file include/halo/%s/%s.hpp" % (module, module), " * The public C++ API of the %s module (namespace %s)." % (module, ns),
         " */", "#pragma once", "", '#include "halo/%s/%s_types.hpp"' % (module, module)]
    U += ['#include "halo/%s/%s.hpp"' % (module, g) for g in GROUPS[module]] + [""]
    emit(os.path.join(ROOT, "include", "halo", module, module + ".hpp"), "\n".join(U))
    nref = sum(len(f.ref) for f in funcs); nptr = sum(len(f.params) for f in funcs)
    print("%s: %d functions -> %d groups; %d of %d parameters became references; %d files written%s"
          % (module, len(funcs), len(out), nref, nptr, len(written), " (dry run)" if dry else ""))


if __name__ == "__main__":
    main()
