import re, struct, subprocess
R = "C:\\Users\\Liam-\\halo-re\\"
exe = open(R + "bin\\halo.exe", "rb").read()


def u32(va):
    return struct.unpack_from("<I", exe, va - 0x400000)[0]   # .text raw == rva for halo.exe (file align 0x1000)


def u8(va):
    return exe[va - 0x400000]


dis = open(R + "scratchpad\\nga.dis").read().split("\n")
ins = []
for l in dis:
    m = re.match(r"\s*([0-9a-f]+):\s+(.*)", l)
    if m:
        ins.append((int(m.group(1), 16), m.group(2).strip()))


def reach(a):
    """the setup and call reached from a case label"""
    out = []
    started = False
    for addr, text in ins:
        if addr == a:
            started = True
        if started:
            out.append(text)
            if text.startswith(("call", "ret")) or (text.startswith("mov    BYTE PTR ds:0x71c2c0,0x0")):
                break
    return " | ".join(out)


print("client (mode 1):")
for k in range(0x38):
    t = u32(0x4da634 + 4 * k)
    print("  case 0x%02x -> %06x  %s" % (k, t, reach(t)))
print("host (mode 2):")
for k in range(0x30):
    idx = u8(0x4da734 + k)
    t = u32(0x4da714 + 4 * idx)
    if t != 0x4da62a:
        print("  case 0x%02x -> %06x  %s" % (k + 6, t, reach(t)))
