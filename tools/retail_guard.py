"""Build-time retail independence.

The standalone build never reads the retail game: every build tool (tools/msvc_build.py, tools/gen_standalone.py,
tools/gen_standalone_link.py) calls forbid_retail() first, which wraps open(), io.open() and subprocess so that any
attempt to read the retail binary (bin/halo.exe, or any halo.exe) or the Ghidra export out/functions.json (derived from
it and not in the repo) stops the build with an error.

Everything the build needs from the retail image lives in committed files under standalone/frozen/:
  layout.json               section placement (image base, pieces, TLS, resources, reserve)
  imports.json              every import / delay-load slot the loader fills
  code_pointer_slots.json   every dword in .rdata/.data that holds a function address (slot, target, retail name,
                            module); the C symbol is attached at build time from src/ headers
  game_crt.json             game CRT function names -> addresses (from out/functions.json's library matches)
  code_address_ret.json     original address -> bytes its first ret pops, for code_address_ thunks
  d3dx_sizes.json           stdcall argument bytes of the SDK's D3DX exports
and the data image itself is standalone/image/*.asm. Only the maintenance tools tools/freeze_retail_inputs.py and
tools/gen_image_source.py read the retail binary, to regenerate those files; they are never part of a build.
"""
import builtins, io, os, subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FROZEN = os.path.join(ROOT, "standalone", "frozen")
_forbidden = False


def is_retail_path(path):
    try:
        p = os.path.normcase(os.path.abspath(os.fspath(path)))
    except TypeError:
        return False
    name = os.path.basename(p)
    return name == "halo.exe" or p == os.path.normcase(os.path.join(ROOT, "out", "functions.json"))


def refuse(path):
    raise SystemExit("the standalone build never reads retail data, but it tried to read %s; the build inputs are the "
                     "committed files in %s (regenerate them with tools/freeze_retail_inputs.py)" % (path, FROZEN))


def forbid_retail():
    """installs the guard (idempotent): from here on this process cannot read the retail binary"""
    global _forbidden
    if _forbidden:
        return
    _forbidden = True
    _open = builtins.open

    def _guarded_open(file, *args, **kwargs):
        if is_retail_path(file):
            refuse(file)
        return _open(file, *args, **kwargs)

    builtins.open = _guarded_open
    io.open = _guarded_open

    _run = subprocess.run
    _popen = subprocess.Popen

    def _check(cmd):
        for a in (cmd if isinstance(cmd, (list, tuple)) else [cmd]):
            if isinstance(a, (str, bytes, os.PathLike)):
                s = os.fsdecode(a)
                if s.lower().endswith("halo.exe") and is_retail_path(s):
                    refuse(s)

    def _guarded_run(cmd, *args, **kwargs):
        _check(cmd)
        return _run(cmd, *args, **kwargs)

    class _GuardedPopen(_popen):
        def __init__(self, cmd, *args, **kwargs):
            _check(cmd)
            super().__init__(cmd, *args, **kwargs)

    subprocess.run = _guarded_run
    subprocess.Popen = _GuardedPopen


def frozen_path(name):
    return os.path.join(FROZEN, name)


def write_frozen(name, data):
    """writes a frozen JSON file only when its content changed (keeps the committed copy's bytes stable)"""
    import json
    os.makedirs(FROZEN, exist_ok=True)
    text = json.dumps(data, indent=1, sort_keys=True) + "\n"
    path = frozen_path(name)
    old = None
    if os.path.exists(path):
        with builtins.open(path, encoding="utf-8") as f:
            old = f.read()
    if old != text:
        with builtins.open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
        return True
    return False


def read_frozen(name):
    import json
    path = frozen_path(name)
    if not os.path.exists(path):
        raise SystemExit("missing build input %s (committed under standalone/frozen/; regenerate it with "
                         "tools/freeze_retail_inputs.py)" % path)
    with builtins.open(path, encoding="utf-8") as f:
        return json.load(f)
