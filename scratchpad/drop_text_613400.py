import json, os
R = "C:\\Users\\Liam-\\halo-re\\"
p = R + "tools\\gen_image_source.py"
t = open(p, encoding="utf-8").read()
old = '''TEXT_DATA = [
    ("text_00613400", 0x613400, 0x614400),   # DIOBJECTDATAFORMAT[256] for SetDataFormat (DIDATAFORMAT 0x64dfdc)
]'''
assert old in t
t = t.replace(old, '''# (none: the DIOBJECTDATAFORMAT[256] at 0x613400 behind the retail c_dfDIKeyboard 0x64dfdc was the last one, and the C
# takes c_dfDIKeyboard / c_dfDIMouse2 from dinput8.lib; the switch table at 0x4a0268 became a C switch)
TEXT_DATA = []''', 1)
i = t.index("Pieces: all of .rdata")
j = t.index("Import slots keep")
t = t[:i] + '''Pieces: all of .rdata, the initialised part of .data (the zero tail is the reserve's own zero fill), .tls and .rsrc.
Nothing of .text: it is original code, and the ranges the C once read as data are gone (see TEXT_DATA). It stays
reserved, zero and not executable in the standalone.
''' + t[j:]
open(p, "w", encoding="utf-8", newline="\n").write(t)
pj = R + "standalone\\image\\pieces.json"
pieces = [x for x in json.load(open(pj)) if x["label"] != "text_00613400"]
open(pj, "w", newline="\n").write(json.dumps(pieces, indent=1) + "\n")
os.remove(R + "standalone\\image\\text_00613400.asm")
print("ok")
