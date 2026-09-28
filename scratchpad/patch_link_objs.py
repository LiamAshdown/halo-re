import os
os.chdir(r'C:\Users\Liam-\halo-re')
p = 'tools/gen_standalone_link.py'
s = open(p, encoding='utf-8').read()
old = '''    objs = glob.glob(os.path.join(ROOT, "build", "obj", "*", "*.obj")) + extra'''
new = '''    # only objects whose source still exists: a renamed or deleted .c leaves its old object behind, which would link
    # stale code and its unbound references (shell_console_window_state_initialize.obj: nine '?' traps) (2026-09-28)
    objs = [o for o in glob.glob(os.path.join(ROOT, "build", "obj", "*", "*.obj"))
            if os.path.exists(os.path.join(ROOT, "src", os.path.basename(os.path.dirname(o)),
                                           os.path.splitext(os.path.basename(o))[0] + ".c"))] + extra'''
assert old in s
s = s.replace(old, new, 1)
open(p, 'w', encoding='utf-8').write(s)
print('patched')
