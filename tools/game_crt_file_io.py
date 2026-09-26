"""Route every C-library file call in live rewrite code through the GAME's statically linked CRT, so a FILE*
opened by original code can be used by a rewrite and vice versa (libcmt's FILE objects live in a different
runtime). Calls are renamed to the single-underscore form (_fopen, _fprintf, ...), which harness/gen_link.py
resolves to the game CRT address (out/functions.json; fopen = 0x624186) and harness/gen_hooks.py's CRT list
treats as safe. Files that include <stdio.h> and had no declaration of their own get one.
Usage: python tools/game_crt_file_io.py [--apply]"""
import re, glob, os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools")); from name_fun_refs import code_comment_split
NAMES = ["fopen", "fclose", "fprintf", "fwrite", "fread", "fgets", "fscanf", "fseek", "ftell", "fflush"]
DECL = {"fopen": "extern void *_fopen(const char *path, const char *mode);                 // 0x624186 (game CRT)",
        "fclose": "extern int _fclose(void *file);                                           // game CRT",
        "fprintf": "extern int _fprintf(void *file, const char *format, ...);                // game CRT",
        "fwrite": "extern unsigned int _fwrite(const void *p, unsigned int size, unsigned int n, void *file); // game CRT",
        "fread": "extern unsigned int _fread(void *p, unsigned int size, unsigned int n, void *file); // game CRT",
        "fgets": "extern char *_fgets(char *s, int n, void *file);                          // game CRT",
        "fscanf": "extern int _fscanf(void *file, const char *format, ...);                 // game CRT",
        "fseek": "extern int _fseek(void *file, long offset, int origin);                   // game CRT",
        "ftell": "extern long _ftell(void *file);                                           // game CRT",
        "fflush": "extern int _fflush(void *file);                                          // game CRT"}
PAT = re.compile(r"(?<![\w.>])_{0,2}(" + "|".join(NAMES) + r")(?=\s*\()")

def main():
    apply = "--apply" in sys.argv
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        t = open(p, encoding="utf-8", errors="replace").read(); cut = t.rfind("#if 0")
        b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        out, used, declared = [], set(), set()
        for l in b.split("\n"):
            c, cm = code_comment_split(l)
            is_decl = c.lstrip().startswith("extern")
            for m in PAT.finditer(c): (declared if is_decl else used).add(m.group(1))
            out.append(PAT.sub(lambda m: "_" + m.group(1), c) + cm)
        if not used: continue
        nb = "\n".join(out)
        missing = sorted(used - declared)
        if missing:
            inc = list(re.finditer(r"^#include[^\n]*\n", nb, re.M))
            at = inc[-1].end() if inc else 0
            nb = nb[:at] + "".join(DECL[n] + "\n" for n in missing) + nb[at:]
        if nb != b:
            print(os.path.relpath(p, ROOT), sorted(used), "declared:", missing)
            if apply: open(p, "w", encoding="utf-8", newline="").write(nb + tail)

if __name__ == "__main__": main()
