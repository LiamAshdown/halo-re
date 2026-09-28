import json, os
R = "C:\\Users\\Liam-\\halo-re\\"
p = R + "tools\\gen_image_source.py"
t = open(p, encoding="utf-8").read()
old = '''    ("text_004a0268", 0x4a0268, 0x4a029c),   # 25 case addresses + 25 index bytes (0x4a0283), padded to a dword
'''
assert old in t
t = t.replace(old, "", 1)
t = t.replace('''DIDATAFORMAT at 0x64dfdc points there) and the switch table at 0x4a0268 that ui_controls_options_populate_from_profile
reads.''', '''DIDATAFORMAT at 0x64dfdc points there).''', 1)
open(p, "w", encoding="utf-8", newline="\n").write(t)
pj = R + "standalone\\image\\pieces.json"
pieces = [x for x in json.load(open(pj)) if x["label"] != "text_004a0268"]
open(pj, "w", newline="\n").write(json.dumps(pieces, indent=1) + "\n")
os.remove(R + "standalone\\image\\text_004a0268.asm")
print("ok")
