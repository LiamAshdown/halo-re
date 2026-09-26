"""Rank what keeps rewritten functions from being hookable, by how many unsafe functions each root blocks
(following gen_hooks' "via X" chains to the end). Run after harness/gen_hooks.py.
Kinds:  calls original X   -> X's C is missing, known-bad or incomplete: rewrite/fix X itself
        declares X          -> a caller's extern for X disagrees with X's definition: fix the callers' declarations
                               and calls against the binary call sites
        broken rewrite X    -> X is listed in harness/known_bad.txt or harness/incomplete_rewrites.txt
Usage: python tools/rank_blockers.py [N]"""
import os, re, sys, json, collections

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    top = int(sys.argv[1]) if len(sys.argv) > 1 else 25
    report = json.load(open(os.path.join(ROOT, "harness", "build", "hooks_report.json")))
    unsafe = report["unsafe (reach an original register-convention function)"]

    def root(name, seen=()):
        reason = unsafe.get(name)
        if reason is None or name in seen:
            return ("unknown", name)
        m = re.match(r"via (\w+)", reason)
        if m:
            return root(m.group(1), seen + (name,))
        m = re.search(r"declares (\w+) differently", reason)
        if m:
            return ("declares", m.group(1))
        m = re.search(r"calls original _(\w+)", reason)
        if m:
            return ("calls original", m.group(1))
        if "known_bad" in reason:
            return ("broken rewrite", name)
        return ("other", name)

    counts = collections.Counter(root(n) for n in unsafe)
    print("unsafe functions: %d, distinct roots: %d" % (len(unsafe), len(counts)))
    for (kind, name), n in counts.most_common(top):
        print("%4d  %-15s %s" % (n, kind, name))


if __name__ == "__main__":
    main()
