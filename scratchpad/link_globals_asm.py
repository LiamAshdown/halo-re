p = "C:\\Users\\Liam-\\halo-re\\tools\\gen_standalone_link.py"
t = open(p, encoding="utf-8").read()


def sub(old, new):
    global t
    assert t.count(old) == 1, old[:70]
    t = t.replace(old, new)


sub('''    image_objs, alternates = image_source()
    extra += image_objs
''', '''    image_objs, alternates = image_source()
    extra += image_objs
    # the engine globals at their fixed original addresses: committed source (tools/update_globals.py maintains it)
    extra.append(assemble(os.path.join(SA, "globals.asm"), os.path.join(OUT, "globals.obj")))
''')
sub('''        if k == "data" and a is not None:
            data_eq.append("PUBLIC %s\\n%s EQU 0%Xh" % (s, s, a)); report["global (absolute)"] += 1; continue''',
    '''        if k == "data" and a is not None:
            # a global standalone/globals.asm does not list yet: resolved from its declaration's address comment for
            # this link; tools/update_globals.py adds it to the committed file
            data_eq.append("PUBLIC %s\\n%s EQU 0%Xh" % (s, s, a)); report["global (absolute)"] += 1; continue''')
t_end = t.rindex("\n")
open(p, "w", encoding="utf-8", newline="\n").write(t)

# the notice after the summary line
t = open(p, encoding="utf-8").read()
i = t.index('    print("left unresolved:')
t = t[:i] + '''    if report["global (absolute)"]:
        print("NOTE: %d globals are not in standalone/globals.asm (resolved from their address comments this time); "
              "add them with: python tools/update_globals.py" % report["global (absolute)"])
''' + t[i:]
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
