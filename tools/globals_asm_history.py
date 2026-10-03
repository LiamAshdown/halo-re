"""standalone/globals.asm, the table of absolute EQU symbols the engine globals once were, is gone: every global is a C
definition in standalone/data/*.c. The globals checkers still compare against it:
  current()   the file as it is now ("" once deleted: no EQU left)
  original()  the last committed version that had it (git history), for the original address of every name
Stdlib only."""
import os, subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PATH = os.path.join(ROOT, "standalone", "globals.asm")


def current():
    return open(PATH, encoding="utf-8", errors="replace").read() if os.path.exists(PATH) else ""


def _git(*args):
    return subprocess.run(["git"] + list(args), cwd=ROOT, capture_output=True, text=True).stdout


def original():
    if os.path.exists(PATH):
        return current()
    gone = _git("log", "-1", "--format=%H", "--diff-filter=D", "--", "standalone/globals.asm").strip()
    text = _git("show", (gone + "^" if gone else "HEAD") + ":standalone/globals.asm")
    if not text:
        raise SystemExit("standalone/globals.asm: neither on disk nor in the git history")
    return text
