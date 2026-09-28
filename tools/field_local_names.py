"""Names Ghidra locals (iVar3, fVar1, uVar2, bVar5, puVar7, sVar4, local_10, ...) that are assigned exactly once, from
a field access (x = a->b.field / x = *(T *)&a->field / T x = a->field), after that field. A local assigned more
than once is left alone (Ghidra often gives one name to unrelated compiler temporaries). Renames through
tools/rename_in.py (live code only, never onto a name already used). Names never change the compiled code.
  python tools/field_local_names.py [--dry] [module ...]"""
import glob, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOCAL = r"(?:[a-z]{1,3}Var\d+|local_[0-9a-f]+)"
BAD = re.compile(r"^(unknown\w*|_pad\w*|pad\w*|data|value|flags|type|count|index|pointer|i|j|k|x|y|z|base)$")


def main():
    dry = "--dry" in sys.argv
    mods = [a for a in sys.argv[1:] if not a.startswith("--")] or sorted(os.listdir(os.path.join(ROOT, "src")))
    total = 0
    for mod in mods:
        for f in sorted(glob.glob(os.path.join(ROOT, "src", mod, "*.c"))):
            text = open(f, encoding="utf-8").read()
            live = text.split("\n#if 0")[0]
            code = re.sub(r"//[^\n]*|/\*.*?\*/", "", live, flags=re.S)
            code = re.sub(r"^[ \t]*extern\b[^;]*;", "", code, flags=re.M | re.S)
            locals_ = set(re.findall(r"\b(%s)\b" % LOCAL, code))
            renames = {}
            used = set(re.findall(r"\b\w+\b", code))
            for v in sorted(locals_):
                assigns = re.findall(r"(?<![\w.>=!<])%s\s*(?:=(?!=)|\+=|-=|\*=|/=|\|=|&=|\+\+|--)\s*([^;]*);" % v, code)
                pre = re.findall(r"(?:\+\+|--)\s*%s\b" % v, code)
                if len(assigns) != 1 or pre:
                    continue
                rhs = assigns[0].strip()
                m = re.fullmatch(r"(?:\([\w\s\*]+\)\s*)?(?:\*\s*\([\w\s\*]+\)\s*&\s*)?\(?[\w\s\*\(\)]*?\)?(?:->|\.)[\w.>-]*?(\w+)", rhs)
                if not m:
                    continue
                name = m.group(1)
                if BAD.match(name) or len(name) < 4 or name in used or name in renames.values():
                    continue
                renames[v] = name
            if not renames:
                continue
            total += len(renames)
            print("%-60s %s" % (os.path.relpath(f, ROOT), " ".join("%s=%s" % kv for kv in renames.items())))
            if not dry:
                subprocess.run([sys.executable, os.path.join(ROOT, "tools", "rename_in.py"), f] +
                               ["%s=%s" % kv for kv in renames.items()], check=True, capture_output=True)
    print(total, "locals named")


if __name__ == "__main__":
    main()
