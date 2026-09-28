p = r"C:\Users\Liam-\halo-re\tools\gen_standalone_link.py"
t = open(p, encoding="utf-8").read()

old = 'sys.path.insert(0, os.path.join(ROOT, "harness"))\nimport gen_link as gl\n'
assert old in t
t = t.replace(old, 'sys.path.insert(0, os.path.join(ROOT, "harness"))\nsys.path.insert(0, os.path.join(ROOT, "tools"))\n'
              'import retail_guard as rg   # HALO_NO_RETAIL=1: build from standalone/frozen/ only\nimport gen_link as gl\n', 1)

old = '''def d3dx_sizes():
    """stdcall argument bytes of the SDK's D3DX exports (halo.exe linked an older D3DX statically)"""
    cache = os.path.join(OUT, "d3dx_sizes.json")
    if os.path.exists(cache):
        return json.load(open(cache))
    r = subprocess.run([gl.tool("dumpbin"), "/nologo", "/linkermember:1", os.path.join(DXSDK_LIB, "d3dx9.lib")],
                       capture_output=True, text=True, env=gl.env, errors="replace")
    sizes = {}
    for m in re.finditer(r"(?<![\\w@])_(D3DX\\w*)@(\\d+)", r.stdout):
        sizes.setdefault(m.group(1), int(m.group(2)))
    json.dump(sizes, open(cache, "w"))
    return sizes


def original_ret_bytes(address):
    """bytes a function at an original address pops on return (the immediate of its first ret), None if not found"""
    r = subprocess.run(["objdump", "-d", "-M", "intel", "--start-address=0x%x" % address,
                        "--stop-address=0x%x" % (address + 0x4000), os.path.join(ROOT, "bin", "halo.exe")],
                       capture_output=True, text=True, errors="replace")
    for line in r.stdout.split("\\n"):
        m = re.search(r"\\tret\\s*(0x[0-9a-f]+)?\\s*$", line)
        if m:
            return int(m.group(1), 16) if m.group(1) else 0
    return None
'''
assert old in t, "d3dx/ret"
new = '''def d3dx_sizes():
    """stdcall argument bytes of the SDK's D3DX exports (halo.exe linked an older D3DX statically); frozen in
    standalone/frozen/d3dx_sizes.json so the build needs neither the SDK library listing nor a local cache"""
    if os.path.exists(rg.frozen_path("d3dx_sizes.json")):
        return rg.read_frozen("d3dx_sizes.json")
    r = subprocess.run([gl.tool("dumpbin"), "/nologo", "/linkermember:1", os.path.join(DXSDK_LIB, "d3dx9.lib")],
                       capture_output=True, text=True, env=gl.env, errors="replace")
    sizes = {}
    for m in re.finditer(r"(?<![\\w@])_(D3DX\\w*)@(\\d+)", r.stdout):
        sizes.setdefault(m.group(1), int(m.group(2)))
    rg.write_frozen("d3dx_sizes.json", sizes)
    return sizes


def original_ret_bytes(address):
    """bytes a function at an original address pops on return (the immediate of its first ret), None if not found.
    Answers come from standalone/frozen/code_address_ret.json; only a retail build (no HALO_NO_RETAIL) disassembles
    bin/halo.exe for an address not frozen yet, and adds it."""
    frozen = rg.read_frozen("code_address_ret.json") if os.path.exists(rg.frozen_path("code_address_ret.json")) else {}
    key = "0x%06x" % address
    if key in frozen:
        return frozen[key]
    if rg.NO_RETAIL:
        return None
    r = subprocess.run(["objdump", "-d", "-M", "intel", "--start-address=0x%x" % address,
                        "--stop-address=0x%x" % (address + 0x4000), os.path.join(ROOT, "bin", "halo.exe")],
                       capture_output=True, text=True, errors="replace")
    for line in r.stdout.split("\\n"):
        m = re.search(r"\\tret\\s*(0x[0-9a-f]+)?\\s*$", line)
        if m:
            frozen[key] = int(m.group(1), 16) if m.group(1) else 0
            rg.write_frozen("code_address_ret.json", frozen)
            return frozen[key]
    return None
'''
t = t.replace(old, new, 1)

old = '    crt = gl.game_crt()\n'
assert old in t
t = t.replace(old, '''    # game CRT functions by name (Ghidra's library matches, frozen by gen_standalone.py), plus two it never matched
    crt = rg.read_frozen("game_crt.json")
    crt.setdefault("fopen", 0x624186)
    crt.setdefault("wcscpy", 0x625bba)
''', 1)
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
