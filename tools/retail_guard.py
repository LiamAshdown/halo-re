"""Build-time retail independence (retail-independence step 2).

HALO_NO_RETAIL=1 makes the standalone build tools run from committed files only: importing this module then wraps
open(), io.open() and subprocess so that any attempt to read the retail binary (bin/halo.exe, or any halo.exe) or the
Ghidra export out/functions.json (derived from it and not in the repo) stops the build with an error.

Without the variable the tools read halo.exe as before and refresh the frozen copies under standalone/frozen/
(committed), which is everything the standalone build needs from the retail image:
  layout.json               section placement (image base, pieces, TLS, resources, reserve)
  imports.json              every import / delay-load slot the loader fills
  code_pointer_slots.json   every dword in .rdata/.data that holds a function address (slot, target, retail name,
                            module); the C symbol is attached at build time from src/ headers
  game_crt.json             game CRT function names -> addresses (from out/functions.json's library matches)
  code_address_ret.json     original address -> bytes its first ret pops, for code_address_ thunks
halo_image.bin (the retail image, now only a reference for tools/verify_image_source.py) is not written in no-retail
mode; the exe carries its data image as standalone/image/*.asm (written once by the retail-only tools/gen_image_source.py).
"""
import builtins, io, os, subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FROZEN = os.path.join(ROOT, "standalone", "frozen")
NO_RETAIL = os.environ.get("HALO_NO_RETAIL") == "1"


def is_retail_path(path):
    try:
        p = os.path.normcase(os.path.abspath(os.fspath(path)))
    except TypeError:
        return False
    name = os.path.basename(p)
    return name == "halo.exe" or p == os.path.normcase(os.path.join(ROOT, "out", "functions.json"))


def refuse(path):
    raise SystemExit("HALO_NO_RETAIL=1: the build tried to read %s (retail data); use the frozen copies in %s"
                     % (path, FROZEN))


if NO_RETAIL:
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
        raise SystemExit("missing frozen build input %s: run once without HALO_NO_RETAIL to create it" % path)
    with builtins.open(path, encoding="utf-8") as f:
        return json.load(f)
