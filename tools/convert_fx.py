"""Convert Halo PC's shaders\\fx.bin from the 2003 D3DX compiled-effect format to fx_2_0, readable by d3dx9_43.

fx.bin (after TEA decryption, see src/cseries/tea_decrypt_buffer.c) is 122 records [int32 size][compiled effect]
followed by a 33-byte MD5 hex digest of everything before it. Each compiled effect in the 2003 format is:
    0xffffffff, blob size, blob (typedefs, values and names, referenced by offset from the blob start)
    parameter count, technique count, object count           <- fx_2_0 has an extra dword before the object count
    parameters  (typedef, value, flags, annotation count, annotations (typedef, value))
    techniques  (name, annotation count, pass count, annotations,
                 passes (name, annotation count, state count, annotations, states (operation, index, typedef, value)))
    the object data section (see parse())
fx_2_0 (0xfeff0901) is the same apart from the extra header dword and the state operation numbering, which follows
D3DX's own state table (tools/fx_state_tables.py maps the legacy indices by name).
Usage: python tools/convert_fx.py <fx.bin> <out fx.bin> [--dump N]"""
import os, sys, struct

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

KEY = (0x3fffffdd, 0x7fc3, 0xe5, 0x3fffef)   # rasterizer_resource_file_verify_signature's TEA key


def tea_block(v0, v1, decrypt):
    k0, k1, k2, k3 = KEY
    if decrypt:
        s = 0xc6ef3720
        for _ in range(32):
            v1 = (v1 - ((((v0 << 4) & 0xffffffff) + k2) ^ ((v0 + s) & 0xffffffff) ^ ((v0 >> 5) + k3))) & 0xffffffff
            v0 = (v0 - ((((v1 << 4) & 0xffffffff) + k0) ^ ((v1 + s) & 0xffffffff) ^ ((v1 >> 5) + k1))) & 0xffffffff
            s = (s + 0x61c88647) & 0xffffffff
    else:
        s = 0
        for _ in range(32):
            s = (s + 0x9e3779b9) & 0xffffffff
            v0 = (v0 + ((((v1 << 4) & 0xffffffff) + k0) ^ ((v1 + s) & 0xffffffff) ^ ((v1 >> 5) + k1))) & 0xffffffff
            v1 = (v1 + ((((v0 << 4) & 0xffffffff) + k2) ^ ((v0 + s) & 0xffffffff) ^ ((v0 >> 5) + k3))) & 0xffffffff
    return v0, v1


def tea_buffer(data, decrypt):
    """tea_decrypt_buffer 0x618350 and its inverse: the overlapping last block is done first when decrypting, so
    encryption does the full blocks first and the overlapping last block last"""
    d = bytearray(data)
    n = len(d)

    def block(o):
        v0, v1 = struct.unpack_from("<II", d, o)
        struct.pack_into("<II", d, o, *tea_block(v0, v1, decrypt))
    if n < 8:
        return bytes(d)
    if decrypt:
        if n % 8:
            block(n - 8)
        for o in range(0, n // 8 * 8, 8):
            block(o)
    else:
        for o in range(0, n // 8 * 8, 8):
            block(o)
        if n % 8:
            block(n - 8)
    return bytes(d)


class Reader:
    def __init__(self, data, pos):
        self.d, self.p = data, pos

    def u32(self):
        v = struct.unpack_from("<I", self.d, self.p)[0]
        self.p += 4
        return v


def parse(effect):
    """legacy effect -> dict of its parts (offsets kept as they are: they are relative to the blob)"""
    r = Reader(effect, 0)
    tag, blob_size = r.u32(), r.u32()
    assert tag == 0xffffffff, hex(tag)
    blob = effect[8:8 + blob_size]
    r.p = 8 + blob_size
    fx = {"blob": blob, "params": [], "techniques": []}
    n_params, n_techniques, n_objects = r.u32(), r.u32(), r.u32()
    fx["objects"] = n_objects
    for _ in range(n_params):
        typedef, value, flags, n_ann = r.u32(), r.u32(), r.u32(), r.u32()
        fx["params"].append((typedef, value, flags, [(r.u32(), r.u32()) for _ in range(n_ann)]))
    for _ in range(n_techniques):
        name, n_ann, n_passes = r.u32(), r.u32(), r.u32()
        anns = [(r.u32(), r.u32()) for _ in range(n_ann)]
        passes = []
        for _ in range(n_passes):
            pname, p_ann, n_states = r.u32(), r.u32(), r.u32()
            panns = [(r.u32(), r.u32()) for _ in range(p_ann)]
            states = [(r.u32(), r.u32(), r.u32(), r.u32()) for _ in range(n_states)]
            passes.append((pname, panns, states))
        fx["techniques"].append((name, anns, passes))
    fx["tail"] = effect[r.p:]
    return fx


def blob_string(blob, offset):
    n = struct.unpack_from("<I", blob, offset)[0]
    return blob[offset + 4:offset + 4 + n].split(b"\0")[0].decode("latin1")


def legacy_expression_parameters(effects):
    names = set()
    for e in effects:
        t = parse(e)["tail"]
        r = Reader(t, 0)
        n_strings, n_resources = r.u32(), r.u32()
        for _ in range(n_strings):
            r.u32()
            size = r.u32()
            r.p += (size + 3) & ~3
        for _ in range(n_resources):
            technique, index, state, usage, n = [r.u32() for _ in range(5)]
            data = t[r.p:r.p + n]
            r.p += (n + 3) & ~3
            if usage == 0 and data[:4] == b"\x00\x02XF":
                names.add(expression_parameter(data))
    return names


def records(plain):
    out, c = [], 0
    end = len(plain) - 0x21
    while c + 4 <= end:
        n = struct.unpack_from("<i", plain, c)[0]
        out.append(plain[c + 4:c + 4 + n])
        c += 4 + n
    assert c == end, (c, end)
    return out


SAMPLER_TYPES = (10, 11, 12, 13, 14)   # D3DXPT_SAMPLER, SAMPLER1D, SAMPLER2D, SAMPLER3D, SAMPLERCUBE
SDK = r"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)"
WORK = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build", "fx")


def expression_parameter(data):
    """name of the parameter a legacy FXLC expression reads, after checking it is the only shape fx.bin uses:
    CTAB (20-byte legacy header) with one float4 in register c0, and one 4-component mov from it to the output"""
    size, creator, version, count, info = struct.unpack_from("<5I", data, 12)
    ct = data[12:]
    name_off, register, register_count, typeinfo, default = struct.unpack_from("<5I", ct, info)
    assert (size, count, register, register_count) == (20, 1, 2, 1), "unexpected CTAB"
    assert struct.unpack_from("<6H", ct, typeinfo) == (1, 3, 1, 4, 1, 0), "not a float4"
    assert b"FXLC" in data and struct.pack("<3I", 0x10000004, 4, 4) in data, "not a single float4 mov"
    return ct[name_off:ct.index(b"\0", name_off)].decode()


def parse_modern_resources(fxo):
    """fx_2_0 binary -> {(technique, pass, state): (usage, data)}"""
    r = Reader(fxo, 4)
    r.p = 8 + r.u32()
    n_params, n_techniques, _, n_objects = r.u32(), r.u32(), r.u32(), r.u32()
    for _ in range(n_params):
        r.p += 12
        annotations = r.u32()
        r.p += 8 * annotations
    for _ in range(n_techniques):
        r.u32()
        anns, passes = r.u32(), r.u32()
        r.p += 8 * anns
        for _ in range(passes):
            r.u32()
            panns, states = r.u32(), r.u32()
            r.p += 8 * panns + 16 * states
    n_strings, n_resources = r.u32(), r.u32()
    for _ in range(n_strings):
        r.u32()
        size = r.u32()
        r.p += (size + 3) & ~3
    out = {}
    for _ in range(n_resources):
        technique, index, element, state, usage, n = [r.u32() for _ in range(6)]
        out[(technique, index, state)] = (usage, fxo[r.p:r.p + n])
        r.p += (n + 3) & ~3
    return out


def modern_expressions(names):
    """{parameter name: the FXLC expression June 2010 compiles for `PixelShaderConstant[0] = <name>`}"""
    import subprocess
    os.makedirs(WORK, exist_ok=True)
    exe = os.path.join(WORK, "fx_compile.exe")
    if not os.path.exists(exe):
        sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "harness"))
        import gen_link as gl
        src = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fx_compile.c")
        r = subprocess.run([gl.tool("cl"), "/nologo", "/I" + SDK + r"\Include", "/Fe" + exe, "/Fo" + WORK + "\\", src,
                            "/link", "/LIBPATH:" + SDK + r"\Lib\x86", "d3dx9.lib"],
                           capture_output=True, text=True, env=gl.env)
        assert r.returncode == 0, r.stdout
    names = sorted(names)
    fx = ["float4 %s;" % n for n in names]
    fx += ["technique t%d { pass p { PixelShaderConstant[0] = <%s>; } }" % (i, n) for i, n in enumerate(names)]
    path = os.path.join(WORK, "constants.fx")
    open(path, "w").write("\n".join(fx) + "\n")
    r = subprocess.run([exe, path, path + "o"], capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    res = parse_modern_resources(open(path + "o", "rb").read())
    out = {}
    for i, n in enumerate(names):
        usage, data = res[(i, 0, 0)]
        assert usage == 0 and b"FXLC" in data
        out[n] = data
    return out


def convert(effect, state_map, expressions):
    """legacy effect bytes -> fx_2_0 effect bytes"""
    fx = parse(effect)
    blob = bytearray(fx["blob"])
    u = lambda o: struct.unpack_from("<I", blob, o)[0]

    # sampler parameters keep their state list in the blob: count, then (operation, index, typedef, value)
    for typedef, value, flags, anns in fx["params"]:
        ptype, pclass, elements = u(typedef), u(typedef + 4), u(typedef + 16)
        if ptype not in SAMPLER_TYPES:
            continue
        assert pclass == 4, "sampler with class %d" % pclass
        v = value
        for _ in range(max(elements, 1)):
            count = u(v)
            for k in range(count):
                o = v + 4 + 16 * k
                struct.pack_into("<I", blob, o, state_map[u(o)])
            v += 4 + 16 * count

    # shader-constant states (legacy operations 0x8f..0x9e): the 2003 compiler typed their value as a 4x1
    # D3DXPC_MATRIX_ROWS (class 2); June 2010 writes and requires D3DXPC_MATRIX_COLUMNS (class 3). The values are
    # bound by parameter name (resource usage 1) and hold zeros, so only the class changes.
    patched = set()
    for name, anns, passes in fx["techniques"]:
        for pname, panns, states in passes:
            for op, index, typedef, value in states:
                if 0x8f <= op <= 0x9e and typedef not in patched and u(typedef + 4) == 2:
                    struct.pack_into("<I", blob, typedef + 4, 3)
                    patched.add(typedef)

    # fx_2_0's extra header dword: a capacity d3dx9_43 checks its internal block numbering against ("Bad block
    # count" below it; any value at or above the compiler's exact figure loads). The exact rule is not derived, so
    # an upper bound is written: every parameter, state, sampler state, pass and technique
    states = sum(len(s) for _, _, ps in fx["techniques"] for _, _, s in ps)
    passes = sum(len(ps) for _, _, ps in fx["techniques"])
    sampler_states = 0
    for typedef, value, flags, anns in fx["params"]:
        if u(typedef) in SAMPLER_TYPES:
            v = value
            for _ in range(max(u(typedef + 16), 1)):
                sampler_states += u(v)
                v += 4 + 16 * u(v)
    capacity = len(fx["params"]) + states + sampler_states + passes + len(fx["techniques"])
    out = [struct.pack("<II", 0xfeff0901, len(blob)), bytes(blob),
           struct.pack("<IIII", len(fx["params"]), len(fx["techniques"]), capacity, fx["objects"])]
    for typedef, value, flags, anns in fx["params"]:
        out.append(struct.pack("<IIII", typedef, value, flags, len(anns)))
        out += [struct.pack("<II", *a) for a in anns]
    for name, anns, passes in fx["techniques"]:
        out.append(struct.pack("<III", name, len(anns), len(passes)))
        out += [struct.pack("<II", *a) for a in anns]
        for pname, panns, states in passes:
            out.append(struct.pack("<III", pname, len(panns), len(states)))
            out += [struct.pack("<II", *a) for a in panns]
            out += [struct.pack("<IIII", state_map[op], index, typedef, value) for op, index, typedef, value in states]

    # object data: the strings (id, size, data) are unchanged; each resource record gains fx_2_0's element index
    t = fx["tail"]
    r = Reader(t, 0)
    n_strings, n_resources = r.u32(), r.u32()
    start = r.p
    for _ in range(n_strings):
        r.u32()
        n = r.u32()
        r.p += (n + 3) & ~3
    out.append(t[:r.p])
    for _ in range(n_resources):
        technique, index, state, usage = r.u32(), r.u32(), r.u32(), r.u32()
        assert usage in (0, 1), usage
        n = r.u32()
        data = t[r.p:r.p + ((n + 3) & ~3)]
        r.p += (n + 3) & ~3
        if usage == 0 and data[:4] == b"\x00\x02XF":
            # a legacy expression (only ever a float4 parameter copied into a shader constant): the June 2010
            # compiler's encoding of the same assignment
            new = expressions[expression_parameter(data[:n])]
            n = len(new)
            data = new + b"\x00" * ((4 - n % 4) % 4)
        # element index: 0xffffffff for pass states, 0 for a (non-array) sampler parameter's own states, as the
        # June 2010 compiler writes them
        element = 0 if technique == 0xffffffff else 0xffffffff
        out.append(struct.pack("<IIIIII", technique, index, element, state, usage, n) + data)
    assert r.p == len(t)
    return b"".join(out)


def main():
    src = open(sys.argv[1], "rb").read()
    plain = tea_buffer(src, True)
    effects = records(plain)
    if "--dump" not in sys.argv:
        import hashlib
        import fx_state_tables
        state_map = fx_state_tables.mapping()[2]
        expressions = modern_expressions(legacy_expression_parameters(effects))
        body = b"".join(struct.pack("<i", len(e)) + e for e in (convert(x, state_map, expressions) for x in effects))
        plain_out = body + hashlib.md5(body).hexdigest().encode() + b"\0"
        open(sys.argv[2], "wb").write(tea_buffer(plain_out, False))
        if "--plain" in sys.argv:
            open(sys.argv[2] + ".plain", "wb").write(plain_out)
        print("converted %d effects: %d -> %d bytes" % (len(effects), len(src), len(plain_out)))
        return
    if "--dump" in sys.argv:
        k = int(sys.argv[sys.argv.index("--dump") + 1])
        fx = parse(effects[k])
        b = fx["blob"]
        print("params %d techniques %d objects %d, tail %d bytes" % (len(fx["params"]), len(fx["techniques"]),
                                                                    fx["objects"], len(fx["tail"])))
        for t, v, f, a in fx["params"]:
            print("  param %-28s type %d class %d value@%x flags %x ann %d" % (
                blob_string(b, struct.unpack_from("<I", b, t + 8)[0]), struct.unpack_from("<I", b, t)[0],
                struct.unpack_from("<I", b, t + 4)[0], v, f, len(a)))
        for name, anns, passes in fx["techniques"]:
            print("  technique", blob_string(b, name))
            for pname, panns, states in passes:
                print("    pass", blob_string(b, pname), "states", [(hex(o), i) for o, i, _, _ in states])
        tail = fx["tail"]
        print("  tail:", " ".join("%08x" % x for x in struct.unpack_from("<%dI" % min(24, len(tail) // 4), tail)))
        return


if __name__ == "__main__":
    main()
