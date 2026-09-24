"""Run harness/build/difftest.exe on every hookable function, one process per function (a wild write through a
random pointer can corrupt the tester, and a random count can make a function loop forever), 8 at a time, each with
a time limit. Writes build/difftest_all.txt and build/difftest_results.json, prints a per-module summary.
Build difftest.exe first: python harness/build_difftest.py   Usage: python harness/difftest_all.py [module ...]"""
import os, re, sys, json, subprocess, collections
from concurrent.futures import ThreadPoolExecutor
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "harness", "build", "difftest.exe"); HALO = os.path.join(ROOT, "bin", "halo.exe")
entries = re.findall(r'hk_(\w+), "\w+", "(\w+)", "([^"]*)", \'(\w)\', (\d)\}', open(os.path.join(ROOT, "harness", "gen", "difftest_table.c")).read())
want = set(sys.argv[1:])
todo = [(n, m) for n, m, shape, ret, safe in entries if safe == "1" and (not want or m in want or n in want)]

def run(item):
    name, mod = item
    try:
        o = subprocess.run([EXE, HALO, name, "-n", "200", "-v"], capture_output=True, text=True, timeout=20).stdout
    except subprocess.TimeoutExpired:
        return name, mod, "hang", "no result within 20 s (a random input may loop forever in one version)"
    if "functions tested" not in o: return name, mod, "died", "tester process died"
    m = re.search(r"^FAIL .*?(\d+)/\s*(\d+) differ.*\n\s+first difference: (.*)", o, re.M)
    if m: return name, mod, "differ", f"{m.group(1)}/{m.group(2)}: {m.group(3)}"
    ran = re.search(r"(\d+)/\s*(\d+) differ", o)
    n_ran = int(ran.group(2)) if ran else 0
    # too few samples actually compared (the original faults on random inputs: it needs live game state)
    return (name, mod, "match", f"{n_ran} samples") if n_ran >= 20 else (name, mod, "untested", f"only {n_ran} samples ran")

with ThreadPoolExecutor(8) as ex: results = list(ex.map(run, todo))
json.dump([dict(zip(("name", "module", "result", "detail"), r)) for r in results], open(os.path.join(ROOT, "build", "difftest_results.json"), "w"), indent=0)
open(os.path.join(ROOT, "build", "difftest_all.txt"), "w").write("".join(f"{r[2]:<7} {r[0]:<50} {r[1]:<12} {r[3]}\n" for r in results))
by = collections.defaultdict(collections.Counter)
for n, m, res, _ in results: by[m][res] += 1
for m in sorted(by): print(f"{m:<12} " + "  ".join(f"{k} {v}" for k, v in sorted(by[m].items())))
tot = collections.Counter(r[2] for r in results); print("total:", dict(tot))
