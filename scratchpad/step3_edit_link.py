p = r"C:\Users\Liam-\halo-re\tools\gen_standalone_link.py"
t = open(p, encoding="utf-8").read()

old = '''def link(objs, force):
    rsp = os.path.join(OUT, "objs.rsp")
    open(rsp, "w").write("\\n".join('"%s"' % o for o in objs))
'''
assert old in t
t = t.replace(old, '''# extra linker options (the image source's /ALTERNATENAME bindings), written into the response file
LINK_OPTIONS = []


def image_source():
    """assembles the data image (standalone/image/*.asm, written by tools/gen_image_source.py) and returns its objects
    plus the /ALTERNATENAME options that bind each halo_code_<address> it stores to the C rewrite of that function"""
    image = os.path.join(SA, "image")
    objs = [assemble(os.path.join(image, p["label"] + ".asm"), os.path.join(OUT, "image_%s.obj" % p["label"]))
            for p in json.load(open(os.path.join(image, "pieces.json")))]
    std = stdcall_definitions()
    fast = fastcall_definitions()
    bound = {}
    for p in json.load(open(os.path.join(OUT, "code_pointers.json"))):
        if "c_symbol" in p:
            bound[p["target"]] = c_symbol(p["c_symbol"], std, fast)
    return objs, ["/ALTERNATENAME:halo_code_%06x=%s" % (a, s) for a, s in sorted(bound.items())]


def link(objs, force):
    rsp = os.path.join(OUT, "objs.rsp")
    open(rsp, "w").write("\\n".join(['"%s"' % o for o in objs] + LINK_OPTIONS))
''', 1)

old = '''    extra.append(assemble(os.path.join(OUT, "code_pointers.asm"), os.path.join(OUT, "code_pointers.obj")))
'''
assert old in t
t = t.replace(old, old + '''    image_objs, alternates = image_source()
    extra += image_objs
    LINK_OPTIONS[:] = alternates
''', 1)
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
