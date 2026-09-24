"""invader tag definitions (JSON) -> types/tags.h, a C header Ghidra's CParser can ingest.
Every struct's computed size is checked against the JSON "size"; mismatches are reported and the
struct is padded/flagged rather than silently emitted wrong."""
import json, glob, os, sys, re

defdir = sys.argv[1] if len(sys.argv) > 1 else "vendor/invader/src/tag/hek/definition"
out = sys.argv[2] if len(sys.argv) > 2 else "types/tags.h"

# primitive name -> (C type, size)
PRIM = {
    "int8": ("int8_t", 1), "uint8": ("uint8_t", 1), "int16": ("int16_t", 2), "uint16": ("uint16_t", 2),
    "int32": ("int32_t", 4), "uint32": ("uint32_t", 4), "float": ("float", 4),
    "Angle": ("float", 4), "Fraction": ("float", 4), "Index": ("uint16_t", 2),
    "TagID": ("TagID", 4), "Pointer": ("uint32_t", 4), "TagFourCC": ("uint32_t", 4),
    "TagString": ("TagString", 32), "TagDependency": ("TagDependency", 16), "TagReflexive": ("TagReflexive", 12),
    "TagDataOffset": ("TagDataOffset", 20), "ColorARGBInt": ("ColorARGBInt", 4), "ColorARGB": ("ColorARGB", 16),
    "ColorRGB": ("ColorRGB", 12), "Point2D": ("Point2D", 8), "Point3D": ("Point3D", 12), "Point2DInt": ("Point2DInt", 4),
    "Vector2D": ("Vector2D", 8), "Vector3D": ("Vector3D", 12), "Euler2D": ("Euler2D", 8), "Euler3D": ("Euler3D", 12),
    "Plane2D": ("Plane2D", 12), "Plane3D": ("Plane3D", 16), "Quaternion": ("Quaternion", 16),
    "Rectangle2D": ("Rectangle2D", 8), "Matrix": ("Matrix", 36), "ScenarioScriptNodeValue": ("uint32_t", 4),
}
PRELUDE = """// Generated from invader tag definitions. Do not edit by hand; edit tools/gen_tag_header.py.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;
typedef struct TagID { uint16_t index; uint16_t id; } TagID;
typedef struct TagString { char string[32]; } TagString;
typedef struct TagDependency { uint32_t tag_fourcc; uint32_t path_pointer; uint32_t path_size; TagID tag_id; } TagDependency;
typedef struct TagReflexive { uint32_t count; uint32_t pointer; uint32_t definition; } TagReflexive;
typedef struct TagDataOffset { uint32_t size; uint32_t flags; uint32_t file_offset; uint32_t pointer; uint32_t definition; } TagDataOffset;
typedef struct ColorARGBInt { uint8_t blue, green, red, alpha; } ColorARGBInt;
typedef struct ColorARGB { float alpha, red, green, blue; } ColorARGB;
typedef struct ColorRGB { float red, green, blue; } ColorRGB;
typedef struct Point2D { float x, y; } Point2D;
typedef struct Point3D { float x, y, z; } Point3D;
typedef struct Point2DInt { int16_t x, y; } Point2DInt;
typedef struct Vector2D { float i, j; } Vector2D;
typedef struct Vector3D { float i, j, k; } Vector3D;
typedef struct Euler2D { float yaw, pitch; } Euler2D;
typedef struct Euler3D { float yaw, pitch, roll; } Euler3D;
typedef struct Plane2D { Vector2D vector; float w; } Plane2D;
typedef struct Plane3D { Vector3D vector; float w; } Plane3D;
typedef struct Quaternion { float i, j, k, w; } Quaternion;
typedef struct Rectangle2D { int16_t top, left, bottom, right; } Rectangle2D;
typedef struct Matrix { float m[3][3]; } Matrix;
"""

defs = {}
order = []
for f in sorted(glob.glob(os.path.join(defdir, "*.json"))):
    for d in json.load(open(f, encoding="utf-8")):
        if d["name"] not in defs:
            defs[d["name"]] = d
            order.append(d["name"])

def cname(s):
    s = re.sub(r"[^A-Za-z0-9_]", "_", s)
    if not s or s[0].isdigit(): s = "_" + s
    return s

emitted = set()
lines = []
problems = []

def emit(name):
    """Emit definition for `name` (recursively emitting dependencies). Returns size."""
    if name in PRIM: return PRIM[name][1]
    d = defs.get(name)
    if d is None:
        problems.append("unknown type " + name); return None
    if name in emitted: return d.get("_size")
    emitted.add(name)
    t = d["type"]
    if t == "enum":
        opts = d["options"]
        lines.append("typedef enum %s {" % cname(name))
        seen = set()
        for i, o in enumerate(opts):
            v = cname(name + "_" + (o if isinstance(o, str) else o["name"])).lower()
            while v in seen: v += "_"
            seen.add(v)
            lines.append("    %s = %d," % (v, i))
        lines.append("} %s;  // int16" % cname(name))
        d["_size"] = 2
        # enums are stored as int16 in tags; Ghidra CParser sizes enums as int, so we wrap usage in a typedef
        lines.append("typedef int16_t %s_t;" % cname(name))
        return 2
    if t == "bitfield":
        w = d.get("width", 32)
        lines.append("typedef uint%d_t %s;  // bitfield: %s" % (w, cname(name), ", ".join(cname(x if isinstance(x, str) else x["name"]) for x in d["fields"])))
        d["_size"] = w // 8
        return w // 8
    if t == "struct":
        body = []
        off = 0
        parent = d.get("inherits")
        if parent:
            psz = emit(parent)
            body.append("    %s base;  // inherits" % cname(parent)); off += psz or 0
        for i, fld in enumerate(d["fields"]):
            ft = fld["type"]
            if ft == "pad":
                body.append("    uint8_t _pad_%x[%d];" % (off, fld["size"])); off += fld["size"]; continue
            if ft == "TagReflexive":
                inner = fld.get("struct", "")
                if inner and inner not in emitted and inner in defs: emit(inner)
                body.append("    TagReflexive %s;  // %s" % (cname(fld.get("name", "field_%x" % off)), inner)); off += 12; continue
            if ft == "TagDependency":
                body.append("    TagDependency %s;  // %s" % (cname(fld.get("name", "field_%x" % off)), ",".join(fld.get("classes", [])))); off += 16; continue
            sz = emit(ft) if ft not in PRIM else PRIM[ft][1]
            if sz is None:
                body.append("    uint8_t %s[0];  // UNKNOWN TYPE %s" % (cname(fld.get("name","f")), ft)); continue
            ctype = PRIM[ft][0] if ft in PRIM else (cname(ft) + "_t" if defs[ft]["type"] == "enum" else cname(ft))
            count = fld.get("count", 1)
            if fld.get("bounds"): count *= 2
            nm = cname(fld.get("name", "field_%x" % off))
            body.append("    %s %s%s;" % (ctype, nm, ("[%d]" % count) if count != 1 else ""))
            off += sz * count
        want = d.get("size")
        if want is not None and want != off:
            problems.append("%s: computed 0x%x, json says 0x%x" % (name, off, want))
            if off < want: body.append("    uint8_t _size_fix[%d];  // SIZE MISMATCH" % (want - off)); off = want
        lines.append("typedef struct %s {" % cname(name)); lines.extend(body); lines.append("} %s;  // size 0x%x" % (cname(name), off))
        d["_size"] = off
        return off
    problems.append("unhandled def type %s for %s" % (t, name)); return None

for n in order: emit(n)
with open(out, "w") as fh:
    fh.write(PRELUDE); fh.write("\n".join(lines)); fh.write("\n#pragma pack(pop)\n")
print("emitted", len(emitted), "types ->", out)
print("problems:", len(problems))
for p in problems[:30]: print("  ", p)
