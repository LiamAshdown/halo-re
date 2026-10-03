"""python tools/api_prep.py <module>: rewrites `extern "C" ret fn(...) {` definitions into one trailing `extern "C" { ... }` block per file so that
tools/api_convert2.py can mechanise them."""
import glob, os, re, sys
os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
mod = sys.argv[1]
DEF = re.compile(r'^extern "C" (?=(?:[A-Za-z_][\w:]*[ \t\*]+)+?[\*]*\w+\([^;{}]*\)\s*\n?\{)', re.M)
for p in sorted(glob.glob(f"src/{mod}/*.cpp")):
    s = open(p, encoding="utf-8", errors="replace", newline="").read()
    m = DEF.search(s)
    if not m:
        continue
    first = m.start()
    k = s.rfind("/**", 0, first)
    if k >= 0 and "*/" in s[k:first] and s[s.index("*/", k) + 2:first].strip() == "":
        first = k
    head, tail = s[:first], s[first:]
    tail = DEF.sub("", tail)
    s = head + 'extern "C" {\n\n' + tail.rstrip() + "\n\n}\n"
    open(p, "w", encoding="utf-8", newline="").write(s)
    print("prepped", p)
